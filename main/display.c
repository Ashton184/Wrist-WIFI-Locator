#include "display.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "display";

/* SPI device handle */
static spi_device_handle_t spi;

/* Backlight PWM channel */
#define LEDC_CHANNEL      LEDC_CHANNEL_0
#define LEDC_FREQ_HZ      5000
#define LEDC_DUTY_RES     LEDC_TIMER_10_BIT
#define LEDC_TIMER        LEDC_TIMER_0

/* ───────────────────────────────────────────────────────────────────────────
 * Low-level SPI communication
 * ─────────────────────────────────────────────────────────────────────────── */

/**
 * Send a command byte to the ILI9341
 */
static void ili9341_write_command(uint8_t cmd)
{
    gpio_set_level(PIN_NUM_DC, 0);  // Command mode
    spi_transaction_t t = {
        .flags = SPI_TRANS_USE_TXDATA,
        .length = 8,
        .tx_data[0] = cmd
    };
    spi_device_transmit(spi, &t);
}

/**
 * Send data bytes to the ILI9341
 */
static void ili9341_write_data(const uint8_t *data, size_t len)
{
    gpio_set_level(PIN_NUM_DC, 1);  // Data mode
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data
    };
    spi_device_transmit(spi, &t);
}

/**
 * Send a command with parameters
 */
static void ili9341_write_command_data(uint8_t cmd, const uint8_t *data, size_t len)
{
    ili9341_write_command(cmd);
    ili9341_write_data(data, len);
}

/* ───────────────────────────────────────────────────────────────────────────
 * 8x8 Bitmap Font (basic ASCII)
 * ─────────────────────────────────────────────────────────────────────────── */

