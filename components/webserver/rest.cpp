#include "rest.hpp"
#include "cJSON.h"
#include "common.hpp"
#include <esp_chip_info.h>
#include <esp_log.h>
#include <esp_timer.h>
static const char *TAG = "api";

const char *chip_model_to_string(esp_chip_model_t model) {
  switch (model) {
  case CHIP_ESP32:
    return "ESP32";
  case CHIP_ESP32S2:
    return "ESP32S2";
  case CHIP_ESP32S3:
    return "ESP32S3";
  case CHIP_ESP32C3:
    return "ESP32C3";
  case CHIP_ESP32C2:
    return "ESP32C2";
  case CHIP_ESP32C6:
    return "ESP32C6";
  case CHIP_ESP32H2:
    return "ESP32H2";
  case CHIP_ESP32P4:
    return "ESP32P4";
  case CHIP_POSIX_LINUX:
    return "POSIX_LINUX";
  }
  return "UNKOWN CHIP";
}

esp_err_t sysinfo_handler(httpd_req_t *req) {

  cJSON *root = cJSON_CreateObject();
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  cJSON_AddStringToObject(root, "version", IDF_VER);
  cJSON_AddStringToObject(root, "model", chip_model_to_string(chip_info.model));
  cJSON_AddNumberToObject(root, "cores", chip_info.cores);
  cJSON_AddStringToObject(root, "ip", get_ip_str());

  // or print unformatted, save a couple bytes worth of spaces
  const char *json_str = cJSON_Print(root);

  ESP_ERROR_CHECK(httpd_resp_set_type(req, "application/json"));
  esp_err_t err = httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);

  // Cleanup
  cJSON_Delete(root);
  cJSON_free((void *)json_str);
  return err;
}
httpd_uri_t sysinfo_get = {
    .uri = "/api/sysinfo",
    .method = HTTP_GET,
    .handler = sysinfo_handler,
    .user_ctx = NULL,
};

esp_err_t heartbeat_handler(httpd_req_t *req) {

  cJSON *root = cJSON_CreateObject();

  cJSON_AddNumberToObject(root, "uptimems", esp_timer_get_time() / 1000);

  const char *json_str = cJSON_Print(root);

  ESP_ERROR_CHECK(httpd_resp_set_type(req, "application/json"));
  esp_err_t err = httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);

  // Cleanup
  cJSON_Delete(root);
  cJSON_free((void *)json_str);
  return err;
}

httpd_uri_t heartbeat_get = {
    .uri = "/api/heartbeat",
    .method = HTTP_GET,
    .handler = heartbeat_handler,
    .user_ctx = NULL,
};

esp_err_t init_rest_server(httpd_handle_t server) {

  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &sysinfo_get));
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &heartbeat_get));

  ESP_LOGI(TAG, "REST server started");
  return ESP_OK;
}