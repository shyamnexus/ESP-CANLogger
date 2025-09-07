#include "gps.h"
#include <stdio.h>
// TODO: connect to Neo-M8N via UART
int gps_parse_gprmc(char *line, gps_data_t *gps) {
    gps->fix = 1; gps->lat = 12.34; gps->lon = 56.78;
    gps->year = 2025; gps->month = 9; gps->day = 4;
    gps->hour = 12; gps->minute = 34; gps->second = 56;
    return 1;
}
void gps_readline(char *buf, int maxlen) {
    snprintf(buf, maxlen, "$GPRMC,..."); // stub
}
