#include "defines.h"
#include "status_led.h"
#include <WebLog.h>
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

void on_wifi_connect(void *event_handler_arg, esp_event_base_t event_base,
                     int32_t event_id, void *event_data) {
  ESP_LOGI(TAG, "Wifi Connected");
}

void init_wifi_ap() {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_connect, NULL, NULL));

  wifi_config_t wifi_config = {
      .ap =
          {
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
          },
  };
  if (strlen(NETWORK_PASS) == 0) {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
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

  ESP_LOGI(TAG, "Initializing WiFi AP...");
  init_wifi_ap();

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