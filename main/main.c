#include <stdio.h>
#include <string.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mdns.h>
#include "status_led.h"
#include "defines.h"



void on_wifi_connect(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{

}

void init_wifi_ap()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_connect, NULL, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = NETWORK_NAME,
            .ssid_len = strlen(NETWORK_NAME),
            .channel = 0,
            .password = NETWORK_PASS,
            .max_connection = 10,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = {
                    .required = false,
            },
        },
    };
    if (strlen(NETWORK_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void init_mdns()
{
    //initialize mDNS service
    esp_err_t err = mdns_init();
    if (err) {
        printf("MDNS Init failed: %d\n", err);
        return;
    }

    //set hostname
    mdns_hostname_set(MDNS_NAME);

    //add websocket service
    // ...may not be needed?
    mdns_service_add(NULL, "_http", "_tcp", 8765, NULL, 0);

    //set default instance
    mdns_instance_name_set("VEX V5 Debug Board");

    
}

bool setup_finished = false;

void app_main(void)
{
    printf("Initializing NVS...\n");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    printf("NVS Initialized.\n");

    printf("Initializing GPIO...\n");
    status_led_init(GPIO_NUM_4);
    printf("GPIO Initialized.\n");

    printf("Initializing WiFi AP...\n");
    init_wifi_ap();
    printf("WiFi AP Initialized.\n");
    
    printf("Initializing MDNS...\n");
    init_mdns();
    printf("MDNS Initialized.\n");

    printf("Finished Initialization.\n");
    setup_finished = true;

    delay(5000);
    status_led_set(IDLE);
    delay(5000);
    status_led_set(CONNECTED);
    delay(5000);
    status_led_set(FAULTED);
    delay(5000);
    status_led_set(CONNECTED);
    delay(5000);
    status_led_signal_wifi_conn();

    
    while(true)
    {
        delay(1000);
    }

    
}