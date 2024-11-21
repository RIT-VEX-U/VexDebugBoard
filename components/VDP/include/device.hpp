#pragma once
#include "vdb/protocol.hpp"
#include <functional>

namespace VDB {

class Device : public VDP::AbstractDevice {
public:
  explicit Device();
  bool send_packet(const VDP::Packet &packet) override;
  void register_receive_callback(
      std::function<void(const VDP::Packet &packet)> callback) override;

private:
};

} // namespace VDB
