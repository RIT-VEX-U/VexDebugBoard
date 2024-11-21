#pragma once
#include "driver/uart.h"
#include "esp_err.h"
#include "vdb/registry.hpp"

esp_err_t init_serial(uart_port_t uart_num, int tx_num, int rx_num, int rts_num,
                      int baud, VDP::Registry &reg);

namespace VDB {
using WirePacket = std::vector<uint8_t>; // 0x00 delimeted, cobs encoded

void CobsEncode(const VDP::Packet &in, WirePacket &out);
void CobsDecode(const WirePacket &in, VDP::Packet &out);

} // namespace VDB
