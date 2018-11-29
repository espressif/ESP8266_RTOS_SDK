#include "modbus_master.h"

Modbus_Master Master;

static QueueHandle_t uart0_queue;
static QueueHandle_t uart1_queue;

static void uart0_event_task(void *pvParameters)
{
    uart_event_t event;
    while(1)
    {
        if(xQueueReceive(uart0_queue, (void *)&event, (portTickType)portMAX_DELAY))
        {
            switch (event.type)
            {
                case UART_DATA:
                    break;
                case UART_FIFO_OVF:
                    break;
                case UART_BUFFER_FULL:
                    break;
                case UART_PARITY_ERR:
                    break;
                case UART_FRAME_ERR:
                    break;
                default:
                    break;
            }
        }
    }
}

static void uart1_event_task(void *pvParameters)
{

}

void Modbus_Master_Init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 74880,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(0, &uart_config);
    uart_param_config(1, &uart_config);

    uart_driver_install(0, 600, 600, 100, &uart0_queue);
    uart_driver_install(1, 600, 600, 100, &uart1_queue);
    xTaskCreate(uart0_event_task, "uart0_event_task", 2048, NULL, 12, NULL);
    xTaskCreate(uart1_event_task, "uart1_event_task", 2048, NULL, 12, NULL);
}