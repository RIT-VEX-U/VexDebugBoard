#include "rest.hpp"
#include "cJSON.h"
#include <esp_check.h>
#include <esp_chip_info.h>
#include <esp_log.h>
#include <esp_timer.h>

#include "common.hpp"
#include "website.h"

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
  case CHIP_ESP32C61:
    return "CHIP_ESP32C61";
  case CHIP_POSIX_LINUX:
    return "POSIX_LINUX";
  }
  return "UNKOWN CHIP";
}

esp_err_t sysinfo_handler(httpd_req_t *req) {

  cJSON *root = cJSON_CreateObject();
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  cJSON_AddStringToObject(root, "esp_version", IDF_VER);
  cJSON_AddStringToObject(root, "sw_version", get_sw_version());
  cJSON_AddStringToObject(root, "model", chip_model_to_string(chip_info.model));
  cJSON_AddNumberToObject(root, "cores", chip_info.cores);
  cJSON_AddStringToObject(root, "ip", get_ip_str());
  cJSON_AddNumberToObject(root, "bootcount", get_bootcount());

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

esp_err_t config_get_handler(httpd_req_t *req) {
  VDBConfig config = get_config();

  cJSON *root = cJSON_CreateObject();

  cJSON_AddBoolToObject(root, "use_mdns", config.use_mdns);
  cJSON_AddStringToObject(root, "mdns_hostname", config.mdns_hostname.c_str());
  cJSON_AddBoolToObject(root, "trial_run", config.trial_run);

  // cJSON_AddObjectToObject()

  const char *json_str = cJSON_Print(root);

  ESP_ERROR_CHECK(httpd_resp_set_type(req, "application/json"));
  esp_err_t err = httpd_resp_send(req, json_str, HTTPD_RESP_USE_STRLEN);

  // Cleanup
  cJSON_Delete(root);
  cJSON_free((void *)json_str);
  return err;
}

httpd_uri_t config_get = {
    .uri = "/api/config",
    .method = HTTP_GET,
    .handler = config_get_handler,
    .user_ctx = NULL,
};

esp_err_t init_rest_server(httpd_handle_t server) {

  ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &sysinfo_get), TAG,
                      "Failed to init GET handler for /api/sysinfo");
  ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &heartbeat_get), TAG,
                      "Failed to init GET handler for /api/heartbeat");

  ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &config_get), TAG,
                      "Failed to init GET handler for /api/VDBConfig");

  ESP_LOGI(TAG, "REST server started");
  return ESP_OK;
}

struct flash_file {
  const char *name;
  const char *buf;
  unsigned int size;
  const char *type;
  bool gzipped;
};

esp_err_t file_get_handler(httpd_req_t *req) {
  flash_file *file = (flash_file *)req->user_ctx;
  ESP_LOGI(TAG, "HTTP Get of %s: size %u type %s: gz: %d", file->name,
           file->size, file->type, (int)file->gzipped);
  if (file == NULL) {
    const char *resp = "Error: File requested was not ready";
    return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
  }

  ESP_ERROR_CHECK(httpd_resp_set_type(req, file->type));
  if (file->gzipped) {
    ESP_ERROR_CHECK(httpd_resp_set_hdr(req, "Content-Encoding", "gzip"));
  }
  return httpd_resp_send(req, file->buf, file->size);
}

/* URI handler structure for root GET / */
struct flash_file index_html;
httpd_uri_t index_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = file_get_handler,
    .user_ctx = &index_html,
};

struct flash_file elm_min_js;
httpd_uri_t elm_min_js_get = {
    .uri = "/elm.js",
    .method = HTTP_GET,
    .handler = file_get_handler,
    .user_ctx = &elm_min_js,
};

struct flash_file favicon_ico;
httpd_uri_t favicon_ico_get = {
    .uri = "/favicon.ico",
    .method = HTTP_GET,
    .handler = file_get_handler,
    .user_ctx = &favicon_ico,
};

esp_err_t init_static_files(httpd_handle_t server) {
  index_html.name = "index.html";
  index_html.buf = get_index_html();
  index_html.size = get_index_html_size();
  index_html.type = "text/html";
  index_html.gzipped = false;
  ESP_LOGI(TAG, "Initialized %s of size %d to %p", index_html.name,
           index_html.size, index_html.buf);

  elm_min_js.name = "elm.min.js";
  elm_min_js.buf = get_elm_min_js();
  elm_min_js.size = get_elm_min_js_size();
  elm_min_js.type = "application/javascript";
  elm_min_js.gzipped = true;
  ESP_LOGI(TAG, "Initialized %s of size %d to %p", elm_min_js.name,
           elm_min_js.size, elm_min_js.buf);

  favicon_ico.name = "favicon.ico";
  favicon_ico.buf = get_favicon();
  favicon_ico.size = get_favicon_size();
  favicon_ico.type = "image/png";
  favicon_ico.gzipped = true;
  ESP_LOGI(TAG, "Initialized %s of size %d to %p", favicon_ico.name,
           favicon_ico.size, favicon_ico.buf);

  // UI Files
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &index_get));
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &elm_min_js_get));
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &favicon_ico_get));

  return ESP_OK;
}