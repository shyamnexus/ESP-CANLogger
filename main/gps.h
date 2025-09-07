#pragma once
#include <stdint.h>

// Neo-M8N UART configuration
#define GPS_UART_NUM     UART_NUM_2
#define GPS_TX_PIN       GPIO_NUM_17
#define GPS_RX_PIN       GPIO_NUM_16
#define GPS_BAUD_RATE    38400

typedef struct {
    int fix;
    int year, month, day, hour, minute, second;
    double lat, lon;
    float speed;        // knots
    float course;       // degrees
    float hdop;         // horizontal dilution of precision
} gps_data_t;

// Function declarations
void gps_init(void);
int gps_parse_gprmc(char *line, gps_data_t *gps);
void gps_readline(char *buf, int maxlen);
int gps_has_fix(void);
