#pragma once

#include <array>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "vdb/crc32.hpp"

namespace VDB {
uint32_t time_ms();
void delay_ms(uint32_t ms);
} // namespace VDB

// #define VDPTRACE
#define VDPDEBUG
#define VDPWARN

#ifdef VDPWARN
#define VDPWarnf(fmt, ...) printf("WARN: " fmt "\n", __VA_ARGS__)
#else
#define VDPWarnf(...)
#endif

#ifdef VDPDEBUG
#define VDPDebugf(fmt, ...) printf("DEBUG: " fmt "\n", __VA_ARGS__)
#else
#define VDPDebugf(...)
#endif

#ifdef VDPTRACE
#define VDPTracef(fmt, ...) printf("TRACE: " fmt "\n", __VA_ARGS__)
#else
#define VDPTracef(...)
#endif

namespace VDP {
constexpr size_t MAX_CHANNELS = 256;

class Part;
using PartPtr = std::shared_ptr<Part>;
using Packet = std::vector<uint8_t>;

using ChannelID = uint8_t;
class Channel {
public:
  friend class Registry;
  explicit Channel(PartPtr schema_data) : data(schema_data) {}
  PartPtr data;

  ChannelID getID() const;

private:
  Channel(PartPtr schema_data, ChannelID channel_id)
      : data(schema_data), id(channel_id) {}

  ChannelID id = 0;
  Packet packet_scratch_space;
  bool acked = false;
  // std::vector
};

void dump_packet(const Packet &pac);
std::pair<ChannelID, PartPtr> decode_broadcast(const Packet &packet);

enum class PacketValidity : uint8_t {
  Ok,
  BadChecksum,
  TooSmall,
};

enum class PacketType : uint8_t {
  Broadcast = 0,
  Data = 1,
};
enum class PacketFunction : uint8_t {
  Send = 0,
  Acknowledge = 1,
};
struct PacketHeader {
  PacketType type;
  PacketFunction func;
};

uint8_t make_header_byte(PacketHeader head);
PacketHeader decode_header_byte(uint8_t hb);

enum class Type : uint8_t {
  Record = 0,
  String = 1,
  // Enum

  Double = 3,
  Float = 4,

  Uint8 = 5,
  Uint16 = 6,
  Uint32 = 7,
  Uint64 = 8,

  Int8 = 9,
  Int16 = 10,
  Int32 = 11,
  Int64 = 12,

};

std::string to_string(Type t);
void add_indents(std::stringstream &ss, size_t indent);

class PacketReader;
class PacketWriter;
class Visitor;

PartPtr make_decoder(PacketReader &pac);

class Part {
  friend class PacketReader;
  friend class PacketWriter;
  friend class Record;

public:
  Part(std::string name);
  virtual ~Part();
  std::string pretty_print() const;
  std::string pretty_print_data() const;

  virtual void fetch() = 0;
  virtual void read_data_from_message(PacketReader &reader) = 0;

  std::string get_name() const;
  virtual void Visit(Visitor *) const = 0;

protected:
  // These are needed to decode correctly but you shouldn't call them directly
  virtual void write_schema(PacketWriter &sofar) const = 0;
  virtual void write_message(PacketWriter &sofar) const = 0;

  virtual void pprint(std::stringstream &ss, size_t indent) const = 0;
  virtual void pprint_data(std::stringstream &ss, size_t indent) const = 0;

  std::string name;
};

class PacketReader {
public:
  PacketReader(Packet pac);
  PacketReader(Packet pac, size_t start);
  uint8_t get_byte();
  Type get_type();
  std::string get_string();

  template <typename Number> Number get_number() {
    static_assert(std::is_floating_point<Number>::value ||
                      std::is_integral<Number>::value,
                  "This function should only be used on numbers");

    if (read_head + sizeof(Number) > pac.size()) {
      printf("%s:%d: Reading a number[%d] at position %d would read past "
             "buffer of "
             "size %d\n",
             __FILE__, __LINE__, sizeof(Number), read_head, pac.size());
      return 0;
    }
    Number value = 0;
    std::memcpy(&value, &pac[read_head], sizeof(Number));
    read_head += sizeof(Number);
    return value;
  }

private:
  Packet pac;
  size_t read_head;
};

class PacketWriter {
public:
  explicit PacketWriter(Packet &scratch_space);
  void clear();
  size_t size();
  void write_byte(uint8_t b);

  void write_type(Type t);
  void write_string(const std::string &str);

  void write_channel_acknowledge(const Channel &chan);
  void write_channel_broadcast(const Channel &chan);
  void write_data_message(const Channel &part);

  const Packet &get_packet() const;

  template <typename Number> void write_number(const Number &num) {
    std::array<uint8_t, sizeof(Number)> bytes;
    std::memcpy(&bytes, &num, sizeof(Number));
    for (const uint8_t b : bytes) {
      write_byte(b);
    }
  }

private:
  Packet &sofar;
};

class AbstractDevice {
public:
  // Send a packet over some transmission medium
  // It is not specified how the packet reaches the partner
  // The transmission medium and wire format are left to the user
  virtual bool send_packet(const VDP::Packet &packet) = 0;

  // @param callback a function that will be called when a new packet is
  // available
  virtual void register_receive_callback(
      std::function<void(const VDP::Packet &packet)> callback) = 0;

  virtual ~AbstractDevice();
};

} // namespace VDP
