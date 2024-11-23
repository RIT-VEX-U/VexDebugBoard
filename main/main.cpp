#include "connection_manager.h"
#include "defines.h"
#include "status_led.hpp"
#include "webserver.hpp"

#include "common.hpp"

#include "vdb/registry.hpp"
#include "vdb/types.hpp"

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

#define BRAIN_BAUD_RATE (115200 * 2)
#define BRAIN_UART UART_NUM_1

class JSONVisitor : public VDP::UpcastNumbersVisitor {
public:
  void VisitRecord(const VDP::Record *record) {
    printf("got record\n");
    for (const VDP::PartPtr &field : record->get_fields()) {
      field->Visit(this);
    }
  }
  void VisitString(const VDP::String *str) { printf("got string\n"); }
  void VisitAnyFloat(const std::string &name, double value,
                     const VDP::Part *) override {
    printf("Got float\n");
  }
  void VisitAnyInt(const std::string &name, int64_t value,
                   const VDP::Part *) override {
    printf("Got int\n");
  }
  void VisitAnyUint(const std::string &name, uint64_t value,
                    const VDP::Part *) override {
    printf("Got uint\n");
  }
};

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
  VDBDevice dev{BRAIN_UART, BRAIN_UART_TXD, BRAIN_UART_RXD, BRAIN_UART_RTS,
                BRAIN_BAUD_RATE};
  VDP::Registry reg{&dev, VDP::Registry::Listener};

  httpd_handle_t server_handle = webserver_start(80);
  if (server_handle == NULL) {
    ESP_LOGE(TAG, "Failed to initialize log endpoint");
  }

  reg.install_broadcast_callback(
      [](const VDP::Channel &chan) { printf("got channel\n"); });

  reg.install_data_callback([](const VDP::Channel &chan) {
    JSONVisitor visitor;
    chan.data->Visit(&visitor);
    printf("got data\n");
  });

  ESP_LOGI(TAG, "Finished Initialization.");
  setup_finished = true;

  status_led_signal_wifi_conn();

  while (true) {

    delay(1000);
  }
}
