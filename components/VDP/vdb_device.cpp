#include "vdb_device.h"
#include "protocol.h"

#include "esp_check.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>
static constexpr char *TAG = "VDB";

namespace VDP {
esp_err_t init_serial(uart_port_t uart_num, int tx_num, int rx_num, int rts_num,
                      int baud) {
  constexpr int read_timeout = 3;
  constexpr int buf_size = 256;

  uart_config_t uart_config = {
      .baud_rate = baud,
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
  ESP_ERROR_CHECK(uart_driver_install(uart_num, buf_size * 2, 0, 0, NULL, 0));

  // Configure UART parameters
  ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));

  ESP_LOGI(TAG, "UART set pins, mode and install driver.");

  // Set UART pins as per KConfig settings
  ESP_ERROR_CHECK(uart_set_pin(uart_num, tx_num, rx_num, rts_num, rts_num));

  // Set RS485 half duplex mode
  ESP_ERROR_CHECK(uart_set_mode(uart_num, UART_MODE_RS485_HALF_DUPLEX));

  // Set read timeout of UART TOUT feature
  ESP_ERROR_CHECK(uart_set_rx_timeout(uart_num, read_timeout));
  return ESP_OK;
}

uint32_t crc32_one(uint32_t accum, uint8_t b) {
  return 0;
  // return VDB::dummy_vexlink::crc32_one(accum, b);
}
uint32_t crc32_buf(uint32_t accum, const uint8_t *b, uint32_t length) {
  // return VDB::dummy_vexlink::crc32_buf(accum, b, length);
  return 0;
}
} // namespace VDP
