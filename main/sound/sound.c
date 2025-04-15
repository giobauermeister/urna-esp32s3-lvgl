#include "sound/sound.h"
#include "i2s/i2s_init.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <dirent.h>
#include "esp_heap_caps.h"

static const char* TAG_SPIFFS = "SPIFFS";
static const char* TAG_SOUND = "Sound";

SemaphoreHandle_t xSoundSemaphore;

void init_sound_semaphore()
{
    // Initialize the semaphore before any tasks start
    xSoundSemaphore = xSemaphoreCreateBinary();
    if (xSoundSemaphore != NULL) {
        // Initialize to give the first sound task access
        xSemaphoreGive(xSoundSemaphore);
    }
}

uint8_t* load_sound_to_memory(const char* file_path, size_t* file_size)
{
    FILE *f = fopen(file_path, "rb");
    if(f == NULL) {
        ESP_LOGE(TAG_SPIFFS, "Failed to open file for reading");
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f) - 44;
    fseek(f, 44, SEEK_SET);

    uint8_t *wav_data = (uint8_t*) heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!wav_data) {
        ESP_LOGE(TAG_SPIFFS, "Failed to allocate memory for WAV file");
        fclose(f);
        return NULL;
    }

    // Read file content into memory
    size_t bytes_read = fread(wav_data, 1, size, f);
    if (bytes_read != size) {
        ESP_LOGE(TAG_SPIFFS, "Error reading WAV file, read only %d of %d bytes", bytes_read, size);
        free(wav_data);
        fclose(f);
        return NULL;
    }

    *file_size = size;
    fclose(f);
    ESP_LOGI(TAG_SPIFFS, "Successfully loaded WAV file into memory");
    return wav_data;
}

void play_sound_from_memory(const uint8_t* wav_data, size_t size)
{
    size_t bytes_written = 0;
    size_t total_bytes_written = 0;

    while(total_bytes_written < size) {
        size_t chunk_size = (size - total_bytes_written > 512) ? 512 : (size - total_bytes_written);
        i2s_channel_write(tx_channel, wav_data + total_bytes_written, chunk_size, &bytes_written, portMAX_DELAY);
        total_bytes_written += bytes_written;
        vTaskDelay(pdMS_TO_TICKS(1)); // Small delay to avoid hogging CPU
    }

    ESP_LOGI(TAG_SOUND, "Successfully played sound from memory");
}

void play_sound_from_memory_task(void *pvParameters)
{
    // Cast the passed parameters back to a struct
    uint8_t *wav_data = ((uint8_t **)pvParameters)[0];
    size_t size = *((size_t *)((uint8_t **)pvParameters)[1]);

    // Wait for the semaphore (to ensure no other sound is playing)
    if (xSemaphoreTake(xSoundSemaphore, portMAX_DELAY) == pdTRUE)
    {
        // Play the sound from memory
        play_sound_from_memory(wav_data, size);

        // After playing, give back the semaphore to allow the next sound to play
        xSemaphoreGive(xSoundSemaphore);

        // After playing, delete the task to make it one-shot
        vTaskDelete(NULL);
    }
}

void start_play_sound_from_memory_task(uint8_t *wav_data, size_t size)
{
    // Create a structure to hold the sound data and size to pass to the task
    uint8_t *params[] = { wav_data, (uint8_t *)&size };

    // Create a FreeRTOS task for playing the sound from memory
    xTaskCreate(
        play_sound_from_memory_task,  // Task function
        "PlaySoundTask",              // Name of the task
        8192,                         // Stack size in bytes
        params,                       // Parameters passed to the task
        3,                            // Task priority (set this as needed)
        NULL                          // Task handle (can be NULL if not needed)
    );
}

void play_urna_sound_long()
{
    if (urna_sound_long != NULL) {
        start_play_sound_from_memory_task(urna_sound_long, urna_sound_long_size);
    } else {
        ESP_LOGI(TAG_SOUND, "Could not play Urna sound long");
    }
}

void play_urna_sound_short()
{
    if (urna_sound_short != NULL) {
        start_play_sound_from_memory_task(urna_sound_short, urna_sound_short_size);
    } else {
        ESP_LOGI(TAG_SOUND, "Could not play Urna sound short");
    }
}