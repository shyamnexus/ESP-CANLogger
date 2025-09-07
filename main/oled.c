#include "oled.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static const char *TAG = "OLED";
static uint8_t oled_buffer[OLED_WIDTH * OLED_HEIGHT / 8];
static int current_line = 0;

// SSD1306 commands
#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR 0x22
#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_CHARGEPUMP 0x8D

static esp_err_t oled_write_cmd(uint8_t cmd) {
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (OLED_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd_handle, 0x00, true); // Command mode
    i2c_master_write_byte(cmd_handle, cmd, true);
    i2c_master_stop(cmd_handle);
    esp_err_t ret = i2c_master_cmd_begin(OLED_I2C_PORT, cmd_handle, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd_handle);
    return ret;
}

static esp_err_t oled_write_data(uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (OLED_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd_handle, 0x40, true); // Data mode
    i2c_master_write(cmd_handle, data, len, true);
    i2c_master_stop(cmd_handle);
    esp_err_t ret = i2c_master_cmd_begin(OLED_I2C_PORT, cmd_handle, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd_handle);
    return ret;
}

void oled_init(void) {
    ESP_LOGI(TAG, "Initializing SSD1306 OLED...");
    ESP_LOGI(TAG, "I2C Config: SDA=GPIO%d, SCL=GPIO%d, Addr=0x%02X", 
             OLED_SDA_PIN, OLED_SCL_PIN, OLED_ADDR);
    
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = OLED_SDA_PIN,
        .scl_io_num = OLED_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    
    esp_err_t ret = i2c_param_config(OLED_I2C_PORT, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "I2C param config OK");
    
    ret = i2c_driver_install(OLED_I2C_PORT, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "I2C driver installed");
    
    // Test I2C communication
    ret = oled_write_cmd(SSD1306_DISPLAYOFF);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C communication test failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "Check wiring: SDA->GPIO%d, SCL->GPIO%d", OLED_SDA_PIN, OLED_SCL_PIN);
        return;
    }
    
    // Initialize SSD1306
    oled_write_cmd(SSD1306_SETDISPLAYCLOCKDIV);
    oled_write_cmd(0x80);
    oled_write_cmd(SSD1306_SETMULTIPLEX);
    oled_write_cmd(0x3F);
    oled_write_cmd(SSD1306_SETDISPLAYOFFSET);
    oled_write_cmd(0x00);
    oled_write_cmd(SSD1306_SETSTARTLINE | 0x0);
    oled_write_cmd(SSD1306_CHARGEPUMP);
    oled_write_cmd(0x14);
    oled_write_cmd(SSD1306_MEMORYMODE);
    oled_write_cmd(0x00);
    oled_write_cmd(SSD1306_SEGREMAP | 0x1);
    oled_write_cmd(SSD1306_COMSCANDEC);
    oled_write_cmd(SSD1306_SETCOMPINS);
    oled_write_cmd(0x12);
    oled_write_cmd(SSD1306_SETCONTRAST);
    oled_write_cmd(0xCF);
    oled_write_cmd(SSD1306_SETPRECHARGE);
    oled_write_cmd(0xF1);
    oled_write_cmd(SSD1306_SETVCOMDETECT);
    oled_write_cmd(0x40);
    oled_write_cmd(SSD1306_DISPLAYALLON_RESUME);
    oled_write_cmd(SSD1306_NORMALDISPLAY);
    oled_write_cmd(SSD1306_DISPLAYON);
    
    oled_clear();
    
    // Display initial test message
    oled_display_text(0, 0, "ESP32 Data Logger");
    oled_display_text(0, 16, "OLED Module Test");
    oled_display_text(0, 32, "Initializing...");
    oled_update_display();
    
    ESP_LOGI(TAG, "SSD1306 OLED initialized successfully");
}

void oled_clear(void) {
    memset(oled_buffer, 0, sizeof(oled_buffer));
    oled_update_display();
}