static const uint8_t font_8x8[96][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // space 0x20
    {0x00,0x00,0x5F,0x00,0x00,0x00,0x00,0x00}, // !
    {0x00,0x07,0x00,0x07,0x00,0x00,0x00,0x00}, // "
    {0x14,0x7F,0x14,0x7F,0x14,0x00,0x00,0x00}, // #
    {0x24,0x2A,0x7F,0x2A,0x12,0x00,0x00,0x00}, // $
    {0x23,0x13,0x08,0x64,0x62,0x00,0x00,0x00}, // %
    {0x36,0x49,0x56,0x20,0x50,0x00,0x00,0x00}, // &
    {0x00,0x05,0x03,0x00,0x00,0x00,0x00,0x00}, // '
    {0x00,0x1C,0x22,0x41,0x00,0x00,0x00,0x00}, // (
    {0x00,0x41,0x22,0x1C,0x00,0x00,0x00,0x00}, // )
    {0x14,0x08,0x3E,0x08,0x14,0x00,0x00,0x00}, // *
    {0x08,0x08,0x3E,0x08,0x08,0x00,0x00,0x00}, // +
    {0x00,0x50,0x30,0x00,0x00,0x00,0x00,0x00}, // ,
    {0x10,0x10,0x10,0x10,0x10,0x00,0x00,0x00}, // -
    {0x00,0x60,0x60,0x00,0x00,0x00,0x00,0x00}, // .
    {0x20,0x10,0x08,0x04,0x02,0x00,0x00,0x00}, // /
    {0x3E,0x51,0x49,0x45,0x3E,0x00,0x00,0x00}, // 0
    {0x00,0x42,0x7F,0x40,0x00,0x00,0x00,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46,0x00,0x00,0x00}, // 2
    {0x21,0x41,0x45,0x4B,0x31,0x00,0x00,0x00}, // 3
    {0x18,0x14,0x12,0x7F,0x10,0x00,0x00,0x00}, // 4
    {0x27,0x45,0x45,0x45,0x39,0x00,0x00,0x00}, // 5
    {0x3C,0x4A,0x49,0x49,0x30,0x00,0x00,0x00}, // 6
    {0x01,0x71,0x09,0x05,0x03,0x00,0x00,0x00}, // 7
    {0x36,0x49,0x49,0x49,0x36,0x00,0x00,0x00}, // 8
    {0x06,0x49,0x49,0x29,0x1E,0x00,0x00,0x00}, // 9
    {0x00,0x36,0x36,0x00,0x00,0x00,0x00,0x00}, // :
    {0x00,0x56,0x36,0x00,0x00,0x00,0x00,0x00}, // ;
    {0x08,0x14,0x22,0x41,0x00,0x00,0x00,0x00}, // <
    {0x14,0x14,0x14,0x14,0x14,0x00,0x00,0x00}, // =
    {0x00,0x41,0x22,0x14,0x08,0x00,0x00,0x00}, // >
    {0x02,0x01,0x51,0x09,0x06,0x00,0x00,0x00}, // ?
    {0x32,0x49,0x79,0x41,0x3E,0x00,0x00,0x00}, // @
    {0x7E,0x11,0x11,0x11,0x7E,0x00,0x00,0x00}, // A
    {0x7F,0x49,0x49,0x49,0x36,0x00,0x00,0x00}, // B
    {0x3E,0x41,0x41,0x41,0x22,0x00,0x00,0x00}, // C
    {0x7F,0x41,0x41,0x22,0x1C,0x00,0x00,0x00}, // D
    {0x7F,0x49,0x49,0x49,0x41,0x00,0x00,0x00}, // E
    {0x7F,0x09,0x09,0x09,0x01,0x00,0x00,0x00}, // F
    {0x3E,0x41,0x49,0x49,0x7A,0x00,0x00,0x00}, // G
    {0x7F,0x08,0x08,0x08,0x7F,0x00,0x00,0x00}, // H
    {0x00,0x41,0x7F,0x41,0x00,0x00,0x00,0x00}, // I
    {0x20,0x40,0x41,0x3F,0x01,0x00,0x00,0x00}, // J
    {0x7F,0x08,0x14,0x22,0x41,0x00,0x00,0x00}, // K
    {0x7F,0x40,0x40,0x40,0x40,0x00,0x00,0x00}, // L
    {0x7F,0x02,0x0C,0x02,0x7F,0x00,0x00,0x00}, // M
    {0x7F,0x04,0x08,0x10,0x7F,0x00,0x00,0x00}, // N
    {0x3E,0x41,0x41,0x41,0x3E,0x00,0x00,0x00}, // O
    {0x7F,0x09,0x09,0x09,0x06,0x00,0x00,0x00}, // P
    {0x3E,0x41,0x51,0x21,0x5E,0x00,0x00,0x00}, // Q
    {0x7F,0x09,0x19,0x29,0x46,0x00,0x00,0x00}, // R
    {0x46,0x49,0x49,0x49,0x31,0x00,0x00,0x00}, // S
    {0x01,0x01,0x7F,0x01,0x01,0x00,0x00,0x00}, // T
    {0x3F,0x40,0x40,0x40,0x3F,0x00,0x00,0x00}, // U
    {0x1F,0x20,0x40,0x20,0x1F,0x00,0x00,0x00}, // V
    {0x3F,0x40,0x38,0x40,0x3F,0x00,0x00,0x00}, // W
    {0x63,0x14,0x08,0x14,0x63,0x00,0x00,0x00}, // X
    {0x07,0x08,0x70,0x08,0x07,0x00,0x00,0x00}, // Y
    {0x61,0x51,0x49,0x45,0x43,0x00,0x00,0x00}, // Z
    {0x00,0x7F,0x41,0x41,0x00,0x00,0x00,0x00}, // [
    {0x02,0x04,0x08,0x10,0x20,0x00,0x00,0x00}, // backslash
    {0x00,0x41,0x41,0x7F,0x00,0x00,0x00,0x00}, // ]
    {0x04,0x02,0x01,0x02,0x04,0x00,0x00,0x00}, // ^
    {0x40,0x40,0x40,0x40,0x40,0x00,0x00,0x00}, // _
    {0x00,0x01,0x02,0x04,0x00,0x00,0x00,0x00}, // `
    {0x20,0x54,0x54,0x54,0x78,0x00,0x00,0x00}, // a
    {0x7F,0x48,0x44,0x44,0x38,0x00,0x00,0x00}, // b
    {0x38,0x44,0x44,0x44,0x20,0x00,0x00,0x00}, // c
    {0x38,0x44,0x44,0x48,0x7F,0x00,0x00,0x00}, // d
    {0x38,0x54,0x54,0x54,0x18,0x00,0x00,0x00}, // e
    {0x08,0x7E,0x09,0x01,0x02,0x00,0x00,0x00}, // f
    {0x0C,0x52,0x52,0x52,0x3E,0x00,0x00,0x00}, // g
    {0x7F,0x08,0x04,0x04,0x78,0x00,0x00,0x00}, // h
    {0x00,0x44,0x7D,0x40,0x00,0x00,0x00,0x00}, // i
    {0x20,0x40,0x44,0x3D,0x00,0x00,0x00,0x00}, // j
    {0x7F,0x10,0x28,0x44,0x00,0x00,0x00,0x00}, // k
    {0x00,0x41,0x7F,0x40,0x00,0x00,0x00,0x00}, // l
    {0x7C,0x04,0x18,0x04,0x78,0x00,0x00,0x00}, // m
    {0x7C,0x08,0x04,0x04,0x78,0x00,0x00,0x00}, // n
    {0x38,0x44,0x44,0x44,0x38,0x00,0x00,0x00}, // o
    {0x7C,0x14,0x14,0x14,0x08,0x00,0x00,0x00}, // p
    {0x08,0x14,0x14,0x18,0x7C,0x00,0x00,0x00}, // q
    {0x7C,0x08,0x04,0x04,0x08,0x00,0x00,0x00}, // r
    {0x48,0x54,0x54,0x54,0x20,0x00,0x00,0x00}, // s
    {0x04,0x3F,0x44,0x40,0x20,0x00,0x00,0x00}, // t
    {0x3C,0x40,0x40,0x20,0x7C,0x00,0x00,0x00}, // u
    {0x1C,0x20,0x40,0x20,0x1C,0x00,0x00,0x00}, // v
    {0x3C,0x40,0x30,0x40,0x3C,0x00,0x00,0x00}, // w
    {0x44,0x28,0x10,0x28,0x44,0x00,0x00,0x00}, // x
    {0x0C,0x50,0x50,0x50,0x3C,0x00,0x00,0x00}, // y
    {0x44,0x64,0x54,0x4C,0x44,0x00,0x00,0x00}, // z
    {0x14,0x14,0x14,0x14,0x14,0x00,0x00,0x00}, // horiz lines
};

