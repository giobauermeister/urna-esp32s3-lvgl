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
#include "wifi.h"

uint8_t* urna_sound_long = NULL;
size_t urna_sound_long_size = 0;

uint8_t* urna_sound_short = NULL;
size_t urna_sound_short_size = 0;

Candidate candidate1 = {
    .name = "New Candidate",
    .number = "1234",
    .role_id = 1,
    .party_id = 1,
};

Party party1 = {
    .name = "Partido1",
};

Role role1 = {
    .name = "Prefeito",
};

void app_main(void)
{
    printf("Hello World!\n");

    i2s_init();
    i2c_master_init();
    init_sd_card();

    //del_file_content("/sd/candidates.jdb");
    //del_file_content("/sd/roles.jdb");
    //del_file_content("/sd/parties.jdb");

    //add_party(party1);
    //add_role(role1);
    //add_candidate(candidate1);

    // del_candidate_by_id(1);
    // del_candidate_by_id(2);
    // del_candidate_by_id(3);
    // del_candidate_by_id(5);
    
    init_spiffs();
    // list_spiffs_files();

    init_sound_semaphore();
    urna_sound_long = load_sound_to_memory("/spiffs/urna_sound_long.wav", &urna_sound_long_size);
    urna_sound_short = load_sound_to_memory("/spiffs/urna_sound_short.wav", &urna_sound_short_size);

    play_urna_sound_long();
    play_urna_sound_short();

    wifi_init_sta();

    init_lcd_panel();
    init_lvgl();

    // Create UI elements
    create_ui();

    create_keypad_interrupt_task();
    
    vTaskDelay(pdMS_TO_TICKS(500));
    init_lcd_backlight();

    create_lvgl_task();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay to avoid overload
    }
}
