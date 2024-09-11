#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "lcd_init.h"
#include "lvgl_init.h"
#include "ui.h"
#include "lvgl.h"

void app_main(void)
{
    printf("Hello World!\n");

    init_lcd_panel();
    init_backlight();
    init_lvgl();

    // Create UI elements
    create_ui();

    // Start handling LVGL tasks
    while (1) {
        lv_timer_handler();     // Let LVGL handle UI updates
        vTaskDelay(pdMS_TO_TICKS(10));  // Delay to avoid overload
    }
}
