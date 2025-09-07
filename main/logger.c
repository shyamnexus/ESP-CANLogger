#include "logger.h"
#include "thermocouple.h"
#include "gps.h"
#include "oled.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "canbus.h"
#include "wifi_config.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "LOGGER";
static FILE* log_file = NULL;
static char log_filename[64];

void logger_create_csv_header(void) {
    if (log_file) {
        fprintf(log_file, "Timestamp,T1,T2,T3,T4,GPS_Fix,Latitude,Longitude,CAN_ID,CAN_Data\n");
        fflush(log_file);
        ESP_LOGI(TAG, "CSV header written");
    }
}

void logger_write_entry(const log_entry_t* entry) {
    if (!log_file) return;
    
    // Write timestamp
    struct tm* tm_info = localtime(&entry->timestamp);
    fprintf(log_file, "%04d-%02d-%02d %02d:%02d:%02d,",
            tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
            tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    
    // Write temperatures
    for (int i = 0; i < 4; i++) {
        fprintf(log_file, "%.2f,", entry->temperatures[i]);
    }
    
    // Write GPS data
    fprintf(log_file, "%d,%.6f,%.6f,", entry->gps_fix, entry->gps_lat, entry->gps_lon);
    
    // Write CAN data
    if (entry->can_id >= 0) {
        fprintf(log_file, "0x%03X,", entry->can_id);
        for (int i = 0; i < entry->can_len && i < 8; i++) {
            fprintf(log_file, "%02X", entry->can_data[i]);
        }
    } else {
        fprintf(log_file, "--,");
    }
    
    fprintf(log_file, "\n");
    fflush(log_file);
}

void test_task(void *pv){
			  ssd1306_scrolllog_init("log.txt");
    ssd1306_scrolllog_printf("=== Data Logger Started ===");
	 while (1) {

oled_update_display();
		  vTaskDelay(pdMS_TO_TICKS(1000));
		 }
}
void logger_task(void *pv) {
    // Create log filename with timestamp
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    snprintf(log_filename, sizeof(log_filename), "/sdcard/log_%04d%02d%02d_%02d%02d%02d.csv",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    
    log_file = fopen(log_filename, "w");
    if (!log_file) {
        ESP_LOGE(TAG, "Failed to create log file: %s", log_filename);
        return;
    }
    
    ESP_LOGI(TAG, "Log file created: %s", log_filename);
    logger_create_csv_header();
    
    ssd1306_scrolllog_init(log_filename);
    ssd1306_scrolllog_printf("=== Data Logger Started ===");

    gps_data_t gps = {0};
    char gpsline[128];
    float temps[4];
    log_entry_t log_entry;
    int64_t epoch_us;

    int current_can_speed = get_can_speed();
    canbus_init(current_can_speed);

    twai_message_t can_msg;
    int can_valid = 0;
    int log_count = 0;

    while (1) {
        int rate_hz = get_log_rate();
        if (rate_hz < 1) rate_hz = 1;
        int delay_ms = 1000 / rate_hz;

        int new_can_speed = get_can_speed();
        canbus_reinit_if_needed(new_can_speed);
        current_can_speed = new_can_speed;

        // Read GPS data
        gps_readline(gpsline, sizeof(gpsline));
        int fix = gps_parse_gprmc(gpsline, &gps);

        // Read thermocouple temperatures
        for (int i = 0; i < 4; i++) {
            temps[i] = thermocouple_read(i);
        }

        // Read CAN data
        if (canbus_receive(&can_msg, 5)) {
            can_valid = 1;
        } else {
            can_valid = 0;
        }

        // Update live status for web interface
        update_live_status(temps, fix, gps.lat, gps.lon,
                           can_valid ? can_msg.identifier : -1,
                           can_valid ? can_msg.data_length_code : 0,
                           can_valid ? can_msg.data : NULL);

        // Update OLED display
        oled_clear();
        oled_display_temperatures(temps);
        oled_display_gps_status(fix, gps.lat, gps.lon);
        oled_display_can_status(can_valid ? can_msg.identifier : -1, 
                               can_valid ? can_msg.data_length_code : 0,
                               can_valid ? can_msg.data : NULL);
        oled_update_display();

        // Log data to SD card
        gettimeofday((struct timeval*)&log_entry.timestamp, NULL);
        memcpy(log_entry.temperatures, temps, sizeof(temps));
        log_entry.gps_fix = fix;
        log_entry.gps_lat = gps.lat;
        log_entry.gps_lon = gps.lon;
        log_entry.can_id = can_valid ? can_msg.identifier : -1;
        log_entry.can_len = can_valid ? can_msg.data_length_code : 0;
        if (can_valid && can_msg.data_length_code > 0) {
            memcpy(log_entry.can_data, can_msg.data, 
                   can_msg.data_length_code > 8 ? 8 : can_msg.data_length_code);
        }
        
        logger_write_entry(&log_entry);
        log_count++;

        // Log status every 100 entries
        if (log_count % 100 == 0) {
            ESP_LOGI(TAG, "Logged %d entries", log_count);
            ssd1306_scrolllog_printf("Logged: %d", log_count);
        }

        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

void logger_start(void) {
 //   xTaskCreate(logger_task, "logger_task", 8192, NULL, 5, NULL);
   xTaskCreate(test_task, "test_task", 8192, NULL, 5, NULL);

}
