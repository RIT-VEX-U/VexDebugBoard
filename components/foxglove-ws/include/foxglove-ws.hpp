#ifndef _FOXGLOVE_WS
#define _FOXGLOVE_WS

#include "esp_http_server.h"
#include "esp_log.h"

void foxglove_init_ws(void* arg_server);
void foxglove_end_ws(void* arg_server);

#endif