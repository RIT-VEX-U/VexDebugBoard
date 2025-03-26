#include "cJSON.h"
#include "vdb/registry.hpp"
#include "vdb/types.hpp"

class DataJSONVisitor : public VDP::UpcastNumbersVisitor {
public:
  DataJSONVisitor();
  ~DataJSONVisitor();

  void VisitRecord(const VDP::Record *record);
  void VisitString(const VDP::String *str);
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

  void VisitRecord(const VDP::Record *record);

  void VisitString(const VDP::String *str);
  void VisitAnyFloat(const std::string &name, double value, const VDP::Part *);
  void VisitAnyInt(const std::string &name, int64_t value, const VDP::Part *);
  void VisitAnyUint(const std::string &name, uint64_t value, const VDP::Part *);

  cJSON *current_node();
  std::string get_string();

  // private:
  cJSON *root;
  std::vector<cJSON *> node_stack;
};