#include "screens.h"
#include "display.h"
#include <stdio.h>
#include <math.h>

/* ───────────────────────────────────────────────────────────────────────────
 * Splash Screen
 * ─────────────────────────────────────────────────────────────────────────── */

void screen_splash(void)
{
    /* Clear to black */
    display_fill(COLOR_BLACK);

    /* Device name - large, centered at top third */
    display_draw_string_centered(80, "WARDRIVER", COLOR_WHITE, COLOR_BLACK);

    /* Subtitle */
    display_draw_string_centered(120, "Initializing...", COLOR_LIGHTGREY, COLOR_BLACK);

    /* Loading bar background */
    uint16_t bar_x = 40;
    uint16_t bar_y = 200;
    uint16_t bar_w = 160;
    uint16_t bar_h = 12;

    display_fill_rect(bar_x, bar_y, bar_w, bar_h, COLOR_DARKGREY);

    /* Loading bar fill (dummy - static 60% for now) */
    uint16_t fill_w = (bar_w - 4) * 60 / 100;
    display_fill_rect(bar_x + 2, bar_y + 2, fill_w, bar_h - 4, COLOR_GREEN);

    /* Loading bar border */
    display_draw_rect(bar_x, bar_y, bar_w, bar_h, COLOR_WHITE);
}

/* ───────────────────────────────────────────────────────────────────────────
 * Dashboard Screen
 * ─────────────────────────────────────────────────────────────────────────── */

void screen_dashboard(void)
{
    /* Clear to dark blue background */
    display_fill(COLOR_NAVY);

    /* Top bar: stage indicator + device count */
    display_fill_rect(0, 0, DISPLAY_WIDTH, 40, COLOR_DARKCYAN);

    /* Stage indicator: "BT_SCAN" with LED dot */
    display_fill_circle(20, 20, 6, COLOR_RED);  /* LED dot */
    display_fill_circle(20, 20, 4, COLOR_RED);  /* Inner LED */
    display_draw_string(35, 16, "BT_SCAN", COLOR_WHITE, COLOR_DARKCYAN);

    /* Device count on right */
    char count_str[8];
    snprintf(count_str, sizeof(count_str), "[%02d]", 12);  /* Dummy: 12 devices */
    display_draw_string(DISPLAY_WIDTH - 50, 16, count_str, COLOR_WHITE, COLOR_DARKCYAN);

    /* RSSI section */
    display_draw_string(10, 60, "RSSI:", COLOR_WHITE, COLOR_NAVY);

    /* Signal bar background */
    uint16_t rssi_x = 70;
    uint16_t rssi_y = 58;
    uint16_t rssi_w = 100;
    uint16_t rssi_h = 14;
    display_fill_rect(rssi_x, rssi_y, rssi_w, rssi_h, COLOR_DARKGREY);

    /* Signal bar fill (dummy: -67dBm = ~60%) */
    uint16_t rssi_fill = (rssi_w - 4) * 60 / 100;
    display_fill_rect(rssi_x + 2, rssi_y + 2, rssi_fill, rssi_h - 4, COLOR_GREEN);

    /* RSSI value */
    display_draw_string(rssi_x + rssi_w + 10, rssi_y + 3, "-67dBm", COLOR_WHITE, COLOR_NAVY);

    /* GPS placeholder */
    display_draw_string(10, 100, "GPS:", COLOR_WHITE, COLOR_NAVY);
    display_draw_string(60, 100, "--:-- / --:--", COLOR_LIGHTGREY, COLOR_NAVY);

    /* Heading placeholder */
    display_draw_string(10, 130, "HDG:", COLOR_WHITE, COLOR_NAVY);
    display_draw_string(60, 130, "---", COLOR_LIGHTGREY, COLOR_NAVY);
}

/* ───────────────────────────────────────────────────────────────────────────
 * Radar / Direction Screen
 * ─────────────────────────────────────────────────────────────────────────── */

/* Helper: Draw a line from center at a given angle */
static void draw_radial_line(int16_t cx, int16_t cy, uint16_t radius, float angle_deg, uint16_t color)
{
    float angle_rad = angle_deg * (3.14159f / 180.0f);
    int16_t x1 = cx + (int16_t)(radius * cosf(angle_rad));
    int16_t y1 = cy - (int16_t)(radius * sinf(angle_rad));  /* Y is inverted on display */
    display_draw_line(cx, cy, x1, y1, color);
}

