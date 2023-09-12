#pragma once
#include "esp_http_server.h"
#include "esp_log.h"
#include "websocket_server.hpp"

// void send_data(channel_name, data); // or something. external facing api
void foxglove_init_ws(void* arg_server);
void foxglove_end_ws(void* arg_server);

