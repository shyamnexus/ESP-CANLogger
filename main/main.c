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

#include "logger.h"
#include "thermocouple.h"
#include "gps.h"
#include "oled.h"
#include "canbus.h"
#include "wifi_config.h"

static const char *TAG = "MAIN";

// SD Card configuration
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

// Power management GPIO
#define POWER_MONITOR_GPIO GPIO_NUM_34

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
 //   ESP_ERROR_CHECK(esp_netif_init());
 //   ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize power monitoring
 //   init_power_monitoring();

    // Initialize SD card
 //   init_sdcard();

    // Initialize SPIFFS for web interface
 //   wifi_config_spiffs_init();

    // Initialize hardware components
  //  thermocouple_init();
  //  gps_init();
    oled_init();
    
    // Start WiFi AP and web server
 //   wifi_start_ap();
 //   webserver_start();

    // Start data logging task
    logger_start();

    ESP_LOGI(TAG, "ESP32 Data Logger Started Successfully");
    ESP_LOGI(TAG, "Connect to WiFi: ESP32-LOGGER");
    ESP_LOGI(TAG, "Web interface: http://10.10.10.10");
}

// FreeRTOS hook functions
void vApplicationIdleHook(void)
{
    // Called on each iteration of the idle task
    // Can be used for low power mode or background tasks
}

void vApplicationTickHook(void)
{
    // Called from every tick interrupt
    // Can be used for time-critical operations
}

