#include "sound.h"
#include "i2s_init.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <dirent.h>

void play_sound(const char* file_path)
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