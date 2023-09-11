#include <stdio.h>
#include <string.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mdns.h>
#include <esp_log.h>
#include "esp_http_server.h"

// #include <foxglove-ws.h>

#include "status_led.h"
#include "defines.h"



static const char *TAG = "main";

void on_wifi_connect(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    status_led_signal_wifi_conn();
}

esp_err_t get_handler(httpd_req_t *req)
{
    /* Send a simple response */
    ESP_LOGI(TAG, "Got GET Request");
    const char resp[] = "This is richie's GET at long last!!!!!!";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t err404_handler(httpd_req_t *req)
{
    /* Send a simple response */
    ESP_LOGI(TAG, "Got 404ed");
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "404 not found.");
    return ESP_FAIL;
}

/* URI handler structure for GET /uri */
httpd_uri_t uri_get = {
    .uri = "/uri",
    .method = HTTP_GET,
    .handler = get_handler,
    .user_ctx = NULL,
    .is_websocket = false,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL,
    };

/* Function for starting the webserver */
httpd_handle_t start_webserver(uint16_t port)
{
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

    /* Empty handle to esp_http_server */
    httpd_handle_t server = NULL;

    /* Start the httpd server */
    if (httpd_start(&server, &config) == ESP_OK)
    {
        /* Register URI handlers */
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri_get));
        // ESP_ERROR_CHECK(httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, &err404_handler));
        // httpd_register_uri_handler(server, &uri_post);
    }
    /* If server failed to start, handle will be NULL */
    return server;
}

/* Function for stopping the webserver */
void stop_webserver(httpd_handle_t server)
{
    if (server)
    {
        /* Stop the httpd server */
        httpd_stop(server);
    }
}

void init_wifi_ap()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_AP_STAIPASSIGNED, &on_wifi_connect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_connect, NULL, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = NETWORK_NAME,
            .password = NETWORK_PASS,
            .ssid_len = strlen(NETWORK_NAME),
            .channel = 0,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .ssid_hidden = 0,
            .max_connection = 10,
            .beacon_interval = 100,
            .ftm_responder = false,
            .pmf_cfg = {
                .required = false,
            },
        },
    };
    if (strlen(NETWORK_PASS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void init_mdns()
{
    // initialize mDNS service
    esp_err_t err = mdns_init();
    if (err)
    {
        printf("MDNS Init failed: %d\n", err);
        return;
    }

    // set hostname
    mdns_hostname_set(MDNS_NAME);

    // add websocket service
    //  ...may not be needed?
    //  mdns_service_add(NULL, "_http", "_tcp", 8765, NULL, 0);

    // set default instance
    mdns_instance_name_set("VEX V5 Debug Board");
}

bool setup_finished = false;

void app_main(void)
{

    ESP_LOGI(TAG, "Initializing NVS...");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Initializing GPIO...");
    status_led_init(GPIO_NUM_4);

    ESP_LOGI(TAG, "Initializing WiFi AP...");
    init_wifi_ap();

    ESP_LOGI(TAG, "Initializing MDNS...");
    init_mdns();

    ESP_LOGI(TAG, "Initializing http server");
    httpd_handle_t server = start_webserver(80);
    ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0));
    ESP_ERROR_CHECK(mdns_service_instance_name_set("_http", "_tcp", "VexDebugWeb"));

    ESP_LOGI(TAG, "Webserver Status: %s", (server == NULL) ? "bad" : "good");


    ESP_LOGI(TAG, "Finished Initialization.");
    setup_finished = true;

    // delay(5000);
    // status_led_set(IDLE);
    // delay(5000);
    // status_led_set(CONNECTED);
    // delay(5000);
    // status_led_set(FAULTED);
    // delay(5000);
    // status_led_set(CONNECTED);
    // delay(5000);
    // status_led_signal_wifi_conn();

    while (true)
    {

        delay(1000);
    }
    stop_webserver(server);
}
