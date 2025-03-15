#include "spiffs_setup.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include <dirent.h>

static const char *TAG = "SPIFFS_SETUP";

esp_err_t init_spiffs(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs_data",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0, used = 0;
    esp_spiffs_info(conf.partition_label, &total, &used);
    ESP_LOGI("SPIFFS_SETUP", "SPIFFS total: %d, used: %d", total, used);
    return ESP_OK;
}


