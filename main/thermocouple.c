#include "thermocouple.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "THERMOCOUPLE";
static spi_device_handle_t spi_handle = NULL;

// CS pins for each thermocouple
static const gpio_num_t cs_pins[4] = {
    MAX31855_CS_0, MAX31855_CS_1, MAX31855_CS_2, MAX31855_CS_3
};

void thermocouple_init(void) {
    ESP_LOGI(TAG, "Initializing MAX31855 thermocouple interface...");
    
    // Configure CS pins
    for (int i = 0; i < 4; i++) {
        gpio_config_t io_conf = {
            .intr_type = GPIO_INTR_DISABLE,
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = (1ULL << cs_pins[i]),
            .pull_down_en = 0,
            .pull_up_en = 0,
        };
        gpio_config(&io_conf);
        gpio_set_level(cs_pins[i], 1); // CS high (inactive)
    }

    // Configure SPI
    spi_bus_config_t buscfg = {
        .miso_io_num = MAX31855_MISO,
        .mosi_io_num = MAX31855_MOSI,
        .sclk_io_num = MAX31855_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,  // 1 MHz
        .mode = 0,                  // SPI mode 0
        .spics_io_num = -1,         // We'll handle CS manually
        .queue_size = 1,
    };

    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus");
        return;
    }

    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device");
        return;
    }

    ESP_LOGI(TAG, "MAX31855 interface initialized successfully");
}

float thermocouple_read(int channel) {
    if (channel < 0 || channel >= 4 || spi_handle == NULL) {
        return -999.0; // Error value
    }

    // Select the appropriate CS pin
    gpio_set_level(cs_pins[channel], 0); // CS low (active)
    
    // Read 32-bit data from MAX31855
    uint32_t data = 0;
    spi_transaction_t t = {
        .length = 32,
        .rx_buffer = &data,
    };
    
    esp_err_t ret = spi_device_transmit(spi_handle, &t);
    gpio_set_level(cs_pins[channel], 1); // CS high (inactive)
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transaction failed for channel %d", channel);
        return -999.0;
    }

    // Check for faults
    if (data & 0x7) {
        ESP_LOGW(TAG, "Thermocouple %d fault: 0x%02X", channel, (uint8_t)(data & 0x7));
        return -999.0;
    }

    // Extract temperature (14-bit signed value)
    int16_t temp_raw = (data >> 18) & 0x3FFF;
    if (temp_raw & 0x2000) { // Negative temperature
        temp_raw |= 0xC000; // Sign extend
    }
    
    float temperature = temp_raw * 0.25; // 0.25°C per LSB
    return temperature;
}

int thermocouple_get_fault(int channel) {
    if (channel < 0 || channel >= 4 || spi_handle == NULL) {
        return -1;
    }

    gpio_set_level(cs_pins[channel], 0);
    uint32_t data = 0;
    spi_transaction_t t = {
        .length = 32,
        .rx_buffer = &data,
    };
    
    esp_err_t ret = spi_device_transmit(spi_handle, &t);
    gpio_set_level(cs_pins[channel], 1);
    
    if (ret != ESP_OK) {
        return -1;
    }

    return (int)(data & 0x7); // Return fault bits
}
