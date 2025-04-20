#pragma once
#include "cJSON.h"
#include "vdb/protocol.hpp"
#include "vdb/registry-listener.hpp"
#include "vdb/types.hpp"

class DataJSONVisitor : public VDP::UpcastNumbersVisitor {
public:
  DataJSONVisitor();
  ~DataJSONVisitor();

  void VisitRecord(VDP::Record *record);
  void VisitString(VDP::String *str);
  void VisitAnyFloat(const std::string &name, double value, const VDP::Part *);
  void VisitAnyInt(const std::string &name, int64_t value, const VDP::Part *);
  void VisitAnyUint(const std::string &name, uint64_t value, const VDP::Part *);

  cJSON *current_node();
  std::string get_string();

  // private:
  cJSON *root;
  std::vector<cJSON *> node_stack;
};

class ChannelVisitor : public VDP::UpcastNumbersVisitor {
public:
  ChannelVisitor();
  ~ChannelVisitor();
  ChannelVisitor(const ChannelVisitor &) = delete;
  ChannelVisitor &operator=(const ChannelVisitor &) = delete;

  ChannelVisitor(ChannelVisitor &&) = default;
  ChannelVisitor &operator=(ChannelVisitor &&) = default;

  void VisitRecord(VDP::Record *record);

  void VisitString(VDP::String *str);
  void VisitAnyFloat(const std::string &name, double value, const VDP::Part *);
  void VisitAnyInt(const std::string &name, int64_t value, const VDP::Part *);
  void VisitAnyUint(const std::string &name, uint64_t value, const VDP::Part *);

  cJSON *current_node();
  std::string get_string();

  // private:
  cJSON *root;
  std::vector<cJSON *> node_stack;
};
class ReceiveVisitor : public VDP::Visitor {
public:
  ReceiveVisitor(std::string json_str, RegistryListener reg);
  ReceiveVisitor(cJSON *input_json, RegistryListener reg);
  ~ReceiveVisitor();
  ReceiveVisitor(const ReceiveVisitor &) = delete;
  ReceiveVisitor &operator=(const ReceiveVisitor &) = delete;

  ReceiveVisitor(ReceiveVisitor &&) = default;
  ReceiveVisitor &operator=(ReceiveVisitor &&) = default;

  void VisitRecord(VDP::Record *record) override;

  void VisitString(VDP::String *str) override;

  void VisitFloat(VDP::Float *float_part) override;
  void VisitDouble(VDP::Double *double_part) override;

  void VisitInt64(VDP::Int64 *int64_part) override;
  void VisitInt32(VDP::Int32 *int32_part) override;
  void VisitInt16(VDP::Int16 *int16_part) override;
  void VisitInt8(VDP::Int8 *int8_part) override;

  void VisitUint64(VDP::Uint64 *Uint64_part) override;
  void VisitUint32(VDP::Uint32 *Uint32_part) override;
  void VisitUint16(VDP::Uint16 *Uint16_part) override;
  void VisitUint8(VDP::Uint8 *Uint8_part) override;

  VDP::ChannelID get_id();
  VDP::PacketType get_type();

  VDP::PartPtr get_data();

  void set_data();

  void send_to_reg();

  std::vector<VDP::PartPtr> data_parts;
  VDP::ChannelID id;
  VDP::PacketType type;
  VDP::PartPtr data;
  cJSON *input_json;
  RegistryListener reg;

  cJSON *buffer_json;

  std::string get_string();
};