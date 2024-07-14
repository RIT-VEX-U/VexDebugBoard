#include "rest.hpp"

esp_err_t heartbeat_handler(httpd_req_t *req) {
  ESP_ERROR_CHECK(httpd_resp_set_type(req, "application/json"));

  return httpd_resp_send(req, R"_({"ticks": 1})_", HTTPD_RESP_USE_STRLEN);
}
httpd_uri_t heartbeat_get = {
    .uri = "/api/heartbeat",
    .method = HTTP_GET,
    .handler = heartbeat_handler,
    .user_ctx = NULL,
};

esp_err_t init_rest_server(httpd_handle_t server) {

  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &heartbeat_get));

  return ESP_OK;
}