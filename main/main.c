#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "display.h"
#include "screens.h"

static const char *TAG = "main";

/* Screen timing */
#define SPLASH_DURATION_MS  2000
#define SCREEN_CYCLE_MS     3000

void app_main(void)
{
    ESP_LOGI(TAG, "Wardriver starting...");

    /* Initialize display */
    esp_err_t ret = display_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Display initialization failed!");
        return;
    }
    ESP_LOGI(TAG, "Display initialized");

    /* Show splash screen for 2 seconds */
    ESP_LOGI(TAG, "Showing splash screen");
    screen_splash();
    vTaskDelay(pdMS_TO_TICKS(SPLASH_DURATION_MS));

    /* Screen cycling state machine */
    typedef enum {
        SCREEN_DASHBOARD = 0,
        SCREEN_RADAR,
        SCREEN_SETTINGS,
        SCREEN_COUNT
    } screen_id_t;

    screen_id_t current_screen = SCREEN_DASHBOARD;

    /* Screen function table */
    void (*screen_funcs[])(void) = {
        screen_dashboard,
        screen_radar,
        screen_settings
    };

    ESP_LOGI(TAG, "Starting screen cycle");

    /* Main loop - cycle through screens */
    while (1) {
        /* Render current screen */
        screen_funcs[current_screen]();

        /* Wait before next screen */
        vTaskDelay(pdMS_TO_TICKS(SCREEN_CYCLE_MS));

        /* Advance to next screen */
        current_screen = (screen_id_t)((current_screen + 1) % SCREEN_COUNT);
    }
}
