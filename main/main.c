#include "esp_spiffs.h"
#include "esp_log.h"

static const char *TAG = "SPIFFS";

static void init_spiffs(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs",
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS mount failed (%s)", esp_err_to_name(ret));
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS: total=%d, used=%d", total, used);
    } else {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition info");
    }
}

void app_main(void)
{
    init_spiffs();     // mount /spiffs
    webserver_start(); // start your HTTP server
}

