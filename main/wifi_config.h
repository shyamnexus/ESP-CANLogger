#pragma once

// Configuration getters
int get_log_rate(void);
int get_can_speed(void);

// Live status update
void update_live_status(float temps[4], int fix, double lat, double lon,
                       int can_id, int can_len, uint8_t *can_bytes);

// SPIFFS initialization
void wifi_config_spiffs_init(void);

// WiFi and web server functions
void wifi_start_ap(void);
void webserver_start(void);