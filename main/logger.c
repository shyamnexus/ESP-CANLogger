#include "logger.h"
#include "thermocouple.h"
#include "gps.h"
#include "oled.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"

//#include "ssd1306.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "canbus.h"
#include "wifi_config.h"

static const char *TAG = "LOGGER";

void logger_task(void *pv) {
    ssd1306_scrolllog_init("/sdcard/log.txt");
    ssd1306_scrolllog_printf("=== Data Logger Started ===");

    gps_data_t gps = {0};
    char gpsline[128];
    float temps[4];
    int64_t epoch_us;

    int current_can_speed = get_can_speed();
    canbus_init(current_can_speed);

    twai_message_t can_msg;
    int can_valid = 0;

    while (1) {
        int rate_hz = get_log_rate();
        if (rate_hz < 1) rate_hz = 1;
        int delay_ms = 1000 / rate_hz;

        int new_can_speed = get_can_speed();
        canbus_reinit_if_needed(new_can_speed);
        current_can_speed = new_can_speed;

        gps_readline(gpsline, sizeof(gpsline));
        int fix = gps_parse_gprmc(gpsline, &gps);

        for (int i = 0; i < 4; i++) temps[i] = thermocouple_read(i);

        if (canbus_receive(&can_msg, 5)) {
            can_valid = 1;
        } else {
            can_valid = 0;
        }

        update_live_status(temps, fix, gps.lat, gps.lon,
                           can_valid ? can_msg.identifier : -1,
                           can_valid ? can_msg.data_length_code : 0,
                           can_valid ? can_msg.data : NULL);

        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

void logger_start() {
    xTaskCreate(logger_task, "logger_task", 8192, NULL, 5, NULL);
}
