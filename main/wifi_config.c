#include "wifi_config.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "lwip/ip4_addr.h"
#include <string.h>
#include <stdio.h>
#include <dirent.h>

static const char *TAG = "WIFI_AP";

#define NVS_NAMESPACE "config"

// --- Config values ---
static int log_rate_hz = 10;
static int can_speed   = 500; // kbps

// --- Live status values ---
static float live_temps[4] = {0};
static double live_lat = 0.0, live_lon = 0.0;
static int live_fix = 0;
static int live_can_id = -1;
static int live_can_len = 0;
static uint8_t live_can_data[8] = {0};

// --- Getters for logger ---
int get_log_rate(void) { return log_rate_hz; }
int get_can_speed(void) { return can_speed; }

// --- Update live data from logger ---
void update_live_status(float temps[4], int fix, double lat, double lon,
                        int can_id, int can_len, uint8_t *can_bytes) {
    for (int i = 0; i < 4; i++) live_temps[i] = temps[i];
    live_fix = fix;
    live_lat = lat;
    live_lon = lon;
    live_can_id = can_id;
    live_can_len = can_len;
    if (can_len > 0 && can_bytes) {
        for (int i = 0; i < can_len; i++) live_can_data[i] = can_bytes[i];
    }
}

// --- NVS persistence ---
static void save_config() {
    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs) == ESP_OK) {
        nvs_set_i32(nvs, "rate", log_rate_hz);
        nvs_set_i32(nvs, "can", can_speed);
        nvs_commit(nvs);
        nvs_close(nvs);
        ESP_LOGI(TAG, "Saved config: rate=%dHz can=%dkbps", log_rate_hz, can_speed);
    }
}

static void load_config() {
    nvs_handle_t nvs;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs) == ESP_OK) {
        int32_t rate, can;
        if (nvs_get_i32(nvs, "rate", &rate) == ESP_OK) log_rate_hz = rate;
        if (nvs_get_i32(nvs, "can", &can) == ESP_OK)   can_speed = can;
        nvs_close(nvs);
        ESP_LOGI(TAG, "Loaded config: rate=%dHz can=%dkbps", log_rate_hz, can_speed);
    }
}

// --- SPIFFS init ---
void wifi_config_spiffs_init(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SPIFFS (%s)", esp_err_to_name(ret));
        return;
    }
    size_t total=0, used=0;
    esp_spiffs_info(NULL, &total, &used);
    ESP_LOGI(TAG, "SPIFFS: total=%d, used=%d", total, used);
}

