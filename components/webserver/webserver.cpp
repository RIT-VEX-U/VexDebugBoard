#include "webserver.hpp"

#include <esp_log.h>
#include <mdns.h>
#include <stdio.h>

#include "freertos/semphr.h"
#include "rest.hpp"
#include "website.h"

static const char *TAG = "webserver";

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

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err) {
  httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "404 not found.");

  return ESP_FAIL;
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

/**
 * @brief Function for starting the webserver
 * @pre mDNS is initialized
 * @param port port to run server on. For browsers to see it should be 80
 */
httpd_handle_t webserver_start(uint16_t port) {
  /* Generate default configuration */
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = port;

  ESP_LOGI(TAG, "Starting http log server on port: '%d'", config.server_port);

  // Initialize static files
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

  /* Empty handle to esp_http_server */
  httpd_handle_t server = NULL;

  /* Start the httpd server */
  ESP_ERROR_CHECK(httpd_start(&server, &config));

  if (server == NULL) {
    ESP_LOGE(TAG, "Failed to start HTTP server");
    return NULL;
  }
  /* Register URI handlers */
  // UI Files
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &index_get));
  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &elm_min_js_get));
  // 404 Page
  ESP_ERROR_CHECK(httpd_register_err_handler(server, HTTPD_404_NOT_FOUND,
                                             &http_404_error_handler));
  init_rest_server(server);
  /* If server failed to start, handle will be NULL */
  return server;
}

/* Function for stopping the webserver */
void webserver_stop(httpd_handle_t server) {
  if (server) {
    /* Stop the httpd server */
    httpd_stop(server);
  }
}