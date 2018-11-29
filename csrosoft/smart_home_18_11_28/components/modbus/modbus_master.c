#include "modbus_master.h"

Modbus_Master Master;

static QueueHandle_t    uart0_queue;
static QueueHandle_t    uart1_queue;
static os_timer_t       uart0_rx_timer;

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
                    os_timer_arm(&uart0_rx_timer, 1, false);
                    uart_read_bytes(0, Master.rx_buf, event.size, portMAX_DELAY);
                    break;
                case UART_FIFO_OVF:
                    uart_flush_input(0);
                    xQueueReset(uart0_queue);
                    break;
                case UART_BUFFER_FULL:
                    uart_flush_input(0);
                    xQueueReset(uart0_queue);
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


static void uart0_rx_timeout(void* arg)
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

    os_timer_disarm(&uart0_rx_timer);
    os_timer_setfn(&uart0_rx_timer, (os_timer_func_t*)uart0_rx_timeout, 0);


    xTaskCreate(uart0_event_task, "uart0_event_task", 2048, NULL, 12, NULL);
    xTaskCreate(uart1_event_task, "uart1_event_task", 2048, NULL, 12, NULL);
}