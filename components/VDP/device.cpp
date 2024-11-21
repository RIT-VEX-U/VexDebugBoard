#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "device.hpp"
#include "vdb/protocol.hpp"
namespace VDB {
Device::Device() {}

bool Device::send_packet(const VDP::Packet &packet) {
  return false;
  // return underlying.send_packet(packet);
}
void Device::register_receive_callback(
    std::function<void(const VDP::Packet &packet)> callback) {
  // underlying.register_recieve_callback(callback);
}
