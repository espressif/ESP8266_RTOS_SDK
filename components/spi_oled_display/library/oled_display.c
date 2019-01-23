#include "oled_display.h"


enum OLEDDISPLAY_COLOR {
  BLACK = 0,
  WHITE = 1,
  INVERSE = 2
};

uint16_t  displayWidth                     = 128;
uint16_t  displayHeight                    = 64;
uint16_t  displayBufferSize                = 1024;
uint8_t   *dispaly_image_buffer            = NULL;
uint8_t   *dispaly_image_buffer_back       = NULL;

uint8_t   color                            = WHITE;

#define OLED_BACK_IMAGE_ENABLE 


# define memset(dst, c, n) xmemset(dst, c, n)

static void *
xmemset(void *dst, int c, size_t n)
{
    unsigned char *     dst_ = (unsigned char *) dst;
    const unsigned char c_   = (unsigned char) c;
    size_t              i;

    for (i = 0; i < n; i++) {
        dst_[i] = c_;
    }
    return dst;
}



static uint8_t oled_dc_level = 0;

 esp_err_t oled_delay_ms(uint32_t time)
{
    vTaskDelay(time / portTICK_RATE_MS);
    return ESP_OK;
}

static esp_err_t oled_set_dc(uint8_t dc)
{
    oled_dc_level = dc;
    return ESP_OK;
}

// Write an 8-bit cmd
static esp_err_t oled_write_cmd(uint8_t data)
{
    uint32_t buf = data << 24;
    spi_trans_t trans = {0};
    trans.mosi = &buf;
    trans.bits.mosi = 8;
    oled_set_dc(0);
    spi_trans(HSPI_HOST, trans);
    return ESP_OK;
}

// Write an 8-bit data
static esp_err_t oled_write_byte(uint8_t data)
{
    uint32_t buf = data << 24;
    spi_trans_t trans = {0};
    trans.mosi = &buf;
    trans.bits.mosi = 8;
    oled_set_dc(1);
    spi_trans(HSPI_HOST, trans);
    return ESP_OK;
}

esp_err_t oled_rst()
{
    gpio_set_level(OLED_RST_GPIO, 0);
    oled_delay_ms(200);
    gpio_set_level(OLED_RST_GPIO, 1);
    oled_delay_ms(100);
    return ESP_OK;
}

 esp_err_t oled_init()
{
    oled_rst(); // Reset OLED
    oled_write_cmd(0xAE);    // Set Display ON/OFF (AEh/AFh)
    oled_write_cmd(0x00);    // Set Lower Column Start Address for Page Addressing Mode (00h~0Fh)
    oled_write_cmd(0x10);    // Set Higher Column Start Address for Page Addressing Mode (10h~1Fh)
    oled_write_cmd(0x40);    // Set Display Start Line (40h~7Fh)
    oled_write_cmd(0x81);    // Set Contrast Control for BANK0 (81h)
    oled_write_cmd(0xCF);    //
    oled_write_cmd(0xA1);    // Set Segment Re-map (A0h/A1h)
    oled_write_cmd(0xC8);    // Set COM Output Scan Direction (C0h/C8h)
    oled_write_cmd(0xA6);    // Set Normal/Inverse Display (A6h/A7h)
    oled_write_cmd(0xA8);    // Set Multiplex Ratio (A8h)
    oled_write_cmd(0x3F);    // Set Heigh (0-63 0x00-0x3F)
    oled_write_cmd(0xD3);    // Set Display Offset (D3h)
    oled_write_cmd(0x00);    // Set Lower Column Start Address for Page Addressing Mode (00h~0Fh)
    oled_write_cmd(0xD5);    // Set Display Clock Divide Ratio/ Oscillator Frequency (D5h)
    oled_write_cmd(0x80);    //
    oled_write_cmd(0xD9);    // Set Pre-charge Period (D9h)
    oled_write_cmd(0xF1);    //
    oled_write_cmd(0xDA);    // Set COM Pins Hardware Configuration (DAh)
    oled_write_cmd(0x12);    // Set Higher Column Start Address for Page Addressing Mode (10h~1Fh)
    oled_write_cmd(0xDB);    // Set VCOMH  Deselect Level (DBh)
    oled_write_cmd(0x40);    // Set Display Start Line (40h~7Fh)
    oled_write_cmd(0x20);    // Set Memory Addressing Mode (20h)
    oled_write_cmd(0x02);    // Set 页地址模式(A[1:0]=10b) 水平地址模式(A[1:0]=00b) 垂直地址模式(A[1:0]=01b)
    oled_write_cmd(0x8D);    //
    oled_write_cmd(0x14);    // Set Higher Column Start Address for Page Addressing Mode (10h~1Fh)
    oled_write_cmd(0xA4);    // Entire Display ON (A4h/A5h)
    oled_write_cmd(0xA6);    // Set Normal/Inverse Display (A6h/A7h)
    oled_write_cmd(0xAF);    // Set Display ON/OFF (AEh/AFh)
    return ESP_OK;
}

 esp_err_t oled_set_pos(uint8_t x_start, uint8_t y_start)
{
    oled_write_cmd(0xb0 + y_start);
    oled_write_cmd(((x_start & 0xf0) >> 4) | 0x10);
    //oled_write_cmd((x_start & 0x0f) | 0x01);
    oled_write_cmd((x_start & 0x0f));

    return ESP_OK;
}

 esp_err_t oled_set_pos_range(uint8_t minBoundX, uint8_t minBoundY,uint8_t maxBoundX, uint8_t maxBoundY)
{
    oled_write_cmd(0x21);//COLUMNADDR
    oled_write_cmd(minBoundX);
    oled_write_cmd(maxBoundX);

    oled_write_cmd(0x22);//PAGEADDR
    oled_write_cmd(minBoundY);
    oled_write_cmd(maxBoundY);

    return ESP_OK;
}

 esp_err_t oled_clear(uint8_t data)
{
    uint8_t x;
    uint32_t buf[16];
    spi_trans_t trans = {0};
    trans.mosi = buf;
    trans.bits.mosi = 64 * 8;

    for (x = 0; x < 16; x++) {
        buf[x] = data << 24 | data << 16 | data << 8 | data;
    }

    // SPI transfers 64 bytes at a time, transmits twice, increasing the screen refresh rate
    for (x = 0; x < 8; x++) {
        oled_set_pos(0, x);
        oled_set_dc(1);
        spi_trans(HSPI_HOST, trans);
        spi_trans(HSPI_HOST, trans);
    }

    return ESP_OK;
}

 void IRAM_ATTR spi_event_callback(int event, void *arg)
{
    switch (event) {
        case SPI_INIT_EVENT: {

        }
        break;

        case SPI_TRANS_START_EVENT: {
            gpio_set_level(OLED_DC_GPIO, oled_dc_level);
        }
        break;

        case SPI_TRANS_DONE_EVENT: {

        }
        break;

        case SPI_DEINIT_EVENT: {
        }
        break;
    }
}



