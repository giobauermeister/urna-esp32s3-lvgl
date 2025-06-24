#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include "ui/ui.h"
#include "lcd/lcd_init.h"
#include "ui/lvgl_init.h"
#include "i2s/i2s_init.h"
#include "i2c/i2c_init.h"
#include "storage/spiffs.h"
#include "storage/sd_init.h"
#include "sound/sound.h"
#include "keypad/keypad.h"
#include "candidate/candidate.h"
#include "wifi/wifi.h"
#include "esp_sntp.h"
#include "web_api/web_api.h"

uint8_t* urna_sound_long = NULL;
size_t urna_sound_long_size = 0;

uint8_t* urna_sound_short = NULL;
size_t urna_sound_short_size = 0;

ui_candidate_t candidate1 = {
    .name = "New Candidate",
    .number = "1234",
    .role_id = 1,
    .party_id = 1,
};

ui_party_t party1 = {
    .name = "Partido1",
};

ui_role_t role1 = {
    .name = "Prefeito",
};

void initialize_sntp(void)
{
    ESP_LOGI("SNTP", "Initializing SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");  // optional: use custom NTP server
    esp_sntp_init();
}

void wait_for_time_sync(void)
{
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int max_retries = 10;

    while (time(&now) < 1600000000 && ++retry < max_retries) {
        ESP_LOGI("SNTP", "Waiting for system time to be set... (%d/%d)", retry, max_retries);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    localtime_r(&now, &timeinfo);
    ESP_LOGI("SNTP", "Time synchronized: %04d-%02d-%02d %02d:%02d:%02d",
        timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
}

void app_main(void)
{
    printf("Hello World!\n");

    i2s_init();
    i2c_master_init();
    init_sd_card();

    //del_file_content("/sd/candidates.jsonl");
    //del_file_content("/sd/roles.jsonl");
    //del_file_content("/sd/parties.jsonl");

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

    wifi_init_sta();
    initialize_sntp();
    wait_for_time_sync();
    start_webserver();
    load_candidate_roles();
    init_lcd_panel();
    init_lvgl();
    play_urna_sound_long();
    create_ui();
    create_keypad_interrupt_task();
    
    vTaskDelay(pdMS_TO_TICKS(500));
    init_lcd_backlight();

    create_lvgl_task();

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay to avoid overload
    }
}