// --- Serve static files from /spiffs ---
static esp_err_t file_get_handler(httpd_req_t *req) {
    char filepath[64] = "/spiffs";
    if (strcmp(req->uri, "/") == 0) {
        strcat(filepath, "/index.html");
    } else {
        strcat(filepath, req->uri);
    }

    FILE *file = fopen(filepath, "r");
    if (!file) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    char buffer[256];
    size_t read;
    while ((read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        httpd_resp_send_chunk(req, buffer, read);
    }
    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// --- API: /api/get (current config) ---
static esp_err_t api_get_handler(httpd_req_t *req) {
    char json[128];
    snprintf(json, sizeof(json),
             "{\"rate\":%d,\"can\":%d}",
             log_rate_hz, can_speed);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
    return ESP_OK;
}

// --- API: /api/set (update config) ---
static esp_err_t api_set_handler(httpd_req_t *req) {
    char buf[128];
    int len = httpd_req_recv(req, buf, sizeof(buf)-1);
    if (len <= 0) return ESP_FAIL;
    buf[len] = '\0';

    int rate, can;
    if (sscanf(buf, "{\"rate\":%d,\"can\":%d}", &rate, &can) == 2) {
        log_rate_hz = rate;
        can_speed = can;
        save_config();
        ESP_LOGI(TAG, "Updated config: rate=%dHz can=%dkbps", log_rate_hz, can_speed);
    }

    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

// --- API: /api/status (live values) ---
static esp_err_t api_status_handler(httpd_req_t *req) {
    char json[256];
    char can_buf[64] = {0};

    int pos = 0;
    for (int i = 0; i < live_can_len; i++) {
        pos += snprintf(can_buf+pos, sizeof(can_buf)-pos, "%02X", live_can_data[i]);
        if (i < live_can_len-1) pos += snprintf(can_buf+pos, sizeof(can_buf)-pos, " ");
    }

    snprintf(json, sizeof(json),
             "{\"fix\":%d,\"lat\":%.6f,\"lon\":%.6f,"
             "\"T1\":%.2f,\"T2\":%.2f,\"T3\":%.2f,\"T4\":%.2f,"
             "\"can_id\":%d,\"can_len\":%d,\"can_data\":\"%s\"}",
             live_fix, live_lat, live_lon,
             live_temps[0], live_temps[1], live_temps[2], live_temps[3],
             live_can_id, live_can_len, can_buf);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
    return ESP_OK;
}

// --- API: /api/files (list CSV logs) ---
static esp_err_t api_files_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "[");
    DIR *dir = opendir("/sdcard");
    if (dir) {
        struct dirent *entry;
        int first = 1;
        while ((entry = readdir(dir)) != NULL) {
            if (strstr(entry->d_name, ".csv")) {
                if (!first) httpd_resp_sendstr_chunk(req, ",");
                httpd_resp_sendstr_chunk(req, "\"");
                httpd_resp_sendstr_chunk(req, entry->d_name);
                httpd_resp_sendstr_chunk(req, "\"");
                first = 0;
            }
        }
        closedir(dir);
    }
    httpd_resp_sendstr_chunk(req, "]");
    httpd_resp_sendstr_chunk(req, NULL); // end response
    return ESP_OK;
}

// --- API: /api/download (get CSV log) ---
static esp_err_t api_download_handler(httpd_req_t *req) {
    char filepath[128] = "/sdcard/";
    char filename[64];
    if (httpd_req_get_url_query_len(req) > 0) {
        char buf[128];
        httpd_req_get_url_query_str(req, buf, sizeof(buf));
        if (httpd_query_key_value(buf, "file", filename, sizeof(filename)) == ESP_OK) {
            strcat(filepath, filename);
        }
    }

    FILE *f = fopen(filepath, "r");
    if (!f) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "text/csv");
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment");

    char buffer[256];
    size_t read;
    while ((read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        httpd_resp_send_chunk(req, buffer, read);
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

// --- Start HTTP server ---
void webserver_start(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    if (httpd_start(&server, &config) == ESP_OK) {
        // Static file serving
        httpd_uri_t file_uri = { .uri="/*", .method=HTTP_GET, .handler=file_get_handler };
        httpd_register_uri_handler(server, &file_uri);

        // APIs
        httpd_uri_t api_get = { .uri="/api/get", .method=HTTP_GET, .handler=api_get_handler };
        httpd_uri_t api_set = { .uri="/api/set", .method=HTTP_POST, .handler=api_set_handler };
        httpd_uri_t api_status = { .uri="/api/status", .method=HTTP_GET, .handler=api_status_handler };
        httpd_uri_t api_files = { .uri="/api/files", .method=HTTP_GET, .handler=api_files_handler };
        httpd_uri_t api_download = { .uri="/api/download", .method=HTTP_GET, .handler=api_download_handler };

        httpd_register_uri_handler(server, &api_get);
        httpd_register_uri_handler(server, &api_set);
        httpd_register_uri_handler(server, &api_status);
        httpd_register_uri_handler(server, &api_files);
        httpd_register_uri_handler(server, &api_download);

        ESP_LOGI(TAG, "Webserver started with API endpoints");
    }
}

// --- WiFi AP startup ---
void wifi_start_ap(void) {
    nvs_flash_init();
    load_config();

    esp_netif_init();
    esp_event_loop_create_default();

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t ap_config = {
        .ap = {
            .ssid = "ESP32-LOGGER",
            .ssid_len = strlen("ESP32-LOGGER"),
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN
        },
    };

    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 10,10,10,10);
    IP4_ADDR(&ip_info.gw, 10,10,10,10);
    IP4_ADDR(&ip_info.netmask, 255,255,255,0);

    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    esp_netif_dhcps_stop(ap_netif);
    esp_netif_set_ip_info(ap_netif, &ip_info);
    esp_netif_dhcps_start(ap_netif);

    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "AP started SSID=ESP32-LOGGER IP=10.10.10.10");
}