esp_err_t oled_display_init()
{
    if(dispaly_image_buffer==NULL) 
    {
        dispaly_image_buffer = (uint8_t*) malloc(sizeof(uint8_t) * displayBufferSize);

        if(!dispaly_image_buffer) 
        {
            DEBUG_OLEDDISPLAY("[OLEDDISPLAY][init] Not enough memory to create display\n");
            return ESP_FAIL;
        }
        memset(dispaly_image_buffer, 0x00, displayBufferSize);
    }

#ifdef OLED_BACK_IMAGE_ENABLE
    if(dispaly_image_buffer_back==NULL) 
    {
        dispaly_image_buffer_back = (uint8_t*) malloc(sizeof(uint8_t) * displayBufferSize);

        if(!dispaly_image_buffer_back) 
        {
            DEBUG_OLEDDISPLAY("[OLEDDISPLAY][init] Not enough memory to create dispaly_image_buffer_back display\n");
            return ESP_FAIL;
        }
        memset(dispaly_image_buffer_back, 0xFF, displayBufferSize);
    }
#endif
    return ESP_OK;
}


void setPixel(int16_t x, int16_t y) {
  if (x >= 0 && x < displayWidth && y >= 0 && y < displayHeight) {
    switch (color) {
      case WHITE:   dispaly_image_buffer[x + (y / 8) * displayWidth] |=  (1 << (y & 7)); break;
      case BLACK:   dispaly_image_buffer[x + (y / 8) * displayWidth] &= ~(1 << (y & 7)); break;
      case INVERSE: dispaly_image_buffer[x + (y / 8) * displayWidth] ^=  (1 << (y & 7)); break;
    }
  }
}

// Bresenham's algorithm - thx wikipedia and Adafruit_GFX
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    _swap_int16_t(x0, y0);
    _swap_int16_t(x1, y1);
  }

  if (x0 > x1) {
    _swap_int16_t(x0, x1);
    _swap_int16_t(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0<=x1; x0++) {
    if (steep) {
      setPixel(y0, x0);
    } else {
      setPixel(x0, y0);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}


// Set 页地址模式(A[1:0]=10b) 
void display()
{

    if(dispaly_image_buffer==NULL)
    {
        DEBUG_OLEDDISPLAY("[OLEDDISPLAY][init] Not init display\n");
        return;
    }

#ifdef OLED_BACK_IMAGE_ENABLE

    if(dispaly_image_buffer_back==NULL)
    {
        DEBUG_OLEDDISPLAY("[OLEDDISPLAY][init] Not BACK init display\n");
        return;
    }
        
    uint8_t minBoundY = UINT8_MAX;
    uint8_t maxBoundY = 0;

    uint8_t minBoundX = UINT8_MAX;
    uint8_t maxBoundX = 0;

    uint8_t x, y;

    // Calculate the Y bounding box of changes
    // and copy buffer[pos] to buffer_back[pos];
    for (y = 0; y < (displayHeight / 8); y++) {
        for (x = 0; x < displayWidth; x++) {
          uint16_t pos = x + y * displayWidth;
          if (dispaly_image_buffer[pos] != dispaly_image_buffer_back[pos]) {
            minBoundY = _min(minBoundY, y);
            maxBoundY = _max(maxBoundY, y);
            minBoundX = _min(minBoundX, x);
            maxBoundX = _max(maxBoundX, x);
          }
          dispaly_image_buffer_back[pos] = dispaly_image_buffer[pos];
        }
    }

    // If the minBoundY wasn't updated
    // we can savely assume that buffer_back[pos] == buffer[pos]
    // holdes true for all values of pos
    if (minBoundY == UINT8_MAX) return;

    //    printf("minx:%d maxx:%d miny:%d maxy:%d\n",minBoundX,maxBoundX,minBoundY,maxBoundY);

    for (y = minBoundY; y <= maxBoundY; y++) {
            oled_set_pos(minBoundX,y);
        for (x = minBoundX; x <= maxBoundX; x++) {
             oled_write_byte(dispaly_image_buffer[x + y * displayWidth]);
        }
    }

#else

    uint8_t x=0,y=0;
    uint16_t pos=0;

    for(y=0;y<displayHeight/8;y++)
    {
        oled_set_pos(x,y);
        for(x=0;x<displayWidth;x++)
        {
            oled_write_byte(dispaly_image_buffer[pos++]);//x + y * displayWidth
        }
    }

#endif
}