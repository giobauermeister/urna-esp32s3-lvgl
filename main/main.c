#include <stdio.h>
#include <math.h>
#include <dirent.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/i2s_std.h"
#include "driver/i2s_types.h"
#include "driver/i2s_common.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "lcd_init.h"
#include "lvgl_init.h"
#include "i2s_init.h"
#include "spiffs.h"
#include "sound.h"
#include "ui.h"
#include "lvgl.h"

void app_main(void)
{
    printf("Hello World!\n");

    i2s_init();
    init_spiffs();
    // list_spiffs_files();

    // play_sound("/spiffs/urna_sound_long.wav");
    // play_sound("/spiffs/urna_sound_short.wav");

    init_lcd_panel();
    init_lvgl();

    // Create UI elements
    create_ui();
    
    vTaskDelay(pdMS_TO_TICKS(500));
    init_lcd_backlight();

    // Create a task for LVGL handling
    xTaskCreate(lvgl_timer_handler_task, "LVGL Task", 4096, NULL, 5, NULL);

    // Start handling LVGL tasks
    while (1) {
        // lv_timer_handler();     // Let LVGL handle UI updates
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay to avoid overload
    }
}
