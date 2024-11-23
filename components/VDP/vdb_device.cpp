#include "vdb_device.h"
#include "vdb/protocol.hpp"
#include "vdb/types.hpp"

#include "esp_check.h"
#include "esp_crc.h"
#include "esp_err.h"
#include "esp_log.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <vector>

#include "esp_timer.h"

static constexpr const char *TAG = "VDB";

struct HardwareThreadData {
  VDP::Registry &reg;
  uart_port_t uart;
};

void VDBDevice::uart_event_task(void *pvParameters) {
  VDBDevice *self = (VDBDevice *)pvParameters;
  uart_port_t uart_num = self->uart_num;

  uart_event_t event;

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
  uint8_t *dtmp = (uint8_t *)malloc(RD_BUF_SIZE);
  int ret;

  for (;;) {
    // Waiting for UART event.
    if (xQueueReceive(self->uart0_queue, (void *)&event,
                      (TickType_t)portMAX_DELAY)) {
      bzero(dtmp, RD_BUF_SIZE);
      switch (event.type) {
      // Event of UART receving data
      /*We'd better handler data event fast, there would be much more data
      events than other types of events. If we take too much time on data event,
      the queue might be full.*/
      case UART_DATA:
        ret = uart_read_bytes(uart_num, dtmp, event.size, portMAX_DELAY);
        if (ret < 0) {
          ESP_LOGE(TAG, "Error reading bytes from uart: %d\n", ret);
        } else {
          self->handle_uart_bytes(dtmp, ret);
        }
        break;
      // Event of HW FIFO overflow detected
      case UART_FIFO_OVF:
        ESP_LOGI(TAG, "hw fifo overflow");
        // If fifo overflow happened, you should consider adding flow control
        // for your application. The ISR has already reset the rx FIFO, As an
        // example, we directly flush the rx buffer here in order to read more
        // data.
        uart_flush_input(uart_num);
        xQueueReset(self->uart0_queue);
        break;
      // Event of UART ring buffer full
      case UART_BUFFER_FULL:
        ESP_LOGI(TAG, "ring buffer full");
        // If buffer full happened, you should consider increasing your buffer
        // size As an example, we directly flush the rx buffer here in order to
        // read more data.
        uart_flush_input(uart_num);
        xQueueReset(self->uart0_queue);
        break;
      // Event of UART RX break detected
      case UART_BREAK:
        ESP_LOGI(TAG, "uart rx break");
        break;
      // Event of UART parity check error
      case UART_PARITY_ERR:
        ESP_LOGI(TAG, "uart parity error");
        break;
      // Event of UART frame error
      case UART_FRAME_ERR:
        ESP_LOGI(TAG, "uart frame error");
        break;
      // Others
      default:
        ESP_LOGI(TAG, "uart event type: %d", event.type);
        break;
      }
    }
  }
  free(dtmp);
  dtmp = NULL;
  vTaskDelete(NULL);
}

void VDBDevice::handle_uart_bytes(const uint8_t *buf, int size) {
  for (int i = 0; i < size; i++) {
    uint8_t b = buf[i];
    if (b == 0x00 && inbound_buffer.size() > 0) {
      VDB::WirePacket *copy_buf = new VDB::WirePacket(inbound_buffer);

      xQueueSend(packet_queue, (void *)&copy_buf, 4);

      // Starting a new packet now
      inbound_buffer.clear();
    } else if (b != 0x00) {
      inbound_buffer.push_back(b);
    }
  }
}

esp_err_t VDBDevice::init_serial(VDBDevice *self, uart_port_t uart_num,
                                 int tx_num, int rx_num, int rts_num,
                                 int baud) {
  uart_config_t uart_config = {
      .baud_rate = baud,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, // UART_HW_FLOWCTRL_RTS
      .rx_flow_ctrl_thresh = 48,
      .source_clk = UART_SCLK_DEFAULT,

  };
  ESP_LOGI(TAG, "Start RS485 configure UART.");

  // Install UART driver, and get the queue.
  ESP_RETURN_ON_ERROR(uart_param_config(uart_num, &uart_config), TAG,
                      "failed to param_config");

  ESP_RETURN_ON_ERROR(
      uart_set_pin(uart_num, tx_num, rx_num, rts_num, UART_PIN_NO_CHANGE), TAG,
      "Failed to set UART Pins");

  ESP_RETURN_ON_ERROR(uart_driver_install(uart_num, BUF_SIZE * 2, BUF_SIZE * 2,
                                          20, &self->uart0_queue, 0),
                      TAG, "Failed to install driver");

  // Set RS485 half duplex mode
  ESP_RETURN_ON_ERROR(uart_set_mode(uart_num, UART_MODE_RS485_HALF_DUPLEX), TAG,
                      "Failed to set UART mode");

  xTaskCreate(VDBDevice::uart_event_task, "uart_handler_task", 2048, self, 12,
              NULL);

  xTaskCreate(VDBDevice::packet_handler_thread, "packet_handler_thread", 4096,
              self, 12, NULL);

  ESP_LOGI(TAG, "Finish RS485 configure UART.");
  ESP_RETURN_ON_ERROR(uart_set_rts(uart_num, 1), TAG, "failed to set rts");

  return ESP_OK;
}

void VDBDevice::packet_handler_thread(void *pvParameters) {
  VDBDevice *self = (VDBDevice *)pvParameters;

  for (;;) {
    VDB::WirePacket *wire_packet;
    // Waiting for UART event.
    if (xQueueReceive(self->packet_queue, (void *)&wire_packet,
                      (TickType_t)portMAX_DELAY)) {

      VDPTracef("Recieved wire packet of size %d", (int)wire_packet->size());

      VDP::Packet packet;
      VDB::CobsDecode(*wire_packet, packet);
      delete wire_packet;
      self->callback(packet);
    }
  }
}

VDBDevice::VDBDevice(uart_port_t uart_num, int tx_num, int rx_num, int rts_num,
                     int baud)
    : uart_num(uart_num),
      packet_queue(
          xQueueCreate(NUM_INCOMING_PACKETS, sizeof(VDB::WirePacket *))) {
  esp_err_t res = init_serial(this, uart_num, tx_num, rx_num, rts_num, baud);

  if (res != ESP_OK) {
    ESP_LOGE(TAG, "Error initializing VDB Device: %d", res);
  }
}

void VDBDevice::register_receive_callback(
    std::function<void(const VDP::Packet &packet)> new_callback) {
  callback = new_callback;
}
bool VDBDevice::send_packet(const VDP::Packet &pac) {
  VDB::WirePacket out;
  VDB::CobsEncode(pac, out);

  // ESP_ERROR_CHECK_WITHOUT_ABORT(uart_set_rts(uart_num, 0), TAG,
  // "failed to set rts");

  int res = uart_write_bytes(uart_num, (const char *)out.data(), out.size());
  if (res < 0) {
    ESP_LOGW(TAG, "Failed to write bytes: error code %d", (int)res);
  } else if (res != out.size()) {
    ESP_LOGW(TAG, "Didn't write all bytes. Wanted %d got %d", (int)out.size(),
             res);
  }
  // ESP_ERROR_CHECK_WITHOUT_ABORT(uart_set_rts(uart_num, 1), TAG,
  // "failed to set rts");

  return res == ESP_OK;
}

namespace VDB {
uint32_t time_ms() { return esp_timer_get_time() / 1000; }
void delay_ms(uint32_t ms) { vTaskDelay(ms / portTICK_PERIOD_MS); }

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
      out[output_code_head] = (uint8_t)code_value;
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
}

} // namespace VDB
