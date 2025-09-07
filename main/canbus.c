#include "canbus.h"
#include "esp_log.h"

static const char *TAG = "CAN";
static int current_bitrate = 500;

#include "driver/twai.h"

static twai_timing_config_t bitrate_to_config(int kbps) {
    twai_timing_config_t cfg;

    switch (kbps) {
        case 125:
            cfg = (twai_timing_config_t){
                .clk_src = TWAI_CLK_SRC_DEFAULT,
                .quanta_resolution_hz = 2500000,
                .brp = 0,
                .tseg_1 = 15,
                .tseg_2 = 4,
                .sjw = 3,
                .triple_sampling = false
            };
            break;

        case 250:
            cfg = (twai_timing_config_t)TWAI_TIMING_CONFIG_250KBITS();
            break;

        case 500:
            cfg = (twai_timing_config_t)TWAI_TIMING_CONFIG_500KBITS();
            break;

        case 1000:
            cfg = (twai_timing_config_t)TWAI_TIMING_CONFIG_1MBITS();
            break;

        default:
            cfg = (twai_timing_config_t)TWAI_TIMING_CONFIG_500KBITS();
            break;
    }

    return cfg;
}


void canbus_init(int bitrate_kbps) {
    current_bitrate = bitrate_kbps;

    twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, TWAI_MODE_NORMAL);
    twai_timing_config_t  t_config = bitrate_to_config(bitrate_kbps);
    twai_filter_config_t  f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        twai_start();
        ESP_LOGI(TAG, "TWAI started at %d kbps", bitrate_kbps);
    }
}

void canbus_reinit_if_needed(int new_bitrate_kbps) {
    if (new_bitrate_kbps == current_bitrate) return;
    ESP_LOGI(TAG, "Reinit CAN: %d -> %d kbps", current_bitrate, new_bitrate_kbps);
    twai_stop();
    twai_driver_uninstall();
    canbus_init(new_bitrate_kbps);
}

int canbus_receive(twai_message_t *msg, int timeout_ms) {
    if (twai_receive(msg, pdMS_TO_TICKS(timeout_ms)) == ESP_OK) return 1;
    return 0;
}
