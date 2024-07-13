#include "defines.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "status_led.h"
#include "webserver.h"
#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <foxglove-ws.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mdns.h>
#include <nvs_flash.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
static const char *TAG = "main";

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

void init_wifi_ap() {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, NULL, NULL));

  wifi_config_t wifi_config = {.ap = {
                                   .ssid = NETWORK_NAME,
                                   .ssid_len = strlen(NETWORK_NAME),
                                   .channel = 0,
                                   .password = NETWORK_PASS,
                                   .max_connection = 10,
                                   .authmode = WIFI_AUTH_WPA_WPA2_PSK,
                                   .pmf_cfg =
                                       {
                                           .required = false,
                                       },
                               }};
  if (strlen(NETWORK_PASS) == 0) {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
}

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

  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = EXAMPLE_ESP_WIFI_SSID,
              .password = EXAMPLE_ESP_WIFI_PASS,
              /* Authmode threshold resets to WPA2 as default if password
               * matches WPA2 standards (password len => 8). If you want to
               * connect the device to deprecated WEP/WPA networks, Please set
               * the threshold value to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set
               * the password with length and format matching to
               * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
               */
              .threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK,
              .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
              .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
          },
  };
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

  httpd_handle_t server_handle = http_log_start(80);
  if (server_handle == NULL) {
    ESP_LOGE(TAG, "Failed to initialize log endpoint");
  }
}

bool setup_finished = false;

// Note: Some pins on target chip cannot be assigned for UART communication.
// Please refer to documentation for selected board and target to configure pins
// using Kconfig.
#define CONFIG_ECHO_UART_TXD 6
#define CONFIG_ECHO_UART_RXD 7
#define CONFIG_ECHO_UART_RTS 10

#define ECHO_TEST_TXD (CONFIG_ECHO_UART_TXD)
#define ECHO_TEST_RXD (CONFIG_ECHO_UART_RXD)

// RTS for RS485 Half-Duplex Mode manages DE/~RE
#define ECHO_TEST_RTS (CONFIG_ECHO_UART_RTS)

// CTS is not used in RS485 Half-Duplex Mode
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define BUF_SIZE (256)
#define BAUD_RATE 115200
#define BRAIN_UART 1

// Read packet timeout
#define PACKET_READ_TICS (100 / portTICK_PERIOD_MS)
#define ECHO_TASK_STACK_SIZE (2048)
#define ECHO_TASK_PRIO (10)

// Timeout threshold for UART = number of symbols (~10 tics) with unchanged
// state on receive pin
#define ECHO_READ_TOUT (3) // 3.5T * 8 = 28 ticks, TOUT=3 -> ~24..33 ticks

static void echo_send(const int port, const char *str, uint8_t length) {
  int got = uart_write_bytes(port, str, length);
  if (got != length) {
    ESP_LOGE(TAG, "Send data critical failure.. Tried %d got %d", length, got);
  }
}
void init_serial() {
  uart_config_t uart_config = {
      .baud_rate = BAUD_RATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .rx_flow_ctrl_thresh = 122,
      .source_clk = UART_SCLK_DEFAULT,
  };
  ESP_LOGI(TAG, "Start RS485 application test and configure UART.");

  // Install UART driver (we don't need an event queue here)
  // In this example we don't even use a buffer for sending data.
  ESP_ERROR_CHECK(uart_driver_install(BRAIN_UART, BUF_SIZE * 2, 0, 0, NULL, 0));

  // Configure UART parameters
  ESP_ERROR_CHECK(uart_param_config(BRAIN_UART, &uart_config));

  ESP_LOGI(TAG, "UART set pins, mode and install driver.");

  // Set UART pins as per KConfig settings
  ESP_ERROR_CHECK(uart_set_pin(BRAIN_UART, ECHO_TEST_TXD, ECHO_TEST_RXD,
                               ECHO_TEST_RTS, ECHO_TEST_CTS));

  // Set RS485 half duplex mode
  ESP_ERROR_CHECK(uart_set_mode(BRAIN_UART, UART_MODE_RS485_HALF_DUPLEX));

  // Set read timeout of UART TOUT feature
  ESP_ERROR_CHECK(uart_set_rx_timeout(BRAIN_UART, ECHO_READ_TOUT));
}

void app_main(void) {

  ESP_LOGI(TAG, "Initializing NVS...");
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(TAG, "Initializing GPIO...");
  status_led_init(GPIO_NUM_4);

  // ESP_LOGI(TAG, "Initializing WiFi AP...");
  // init_wifi_ap();

  ESP_LOGI(TAG, "Initializing WiFi STA...");
  init_wifi_sta();

  ESP_LOGI(TAG, "Initializing MDNS...");
  init_mdns();

  ESP_LOGI(TAG, "Initializing Serial Connection...");
  init_serial();

  // ESP_LOGI(TAG, "Initializing foxglove-ws...");

  // httpd_handle_t server_handle = NULL;
  // foxglove_init_ws(&server_handle);

  ESP_LOGI(TAG, "Finished Initialization.");
  setup_finished = true;

  status_led_signal_wifi_conn();

  // Allocate buffers for UART
  uint8_t *data = (uint8_t *)malloc(BUF_SIZE);
  ESP_LOGI(TAG, "UART start recieve loop.\r");
  while (1) {

    // Read data from UART
    int len = uart_read_bytes(BRAIN_UART, data, BUF_SIZE, PACKET_READ_TICS);

    // Write data back to UART
    if (len > 0) {
      ESP_LOGI(TAG, "Received %u bytes:", len);
      printf("[ ");
      for (int i = 0; i < len; i++) {
        printf("0x%.2X ", (uint8_t)data[i]);
        echo_send(BRAIN_UART, "eaea", 4);
      }
      printf("] \n");
    } else {
      ESP_ERROR_CHECK(uart_wait_tx_done(BRAIN_UART, 10));
    }
  }

  while (true) {

    delay(1000);
  }
}