#include "common.hpp"
#include "connection_manager.h"
#include "defines.h"
#include "foxglove-ws.hpp"
#include "status_led.hpp"
#include "visitor.hpp"
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

extern "C" void app_main(void) {

  ESP_ERROR_CHECK(init_nvs());

  ESP_LOGI(TAG, "Initializing GPIO...");
  status_led_init(GPIO_NUM_4);

  // ESP_LOGI(TAG, "Initializing WiFi AP...");
  // init_wifi_ap();

  ESP_LOGI(TAG, "Initializing WiFi STA...");
  init_wifi_sta();
  void foxglove_init_ws(void *arg_server);

  ESP_LOGI(TAG, "Initializing MDNS...");
  init_mdns();

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
  std::vector<VDP::Channel> activeChannels{};

  bool data_mode = false;
  // what the webserver does when it recieves a new channel from the brain
  reg.install_broadcast_callback(
      [&data_mode, &activeChannels](VDP::Channel new_chan) {
        if (data_mode) {
          // clear old channels
          data_mode = false;
          activeChannels = {};
        }
        // add new channel to list of active channels
        activeChannels.push_back(new_chan);
      });

  // the webserver sending data it gets from the brain to the websocket
  reg.install_data_callback(
      [&data_mode, &activeChannels](const VDP::Channel &chan) {
        DataJSONVisitor visitor;
        chan.data->Visit(&visitor);
        std::string str = visitor.get_string();
        esp_err_t e = send_string_to_ws(str);
        if (e != ESP_OK) {
          ESP_LOGW(TAG, "couldnt send to websocket");
        }
        if (!data_mode) {
          data_mode = true;
          ChannelVisitor channelVisitor;
          chan.data->Visit(&channelVisitor);
          std::string channelData = channelVisitor.get_string();
          ESP_LOGI(TAG, "%s", channelData.c_str());
          // send_to_ws();
        }
        // send_to_ws(data_json);
      });

  ESP_LOGI(TAG, "Finished Initialization.");
  setup_finished = true;

  status_led_signal_wifi_conn();

  while (true) {

    delay(1000);
  }
}