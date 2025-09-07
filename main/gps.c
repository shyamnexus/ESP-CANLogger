#include "gps.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *TAG = "GPS";
static char gps_buffer[256];
static int buffer_pos = 0;

void gps_init(void) {
    ESP_LOGI(TAG, "Initializing Neo-M8N GPS...");
    
    uart_config_t uart_config = {
        .baud_rate = GPS_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t ret = uart_driver_install(GPS_UART_NUM, 1024, 1024, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver");
        return;
    }

    ret = uart_param_config(GPS_UART_NUM, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART");
        return;
    }

    ret = uart_set_pin(GPS_UART_NUM, GPS_TX_PIN, GPS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins");
        return;
    }

    // Configure GPS for 10Hz update rate
    const char* config_commands[] = {
        "$PUBX,40,RMC,0,1,0,0*47\r\n",  // Enable RMC at 1Hz
        "$PUBX,40,GGA,0,1,0,0*5A\r\n",  // Enable GGA at 1Hz
        "$PUBX,40,VTG,0,1,0,0*5E\r\n",  // Enable VTG at 1Hz
        "$PUBX,40,GSV,0,0,0,0*59\r\n",  // Disable GSV
        "$PUBX,40,GLL,0,0,0,0*5C\r\n",  // Disable GLL
        "$PUBX,40,GSA,0,0,0,0*4E\r\n",  // Disable GSA
        "$PUBX,40,DTM,0,0,0,0*40\r\n",  // Disable DTM
        "$PUBX,40,GBS,0,0,0,0*4B\r\n",  // Disable GBS
        "$PUBX,40,GPQ,0,0,0,0*4F\r\n",  // Disable GPQ
        "$PUBX,40,TXT,0,0,0,0*41\r\n",  // Disable TXT
        "$PUBX,40,ZDA,0,0,0,0*44\r\n",  // Disable ZDA
        "$PUBX,40,THS,0,0,0,0*54\r\n",  // Disable THS
        "$PUBX,40,VLW,0,0,0,0*5F\r\n",  // Disable VLW
        "$PUBX,40,VTG,0,0,0,0*5E\r\n",  // Disable VTG
        "$PUBX,40,VTG,0,1,0,0*5E\r\n",  // Enable VTG at 1Hz
    };

    for (int i = 0; i < sizeof(config_commands) / sizeof(config_commands[0]); i++) {
        uart_write_bytes(GPS_UART_NUM, config_commands[i], strlen(config_commands[i]));
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGI(TAG, "Neo-M8N GPS initialized successfully");
}

void gps_readline(char *buf, int maxlen) {
    uint8_t data[1];
    int len = uart_read_bytes(GPS_UART_NUM, data, 1, pdMS_TO_TICKS(10));
    
    if (len > 0) {
        char c = (char)data[0];
        
        if (c == '\n' || c == '\r') {
            if (buffer_pos > 0) {
                gps_buffer[buffer_pos] = '\0';
                strncpy(buf, gps_buffer, maxlen - 1);
                buf[maxlen - 1] = '\0';
                buffer_pos = 0;
                return;
            }
        } else if (buffer_pos < sizeof(gps_buffer) - 1) {
            gps_buffer[buffer_pos++] = c;
        }
    }
    
    buf[0] = '\0';
}

int gps_parse_gprmc(char *line, gps_data_t *gps) {
    if (strncmp(line, "$GPRMC,", 7) != 0) {
        return 0;
    }

    char *token;
    char *rest = line + 7; // Skip "$GPRMC,"
    int field = 0;
    
    // Initialize GPS data
    memset(gps, 0, sizeof(gps_data_t));
    
    while ((token = strtok_r(rest, ",", &rest)) != NULL && field < 12) {
        switch (field) {
            case 0: // Time (HHMMSS.SSS)
                if (strlen(token) >= 6) {
                    sscanf(token, "%2d%2d%2d", &gps->hour, &gps->minute, &gps->second);
                }
                break;
            case 1: // Status (A=Valid, V=Invalid)
                gps->fix = (token[0] == 'A') ? 1 : 0;
                break;
            case 2: // Latitude (DDMM.MMMM)
                if (strlen(token) > 0) {
                    double lat_deg = atof(token);
                    int lat_deg_int = (int)(lat_deg / 100);
                    double lat_min = lat_deg - (lat_deg_int * 100);
                    gps->lat = lat_deg_int + (lat_min / 60.0);
                }
                break;
            case 3: // Latitude direction (N/S)
                if (token[0] == 'S') {
                    gps->lat = -gps->lat;
                }
                break;
            case 4: // Longitude (DDDMM.MMMM)
                if (strlen(token) > 0) {
                    double lon_deg = atof(token);
                    int lon_deg_int = (int)(lon_deg / 100);
                    double lon_min = lon_deg - (lon_deg_int * 100);
                    gps->lon = lon_deg_int + (lon_min / 60.0);
                }
                break;
            case 5: // Longitude direction (E/W)
                if (token[0] == 'W') {
                    gps->lon = -gps->lon;
                }
                break;
            case 6: // Speed in knots
                if (strlen(token) > 0) {
                    gps->speed = atof(token);
                }
                break;
            case 7: // Course in degrees
                if (strlen(token) > 0) {
                    gps->course = atof(token);
                }
                break;
            case 8: // Date (DDMMYY)
                if (strlen(token) >= 6) {
                    int day, month, year;
                    sscanf(token, "%2d%2d%2d", &day, &month, &year);
                    gps->day = day;
                    gps->month = month;
                    gps->year = 2000 + year;
                }
                break;
        }
        field++;
    }
    
    return gps->fix;
}

int gps_has_fix(void) {
    char line[128];
    gps_data_t gps;
    gps_readline(line, sizeof(line));
    return gps_parse_gprmc(line, &gps);
}
