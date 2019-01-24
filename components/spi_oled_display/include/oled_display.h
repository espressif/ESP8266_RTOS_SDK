

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp8266/gpio_struct.h"
#include "esp8266/spi_struct.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_libc.h"
#include "driver/gpio.h"
#include "driver/spi.h"

// D5 -> CLK GPIO14
// D7 -> MOSI (DOUT) GPIO13
// D0 -> RES GPIO16
// D2 -> DC GPIO4
// D8 -> CS GPIO15

#define OLED_DC_GPIO     4
#define OLED_RST_GPIO    16
#define OLED_CS_GPIO     15
#define OLED_PIN_SEL  (1ULL<<OLED_DC_GPIO) | (1ULL<<OLED_RST_GPIO) | (1ULL<<OLED_CS_GPIO)

#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }

#define _max(a,b) ((a) > (b) ? (a) : (b))
#define _min(a,b) ((a) < (b) ? (a) : (b))


#define DEBUG_OLEDDISPLAY printf


 esp_err_t oled_delay_ms(uint32_t time);

 static esp_err_t oled_set_dc(uint8_t dc);

// Write an 8-bit cmd
 static esp_err_t oled_write_cmd(uint8_t data);

// Write an 8-bit data
 static esp_err_t oled_write_byte(uint8_t data);

 esp_err_t oled_rst();

 esp_err_t oled_init();

 esp_err_t oled_set_pos(uint8_t x_start, uint8_t y_start);

 esp_err_t oled_clear(uint8_t data);

 void IRAM_ATTR spi_event_callback(int event, void *arg);

 void setPixel(int16_t x, int16_t y) ;

 void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1) ;

 void display();

 esp_err_t oled_display_init();

 void sendInitCommands(void) ;
