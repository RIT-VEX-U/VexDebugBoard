#include "common.hpp"
#include "connection_manager.h"
#include "defines.h"
#include "foxglove-ws.hpp"
#include "status_led.hpp"
#include "webserver.hpp"

#include "vdb/registry.hpp"
#include "vdb/types.hpp"

#include "vdb_device.h"

#include <driver/uart.h>
#include <esp_err.h>
#include <esp_http_server.h>
#include <esp_log.h>

static const char *TAG = "main";

bool setup_finished = false;

#include "cJSON.h"

class JSONVisitor : public VDP::UpcastNumbersVisitor {
public:
  JSONVisitor() {
    root = cJSON_CreateObject();
    node_stack.push_back(root);
  }
  ~JSONVisitor() { cJSON_Delete(root); }

  void VisitRecord(const VDP::Record *record) {
    cJSON *oldroot = current_node();
    cJSON *newroot = cJSON_CreateObject();
    node_stack.push_back(newroot);

    for (const VDP::PartPtr &field : record->get_fields()) {
      field->Visit(this);
    }
    node_stack.pop_back();

    std::string name = record->get_name();
    cJSON_AddItemToObject(oldroot, name.c_str(), newroot);
  }
  void VisitString(const VDP::String *str) {
    std::string name = str->get_name();
    std::string value = str->get_value();

    cJSON *oldroot = current_node();
    cJSON_AddStringToObject(oldroot, name.c_str(), value.c_str());
  }
  void VisitAnyFloat(const std::string &name, double value,
                     const VDP::Part *) override {
    cJSON_AddNumberToObject(current_node(), name.c_str(), value);
  }
  void VisitAnyInt(const std::string &name, int64_t value,
                   const VDP::Part *) override {
    cJSON_AddNumberToObject(current_node(), name.c_str(), (double)value);
  }
  void VisitAnyUint(const std::string &name, uint64_t value,
                    const VDP::Part *) override {
    cJSON_AddNumberToObject(current_node(), name.c_str(), (double)value);
  }

  cJSON *current_node() { return node_stack[node_stack.size() - 1]; }
  std::string get_string() {
    const char *json_str = cJSON_Print(root);
    std::string str{json_str};
    cJSON_free((void *)json_str);
    return str;
  }

private:
  cJSON *root;
  std::vector<cJSON *> node_stack;
};

extern "C" void app_main(void) {

  ESP_ERROR_CHECK(init_nvs());

  ESP_LOGI(TAG, "Initializing GPIO...");
  status_led_init(GPIO_NUM_4);

  // ESP_LOGI(TAG, "Initializing WiFi AP...");
  // init_wifi_ap();

  ESP_LOGI(TAG, "Initializing WiFi STA...");
  // init_wifi_sta();
  init_wifi_ap();
  // void foxglove_init_ws(void *arg_server);

  // ESP_LOGI(TAG, "Initializing MDNS...");
  // init_mdns();

  ESP_LOGI(TAG, "Initializing Serial Connection...");
  constexpr int BRAIN_UART_TXD = 6;
  constexpr int BRAIN_UART_RXD = 7;
  constexpr int BRAIN_UART_RTS = 10;

  constexpr int BRAIN_BAUD_RATE = (115200 * 2);
  constexpr uart_port_t BRAIN_UART = UART_NUM_1;

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
    std::string str = visitor.get_string();
    esp_err_t e = send_string_to_ws(str);
    if (e != ESP_OK) {
      ESP_LOGW(TAG, "couldnt send to websocket");
    }
  });

  ESP_LOGI(TAG, "Finished Initialization.");
  setup_finished = true;

  status_led_signal_wifi_conn();

  while (true) {

    delay(1000);
  }
}
