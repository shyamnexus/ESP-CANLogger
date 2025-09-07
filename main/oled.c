#include "oled.h"
#include <stdio.h>
#include <stdarg.h>

void ssd1306_scrolllog_init(const char *logfile) {
    printf("[OLED] Init log: %s\n", logfile);
}

void ssd1306_scrolllog_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}
