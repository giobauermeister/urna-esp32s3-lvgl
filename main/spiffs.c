#include "spiffs.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include <dirent.h>

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