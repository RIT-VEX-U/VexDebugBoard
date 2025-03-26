#include "visitor.hpp"

DataJSONVisitor::DataJSONVisitor() {
  root = cJSON_CreateObject();
  node_stack.push_back(root);
}
DataJSONVisitor::~DataJSONVisitor() { cJSON_Delete(root); }

void DataJSONVisitor::VisitRecord(const VDP::Record *record) {
  cJSON *oldroot = current_node();
  cJSON *newroot = cJSON_CreateObject();
  node_stack.push_back(newroot);

  for (const VDP::PartPtr &field : record->get_fields()) {
    field->Visit(this);
  }
  node_stack.pop_back();

  std::string name = record->get_name();
  cJSON_AddItemToObject(oldroot, name.c_str(), newroot);
}
void DataJSONVisitor::VisitString(const VDP::String *str) {
  std::string name = str->get_name();
  std::string value = str->get_value();

  cJSON *oldroot = current_node();
  cJSON_AddStringToObject(oldroot, name.c_str(), value.c_str());
}
void DataJSONVisitor::VisitAnyFloat(const std::string &name, double value,
                                    const VDP::Part *) {
  cJSON_AddNumberToObject(current_node(), name.c_str(), value);
}
void DataJSONVisitor::VisitAnyInt(const std::string &name, int64_t value,
                                  const VDP::Part *) {
  cJSON_AddNumberToObject(current_node(), name.c_str(), (double)value);
}
void DataJSONVisitor::VisitAnyUint(const std::string &name, uint64_t value,
                                   const VDP::Part *) {
  cJSON_AddNumberToObject(current_node(), name.c_str(), (double)value);
}

cJSON *DataJSONVisitor::current_node() {
  return node_stack[node_stack.size() - 1];
}
std::string DataJSONVisitor::get_string() {
  const char *json_str = cJSON_Print(root);
  std::string str{json_str};
  cJSON_free((void *)json_str);
  return str;
}

ChannelVisitor::ChannelVisitor() {
  root = cJSON_CreateObject();
  node_stack.push_back(root);
}
ChannelVisitor::~ChannelVisitor() { cJSON_Delete(root); }

void ChannelVisitor::VisitRecord(const VDP::Record *record) {
  cJSON *newroot = current_node();

  cJSON_AddStringToObject(newroot, "name", record->get_name().c_str());
  cJSON_AddStringToObject(newroot, "type", "record");
  cJSON *fieldsArray = cJSON_AddArrayToObject(newroot, "fields");

  for (const VDP::PartPtr &field : record->get_fields()) {
    cJSON *elementRoot = cJSON_CreateObject();
    node_stack.push_back(elementRoot);
    field->Visit(this);
    node_stack.pop_back();
    cJSON_AddItemToArray(fieldsArray, elementRoot);
  }
}

void ChannelVisitor::VisitString(const VDP::String *str) {
  std::string name = str->get_name();

  cJSON *oldroot = current_node();
  cJSON_AddStringToObject(oldroot, "name", name.c_str());
  cJSON_AddStringToObject(oldroot, "type", "string");
}
void ChannelVisitor::VisitAnyFloat(const std::string &name, double value,
                                   const VDP::Part *) {
  cJSON_AddStringToObject(current_node(), "name", name.c_str());
  cJSON_AddStringToObject(current_node(), "type", "float");
}
void ChannelVisitor::VisitAnyInt(const std::string &name, int64_t value,
                                 const VDP::Part *) {
  cJSON_AddStringToObject(current_node(), "name", name.c_str());
  cJSON_AddStringToObject(current_node(), "type", "int");
}
void ChannelVisitor::VisitAnyUint(const std::string &name, uint64_t value,
                                  const VDP::Part *) {
  cJSON_AddStringToObject(current_node(), "name", name.c_str());
  cJSON_AddStringToObject(current_node(), "type", "uint");
}

cJSON *ChannelVisitor::current_node() {
  return node_stack[node_stack.size() - 1];
}

std::string ChannelVisitor::get_string() {
  const char *json_str = cJSON_Print(root);
  std::string str{json_str};
  cJSON_free((void *)json_str);
  return str;
}