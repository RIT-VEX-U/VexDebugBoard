#pragma once
#include <esp_err.h>
#include <esp_http_server.h>

esp_err_t init_static_files(httpd_handle_t server);
esp_err_t init_rest_server(httpd_handle_t server);
