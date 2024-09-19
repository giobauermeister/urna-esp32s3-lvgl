#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include "ui.h"
#include "lcd_init.h"
#include "lvgl_init.h"
#include "i2s_init.h"
#include "i2c_init.h"
#include "spiffs.h"
#include "sd_init.h"
#include "sound.h"
#include "keypad.h"
#include "candidate.h"

Candidate candidate1 = {
    .name = "Giovanni",
    .number = "1234",
    .role = "Vereador",
    .party_id = 1001,
};

Party party1 = {
    .name = "Partido1",
};

void app_main(void)
{
    printf("Hello World!\n");

    i2s_init();
    i2c_master_init();
    init_sd_card();

    add_candidate(candidate1);
    add_party(party1);

    // delete_candidate_by_id(1);
    // delete_candidate_by_id(2);
    // delete_candidate_by_id(3);
    // delete_candidate_by_id(5);
    
    init_spiffs();
    // list_spiffs_files();

    play_sound("/spiffs/urna_sound_long.wav");
    // play_sound("/spiffs/urna_sound_short.wav");

    init_lcd_panel();
    init_lvgl();

    // Create UI elements
    create_ui();

    create_keypad_interrupt_task();
    
    vTaskDelay(pdMS_TO_TICKS(500));
    init_lcd_backlight();

    create_lvgl_task();

    // Start handling LVGL tasks
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay to avoid overload
    }
}