/* ───────────────────────────────────────────────────────────────────────────
 * Display initialization
 * ─────────────────────────────────────────────────────────────────────────── */

esp_err_t display_init(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing display");

    /* Configure GPIO pins */
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST)
    };
    gpio_config(&io_conf);

    /* Reset the display */
    gpio_set_level(PIN_NUM_RST, 0);
    esp_rom_delay_us(10000);
    gpio_set_level(PIN_NUM_RST, 1);
    esp_rom_delay_us(10000);

    /* Configure SPI */
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40 * 1000 * 1000,  // 40 MHz
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 7,
        .pre_cb = NULL
    };

    ret = spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    ret = spi_bus_add_device(SPI_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    /* Configure backlight PWM */
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = PIN_NUM_BCKL,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);

    /* ILI9341 initialization sequence */
    ESP_LOGI(TAG, "Initializing ILI9341");

    ili9341_write_command(0xEF);  // Inter Register Enable 2
    ili9341_write_data((uint8_t[]){0x03, 0x80, 0x02}, 3);

    ili9341_write_command(0xCF);  // Power Control B
    ili9341_write_data((uint8_t[]){0x00, 0xC1, 0x30}, 3);

    ili9341_write_command(0xED);  // Power On Sequence
    ili9341_write_data((uint8_t[]){0x64, 0x03, 0x12, 0x81}, 4);

    ili9341_write_command(0xE8);  // Driver Timing Control A
    ili9341_write_data((uint8_t[]){0x85, 0x00, 0x78}, 3);

    ili9341_write_command(0xCB);  // Power Control A
    ili9341_write_data((uint8_t[]){0x39, 0x2C, 0x00, 0x34, 0x02}, 5);

    ili9341_write_command(0xF7);  // Pump Ratio Control
    ili9341_write_data((uint8_t[]){0x20}, 1);

    ili9341_write_command(0xEA);  // Driver Timing Control B
    ili9341_write_data((uint8_t[]){0x00, 0x00}, 2);

    ili9341_write_command(0xC0);  // Power Control 1
    ili9341_write_data((uint8_t[]){0x23}, 1);

    ili9341_write_command(0xC1);  // Power Control 2
    ili9341_write_data((uint8_t[]){0x10}, 1);

    ili9341_write_command(0xC5);  // VCOM Control 1
    ili9341_write_data((uint8_t[]){0x3E, 0x28}, 2);

    ili9341_write_command(0xC7);  // VCOM Control 2
    ili9341_write_data((uint8_t[]){0x86}, 1);

    ili9341_write_command(0x36);  // Memory Access Control
    ili9341_write_data((uint8_t[]){MADCTL_MX | MADCTL_BGR}, 1);  // Portrait, BGR

    ili9341_write_command(0x3A);  // Pixel Format
    ili9341_write_data((uint8_t[]){0x55}, 1);  // 16-bit

    ili9341_write_command(0xB1);  // Frame Rate Control
    ili9341_write_data((uint8_t[]){0x00, 0x18}, 2);

    ili9341_write_command(0xB6);  // Display Function Control
    ili9341_write_data((uint8_t[]){0x08, 0x82, 0x27}, 3);

    ili9341_write_command(0xF2);  // 3Gamma Function Disable
    ili9341_write_data((uint8_t[]){0x00}, 1);

    ili9341_write_command(0x26);  // Gamma Curve Selected
    ili9341_write_data((uint8_t[]){0x01}, 1);

    ili9341_write_command(0xE0);  // Positive Gamma Correction
    ili9341_write_data((uint8_t[]){
        0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
        0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00
    }, 15);

    ili9341_write_command(0xE1);  // Negative Gamma Correction
    ili9341_write_data((uint8_t[]){
        0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
        0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F
    }, 15);

    ili9341_write_command(0x11);  // Sleep Out
    esp_rom_delay_us(120000);

    ili9341_write_command(0x29);  // Display On

    /* Set default brightness to 80% */
    display_set_brightness(80);

    ESP_LOGI(TAG, "Display initialized");
    return ESP_OK;
}

