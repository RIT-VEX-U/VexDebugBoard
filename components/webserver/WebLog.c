#include "WebLog.h"

#include <esp_log.h>
#include <mdns.h>
#include <stdio.h>

#include "freertos/semphr.h"

static const char *TAG = "webserver";

#define MUX_TIMEOUT (500 / portTICK_PERIOD_MS)

/// @brief how much to grow the log by each time
static const size_t log_grow_amt = 256;
/// @brief  initial log capacity
static const size_t log_initial_cap = 256;

/// @brief maximum size of log to avoid taking too much memory TODO scroll log
/// rather than just cutoff
static const size_t max_log_length = 8192;

/// @brief logger structure. use mux to control reads and writes
typedef struct {
  SemaphoreHandle_t mux;
  char *str;
  int len; // what strlen would return
  int cap;
} log_internals;

/// @brief creates a valid log
log_internals create_log_internals() {
  SemaphoreHandle_t mux = xSemaphoreCreateMutex();

  log_internals li = {.mux = mux,
                      .str = malloc(log_initial_cap),
                      .len = 0,
                      .cap = log_initial_cap};
  return li;
}
void destroy_log_internals(log_internals *li) {
  if (xSemaphoreTake(li->mux, MUX_TIMEOUT)) {
    // TODO: does semaphore have to be deleted?
    free(li->str);
    li->cap = 0;
    li->len = 0;
  } else {
    ESP_LOGE(TAG, "Someone holding onto web_log semaphore. programmer error");
  }
}

/// @brief the log that the general log uses
static log_internals web_log;

/// @brief appends a string to the log with all the safeties. External people
/// should use log, logf type functions. Implementation of those functions call
/// this;
/// @param str null terminated string to add to log. can be arbitrary html.
/// Don't abuse that
static bool append_log(const char *str) {
  if (xSemaphoreTake(web_log.mux, MUX_TIMEOUT)) {

    int length = strlen(str);
    int new_log_size = web_log.len + length;

    if (new_log_size > max_log_length) {
      xSemaphoreGive(web_log.mux);
      return false;
    }
    // resize if needed
    if (new_log_size > web_log.cap) {
      web_log.str = realloc(web_log.str, web_log.cap + log_grow_amt);
      web_log.cap = web_log.cap + log_grow_amt;
      if (web_log.str == NULL) {
        ESP_LOGE(TAG, "Error growing log buffer");
        xSemaphoreGive(web_log.mux);
        return false;
      }
    }
    memcpy(web_log.str + web_log.len, str,
           length + 1); // +1 for that null terminator
    web_log.len = new_log_size;

    xSemaphoreGive(web_log.mux);
  }
  return false;
}

void http_log_raw(const char *str) { append_log(str); }

char *build_index_html;
unsigned int build_index_html_len;

char *build_elm_min_js;
unsigned int build_elm_min_js_len;

struct flash_file {
  char *name;
  char *buf;
  unsigned int size;
};

esp_err_t file_get_handler(httpd_req_t *req) {
  struct flash_file *file = req->user_ctx;
  ESP_LOGI(TAG, "HTTP Get of %s: size %u, %p", file->name, file->size,
           file->buf);
  if (file == NULL) {
    char *resp = "Error: File requested was not ready";
    return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
  }

  return httpd_resp_send(req, file->buf, file->size);
  // char*resp = "error";
  // return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
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
    .uri = "/elm.min.js",
    .method = HTTP_GET,
    .handler = file_get_handler,
    .user_ctx = &elm_min_js,
};

/**
 * @brief Function for starting the webserver
 * @pre mDNS is initialized
 * @param port port to run server on. For browsers to see it should be 80
 */
httpd_handle_t http_log_start(uint16_t port) {
  /* Generate default configuration */
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = port;

  ESP_LOGI(TAG, "Starting http log server on port: '%d'", config.server_port);

  /* Initialize log */
  web_log = create_log_internals();
  append_log("<h1> Web Log </h1>\n\n<pre>");

  // Initialize static files
  index_html.name = "index.html";
  index_html.buf = build_index_html;
  index_html.size = build_index_html_len;

  elm_min_js.name = "elm.min.js";
  elm_min_js.buf = build_elm_min_js;
  elm_min_js.size = build_elm_min_js_len;

  /* Empty handle to esp_http_server */
  httpd_handle_t server = NULL;

  /* Start the httpd server */
  if (httpd_start(&server, &config) == ESP_OK) {
    /* Register URI handlers */
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &index_get));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &elm_min_js_get));
    ESP_ERROR_CHECK(httpd_register_err_handler(server, HTTPD_404_NOT_FOUND,
                                               &http_404_error_handler));
  }
  ESP_LOGI(TAG, "Webserver Status: %s", (server == NULL) ? "bad" : "good");
  if (server == NULL) {
    return NULL;
  }

  /* If server failed to start, handle will be NULL */
  return server;
}

/* Function for stopping the webserver */
void http_log_stop(httpd_handle_t server) {
  if (server) {
    /* Stop the httpd server */
    httpd_stop(server);
    destroy_log_internals(&web_log);
  }
}