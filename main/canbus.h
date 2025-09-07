#pragma once
#include "driver/twai.h"

void canbus_init(int bitrate_kbps);
void canbus_reinit_if_needed(int new_bitrate_kbps);
int  canbus_receive(twai_message_t *msg, int timeout_ms);
