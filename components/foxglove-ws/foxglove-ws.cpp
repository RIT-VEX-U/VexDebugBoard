#include "foxglove-ws.hpp"
#include <stdio.h>

static const char *TAG = "foxglove_ws";

esp_err_t foxglove_ws_handler(httpd_req_t *r) { return 0; }

static const httpd_uri_t ws = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = foxglove_ws_handler,
    .user_ctx = NULL,
    .is_websocket = true,
};

void foxglove_init_ws(void *arg_server) {
  httpd_handle_t *server = (httpd_handle_t *)arg_server;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  config.server_port = 8765;

  ESP_LOGI(TAG, "Starting websocket");

  esp_err_t retval = httpd_start(server, &config);
  if (retval == ESP_OK) {
    ESP_LOGI(TAG, "Registering URI Handlers");
    httpd_register_uri_handler(*server, &ws);
  } else {
    ESP_ERROR_CHECK(retval);
  }
}

void foxglove_end_ws(void *arg_server) {
  httpd_handle_t *server = (httpd_handle_t *)arg_server;
  httpd_stop(*server);
}
