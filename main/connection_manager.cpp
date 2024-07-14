#include "connection_manager.h"

#include "esp_event.h"
#include "esp_wifi.h"
#include <esp_err.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <mdns.h>
#include <string.h>

#include "defines.h"

static const char *TAG = "con_man";

#define EXAMPLE_ESP_WIFI_SSID "ATT5MEdBiW"
#define EXAMPLE_ESP_WIFI_PASS "t8+a6m3qgu4t"
#define EXAMPLE_ESP_MAXIMUM_RETRY 5

#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
/* The event group allows multiple bits for each event, but we only care about
 * two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
/* FreeRTOS event group to signal when we are connected*/

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

static void on_wifi_event(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(TAG, "retry to connect to the AP");
    } else {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(TAG, "connect to the AP fail");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

uint8_t network_ssid[32] = NETWORK_NAME;
uint8_t network_pass[32] = NETWORK_PASS;

void init_wifi_ap() {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, NULL, NULL));

  // wifi_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
  wifi_config_t wifi_config = {};

  memcpy(wifi_config.ap.ssid, network_ssid, 32);
  wifi_config.ap.ssid_len = strlen((const char *)network_ssid);

  memcpy(wifi_config.ap.password, network_pass, 32);
  // wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
  wifi_config.ap.authmode = WIFI_AUTH_OPEN;

  wifi_config.ap.channel = 0;
  wifi_config.ap.max_connection = 10;

  wifi_config.ap.pmf_cfg.required = true;

  // wifi_config_t wifi_config = {.ap = {
  //                                  .ssid = NETWORK_NAME,
  //                                  .ssid_len = strlen(NETWORK_NAME),
  //                                  .channel = 0,
  //                                  .password = NETWORK_PASS,
  //                                  .max_connection = 10,
  //                                  .authmode = WIFI_AUTH_WPA_WPA2_PSK,
  //                                  .pmf_cfg =
  //                                      {
  //                                          .required = false,
  //                                      },
  //                              }};
  // if (strlen(NETWORK_PASS) == 0) {

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
}

uint8_t sta_ssid[32] = EXAMPLE_ESP_WIFI_SSID;
uint8_t sta_pass[32] = EXAMPLE_ESP_WIFI_PASS;

void init_wifi_sta(void) {
  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, NULL, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &on_wifi_event, NULL, &instance_got_ip));

  wifi_config_t wifi_config = {};
  memcpy(wifi_config.sta.ssid, sta_ssid, 32);
  memcpy(wifi_config.sta.password, sta_pass, 32);
  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK;
  wifi_config.sta.sae_pwe_h2e = ESP_WIFI_SAE_MODE;
  // wifi_config.sta.sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER;

  // wifi_config_t wifi_config = {
  //     .sta =
  //         {
  //             .ssid = EXAMPLE_ESP_WIFI_SSID,
  //             .password = EXAMPLE_ESP_WIFI_PASS,
  //             /* Authmode threshold resets to WPA2 as default if password
  //              * matches WPA2 standards (password len => 8). If you want to
  //              * connect the device to deprecated WEP/WPA networks, Please
  //              set
  //              * the threshold value to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and
  //              set
  //              * the password with length and format matching to
  //              * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
  //              */
  //             .threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK,
  //             .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
  //             .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
  //         },
  // };
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "wifi_init_sta finished.");

  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or
   * connection failed for the maximum number of re-tries (WIFI_FAIL_BIT). The
   * bits are set by event_handler() (see above) */
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE, pdFALSE, portMAX_DELAY);

  /* xEventGroupWaitBits() returns the bits before the call returned, hence we
   * can test which event actually happened. */
  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", EXAMPLE_ESP_WIFI_SSID,
             EXAMPLE_ESP_WIFI_PASS);
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
  } else {
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
  }
}

void init_mdns() {
  // initialize mDNS service
  esp_err_t err = mdns_init();
  if (err) {
    printf("MDNS Init failed: %d\n", err);
    return;
  }

  // set hostname
  ESP_ERROR_CHECK(mdns_hostname_set(MDNS_NAME));

  ESP_ERROR_CHECK(mdns_service_add(NULL, "_http", "_tcp", 8765, NULL, 0));

  // set default instance
  ESP_ERROR_CHECK(mdns_instance_name_set("VEX V5 Debug Board"));
}