/* ───────────────────────────────────────────────────────────────────────────
 * Drawing primitives
 * ─────────────────────────────────────────────────────────────────────────── */

void display_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    /* Set column address */
    ili9341_write_command(ILI9341_CASET);
    ili9341_write_data((uint8_t[]){
        (x0 >> 8) & 0xFF, x0 & 0xFF,
        (x1 >> 8) & 0xFF, x1 & 0xFF
    }, 4);

    /* Set row address */
    ili9341_write_command(ILI9341_PASET);
    ili9341_write_data((uint8_t[]){
        (y0 >> 8) & 0xFF, y0 & 0xFF,
        (y1 >> 8) & 0xFF, y1 & 0xFF
    }, 4);

    /* Write to RAM */
    ili9341_write_command(ILI9341_RAMWR);
}

void display_fill(uint16_t color)
{
    display_set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);

    /* Prepare color buffer */
    uint16_t *buffer = malloc(DISPLAY_WIDTH * 2 * sizeof(uint16_t));
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate fill buffer");
        return;
    }

    for (int i = 0; i < DISPLAY_WIDTH; i++) {
        buffer[i] = color;
    }

    gpio_set_level(PIN_NUM_DC, 1);  // Data mode

    /* Send buffer in chunks */
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        spi_transaction_t t = {
            .length = DISPLAY_WIDTH * 16,
            .tx_buffer = buffer
        };
        spi_device_transmit(spi, &t);
    }

    free(buffer);
}

void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) {
        return;
    }

    display_set_window(x, y, x, y);

    uint8_t data[2] = {
        (color >> 8) & 0xFF,
        color & 0xFF
    };
    ili9341_write_data(data, 2);
}

