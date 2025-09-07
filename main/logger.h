#pragma once
#include <time.h>

// Data logging structure
typedef struct {
    time_t timestamp;
    float temperatures[4];
    int gps_fix;
    double gps_lat, gps_lon;
    int can_id;
    int can_len;
    uint8_t can_data[8];
} log_entry_t;

// Function declarations
void logger_start(void);
void logger_write_entry(const log_entry_t* entry);
void logger_create_csv_header(void);
