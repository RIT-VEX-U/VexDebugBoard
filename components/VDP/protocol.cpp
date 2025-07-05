#include "vdb/protocol.hpp"
#include "vdb/types.hpp"

#include <cstdint>
#include <cstring>
#include <functional>
#include <sstream>
#include <stdio.h>
#include <string>
#include <utility>
#include <vector>

namespace VDP {
void dump_packet(const Packet &pac) {
  int i = 0;
  for (const uint8_t d : pac) {
    if (i % 16 == 0 && i != 0) {
      printf("\n");
    }
    printf("%02x ", (int)d);
    i++;
  }
  printf("\n");
}

std::string Part::get_name() const { return name; }

std::string Part::pretty_print() const {
  std::stringstream ss;
  this->pprint(ss, 0);

  return ss.str();
}
std::string Part::pretty_print_data() const {
  std::stringstream ss;
  this->pprint_data(ss, 0);
  return ss.str();
}

PacketReader::PacketReader(Packet pac) : pac(std::move(pac)), read_head(0) {}
PacketReader::PacketReader(Packet pac, size_t offset)
    : pac(std::move(pac)), read_head(offset) {}

uint8_t PacketReader::get_byte() {
  const uint8_t b = pac[read_head];
  read_head++;
  return b;
}

Type PacketReader::get_type() {
  const uint8_t val = get_byte();
  return (Type)val;
}

std::string PacketReader::get_string() {
  std::string s;

  while (1) {
    const uint8_t c = get_byte();
    if (c == 0) {
      break;
    }
    s.push_back((char)c);
  }
  return s;
}

VDP::PacketValidity validate_packet(const VDP::Packet &packet) {
  VDPTracef("Validating packet of size %d", (int)packet.size());

  // packet header byte + checksum = 5 bytes,
  static constexpr size_t min_packet_size = 5;

  // checks that the minimum packet size is met
  if (packet.size() < min_packet_size) {
    return VDP::PacketValidity::TooSmall;
  }
  // calculates the checksum for the packet
  uint32_t checksum = CRC32::calculate(packet.data(), packet.size() - 4);

  // recreates the checksum manually
  auto size = packet.size();
  const uint32_t written_checksum =
      (uint32_t(packet[size - 1]) << 24) | (uint32_t(packet[size - 2]) << 16) |
      (uint32_t(packet[size - 3]) << 8) | uint32_t(packet[size - 4]);
  // checks if both checksums match
  if (checksum != written_checksum) {
    VDPWarnf("Checksums do not match: expected: %08lx, got: %08lx", checksum,
             written_checksum);
    return VDP::PacketValidity::BadChecksum;
  }
  // if no problems with the packet are found, packet is Ok
  return VDP::PacketValidity::Ok;
}
PacketWriter::PacketWriter(VDP::Packet &scratch) : sofar(scratch) {}
void PacketWriter::write_byte(uint8_t b) { sofar.push_back(b); }

void PacketWriter::write_type(Type t) { write_byte((uint8_t)t); }
void PacketWriter::write_string(const std::string &str) {
  sofar.insert(sofar.end(), str.begin(), str.end());
  sofar.push_back(0);
}
void PacketWriter::clear() { sofar.clear(); }
size_t PacketWriter::size() { return sofar.size(); }

const Packet &PacketWriter::get_packet() const { return sofar; }

void PacketWriter::write_channel_acknowledge(const Channel &chan) {
  clear();

  const uint8_t header = make_header_byte(
      PacketHeader{
        .type = PacketType::Broadcast,
        .func = PacketFunction::Acknowledge});

  // Header
  write_number<uint8_t>(header);
  write_number<ChannelID>(chan.getID());

  // Checksum
  uint32_t crc = CRC32::calculate(sofar.data(), sofar.size());
  write_number<uint32_t>(crc);
}
void PacketWriter::write_channel_broadcast(const Channel &chan) {
  clear();
  const uint8_t header = make_header_byte(
      PacketHeader{PacketType::Broadcast, PacketFunction::Send});
  // Header
  write_number<uint8_t>(header);
  write_number<ChannelID>(chan.getID());

  // Schema Data
  chan.data->write_schema(*this);

  // Checksum
  uint32_t crc = CRC32::calculate(sofar.data(), sofar.size());

  write_number<uint32_t>(crc);
}
void PacketWriter::write_data_message(const Channel &chan) {
  clear();
  const uint8_t header =
      make_header_byte(PacketHeader{PacketType::Data, PacketFunction::Send});

  // Header
  write_number<uint8_t>(header);
  write_number<ChannelID>(chan.getID());

  // Data
  chan.data->write_message(*this);
  // Checksum
  uint32_t crc = CRC32::calculate(sofar.data(), sofar.size());
  write_number<uint32_t>(crc);
}

/**
 * writes a receive packet to the packets
 * @param chan the channel to request
 */
void PacketWriter::write_response(std::deque<Channel> &channels) {
  printf("writing channel response\n");
  clear();
  // makes a header byte with the type broadcast and the function Receive
  const uint8_t header = make_header_byte(
      PacketHeader{PacketType::Data, PacketFunction::Response});

  // writes the header byte and number of channels to send to the packet
  write_number<uint8_t>(header);

  write_number<size_t>(channels.size());
  write_number<uint8_t>(header);
  write_number<ChannelID>(channels[0].getID());
  channels[0].data->write_message(*this);
  channels.pop_front();

  // creates and writes the Checksum to the packet
  uint32_t crc = CRC32::calculate(sofar.data(), sofar.size());
  write_number<uint32_t>(crc);
}

std::string to_string(Type t) {
  switch (t) {
  case Type::Record:
    return "record";
  case Type::String:
    return "string";

  case Type::Float:
    return "float";
  case Type::Double:
    return "double";

  case Type::Uint8:
    return "uint8";
  case Type::Uint16:
    return "uint16";
  case Type::Uint32:
    return "uint32";
  case Type::Uint64:
    return "uint64";

  case Type::Int8:
    return "int8";
  case Type::Int16:
    return "int16";
  case Type::Int32:
    return "int32";
  case Type::Int64:
    return "int64";
  }

  return "<<UNKNOWN TYPE>>";
}
void add_indents(std::stringstream &ss, size_t indent) {
  for (size_t i = 0; i < indent; i++) {
    ss << "  ";
  }
}

PartPtr make_decoder(PacketReader &pac) {
  const Type t = pac.get_type();
  const std::string name = pac.get_string();

  switch (t) {
  case Type::String:
    return PartPtr(new String(name));
  case Type::Record:
    return PartPtr(new Record(name, pac));

  case Type::Float:
    return PartPtr(new Float(name));
  case Type::Double:
    return PartPtr(new Double(name));

  case Type::Uint8:
    return PartPtr(new Uint8(name));
  case Type::Uint16:
    return PartPtr(new Uint16(name));
  case Type::Uint32:
    return PartPtr(new Uint32(name));
  case Type::Uint64:
    return PartPtr(new Uint64(name));

  case Type::Int8:
    return PartPtr(new Int8(name));
  case Type::Int16:
    return PartPtr(new Int16(name));
  case Type::Int32:
    return PartPtr(new Int32(name));
  case Type::Int64:
    return PartPtr(new Int64(name));
  }
  return nullptr;
}
//thank you jack
void print_bits(uint8_t byte) {
  for (int i = 7; i >= 0; i--) {
    printf("%d", (byte >> i) & 1);
  }
  printf("\n");
}

static constexpr auto PACKET_TYPE_BIT_MASK = 0b10000000;
static constexpr auto PACKET_FUNCTION_BIT_MASK = 0b01100000;

uint8_t make_header_byte(PacketHeader head) {
  return (uint8_t)head.type | (uint8_t)head.func;
}

PacketHeader decode_header_byte(uint8_t hb) {
  const PacketType pt = (PacketType)(hb & PACKET_TYPE_BIT_MASK);
  printf("header type found: %0x\n", (uint8_t)(pt));
  const PacketFunction func =
      (PacketFunction)(hb & PACKET_FUNCTION_BIT_MASK);
  printf("header function found: %0x\n", (uint8_t)(func));

  return {pt, func};
}

std::pair<ChannelID, PartPtr> decode_broadcast(const Packet &packet) {
  VDPTracef("Decoding broadcast of size: %d", (int)packet.size());
  PacketReader reader(packet);
  // header byte, had to be read to know were a braodcast
  (void)reader.get_byte();
  const ChannelID id = reader.get_number<ChannelID>();
  const PartPtr schema = make_decoder(reader);
  return {id, schema};
}
Part::Part(std::string name) : name(std::move(name)) {}

ChannelID Channel::getID() const { return id; }

Part::~Part() {}
AbstractDevice::~AbstractDevice() {}
} // namespace VDP
