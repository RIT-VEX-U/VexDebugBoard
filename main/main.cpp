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

std::string send_advertisement_msg(std::vector<VDP::Channel> activeChannels) {
  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "type", "advertisement");

  cJSON *channelArray = cJSON_AddArrayToObject(root, "channels");

  std::vector<ChannelVisitor *> visitors;

  for (VDP::Channel channel : activeChannels) {
    visitors.emplace_back(new ChannelVisitor());
    channel.data->Visit(visitors[visitors.size() - 1]);
    cJSON *subObject = visitors[visitors.size() - 1]->current_node();
    cJSON *newObject = cJSON_CreateObject();

    cJSON_AddItemReferenceToObject(newObject, "schema", subObject);
    cJSON_AddNumberToObject(newObject, "channel_id", channel.getID());

    cJSON_AddItemToArray(channelArray, newObject);
  }

  const char *json_str = cJSON_Print(root);
  std::string str(json_str);
  cJSON_free((void *)json_str);
  for (auto *v : visitors) {
    delete v;
  }
  cJSON_Delete(root);
  // ESP_LOGI(TAG, "size: %d", (int)visitors[0].node_stack.size());
  return str;
}

std::string send_data_msg(VDP::Channel channel) {

  cJSON *root = cJSON_CreateObject();
  DataJSONVisitor visitor;
  channel.data->Visit(&visitor);
  cJSON_AddNumberToObject(root, "channel-id", channel.getID());
  cJSON_AddStringToObject(root, "type", "data");
  cJSON_AddItemReferenceToObject(root, "data", visitor.current_node());

  const char *json_str = cJSON_Print(root);
  std::string str(json_str);
  cJSON_free((void *)json_str);

  cJSON_Delete(root);
  // ESP_LOGI(TAG, "size: %d", (int)visitors[0].node_stack.size());
  return str;
}

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
        std::string dataStr = send_data_msg(chan);
        ESP_LOGI(TAG, "%s", dataStr.c_str());
        esp_err_t e = send_string_to_ws(dataStr);
        if (e != ESP_OK) {
          ESP_LOGW(TAG, "couldnt send to websocket");
        }
        if (!data_mode) {
          data_mode = true;
          std::string advertisementStr = send_advertisement_msg(activeChannels);
          ESP_LOGI(TAG, "%s", advertisementStr.c_str());
          esp_err_t edos = send_string_to_ws(advertisementStr);
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