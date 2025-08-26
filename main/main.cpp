#include "common.hpp"
#include "connection_manager.h"
#include "defines.h"
#include "foxglove-ws.hpp"
#include "status_led.hpp"
#include "visitor.hpp"
#include "webserver.hpp"

#include "message-format.hpp"
#include "vdb/registry-listener.hpp"
#include "vdb/types.hpp"

#include "vdb_device.h"

#include <driver/uart.h>
#include <esp_err.h>
#include <esp_http_server.h>
#include <esp_log.h>

static const char *TAG = "main";

bool setup_finished = false;

#include "cJSON.h"

std::vector<VDP::Channel> activeChannels{};

extern "C" void app_main(void) {

  ESP_ERROR_CHECK(init_nvs());

  ESP_LOGI(TAG, "Initializing GPIO...");
  status_led_init(GPIO_NUM_4);

  // init_wifi_ap();

  init_wifi_sta("WiFi Name", "WiFi Password", true);
  void foxglove_init_ws(void *arg_server);

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
  VDP::RegistryListener<std::mutex> reg{&dev};

  //callback for when we get data from the websocket to send to the brain
  std::function<void(std::string)> receive_callback =[&reg](std::string json_string) {
    ResponseJSONVisitor RV(json_string, reg);
    printf("setting the data from the websocket...\n");
    RV.set_data();
    printf("sending the data from the websocket to the register...\n");
    RV.send_to_reg();
  };
  
  //sends the advertisement message and returns the message sent
  std::function<std::string()> get_advertisement_message = []() {
    return send_advertisement_msg(activeChannels);
  };

  ws_functions funcs{
      .rec_cb = receive_callback,
      .get_adv_msg = get_advertisement_message,
  };

  httpd_handle_t server_handle = webserver_start(80, &funcs);
  if (server_handle == NULL) {
    ESP_LOGE(TAG, "Failed to initialize log endpoint");
  }

  bool data_mode = false;
  // what the webserver does when it recieves a new channel from the brain
  reg.install_broadcast_callback([&data_mode](VDP::Channel new_chan) {
    //if we are in data mode
    if (data_mode) {
      // clear old channels
      data_mode = false;
      activeChannels = {};
    }
    // add new channel to list of active channels
    activeChannels.push_back(new_chan);
  });

  // the webserver sending data it gets from the brain to the websocket
  reg.install_data_callback([&data_mode](const VDP::Channel &chan) {
    DataJSONVisitor visitor;
    chan.data->Visit(&visitor);
    std::string dataStr = send_data_msg(chan);
    ESP_LOGI(TAG, "%s", dataStr.c_str());
    esp_err_t e = send_string_to_ws(dataStr);
    if (e != ESP_OK) {
      ESP_LOGW(TAG, "couldnt send to websocket");
    }
    //if we aren't in data mode, send an advertisement and switch to data mode
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