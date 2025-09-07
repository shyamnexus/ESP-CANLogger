#pragma once
#include <stdint.h>

// SSD1306 OLED configuration
#define OLED_I2C_PORT    I2C_NUM_0
#define OLED_SDA_PIN     GPIO_NUM_21
#define OLED_SCL_PIN     GPIO_NUM_22
#define OLED_ADDR        0x3C
#define OLED_WIDTH       128
#define OLED_HEIGHT      64

// Function declarations
void oled_init(void);
void oled_clear(void);
void oled_display_text(int x, int y, const char* text);
void oled_display_temperatures(float temps[4]);
void oled_display_gps_status(int fix, double lat, double lon);
void oled_display_can_status(int can_id, int can_len, uint8_t* can_data);
void oled_update_display(void);
void ssd1306_scrolllog_init(const char *logfile);
void ssd1306_scrolllog_printf(const char *fmt, ...);
