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
#include <HTTPLog.h>

// #include <foxglove-ws.h>

#include "status_led.h"
#include "defines.h"



static const char *TAG = "main";

void on_wifi_connect(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    status_led_signal_wifi_conn();
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

    httpd_handle_t server = http_log_start(80);
    
    http_log_raw("GPIO, WIFI AP, MDNS, HTTP Log started successfully\n");




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
    http_log_stop(server);
}