void display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT || w == 0 || h == 0) {
        return;
    }

    /* Clip to screen */
    if (x + w > DISPLAY_WIDTH) w = DISPLAY_WIDTH - x;
    if (y + h > DISPLAY_HEIGHT) h = DISPLAY_HEIGHT - y;

    display_set_window(x, y, x + w - 1, y + h - 1);

    uint16_t *buffer = malloc(w * 2 * sizeof(uint16_t));
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate fill rect buffer");
        return;
    }

    for (int i = 0; i < w; i++) {
        buffer[i] = color;
    }

    gpio_set_level(PIN_NUM_DC, 1);

    for (int row = 0; row < h; row++) {
        spi_transaction_t t = {
            .length = w * 16,
            .tx_buffer = buffer
        };
        spi_device_transmit(spi, &t);
    }

    free(buffer);
}

void display_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color)
{
    display_fill_rect(x, y, w, 1, color);
}

void display_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color)
{
    display_fill_rect(x, y, 1, h, color);
}

void display_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;

    while (1) {
        display_draw_pixel(x0, y0, color);

        if (x0 == x1 && y0 == y1) break;

        int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void display_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    display_draw_hline(x, y, w, color);
    display_draw_hline(x, y + h - 1, w, color);
    display_draw_vline(x, y, h, color);
    display_draw_vline(x + w - 1, y, h, color);
}

void display_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    /* Draw vertical diameter */
    display_draw_vline(x0, y0 - r, 2 * r + 1, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        display_draw_vline(x0 + x, y0 - y, 2 * y + 1, color);
        display_draw_vline(x0 - x, y0 - y, 2 * y + 1, color);
        display_draw_vline(x0 + y, y0 - x, 2 * x + 1, color);
        display_draw_vline(x0 - y, y0 - x, 2 * x + 1, color);
    }
}

void display_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    display_draw_pixel(x0, y0 + r, color);
    display_draw_pixel(x0, y0 - r, color);
    display_draw_pixel(x0 + r, y0, color);
    display_draw_pixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        display_draw_pixel(x0 + x, y0 + y, color);
        display_draw_pixel(x0 - x, y0 + y, color);
        display_draw_pixel(x0 + x, y0 - y, color);
        display_draw_pixel(x0 - x, y0 - y, color);
        display_draw_pixel(x0 + y, y0 + x, color);
        display_draw_pixel(x0 - y, y0 + x, color);
        display_draw_pixel(x0 + y, y0 - x, color);
        display_draw_pixel(x0 - y, y0 - x, color);
    }
}

/* ───────────────────────────────────────────────────────────────────────────
 * Text rendering
 * ─────────────────────────────────────────────────────────────────────────── */

void display_draw_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg)
{
    if (c < 0x20 || c > 0x7F) {
        c = 0x20;  // Replace with space
    }

    const uint8_t *glyph = font_8x8[c - 0x20];

    for (int row = 0; row < 8; row++) {
        uint8_t line = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (line & 0x80) {
                display_draw_pixel(x + col, y + row, color);
            } else {
                display_draw_pixel(x + col, y + row, bg);
            }
            line <<= 1;
        }
    }
}

void display_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg)
{
    if (!str) return;

    while (*str) {
        display_draw_char(x, y, *str, color, bg);
        x += 8;
        str++;
    }
}

void display_draw_string_centered(uint16_t y, const char *str, uint16_t color, uint16_t bg)
{
    if (!str) return;

    size_t len = strlen(str);
    int16_t x = (DISPLAY_WIDTH - (len * 8)) / 2;
    if (x < 0) x = 0;

    display_draw_string(x, y, str, color, bg);
}

/* ───────────────────────────────────────────────────────────────────────────
 * Utility functions
 * ─────────────────────────────────────────────────────────────────────────── */

void display_set_brightness(uint8_t brightness)
{
    if (brightness > 100) brightness = 100;

    uint32_t duty = ((1 << LEDC_DUTY_RES) - 1) * brightness / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
}

uint16_t display_get_width(void)
{
    return DISPLAY_WIDTH;
}

uint16_t display_get_height(void)
{
    return DISPLAY_HEIGHT;
}
