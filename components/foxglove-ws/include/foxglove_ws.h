#ifndef _FOXGLOVE_WS_H
#define _FOXGLOVE_WS_H

#include "esp_http_server.h"
#include "esp_log.h"

void foxglove_init(void* arg_server);
void foxglove_end(void* arg_server);

#endif