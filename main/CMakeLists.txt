idf_component_register(
    SRCS "main.c" "logger.c" "canbus.c" "wifi_config.c" "thermocouple.c" "gps.c" "oled.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_wifi esp_http_server fatfs sdmmc nvs_flash spiffs esp_timer
)
