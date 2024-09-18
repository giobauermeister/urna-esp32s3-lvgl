#include "sd_init.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include <sys/stat.h>

static const char* TAG = "SDCARD";

esp_err_t init_sd_card(void) 
{
    ESP_LOGI(TAG, "Init");

    esp_err_t ret;

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = 5
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI_PIN,
        .miso_io_num = SD_MISO_PIN,
        .sclk_io_num = SD_CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    ret = spi_bus_initialize((spi_host_device_t)host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return ret;
    }

    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = SD_CS_PIN;
    slot_cfg.host_id = (spi_host_device_t)host.slot;

    sdmmc_card_t *card;
    ret = esp_vfs_fat_sdspi_mount("/sd", &host, &slot_cfg, &mount_cfg, &card);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount filesystem");
        return ret;
    }

    sdmmc_card_print_info(stdout, card);
    ESP_LOGI(TAG, "SD card mounted successfully");

    // Test writing to the SD card
    FILE* f = fopen("/sd/test.txt", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }

    fprintf(f, "This is a test write to the SD card.\n");
    fclose(f);
    ESP_LOGI(TAG, "File written successfully to the SD card");

    return ESP_OK;
}

bool is_sd_card_mounted(void) {
    struct stat st;
    if (stat("/sdcard", &st) == 0) {
        ESP_LOGI(TAG, "SD card is mounted!");
        return true;
    } else {
        ESP_LOGE(TAG, "SD card is not mounted");
        return false;
    }
}