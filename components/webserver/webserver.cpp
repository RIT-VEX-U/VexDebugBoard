#include "webserver.hpp"

#include "vdb/protocol.hpp"
#include <esp_http_server.h>
#include <esp_log.h>
#include <mdns.h>
#include <stdio.h>
#include <vector>

#include "freertos/semphr.h"
#include "rest.hpp"

static const char *TAG = "webserver";

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err) {
  httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "404 not found.");

  return ESP_FAIL;
}

/*
 * Structure holding server handle
 * and internal socket fd in order
 * to use out of request send
 */
struct async_resp_arg {
  httpd_handle_t hd;
  int fd;
  std::string str;
};

/*
 * async send function, which we put into the httpd work queue
 */
static void ws_async_send(void *arg) {
  struct async_resp_arg *resp_arg = (struct async_resp_arg *)arg;
  httpd_handle_t hd = resp_arg->hd;
  int fd = resp_arg->fd;
  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
  ws_pkt.payload = (uint8_t *)resp_arg->str.c_str(); // (uint8_t *)data;
  ws_pkt.len = resp_arg->str.size();                 // strlen(data);
  ws_pkt.type = HTTPD_WS_TYPE_TEXT;

  httpd_ws_send_frame_async(hd, fd, &ws_pkt);
  delete resp_arg;
}

static esp_err_t trigger_async_send(httpd_handle_t handle, int fd,
                                    std::string str) {
  struct async_resp_arg *resp_arg = new async_resp_arg{};
  if (resp_arg == NULL) {
    return ESP_ERR_NO_MEM;
  }
  resp_arg->hd = handle;
  resp_arg->fd = fd;
  resp_arg->str = str;

  esp_err_t ret = httpd_queue_work(handle, ws_async_send, resp_arg);
  if (ret != ESP_OK) {
    delete resp_arg;
  }
  return ret;
}

static httpd_handle_t global_handle;
static int global_fd = 0;

esp_err_t ws_handler(httpd_req_t *req) {
  ws_functions *funcs = (ws_functions *)req->user_ctx;
  if (req->method == HTTP_GET) {
    global_handle = req->handle;
    global_fd = httpd_req_to_sockfd(req);

    ESP_LOGI(TAG, "Handshake done, the new connection was opened");
    std::string advertisementStr = (funcs->get_adv_msg)();
    ESP_LOGI(TAG, "%s", advertisementStr.c_str());
    esp_err_t edos = send_string_to_ws(advertisementStr);

    return ESP_OK;
  }

  httpd_ws_frame_t ws_pkt;
  uint8_t *buf = NULL;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
  ws_pkt.type = HTTPD_WS_TYPE_TEXT;
  /* Set max_len = 0 to get the frame len */
  esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
    return ret;
  }
  ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
  if (ws_pkt.len) {
    /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
    buf = (uint8_t *)calloc(1, ws_pkt.len + 1);
    if (buf == NULL) {
      ESP_LOGE(TAG, "Failed to calloc memory for buf");
      return ESP_ERR_NO_MEM;
    }
    ws_pkt.payload = buf;
    /* Set max_len = ws_pkt.len to get the frame payload */
    ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
      free(buf);
      return ret;
    }
    if (ws_pkt.type != HTTPD_WS_TYPE_TEXT) {
      ESP_LOGE(TAG, "websocket was not text, unable to use %d", ret);
      free(buf);
      return ret;
    }
    printf("data received from websocket: \n\n\n%s\n\n\n", (char*)ws_pkt.payload);
    funcs->rec_cb((char *)ws_pkt.payload);

    ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
  }
  ESP_LOGI(TAG, "Packet type: %d", ws_pkt.type);

  ret = httpd_ws_send_frame(req, &ws_pkt);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
  }

  free(buf);
  return ret;
}
static httpd_uri_t ws = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = ws_handler,
    .user_ctx = NULL,

    // Mandatory: set to `true` to handler websocket protocol
    .is_websocket = true,

    // Optional: set to `true` for the handler to receive control packets, too
    .handle_ws_control_frames = false,

    // Optional: set supported subprotocol for this handler
    .supported_subprotocol = "chat",
};

/**
 * @brief Function for starting the webserver
 * @pre mDNS is initialized
 * @param port port to run server on. For browsers to see it should be 80
 */
httpd_handle_t webserver_start(uint16_t port, ws_functions *funcs) {
  /* Generate default configuration */
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = port;

  ESP_LOGI(TAG, "Starting http log server on port: '%d'", config.server_port);

  // Initialize static files
  /* Empty handle to esp_http_server */
  httpd_handle_t server = NULL;

  /* Start the httpd server */
  ESP_ERROR_CHECK(httpd_start(&server, &config));

  if (server == NULL) {
    ESP_LOGE(TAG, "Failed to start HTTP server");
    return NULL;
  }
  ws.user_ctx = (void *)funcs;

  ESP_ERROR_CHECK(httpd_register_uri_handler(server, &ws));

  // 404 Page
  ESP_ERROR_CHECK(httpd_register_err_handler(server, HTTPD_404_NOT_FOUND,
                                             &http_404_error_handler));
  init_static_files(server);
  init_rest_server(server);
  /* If server failed to start, handle will be NULL */
  return server;
}

esp_err_t send_string_to_ws(const std::string &str) {
  if (global_fd == 0) {
    ESP_LOGI(TAG, "Not sending to ws bc websocket unopened");
    return ESP_OK;
  }

  return trigger_async_send(global_handle, global_fd, str);
}

esp_err_t get_string_from_ws(const std::string &str) {
  if (global_fd == 0) {
    ESP_LOGI(TAG, "Not sending to ws bc websocket unopened");
    return ESP_OK;
  }

  return trigger_async_send(global_handle, global_fd, str);
}

/* Function for stopping the webserver */
void webserver_stop(httpd_handle_t server) {
  if (server) {
    /* Stop the httpd server */
    httpd_stop(server);
  }
}