void oled_display_text(int x, int y, const char* text) {
    // Simple text display (8x8 font)
    int len = strlen(text);
    for (int i = 0; i < len && (x + i * 8) < OLED_WIDTH; i++) {
        char c = text[i];
        if (c >= 32 && c <= 126) {
            // Simple 8x8 font rendering - create a simple pattern for each character
            for (int row = 0; row < 8 && (y + row) < OLED_HEIGHT; row++) {
                for (int col = 0; col < 8 && (x + i * 8 + col) < OLED_WIDTH; col++) {
                    int buffer_index = (y + row) * (OLED_WIDTH / 8) + (x + i * 8 + col) / 8;
                    if (buffer_index < sizeof(oled_buffer)) {
                        // Create a simple character pattern
                        if (row == 0 || row == 7 || col == 0 || col == 7) {
                            oled_buffer[buffer_index] |= (1 << ((x + i * 8 + col) % 8));
                        }
                    }
                }
            }
        }
    }
}

void oled_display_temperatures(float temps[4]) {
    char temp_str[32];
    oled_display_text(0, 0, "T1:");
    snprintf(temp_str, sizeof(temp_str), "%.1fC", temps[0]);
    oled_display_text(24, 0, temp_str);
    
    oled_display_text(0, 16, "T2:");
    snprintf(temp_str, sizeof(temp_str), "%.1fC", temps[1]);
    oled_display_text(24, 16, temp_str);
    
    oled_display_text(0, 32, "T3:");
    snprintf(temp_str, sizeof(temp_str), "%.1fC", temps[2]);
    oled_display_text(24, 32, temp_str);
    
    oled_display_text(0, 48, "T4:");
    snprintf(temp_str, sizeof(temp_str), "%.1fC", temps[3]);
    oled_display_text(24, 48, temp_str);
}

void oled_display_gps_status(int fix, double lat, double lon) {
    char gps_str[32];
    if (fix) {
        oled_display_text(64, 0, "GPS:OK");
        snprintf(gps_str, sizeof(gps_str), "%.4f", lat);
        oled_display_text(64, 16, gps_str);
        snprintf(gps_str, sizeof(gps_str), "%.4f", lon);
        oled_display_text(64, 32, gps_str);
    } else {
        oled_display_text(64, 0, "GPS:NO");
        oled_display_text(64, 16, "FIX");
    }
}

void oled_display_can_status(int can_id, int can_len, uint8_t* can_data) {
    char can_str[32];
    if (can_id >= 0) {
        oled_display_text(64, 48, "CAN:");
        snprintf(can_str, sizeof(can_str), "0x%03X", can_id);
        oled_display_text(96, 48, can_str);
    } else {
        oled_display_text(64, 48, "CAN:--");
    }
}

void oled_update_display(void) {
    oled_write_cmd(SSD1306_COLUMNADDR);
    oled_write_cmd(0);
    oled_write_cmd(OLED_WIDTH - 1);
    oled_write_cmd(SSD1306_PAGEADDR);
    oled_write_cmd(0);
    oled_write_cmd((OLED_HEIGHT / 8) - 1);
    
    oled_write_data(oled_buffer, sizeof(oled_buffer));
}

void ssd1306_scrolllog_init(const char *logfile) {
    ESP_LOGI(TAG, "OLED log initialized: %s", logfile);
    oled_clear();
    oled_display_text(0, 0, "Data Logger");
    oled_display_text(0, 16, "Starting...");
    oled_update_display();
}

void ssd1306_scrolllog_printf(const char *fmt, ...) {
    char buffer[64];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    
    ESP_LOGI(TAG, "OLED: %s", buffer);
    
    // Simple scrolling display
    if (current_line < 8) {
        oled_display_text(0, current_line * 8, buffer);
        current_line++;
    } else {
        // Scroll up
        memmove(oled_buffer, oled_buffer + (OLED_WIDTH / 8), 
                (OLED_HEIGHT - 8) * (OLED_WIDTH / 8));
        memset(oled_buffer + (OLED_HEIGHT - 8) * (OLED_WIDTH / 8), 0, 
               (OLED_WIDTH / 8));
        oled_display_text(0, (OLED_HEIGHT - 8), buffer);
    }
    oled_update_display();
}