void screen_radar(void)
{
    /* Clear to black */
    display_fill(COLOR_BLACK);

    int16_t cx = DISPLAY_WIDTH / 2;
    int16_t cy = DISPLAY_HEIGHT / 2;
    uint16_t outer_radius = 100;

    /* Draw compass rose - 8 cardinal directions */
    for (int i = 0; i < 8; i++) {
        float angle = i * 45.0f;
        draw_radial_line(cx, cy, outer_radius, angle, COLOR_DARKGREY);
    }

    /* Draw concentric signal strength rings */
    display_draw_circle(cx, cy, outer_radius, COLOR_DARKGREY);
    display_draw_circle(cx, cy, outer_radius * 2 / 3, COLOR_DARKGREY);
    display_draw_circle(cx, cy, outer_radius / 3, COLOR_DARKGREY);

    /* Center dot (represents user/device) */
    display_fill_circle(cx, cy, 6, COLOR_BLUE);
    display_fill_circle(cx, cy, 3, COLOR_CYAN);

    /* Draw directional arrow pointing to strongest signal (dummy: 120 degrees) */
    float arrow_angle = 120.0f;
    float arrow_rad = arrow_angle * (3.14159f / 180.0f);

    /* Arrow shaft */
    int16_t arrow_len = outer_radius - 20;
    int16_t arrow_tip_x = cx + (int16_t)(arrow_len * cosf(arrow_rad));
    int16_t arrow_tip_y = cy - (int16_t)(arrow_len * sinf(arrow_rad));
    display_draw_line(cx, cy, arrow_tip_x, arrow_tip_y, COLOR_RED);

    /* Arrow head */
    float head_offset = 15.0f;
    float head_angle_left = arrow_angle + 150.0f;
    float head_angle_right = arrow_angle - 150.0f;
    int16_t head_left_x = arrow_tip_x + (int16_t)(head_offset * cosf(head_angle_left * 3.14159f / 180.0f));
    int16_t head_left_y = arrow_tip_y - (int16_t)(head_offset * sinf(head_angle_left * 3.14159f / 180.0f));
    int16_t head_right_x = arrow_tip_x + (int16_t)(head_offset * cosf(head_angle_right * 3.14159f / 180.0f));
    int16_t head_right_y = arrow_tip_y - (int16_t)(head_offset * sinf(head_angle_right * 3.14159f / 180.0f));

    display_draw_line(arrow_tip_x, arrow_tip_y, head_left_x, head_left_y, COLOR_RED);
    display_draw_line(arrow_tip_x, arrow_tip_y, head_right_x, head_right_y, COLOR_RED);

    /* Signal strength label near arrowhead */
    display_draw_string(arrow_tip_x + 5, arrow_tip_y - 10, "12", COLOR_YELLOW, COLOR_BLACK);

    /* Heading degrees at top */
    char heading_str[16];
    snprintf(heading_str, sizeof(heading_str), "NE  %03d", 45);  /* Dummy heading: 45 degrees */
    display_draw_string_centered(10, heading_str, COLOR_WHITE, COLOR_BLACK);
}

/* ───────────────────────────────────────────────────────────────────────────
 * Settings Screen
 * ─────────────────────────────────────────────────────────────────────────── */

void screen_settings(void)
{
    /* Clear to dark background */
    display_fill(COLOR_NAVY);

    /* Title */
    display_draw_string_centered(10, "SETTINGS", COLOR_WHITE, COLOR_NAVY);
    display_draw_hline(10, 28, DISPLAY_WIDTH - 20, COLOR_DARKCYAN);

    /* Settings list */
    const char *settings[] = {
        "Scan Mode      : BLE",
        "Display Mode   : Radar",
        "Brightness     : High",
        "Sound          : ON",
        "Filter RSSI    : -80dBm"
    };
    int num_settings = sizeof(settings) / sizeof(settings[0]);
    int selected = 0;  /* Dummy: always first item selected */

    int16_t start_y = 50;
    int16_t line_height = 24;

    for (int i = 0; i < num_settings; i++) {
        int16_t y = start_y + i * line_height;

        if (i == selected) {
            /* Highlight selected item */
            display_fill_rect(0, y - 8, DISPLAY_WIDTH, line_height, COLOR_DARKCYAN);

            /* Cursor arrow */
            display_draw_string(5, y, ">", COLOR_YELLOW, COLOR_DARKCYAN);
            display_draw_string(20, y, settings[i], COLOR_WHITE, COLOR_DARKCYAN);
        } else {
            /* Normal item */
            display_draw_string(20, y, settings[i], COLOR_LIGHTGREY, COLOR_NAVY);
        }
    }

    /* Hint at bottom */
    display_draw_string_centered(DISPLAY_HEIGHT - 20, "Use UP/DOWN to navigate", COLOR_DARKGREY, COLOR_NAVY);
}
