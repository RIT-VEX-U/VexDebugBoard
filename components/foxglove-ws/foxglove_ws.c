#include <stdio.h>
#include "foxglove-ws.h"

static const char* TAG = "foxglove_ws";

/**
 * Handles sending data over the socket
*/
void foxglove_ws_send(uint8_t *data, size_t len)
{

}

/**
 * Handles received data over the socket
*/
esp_err_t foxglove_ws_handler(httpd_req_t *r)
{
    if(r->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "WS Handshake Complete");
        return ESP_OK;
    }

    httpd_ws_frame_t packet;
    uint8_t *buf = NULL;

    memset(*packet, 0, sizeof(https_ws_frame_t));
    packet.type = HTTPD_WS_TYPE_TEXT;

    return ESP_OK;
}

static const httpd_uri_t ws = 
{
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = foxglove_ws_handler,
    .user_ctx = NULL,
    .is_websocket = true
};

void foxglove_init(void* arg_server)
{
    httpd_handle_t* server = (httpd_handle_t*)arg_server;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    config.server_port = 8765;

    ESP_LOGI(TAG, "Starting websocket");

    esp_err_t retval = httpd_start(server, &config);
    if(retval == ESP_OK)
    {
        ESP_LOGI(TAG, "Registering URI Handlers");
        httpd_register_uri_handler(*server, &ws);
    }else
    {
        ESP_ERROR_CHECK(retval);
    }
}

void foxglove_end(void* arg_server)
{
    httpd_handle_t* server = (httpd_handle_t*)arg_server;
    httpd_stop(*server);
}
