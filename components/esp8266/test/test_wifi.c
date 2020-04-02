/*
 Tests for the Wi-Fi
*/
#include "string.h"
#include "unity.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_log.h"
#include "test_utils.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

static const char* TAG = "test_wifi";

#define GOT_IP_EVENT        0x00000001
#define DISCONNECT_EVENT    0x00000002

#define EVENT_HANDLER_FLAG_DO_NOT_AUTO_RECONNECT 0x00000001
#define FULL_CAHNNEL_SCAN_DURATION_THRESHOLD  1500

static uint32_t wifi_event_handler_flag;

xSemaphoreHandle cb_scan_num_mutex;
xSemaphoreHandle task_scan_num_mutex;
xSemaphoreHandle cb_scan_fail_mutex;
xSemaphoreHandle task_scan_fail_mutex;

static EventGroupHandle_t scan_task_exit;
static EventGroupHandle_t wifi_events;

struct timeval cb_scan_time_list[10];
struct timeval task_scan_time_list[10];

static uint32_t cb_scan_ap_num = 0;
static uint32_t task_scan_ap_num = 0;
static uint32_t cb_scan_fail_times = 0;
static uint32_t task_scan_fail_times = 0;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    printf("ev_handle_called.\n");
    uint16_t num = 0;
    struct timeval cb_tv = {0};
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
            ESP_LOGI(TAG, "got ip:%s\n",
            ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            if (wifi_events) {
                xEventGroupSetBits(wifi_events, GOT_IP_EVENT);
            }
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "SYSTEM_EVENT_STA_DISCONNECTED");
            if (! (EVENT_HANDLER_FLAG_DO_NOT_AUTO_RECONNECT & wifi_event_handler_flag) ) {
                TEST_ESP_OK(esp_wifi_connect());
            }
            if (wifi_events) {
                xEventGroupSetBits(wifi_events, DISCONNECT_EVENT);
            }
            break;
        case SYSTEM_EVENT_SCAN_DONE:
            if (event->event_info.scan_done.status == 0) {
                esp_wifi_scan_get_ap_num(&num);
                xSemaphoreTake(cb_scan_num_mutex, portMAX_DELAY);
                cb_scan_ap_num += num;
                xSemaphoreGive(cb_scan_num_mutex);         
                printf("+SCANDONE\n");
            }
            else {
                gettimeofday(&cb_tv, NULL);
                xSemaphoreTake(cb_scan_fail_mutex, portMAX_DELAY);
                cb_scan_time_list[cb_scan_fail_times++] = cb_tv;
                xSemaphoreGive(cb_scan_fail_mutex); 
                printf("+SCANFAIL\n");
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void start_wifi_as_sta(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.nvs_enable = false;
    // do not auto connect
    wifi_event_handler_flag |= EVENT_HANDLER_FLAG_DO_NOT_AUTO_RECONNECT;
    TEST_ESP_OK(esp_event_loop_init(event_handler, NULL));
    // can't deinit event loop, need to reset leak check
    unity_reset_leak_checks();
    if (wifi_events == NULL) {
        wifi_events = xEventGroupCreate();
    } else {
        xEventGroupClearBits(wifi_events, 0x00ffffff);
    }
    TEST_ESP_OK(esp_wifi_init(&cfg));
    TEST_ESP_OK(esp_wifi_set_mode(WIFI_MODE_STA));
    TEST_ESP_OK(esp_wifi_start());
}

static void stop_wifi(void)
{
    printf("stop wifi\n");
    TEST_ESP_OK(esp_wifi_stop());
    TEST_ESP_OK(esp_wifi_deinit());
    if (wifi_events) {
        vEventGroupDelete(wifi_events);
        wifi_events = NULL;
    }
    vTaskDelay(1000/portTICK_PERIOD_MS);
}

static void scan_task(void *param)
{
    int32_t ret;
    uint16_t ap_num;
    float scan_duration = 0.0;
    struct timeval scan_tv_start = {0}, scan_tv_stop = {0}, task_tv = {0};
    uint8_t task_id = *((uint8_t *) param);
    uint8_t *test_result = (uint8_t *) param;
    wifi_scan_config_t scan_config = {0};
    for (int i = 0; i < 2; i++) {
        ap_num = 0;
        ESP_LOGI(TAG, "[%u] scan start", task_id);
        gettimeofday(&scan_tv_start, NULL);
        ret = esp_wifi_scan_start(&scan_config, true);
        if (ret == ESP_OK) {
            gettimeofday(&scan_tv_stop, NULL);
            scan_duration = (scan_tv_stop.tv_sec - scan_tv_start.tv_sec) + (scan_tv_stop.tv_usec - scan_tv_start.tv_usec) * 1e-6f;
            esp_wifi_scan_get_ap_num(&ap_num);
            ESP_LOGI(TAG, "[%u] scan succeed. duration: %.2f; ap_num: %d; ret: %d", task_id, scan_duration, ap_num, ret);
            if (scan_duration < FULL_CAHNNEL_SCAN_DURATION_THRESHOLD) {
                *test_result = 0;
            }
        } else {
            gettimeofday(&task_tv, NULL);
            xSemaphoreTake(task_scan_fail_mutex, portMAX_DELAY);
            task_scan_time_list[task_scan_fail_times++] = task_tv;
            xSemaphoreGive(task_scan_fail_mutex);
            ESP_LOGI(TAG, "[%u] scan fail, ret: %d", task_id, ret);
            vTaskDelay(300 / portTICK_PERIOD_MS);
        }
        xSemaphoreTake(task_scan_num_mutex, portMAX_DELAY);
        task_scan_ap_num += ap_num;
        xSemaphoreGive(task_scan_num_mutex);
    }
    ESP_LOGI(TAG, "[%u] scan task exit", task_id);
    xEventGroupSetBits(scan_task_exit, task_id);
    vTaskDelete(NULL);
}

TEST_CASE("wifi do block scan before scan finished","[wifi][ignore]")
{
    uint8_t i, ret[2];
    float time_interval = 0.0;
    if (scan_task_exit == NULL) {
        scan_task_exit = xEventGroupCreate();
    }
    if (cb_scan_num_mutex == NULL) {
        cb_scan_num_mutex = xSemaphoreCreateMutex();
    }
    if (task_scan_num_mutex == NULL) {
        task_scan_num_mutex = xSemaphoreCreateMutex();
    }
    if (cb_scan_fail_mutex == NULL) {
        cb_scan_fail_mutex = xSemaphoreCreateMutex();
    }
    if (task_scan_fail_mutex == NULL) {
        task_scan_fail_mutex = xSemaphoreCreateMutex();
    }
    test_case_uses_tcpip();
    start_wifi_as_sta(); 
    xEventGroupClearBits(scan_task_exit, BIT0 | BIT1);
    for (i = 1; i <= 2; i++) {
        ret[i-1] = i;
        if (xTaskCreate(scan_task, "scan task", 2048, &ret[i-1], 1, NULL) != pdTRUE) {
            printf("create scan task fail");
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    xEventGroupWaitBits(scan_task_exit, BIT0 | BIT1, true, true, portMAX_DELAY);
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    ESP_LOGI(TAG, "compare the ap nums got in cb and after esp_wifi_scan_start() finish");
    if (cb_scan_ap_num != task_scan_ap_num) {
        TEST_IGNORE_MESSAGE("the ap nums are different");
    }
    ESP_LOGI(TAG, "compare the times of printing SCANFAIL in cb and esp_wifi_scan_start() return error code");
    if (cb_scan_fail_times != task_scan_fail_times) {
        TEST_IGNORE_MESSAGE("the times are different");
    }
    ESP_LOGI(TAG, "check the time interval between printing SCANFAIL in cb and esp_wifi_scan_start() return error code");
    for (i = 0; i < task_scan_fail_times; i++) {
        time_interval = (cb_scan_time_list[i].tv_sec - task_scan_time_list[i].tv_sec) + (cb_scan_time_list[i].tv_usec - task_scan_time_list[i].tv_usec) * 1e-6f;
        if (abs(time_interval) > 1) {
            TEST_IGNORE_MESSAGE("the time interval more than 1s");
        }
    }
    ESP_LOGI(TAG, "check scan time");
    for (i = 1; i <= 2; i++) {
        if (ret[i-1] != i) {
            TEST_IGNORE_MESSAGE("the scan time less than 1500ms");
        }
    }
    stop_wifi();
    vEventGroupDelete(scan_task_exit);
    vSemaphoreDelete(cb_scan_num_mutex);
    vSemaphoreDelete(task_scan_num_mutex); 
    vSemaphoreDelete(cb_scan_fail_mutex);
    vSemaphoreDelete(task_scan_fail_mutex);
    scan_task_exit = NULL;
    cb_scan_num_mutex = NULL;
    task_scan_num_mutex = NULL;
    cb_scan_fail_mutex = NULL;
    task_scan_fail_mutex = NULL;
    TEST_IGNORE_MESSAGE("this test case is ignored due to the critical memory leak of tcpip_adapter and event_loop.");
}
