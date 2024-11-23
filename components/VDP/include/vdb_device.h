#pragma once
#include "driver/uart.h"
#include "esp_err.h"
#include "vdb/registry.hpp"

class VDBDevice : public VDP::AbstractDevice {

public:
  static constexpr size_t NUM_INCOMING_PACKETS = 10;

  VDBDevice(uart_port_t uart_num, int tx_num, int rx_num, int rts_num,
            int baud);
  bool send_packet(const VDP::Packet &pac) override;

  void register_receive_callback(
      std::function<void(const VDP::Packet &packet)> callback) override;

  static void uart_event_task(void *pvParameters);
  static void packet_handler_thread(void *pvParameters);

  void handle_uart_bytes(const uint8_t *buf, int size);

  esp_err_t init_serial(VDBDevice *self, uart_port_t uart_num, int tx_num,
                        int rx_num, int rts_num, int baud);

private:
  uart_port_t uart_num;
  QueueHandle_t packet_queue;

  std::vector<uint8_t> inbound_buffer;
  std::function<void(const VDP::Packet &packet)> callback =
      [](const VDP::Packet &) {};
  QueueHandle_t uart0_queue;
};

namespace VDB {
using WirePacket = std::vector<uint8_t>; // 0x00 delimeted, cobs encoded

void CobsEncode(const VDP::Packet &in, WirePacket &out);
void CobsDecode(const WirePacket &in, VDP::Packet &out);

} // namespace VDB
