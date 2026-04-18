#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/spi_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ───────────────────────────────────────────────────────────────────────────
 * Display Configuration
 * ─────────────────────────────────────────────────────────────────────────── */

#define DISPLAY_WIDTH   240
#define DISPLAY_HEIGHT  320

/* SPI Configuration - ESP32-S3-DevKitC-1 defaults */
#define SPI_HOST        SPI2_HOST
#define PIN_NUM_MISO    11
#define PIN_NUM_MOSI    10
#define PIN_NUM_CLK     12
#define PIN_NUM_CS      13
#define PIN_NUM_DC      14
#define PIN_NUM_RST     15
#define PIN_NUM_BCKL    16

/* ILI9341 Commands */
#define ILI9341_NOP         0x00
#define ILI9341_SWRESET     0x01
#define ILI9341_RDDID       0x04
#define ILI9341_RDDST       0x09
#define ILI9341_SLPIN       0x10
#define ILI9341_SLPOUT      0x11
#define ILI9341_PTLON       0x12
#define ILI9341_NORON       0x13
#define ILI9341_INVOFF      0x20
#define ILI9341_INVON       0x21
#define ILI9341_GAMMASET    0x26
#define ILI9341_DISPOFF     0x28
#define ILI9341_DISPON      0x29
#define ILI9341_CASET       0x2A
#define ILI9341_PASET       0x2B
#define ILI9341_RAMWR       0x2C
#define ILI9341_COLMOD      0x3A
#define ILI9341_MADCTL      0x36
#define ILI9341_FRMCTR1     0xB1
#define ILI9341_DFUNCTR     0xB9
#define ILI9341_PWCTR1      0xC0
#define ILI9341_PWCTR2      0xC1
#define ILI9341_VMCTR1      0xC5
#define ILI9341_VMCTR2      0xC7
#define ILI9341_GMCTRP1     0xE0
#define ILI9341_GMCTRN1     0xE1
#define ILI9341_EXCTR       0xF0

/* MADCTL bits */
#define MADCTL_MY   0x80
#define MADCTL_MX   0x40
#define MADCTL_MV   0x20
#define MADCTL_ML   0x10
#define MADCTL_RGB  0x00
#define MADCTL_BGR  0x08

/* Colors - RGB565 format */
#define COLOR_BLACK       0x0000
#define COLOR_NAVY        0x000F
#define COLOR_DARKGREEN   0x03E0
#define COLOR_DARKCYAN    0x03EF
#define COLOR_MAROON      0x7800
#define COLOR_PURPLE      0x780F
#define COLOR_OLIVE       0x7BE0
#define COLOR_LIGHTGREY   0xC618
#define COLOR_DARKGREY    0x7BEF
#define COLOR_BLUE        0x001F
#define COLOR_GREEN       0x07E0
#define COLOR_CYAN        0x07FF
#define COLOR_RED         0xF800
#define COLOR_MAGENTA     0xF81F
#define COLOR_YELLOW      0xFFE0
#define COLOR_WHITE       0xFFFF
#define COLOR_ORANGE      0xFD20

/* ───────────────────────────────────────────────────────────────────────────
 * Public API
 * ─────────────────────────────────────────────────────────────────────────── */

/**
 * Initialize the display hardware and ILI9341 controller
 * @return ESP_OK on success
 */
esp_err_t display_init(void);

/**
 * Clear the entire screen to a solid color
 * @param color RGB565 color value
 */
void display_fill(uint16_t color);

/**
 * Set the active drawing region
 * @param x0 Left column
 * @param y0 Top row
 * @param x1 Right column
 * @param y1 Bottom row
 */
void display_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/**
 * Draw a single pixel
 * @param x Column (0-239)
 * @param y Row (0-319)
 * @param color RGB565 color
 */
void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

/**
 * Fill a rectangle with solid color
 * @param x Top-left column
 * @param y Top-left row
 * @param w Width in pixels
 * @param h Height in pixels
 * @param color RGB565 color
 */
void display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * Draw a horizontal line
 * @param x Starting column
 * @param y Row
 * @param w Length in pixels
 * @param color RGB565 color
 */
void display_draw_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color);

/**
 * Draw a vertical line
 * @param x Column
 * @param y Starting row
 * @param h Height in pixels
 * @param color RGB565 color
 */
void display_draw_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color);

/**
 * Draw a line using Bresenham's algorithm
 * @param x0 Start column
 * @param y0 Start row
 * @param x1 End column
 * @param y1 End row
 * @param color RGB565 color
 */
void display_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

/**
 * Draw a rectangle outline
 * @param x Top-left column
 * @param y Top-left row
 * @param w Width
 * @param h Height
 * @param color RGB565 color
 */
void display_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * Draw a filled circle
 * @param x0 Center column
 * @param y0 Center row
 * @param r Radius
 * @param color RGB565 color
 */
void display_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

/**
 * Draw a circle outline
 * @param x0 Center column
 * @param y0 Center row
 * @param r Radius
 * @param color RGB565 color
 */
void display_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

/**
 * Draw a character using 8x8 bitmap font
 * @param x Column
 * @param y Row
 * @param c Character to draw
 * @param color Foreground color
 * @param bg Background color
 */
void display_draw_char(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg);

/**
 * Draw a string using 8x8 bitmap font
 * @param x Starting column
 * @param y Starting row
 * @param str Null-terminated string
 * @param color Foreground color
 * @param bg Background color
 */
void display_draw_string(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg);

/**
 * Draw a string centered horizontally
 * @param y Row
 * @param str Null-terminated string
 * @param color Foreground color
 * @param bg Background color
 */
void display_draw_string_centered(uint16_t y, const char *str, uint16_t color, uint16_t bg);

/**
 * Set display backlight brightness (0-100)
 * @param brightness Percentage 0-100
 */
void display_set_brightness(uint8_t brightness);

/**
 * Get the display width
 * @return Width in pixels
 */
uint16_t display_get_width(void);

/**
 * Get the display height
 * @return Height in pixels
 */
uint16_t display_get_height(void);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H */
