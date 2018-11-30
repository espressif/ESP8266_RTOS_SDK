#include "modbus_master.h"

Modbus_Master Master;

static QueueHandle_t        uart0_queue;
static SemaphoreHandle_t 	uart0_receive_semaphore;


bool modbus_master_validate_rx(Modbus_Master *master)
{
    if (master->rx_len<5) {
        return false;
    }
    uint16_t crc = modbus_crc16(master->rx_buf, master->rx_len - 2);
    if ((crc&0xFF) != master->rx_buf[master->rx_len-2]) {
        return false;
    }
    if ((crc>>8) != master->rx_buf[master->rx_len-1]) {
        return false;
    }
    if (master->rx_buf[0] != master->slave_id) {
        return false;
    }
    if (master->rx_buf[1] != master->func_code) {
        return false;
    }
    return true;
}


bool master_send_receive(uint16_t timeout)
{
    bool result = false;
    uint16_t crc = modbus_crc16(Master.tx_buf, Master.tx_len);
    Master.tx_buf[Master.tx_len++] = crc & 0xFF;
    Master.tx_buf[Master.tx_len++] = crc >> 8;
    xSemaphoreTake(uart0_receive_semaphore, 0);
    Master.rx_enable = true;
    Master.rx_len = 0;
    uart_write_bytes(UART_NUM_1, (const char *)Master.tx_buf, Master.tx_len);
    if(xSemaphoreTake(uart0_receive_semaphore, timeout) == pdTRUE) {
        result = true;
    }
    Master.rx_enable = false;
    return result;
}

void uart0_receive_one_byte(uint8_t data)
{
    if(Master.rx_enable) {
        Master.rx_buf[Master.rx_len] = data;
        Master.rx_len = (Master.rx_len + 1) % 512;
    }
}


void uart0_receive_complete(void)
{
    xSemaphoreGive(uart0_receive_semaphore);
}


static void uart0_event_task(void *pvParameters)
{
    uint8_t *dump = (uint8_t *)malloc(512);
    uart_event_t event;
    for (;;)
    {
        if(xQueueReceive(uart0_queue, (void *)&event, (portTickType)portMAX_DELAY)) {
            switch (event.type)
            {
                case UART_DATA:
                    uart_read_bytes(UART_NUM_0, dump, event.size, portMAX_DELAY);
                    break;
                case UART_FIFO_OVF:
                    uart_flush_input(UART_NUM_0);
                    xQueueReset(uart0_queue);
                    break;
                case UART_BUFFER_FULL:
                    uart_flush_input(UART_NUM_0);
                    xQueueReset(uart0_queue);
                    break;
                default:
                    break;
            }
        }
    }
    free(dump);
    dump = NULL;
    vTaskDelete(NULL);
}


void modbus_master_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(0, &uart_config);
    uart_param_config(1, &uart_config);

    uart0_receive_semaphore = xSemaphoreCreateBinary();

    uart_driver_install(UART_NUM_0, 1024, 1024, 10, &uart0_queue) ;
    uart_driver_install(UART_NUM_1, 1024, 0,    0,  NULL);

    xTaskCreate(uart0_event_task, "uart0_event_task", 1024, NULL, 12, NULL);
    Master.master_command = master_send_receive;
}