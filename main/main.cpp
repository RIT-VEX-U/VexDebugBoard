#include "connection_manager.h"
#include "defines.h"
#include "status_led.hpp"
#include "webserver.hpp"

#include "common.hpp"
#include "vdb_device.h"
#include <driver/uart.h>
#include <esp_err.h>
#include <esp_http_server.h>
#include <esp_log.h>

static const char *TAG = "main";

bool setup_finished = false;

// Note: Some pins on target chip cannot be assigned for UART communication.
// Please refer to documentation for selected board and target to configure pins
// using Kconfig.
#define BRAIN_UART_TXD 6
#define BRAIN_UART_RXD 7
#define BRAIN_UART_RTS 10

// CTS is not used in RS485 Half - Duplex Mode
// #define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define BRAIN_BAUD_RATE 115200
#define BRAIN_UART UART_NUM_1

// Read packet timeout
#define PACKET_READ_TICS (100 / portTICK_PERIOD_MS)
#define ECHO_TASK_STACK_SIZE (2048)
#define ECHO_TASK_PRIO (10)

class Device : public VDP::AbstractDevice {
  bool send_packet(const VDP::Packet &packet) override { return false; };

  // @param callback a function that will be called when a new packet is
  // available
  void register_receive_callback(
      std::function<void(const VDP::Packet &packet)> callback) override {};
};

Device dev{};
VDP::Registry reg{&dev, VDP::Registry::Listener};

extern "C" void app_main(void) {

  ESP_ERROR_CHECK(init_nvs());

  ESP_LOGI(TAG, "Initializing GPIO...");
  status_led_init(GPIO_NUM_4);

  // ESP_LOGI(TAG, "Initializing WiFi AP...");
  // init_wifi_ap();

  ESP_LOGI(TAG, "Initializing WiFi STA...");
  init_wifi_sta();

  ESP_LOGI(TAG, "Initializing MDNS...");
  init_mdns();

  ESP_LOGI(TAG, "Initializing Serial Connection...");
  esp_err_t e = init_serial(BRAIN_UART, BRAIN_UART_TXD, BRAIN_UART_RXD,
                            BRAIN_UART_RTS, BRAIN_BAUD_RATE, reg);
  ESP_ERROR_CHECK(e);

  httpd_handle_t server_handle = webserver_start(80);
  if (server_handle == NULL) {
    ESP_LOGE(TAG, "Failed to initialize log endpoint");
  }

  ESP_LOGI(TAG, "Finished Initialization.");
  setup_finished = true;

  status_led_signal_wifi_conn();

  while (true) {

    delay(1000);
  }
}
