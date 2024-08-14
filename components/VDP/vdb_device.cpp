#include "vdb_device.h"
#include "protocol.h"
#include "types.h"

#include "esp_check.h"
#include "esp_crc.h"

#include "esp_log.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <vector>

static constexpr const char *TAG = "VDB";

TaskHandle_t reader_handle = NULL;
TaskHandle_t writer_handle = NULL;

struct HardwareThreadData {
  VDP::Registry &reg;
  uart_port_t uart;
};

extern "C" void reader_thread(void *ptr) {
  HardwareThreadData data = *(HardwareThreadData *)ptr;
  ESP_LOGI(TAG, "Reader thread began with uart %d", data.uart);
  // static constexpr TickType_t almost_forever = 0xFF; // ~49 days

  constexpr size_t mike_buflen = 512;
  uint8_t buf[mike_buflen] = {0};

  while (true) {
    int read = uart_read_bytes(data.uart, buf, mike_buflen, 100);
    if (read < 0) {
      ESP_LOGW(TAG, "Failure reading uart");
      continue;
    } else if (read == 0) {
      // Read nothing
      ESP_LOGI(TAG, "Uart Read timed out");
      continue;
    }
    ESP_LOGI(TAG, "Read %d bytes\n", read);
    // sleep(1);
  }
}
// extern "C" void writer_thread(void *ptr) {
//   ESP_LOGI(TAG, "Writer thread began");
//   HardwareThreadData data = *(HardwareThreadData *)ptr;

//   VDP::PartPtr schema{new VDP::Float("asdf", []() { return 1.0; })};

//   VDP::PacketWriter writ{};
//   writ.write_channel_broadcast({1, schema});
//   VDB::WirePacket encoded;
//   VDB::CobsEncode(writ.get_packet(), encoded);

//   // data.reg.

//   while (true) {
//     uart_write_bytes(data.uart, encoded.data(), encoded.size());
//     vTaskDelay(0x2F);
//   }
// }
// static void echo_send(const uart_port_t port, const char *str, uint8_t
// length) {
//   int got = uart_write_bytes(port, str, length);
//   if (got != length) {
//     ESP_LOGE(TAG, "Send data critical failure.. Tried %d got %d", length,
//     got);
//   }
// }

esp_err_t init_serial(uart_port_t uart_num, int tx_num, int rx_num, int rts_num,
                      int baud, VDP::Registry &reg) {
  constexpr int read_timeout = 3;

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
  ESP_RETURN_ON_ERROR(uart_driver_install(uart_num, 1024, 0, 0, NULL, 0), TAG,
                      "Failed to install UART Driver");

  // Configure UART parameters
  ESP_RETURN_ON_ERROR(uart_param_config(uart_num, &uart_config), TAG,
                      "Failed to set UART config");

  // Set UART pins as per KConfig settings
  ESP_RETURN_ON_ERROR(
      uart_set_pin(uart_num, tx_num, rx_num, rts_num, UART_PIN_NO_CHANGE), TAG,
      "Failed to set UART Pins");

  // Set RS485 half duplex mode
  ESP_RETURN_ON_ERROR(uart_set_mode(uart_num, UART_MODE_RS485_HALF_DUPLEX), TAG,
                      "Failed to set UART mode");

  // Set read timeout of UART TOUT feature
  ESP_RETURN_ON_ERROR(uart_set_rx_timeout(uart_num, read_timeout), TAG,
                      "Failed to set UART Timeout");

  HardwareThreadData *dat = new HardwareThreadData(reg, uart_num);

  xTaskCreate((TaskFunction_t)reader_thread, "reader thread", 4098, dat,
              tskIDLE_PRIORITY, &reader_handle);

  return ESP_OK;
}

namespace VDP {
uint32_t crc32_one(uint32_t accum, uint8_t b) {

  return crc32_buf(accum, &b, 1);
  // return VDB::dummy_vexlink::crc32_one(accum, b);
}
uint32_t crc32_buf(uint32_t accum, const uint8_t *b, uint32_t length) {
  // return VDB::dummy_vexlink::crc32_buf(accum, b, length);
  return esp_crc32_le(accum, b, length);
}
} // namespace VDP
namespace VDB {

void CobsEncode(const VDP::Packet &in, WirePacket &out) {
  out.clear();
  if (in.size() == 0) {
    return;
  }
  size_t output_size = in.size() + (in.size() / 254) + 1;
  output_size += 2; // delimeter bytes
  out.resize(output_size);
  out[0] = 0;

  size_t output_code_head = 1;
  size_t code_value = 1;

  size_t input_head = 0;
  size_t output_head = 2;
  size_t length = 0;
  while (input_head < in.size()) {
    if (in[input_head] == 0 || length == 255) {
      out[output_code_head] = code_value;
      code_value = 1;
      output_code_head = output_head;
      length = 0;
      output_head++;
      if (in[input_head] == 0) {
        input_head++;
      }
    } else {
      out[output_head] = in[input_head];
      code_value++;
      input_head++;
      output_head++;
    }
    length++;
  }
  out[output_code_head] = code_value;

  // Trailing delimeter
  out[output_head] = 0;
  output_head++;

  out.resize(output_head);
}

void CobsDecode(const WirePacket &in, VDP::Packet &out) {
  out.clear();
  if (in.size() == 0) {
    return;
  }

  out.resize(in.size() + in.size() / 254);
  uint8_t code = 0xff;
  uint8_t left_in_block = 0;
  size_t write_head = 0;
  for (const uint8_t byte : in) {
    if (left_in_block) {
      out[write_head] = byte;
      write_head++;
    } else {
      left_in_block = byte;
      if (left_in_block != 0 && (code != 0xff)) {
        out[write_head] = 0;
        write_head++;
      }
      code = left_in_block;
      if (code == 0) {
        // hit a delimeter
        break;
      }
    }
    left_in_block--;
  }
  out.resize(write_head);

  return;
}

} // namespace VDB
