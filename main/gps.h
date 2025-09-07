#pragma once
typedef struct {
    int fix;
    int year, month, day, hour, minute, second;
    double lat, lon;
} gps_data_t;

int gps_parse_gprmc(char *line, gps_data_t *gps);
void gps_readline(char *buf, int maxlen);
