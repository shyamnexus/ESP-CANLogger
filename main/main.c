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

// ============================================================================
// MODULE TESTING CONFIGURATION
// ============================================================================
// Uncomment the modules you want to test. Comment out modules you don't need.
// This allows you to test individual modules without initializing everything.

// Core system modules (usually needed for basic operation)
#define ENABLE_SDCARD         1
#define ENABLE_POWER_MONITOR  1
#define ENABLE_WIFI_CONFIG    1

// Hardware modules (enable only what you want to test)
#define ENABLE_THERMOCOUPLE   1
#define ENABLE_GPS            1
#define ENABLE_OLED           1
#define ENABLE_CANBUS         0
#define ENABLE_LOGGER         1

// ============================================================================

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

// ============================================================================
// MODULE TEST FUNCTIONS
// ============================================================================

static void test_thermocouple_module(void)
{
    ESP_LOGI(TAG, "=== TESTING THERMOCOUPLE MODULE ===");
    
    // Initialize thermocouple
    thermocouple_init();
    ESP_LOGI(TAG, "Thermocouple initialized");
    
    // Test reading all 4 channels
    for (int i = 0; i < 4; i++) {
        float temp = thermocouple_read(i);
        int fault = thermocouple_get_fault(i);
        
        if (fault) {
            ESP_LOGW(TAG, "Thermocouple %d: FAULT (0x%02X)", i, fault);
        } else {
            ESP_LOGI(TAG, "Thermocouple %d: %.2f°C", i, temp);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    ESP_LOGI(TAG, "=== THERMOCOUPLE TEST COMPLETE ===");
}

static void test_gps_module(void)
{
    ESP_LOGI(TAG, "=== TESTING GPS MODULE ===");
    
    // Initialize GPS
    gps_init();
    ESP_LOGI(TAG, "GPS initialized");
    
    // Test GPS reading for 10 seconds
    char gps_line[256];
    int fix_count = 0;
    
    for (int i = 0; i < 100; i++) { // 10 seconds at 100ms intervals
        gps_readline(gps_line, sizeof(gps_line));
        
        if (strstr(gps_line, "$GPRMC")) {
            gps_data_t gps_data;
            if (gps_parse_gprmc(gps_line, &gps_data)) {
                if (gps_data.fix) {
                    fix_count++;
                    ESP_LOGI(TAG, "GPS Fix: Lat=%.6f, Lon=%.6f, Speed=%.1f knots", 
                             gps_data.lat, gps_data.lon, gps_data.speed);
                } else {
                    ESP_LOGD(TAG, "GPS: No fix");
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    ESP_LOGI(TAG, "GPS test complete. Fixes received: %d/100", fix_count);
    ESP_LOGI(TAG, "=== GPS TEST COMPLETE ===");
}

static void test_oled_module(void)
{
    ESP_LOGI(TAG, "=== TESTING OLED MODULE ===");
    
    // Initialize OLED
    oled_init();
    ESP_LOGI(TAG, "OLED initialized");
    
    // Test basic display functions
    oled_clear();
    oled_display_text(0, 0, "OLED Test");
    oled_display_text(0, 16, "Module Working");
    oled_display_text(0, 32, "ESP32 Logger");
    oled_update_display();
    
    ESP_LOGI(TAG, "OLED display test complete");
    
    // Test temperature display
    float test_temps[4] = {25.5, 30.2, 28.7, 32.1};
    oled_display_temperatures(test_temps);
    oled_update_display();
    
    ESP_LOGI(TAG, "OLED temperature display test complete");
    
    // Test GPS status display
    oled_display_gps_status(1, 40.7128, -74.0060); // NYC coordinates
    oled_update_display();
    
    ESP_LOGI(TAG, "OLED GPS status display test complete");
    ESP_LOGI(TAG, "=== OLED TEST COMPLETE ===");
}

static void test_canbus_module(void)
{
    ESP_LOGI(TAG, "=== TESTING CANBUS MODULE ===");
    
    // Initialize CAN bus at 500 kbps
    canbus_init(500);
    ESP_LOGI(TAG, "CAN bus initialized at 500 kbps");
    
    // Test receiving messages for 5 seconds
    twai_message_t msg;
    int msg_count = 0;
    
    for (int i = 0; i < 50; i++) { // 5 seconds at 100ms intervals
        if (canbus_receive(&msg, 100) == ESP_OK) {
            msg_count++;
            ESP_LOGI(TAG, "CAN RX: ID=0x%03X, Len=%d, Data=[%02X %02X %02X %02X %02X %02X %02X %02X]",
                     msg.identifier, msg.data_length_code,
                     msg.data[0], msg.data[1], msg.data[2], msg.data[3],
                     msg.data[4], msg.data[5], msg.data[6], msg.data[7]);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    ESP_LOGI(TAG, "CAN bus test complete. Messages received: %d", msg_count);
    ESP_LOGI(TAG, "=== CANBUS TEST COMPLETE ===");
}

static void test_logger_module(void)
{
    ESP_LOGI(TAG, "=== TESTING LOGGER MODULE ===");
    
    // Start logger
    logger_start();
    ESP_LOGI(TAG, "Logger started");
    
    // Create test log entry
    log_entry_t test_entry = {
        .timestamp = time(NULL),
        .temperatures = {25.5, 30.2, 28.7, 32.1},
        .gps_fix = 1,
        .gps_lat = 40.7128,
        .gps_lon = -74.0060,
        .can_id = 0x123,
        .can_len = 4,
        .can_data = {0x01, 0x02, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00}
    };
    
    // Write test entry
    logger_write_entry(&test_entry);
    ESP_LOGI(TAG, "Test log entry written");
    
    // Create CSV header
    logger_create_csv_header();
    ESP_LOGI(TAG, "CSV header created");
    
    ESP_LOGI(TAG, "=== LOGGER TEST COMPLETE ===");
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
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize core system components
#if ENABLE_POWER_MONITOR
    init_power_monitoring();
#endif

#if ENABLE_SDCARD
    init_sdcard();
#endif

#if ENABLE_WIFI_CONFIG
    wifi_config_spiffs_init();
#endif

    // ============================================================================
    // MODULE TESTING SECTION
    // ============================================================================
    // Uncomment the test function you want to run for individual module testing
    
    // Test individual modules (uncomment one at a time for testing)
    // test_thermocouple_module();
    // test_gps_module();
    // test_oled_module();
    // test_canbus_module();
    // test_logger_module();
    
    // ============================================================================
    // NORMAL OPERATION MODE
    // ============================================================================
    // The following code runs in normal operation mode when no test functions are called
    
    // Initialize hardware components based on configuration
#if ENABLE_THERMOCOUPLE
    thermocouple_init();
    ESP_LOGI(TAG, "Thermocouple module enabled");
#endif

#if ENABLE_GPS
    gps_init();
    ESP_LOGI(TAG, "GPS module enabled");
#endif

#if ENABLE_OLED
    oled_init();
    ESP_LOGI(TAG, "OLED module enabled");
#endif

#if ENABLE_CANBUS
    canbus_init(500); // Initialize at 500 kbps
    ESP_LOGI(TAG, "CAN bus module enabled");
#endif

#if ENABLE_LOGGER
    logger_start();
    ESP_LOGI(TAG, "Logger module enabled");
#endif

    // Start WiFi and web server if enabled
#if ENABLE_WIFI_CONFIG
    wifi_start_ap();
    webserver_start();
    ESP_LOGI(TAG, "WiFi and web server started");
    ESP_LOGI(TAG, "Connect to WiFi: ESP32-LOGGER");
    ESP_LOGI(TAG, "Web interface: http://10.10.10.10");
#endif

    ESP_LOGI(TAG, "ESP32 Data Logger Started Successfully");
    ESP_LOGI(TAG, "Enabled modules:");
    ESP_LOGI(TAG, "  - SD Card: %s", ENABLE_SDCARD ? "YES" : "NO");
    ESP_LOGI(TAG, "  - Power Monitor: %s", ENABLE_POWER_MONITOR ? "YES" : "NO");
    ESP_LOGI(TAG, "  - WiFi Config: %s", ENABLE_WIFI_CONFIG ? "YES" : "NO");
    ESP_LOGI(TAG, "  - Thermocouple: %s", ENABLE_THERMOCOUPLE ? "YES" : "NO");
    ESP_LOGI(TAG, "  - GPS: %s", ENABLE_GPS ? "YES" : "NO");
    ESP_LOGI(TAG, "  - OLED: %s", ENABLE_OLED ? "YES" : "NO");
    ESP_LOGI(TAG, "  - CAN Bus: %s", ENABLE_CANBUS ? "YES" : "NO");
    ESP_LOGI(TAG, "  - Logger: %s", ENABLE_LOGGER ? "YES" : "NO");
}

