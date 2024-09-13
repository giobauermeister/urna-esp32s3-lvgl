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
#include "ui.h"
#include "lvgl.h"

#define I2S_SAMPLE_RATE (44100)
#define I2S_BITS_PER_SAMPLE I2S_DATA_BIT_WIDTH_16BIT
#define I2S_CHANNEL_MODE I2S_SLOT_MODE_MONO

// Channel handle
i2s_chan_handle_t tx_channel;

void i2s_init(void) 
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, &tx_channel, NULL);

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(I2S_SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_BITS_PER_SAMPLE, I2S_CHANNEL_MODE),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = GPIO_NUM_42,
            .ws = GPIO_NUM_18,
            .dout = GPIO_NUM_17,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false
            }
        }
    };

    i2s_channel_init_std_mode(tx_channel, &std_cfg);
    i2s_channel_enable(tx_channel);    
}

void init_spiffs(void)
{
    esp_vfs_spiffs_conf_t spiffs_cfg = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 10,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&spiffs_cfg);
    if(ret != ESP_OK) {
        ESP_LOGE("SPIFFS", "Failed to mount or format filesystem");
    } else {
        ESP_LOGI("SPIFFS", "SPIFFS mounted successfully");
    }
}

void list_spiffs_files(void)
{
    DIR *dir = opendir("/spiffs");

    if (dir != NULL) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            ESP_LOGI("SPIFFS", "File: %s", ent->d_name);
        }
        closedir(dir);
    } else {
        ESP_LOGE("SPIFFS", "Failed to open directory");
    }
}

void play_urna_sound(const char* file_path)
{
    FILE *f = fopen(file_path, "rb");
    if(f == NULL) {
        ESP_LOGE("SPIFFS", "Failed to open file for reading");
        return;
    } else {
        ESP_LOGI("SPIFFS", "Ok file open");
    }

    fseek(f, 44, SEEK_SET);

    char buffer[512];
    size_t bytes_read = 0;
    size_t bytes_written;

    while((bytes_read = fread(buffer, sizeof(int16_t), sizeof(buffer) / sizeof(int16_t), f)) > 0) {
        i2s_channel_write(tx_channel, buffer, bytes_read * sizeof(int16_t), &bytes_written, portMAX_DELAY);
    }

    fclose(f);
    ESP_LOGI("SPIFFS", "Successfully played WAV file");
}

void app_main(void)
{
    printf("Hello World!\n");

    i2s_init();
    init_spiffs();
    // list_spiffs_files();

    play_urna_sound("/spiffs/urna_sound_long.wav");
    // play_urna_sound("/spiffs/urna_sound_short.wav");

    init_lcd_panel();
    init_lvgl();

    // Create UI elements
    create_ui();
    
    vTaskDelay(pdMS_TO_TICKS(500));
    init_lcd_backlight();

    // Start handling LVGL tasks
    while (1) {
        lv_timer_handler();     // Let LVGL handle UI updates
        vTaskDelay(pdMS_TO_TICKS(10));  // Delay to avoid overload
    }
}
