#include <stdio.h>
#include <string.h>
#include <nvs_flash.h>
#include <esp_wifi.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mdns.h>

#define bit_mask(gpio) (1ull << gpio)

#define NETWORK_NAME "V5 Debug Board"
#define NETWORK_PASS "ch@ngem3!"
#define MDNS_NAME "debug"

void on_wifi_connect(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{

}

void init_gpio()
{
    // Status LED (GPIO 4)
    gpio_config_t stat_led_cfg = {
        .pin_bit_mask = bit_mask(GPIO_NUM_4),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = false,
        .pull_down_en = false,
        .intr_type = GPIO_INTR_DISABLE

    };
    gpio_config(&stat_led_cfg);

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
    mdns_service_add(NULL, "_http", "_udp", 8765, NULL, 0);

    //set default instance
    mdns_instance_name_set("VEX V5 Debug Board");

    
}

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
    init_gpio();
    printf("GPIO Initialized.\n");

    printf("Initializing WiFi AP...\n");
    init_wifi_ap();
    printf("WiFi AP Initialized.\n");
    
    printf("Initializing MDNS...\n");
    init_mdns();
    printf("MDNS Initialized.\n");

    printf("Finished Initialization.\n");

    while(true)
    {
        gpio_set_level(GPIO_NUM_4, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(GPIO_NUM_4, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    

}