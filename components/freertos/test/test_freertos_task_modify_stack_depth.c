/* FreeRTOS timer tests
*/
//#define LOG_LOCAL_LEVEL 5

#include <stdio.h>
#include "unity.h"
#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define TASK_DEPTH_SIZE 8192
#define TAG "test_modify_stack_depth"

static SemaphoreHandle_t s_main_sem, s_sync_sem[3];
static TaskHandle_t s_task_handle[3];

static void test_thread_1(void *p)
{
    for (int i = 0; i < 16; i++) {
        ESP_LOGD(TAG, "Modify tasks' stack depth to be %u", TASK_DEPTH_SIZE - i * 128);

        vTaskModifyStackDepth(NULL, TASK_DEPTH_SIZE - i * 128);
    }

    xSemaphoreGive(s_main_sem);

    vTaskDelete(NULL);
}

static void test_thread_2(void *p)
{
    for (int i = 0; i < 10; i++) {
        vTaskPrioritySet(NULL, 5);

        ESP_LOGD(TAG, "Modify tasks' stack depth to be %u", TASK_DEPTH_SIZE - i * 128);

        for (int j = 0; j < 3; j++) {
            vTaskModifyStackDepth(s_task_handle[j], TASK_DEPTH_SIZE - i * 128);
            xSemaphoreGive(s_sync_sem[j]);
        }

        ESP_LOGD(TAG, "Modify successfully");

        vTaskPrioritySet(NULL, 3);
    }

    xSemaphoreGive(s_main_sem);

    vTaskDelete(NULL);
}

static void test_thread_3(void *p)
{
    int count = 10;
    int num = (int)p;

    while (count--) {
        TEST_ASSERT(xSemaphoreTake(s_sync_sem[num], portMAX_DELAY) == pdTRUE);
    }

    vTaskDelete(NULL);
}

TEST_CASE("Test task modifies itself's stack depth", "[freertos]")
{
    TEST_ASSERT(s_main_sem = xSemaphoreCreateBinary());

    TEST_ASSERT(xTaskCreatePinnedToCore(test_thread_1, "thread_1", TASK_DEPTH_SIZE, NULL, 5, NULL, 0) == pdTRUE);

    TEST_ASSERT(xSemaphoreTake(s_main_sem, portMAX_DELAY) == pdTRUE);

    vQueueDelete(s_main_sem);
    s_main_sem = NULL;

    vTaskDelay(5);
}

TEST_CASE("Test task modifies other tasks' stack depth", "[freertos]")
{
    ESP_LOGD(TAG, "before heap is %u", esp_get_free_heap_size());

    TEST_ASSERT(s_main_sem = xSemaphoreCreateBinary());

    for (int i = 0; i < 3; i++) {
       TEST_ASSERT(s_sync_sem[i] = xSemaphoreCreateBinary()); 
    }

    vTaskSuspendAll();

    TEST_ASSERT(xTaskCreatePinnedToCore(test_thread_2, "thread_2", TASK_DEPTH_SIZE, NULL, 5, NULL, 0) == pdTRUE);

    TEST_ASSERT(xTaskCreatePinnedToCore(test_thread_3, "thread_4", TASK_DEPTH_SIZE, (void *)0, 4, &s_task_handle[0], 0) == pdTRUE);
    TEST_ASSERT(xTaskCreatePinnedToCore(test_thread_3, "thread_5", TASK_DEPTH_SIZE, (void *)1, 5, &s_task_handle[1], 0) == pdTRUE);
    TEST_ASSERT(xTaskCreatePinnedToCore(test_thread_3, "thread_6", TASK_DEPTH_SIZE, (void *)2, 6, &s_task_handle[2], 0) == pdTRUE);

    xTaskResumeAll();

    TEST_ASSERT(xSemaphoreTake(s_main_sem, portMAX_DELAY) == pdTRUE);

    for (int i = 0; i < 3; i++) {
       vQueueDelete(s_sync_sem[i]); 
       s_sync_sem[i] = NULL;
    }

    vQueueDelete(s_main_sem);
    s_main_sem = NULL;

    vTaskDelay(5);

    ESP_LOGD(TAG, "after heap is %u", esp_get_free_heap_size());
}
