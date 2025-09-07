#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Module configuration
#include "module_config.h"

// Conditional module includes
#ifdef ENABLE_LOGGER_MODULE
#include "logger.h"
#endif

#ifdef ENABLE_THERMOCOUPLE_MODULE
#include "thermocouple.h"
#endif

#ifdef ENABLE_GPS_MODULE
#include "gps.h"
#endif

#ifdef ENABLE_OLED_MODULE
#include "oled.h"
#endif

#ifdef ENABLE_CANBUS_MODULE
#include "canbus.h"
#endif

#ifdef ENABLE_WIFI_CONFIG_MODULE
#include "wifi_config.h"
#endif

static const char *TAG = "MAIN";

// SD Card configuration
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

// Power management GPIO
#define POWER_MONITOR_GPIO GPIO_NUM_34

#ifdef ENABLE_SDCARD_MODULE
static void init_sdcard(void)
{
    ESP_LOGI(TAG, "Initializing SD card...");
    
    // Configure SPI bus
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
        return;
    }

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = SPI2_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card;
    ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s).", esp_err_to_name(ret));
        }
        return;
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);
    ESP_LOGI(TAG, "SD card mounted successfully");
}
#endif

#ifdef ENABLE_POWER_MONITORING_MODULE
static void init_power_monitoring(void)
{
    // Configure ADC for battery voltage monitoring
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << POWER_MONITOR_GPIO),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "Power monitoring GPIO configured");
}
#endif

#ifdef ENABLE_OLED_MODULE
static void oled_test_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting OLED test task...");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second for initialization
    
    ESP_LOGI(TAG, "Running OLED test display...");
    
    // Test 1: Clear display
    oled_clear();
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Test 2: Display text
    oled_display_text(0, 0, "ESP32 Working!");
    oled_display_text(0, 16, "OLED Test OK");
    oled_display_text(0, 32, "I2C Comm OK");
    oled_update_display();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Test 3: Counter display
    int counter = 0;
    while (1) {
        oled_clear();
        oled_display_text(0, 0, "Counter:");
        char counter_str[16];
        snprintf(counter_str, sizeof(counter_str), "%d", counter);
        oled_display_text(0, 16, counter_str);
        oled_update_display();
        
        ESP_LOGI(TAG, "OLED: Counter: %d", counter);
        counter++;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
#endif

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32 Data Logger Starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

#ifdef ENABLE_POWER_MONITORING_MODULE
    // Initialize power monitoring
    init_power_monitoring();
#endif

#ifdef ENABLE_SDCARD_MODULE
    // Initialize SD card
    init_sdcard();
#endif

#ifdef ENABLE_WIFI_CONFIG_MODULE
    // Initialize SPIFFS for web interface
    wifi_config_spiffs_init();
#endif

    // Initialize hardware components based on configuration
#ifdef ENABLE_THERMOCOUPLE_MODULE
    thermocouple_init();
#endif

#ifdef ENABLE_GPS_MODULE
    gps_init();
#endif

#ifdef ENABLE_OLED_MODULE
    oled_init();
    
    // Start OLED test task
    xTaskCreate(oled_test_task, "oled_test_task", 4096, NULL, 5, NULL);
#endif

#ifdef ENABLE_CANBUS_MODULE
    canbus_init(500); // Default 500 kbps
#endif

#ifdef ENABLE_WIFI_CONFIG_MODULE
    // Start WiFi AP and web server
    wifi_start_ap();
    webserver_start();
#endif

#ifdef ENABLE_LOGGER_MODULE
    // Start data logging task
    logger_start();
#endif

    ESP_LOGI(TAG, "ESP32 Data Logger Started Successfully");
    
#ifdef ENABLE_WIFI_CONFIG_MODULE
    ESP_LOGI(TAG, "Connect to WiFi: ESP32-LOGGER");
    ESP_LOGI(TAG, "Web interface: http://10.10.10.10");
#endif

    // Display enabled modules
    ESP_LOGI(TAG, "Enabled modules:");
#ifdef ENABLE_OLED_MODULE
    ESP_LOGI(TAG, "  - OLED Display");
#endif
#ifdef ENABLE_THERMOCOUPLE_MODULE
    ESP_LOGI(TAG, "  - Thermocouple");
#endif
#ifdef ENABLE_GPS_MODULE
    ESP_LOGI(TAG, "  - GPS");
#endif
#ifdef ENABLE_CANBUS_MODULE
    ESP_LOGI(TAG, "  - CAN Bus");
#endif
#ifdef ENABLE_WIFI_CONFIG_MODULE
    ESP_LOGI(TAG, "  - WiFi Configuration");
#endif
#ifdef ENABLE_LOGGER_MODULE
    ESP_LOGI(TAG, "  - Data Logger");
#endif
#ifdef ENABLE_SDCARD_MODULE
    ESP_LOGI(TAG, "  - SD Card");
#endif
#ifdef ENABLE_POWER_MONITORING_MODULE
    ESP_LOGI(TAG, "  - Power Monitoring");
#endif
}

