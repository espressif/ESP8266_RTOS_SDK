#include "modbus_master.h"

Modbus_Master Master;

static QueueHandle_t    uart0_queue;

uint8_t     uart0data[1024];
uint16_t    uart0index = 0;

static void uart0_event_task(void *pvParameters)
{
    uart_event_t event;
    for (;;)
    {
        if(xQueueReceive(uart0_queue, (void *)&event, (portTickType)portMAX_DELAY))
        {
            switch (event.type)
            {
                case UART_DATA:
                    if(Master.rx_len+event.size<512)
                    {
                        uart_read_bytes(0, Master.rx_buf, event.size, portMAX_DELAY);
                    }
                    break;
                case UART_FIFO_OVF || UART_BUFFER_FULL:
                    uart_flush_input(0);
                    xQueueReset(uart0_queue);
                    debug("FIFO BURRER ERROR");
                    break;
                default:
                    debug("OTHER ERRORS");
                    break;
            }
        }
    }
}

void uart0_receive_one_byte(uint8_t data)
{
    uart0data[uart0index] = data;
    uart0index++;
}

void uart0_receive_complete(void)
{
    char message[100];
    sprintf(message, "Receive message! message length is %d\r\n", strlen((char *)uart0data));
    memset(uart0data, 0, 1024);
    uart0index = 0;
    uart_write_bytes(UART_NUM_1, message, strlen(message));
}


static void uart0_timer_callback(void* arg)
{
    char message[512];
    sprintf(message, "Receive message! message length is %d\r\n", Master.rx_len);
    Master.rx_len = 0;
    uart_write_bytes(UART_NUM_1, message, strlen(message));
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

    uart_driver_install(UART_NUM_0, 1024, 1024, 10, &uart0_queue) ;
    uart_driver_install(UART_NUM_1, 1024, 0,    0,  NULL);
    
    xTaskCreate(uart0_event_task, "uart0_event_task", 2048, NULL, configMAX_PRIORITIES, NULL);
}