#include "visitor.hpp"
#include "cJSON_Utils.h"
#include <limits>

DataJSONVisitor::DataJSONVisitor() {
  root = cJSON_CreateObject();
  node_stack.push_back(root);
}
DataJSONVisitor::~DataJSONVisitor() { cJSON_Delete(root); }

void DataJSONVisitor::VisitRecord(VDP::Record *record) {
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

void DataJSONVisitor::VisitString(VDP::String *str) {
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

void ChannelVisitor::VisitRecord(VDP::Record *record) {
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

void ChannelVisitor::VisitString(VDP::String *str) {
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

ResponseJSONVisitor::ResponseJSONVisitor(std::string json_str,
                               VDP::RegistryListener<std::mutex> &reg)
    : reg(reg) {
  input_json = cJSON_Parse(json_str.c_str());
}

ResponseJSONVisitor::ResponseJSONVisitor(const cJSON *input_json,
                               VDP::RegistryListener<std::mutex> &reg)
    : input_json(input_json), reg(reg) {}

void ResponseJSONVisitor::VisitRecord(VDP::Record *record) {
  std::vector<VDP::PartPtr> record_parts;
  const cJSON *record_json;
  if(std::string(input_json->string) == record->get_name()){
    record_json = input_json;
  }
  else{
    record_json = cJSON_GetObjectItem(input_json, record->get_name().c_str());
  }
  
  for (int i = 0; i < record->get_fields().size(); i++) {
    cJSON* buffer_json = cJSON_GetArrayItem(record_json, i);
    ResponseJSONVisitor new_RV(buffer_json, this->reg);
    record->get_fields().at(i)->Visit(&new_RV);
  }
}

void ResponseJSONVisitor::VisitString(VDP::String *str) {
  if(std::string(input_json->valuestring) == "N/A"){
    str->set_value("N/A");
  }
  else{
    str->set_value(input_json->valuestring);
  }
}
void ResponseJSONVisitor::VisitFloat(VDP::Float *float_part) {
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
    float_part->set_value(std::numeric_limits<float>().signaling_NaN());
  }
  }
  else{
  float_part->set_value(input_json->valuedouble);
  }
}
void ResponseJSONVisitor::VisitDouble(VDP::Double *double_part) {\
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
      double_part->set_value(std::numeric_limits<float>().signaling_NaN());
    }
  }
  else{
    double_part->set_value(input_json->valuedouble);
  }
}
void ResponseJSONVisitor::VisitInt64(VDP::Int64 *int64_part) {
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
      int64_part->set_value(std::numeric_limits<int64_t>().min());
    }
  }
  else{
    int64_part->set_value((int64_t)input_json->valueint);
  }
}
void ResponseJSONVisitor::VisitInt32(VDP::Int32 *int32_part) {
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
      int32_part->set_value(std::numeric_limits<int32_t>().min());
    }
  }
  else{
    int32_part->set_value((int32_t)input_json->valueint);
  }
}
void ResponseJSONVisitor::VisitInt16(VDP::Int16 *int16_part) {
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
      int16_part->set_value(std::numeric_limits<int16_t>().min());
    }
  }
  else{
    int16_part->set_value((int16_t)input_json->valueint);
  }
}
void ResponseJSONVisitor::VisitInt8(VDP::Int8 *int8_part) {
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
      int8_part->set_value(std::numeric_limits<int8_t>().min());
    }
  }
  else{
    int8_part->set_value((int8_t)input_json->valueint);
  }
}

void ResponseJSONVisitor::VisitUint64(VDP::Uint64 *Uint64_part) {
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
      Uint64_part->set_value(std::numeric_limits<uint64_t>().min());
    }
  }
  else{
    Uint64_part->set_value((uint64_t)input_json->valueint);
  }
}
void ResponseJSONVisitor::VisitUint32(VDP::Uint32 *Uint32_part) {
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
      Uint32_part->set_value(std::numeric_limits<uint32_t>().min());
    }
  }
  else{
    Uint32_part->set_value((uint32_t)input_json->valueint);
  }
}
void ResponseJSONVisitor::VisitUint16(VDP::Uint16 *Uint16_part) {
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
      Uint16_part->set_value(std::numeric_limits<uint16_t>().min());
    }
  }
  else{
    Uint16_part->set_value((uint16_t)input_json->valueint);
  }
}
void ResponseJSONVisitor::VisitUint8(VDP::Uint8 *Uint8_part) {
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
      Uint8_part->set_value(std::numeric_limits<uint8_t>().min());
    }
  }
  else{
    Uint8_part->set_value((uint8_t)input_json->valueint);
  }
}

std::string ResponseJSONVisitor::get_string() {
  const char *json_str = cJSON_Print(input_json);
  std::string str{json_str};
  cJSON_free((void *)json_str);
  return str;
}

void ResponseJSONVisitor::set_data() {

  id = cJSON_GetObjectItem(input_json, "channel_id")->valueint;

  std::string type_string = cJSON_GetObjectItem(input_json, "type")->valuestring;

  if (type_string == "data") {
    type = VDP::PacketType::Data;
  } else {
    type = VDP::PacketType::Broadcast;
  }

  cJSON *data_json = cJSON_GetObjectItem(input_json, "data");

  ResponseJSONVisitor RV(data_json, reg);

  reg.get_remote_schema(id)->Visit(&RV);

  data_parts.push_back(reg.get_remote_schema(id));
  for (int i = 0; i < cJSON_GetArraySize(data_json); i++) {
    reg.get_remote_schema(id)->Visit(&RV);
    data_parts.push_back(reg.get_remote_schema(id));
  }
  data = (std::shared_ptr<VDP::Record>)new VDP::Record("data", data_parts);
}

VDP::ChannelID ResponseJSONVisitor::get_id() {
  return cJSON_GetObjectItem(input_json, "channel_id")->valueint;
}

VDP::PacketType ResponseJSONVisitor::get_type() {
  std::string type = cJSON_GetObjectItem(input_json, "type")->valuestring;
  if (type == "data") {
    return VDP::PacketType::Data;
  } else {
    return VDP::PacketType::Broadcast;
  }
}

VDP::PartPtr ResponseJSONVisitor::get_data() {
  ResponseJSONVisitor RV(input_json, reg);

  cJSON *data_json = cJSON_GetObjectItem(input_json, "data");
  for (int i = 0; i < cJSON_GetArraySize(data_json); i++) {
    reg.get_remote_schema(id)->Visit(&RV);
    data_parts.push_back(reg.get_remote_schema(id));
  }
  VDP::PartPtr data_part =
      (std::shared_ptr<VDP::Record>)new VDP::Record("data", data_parts);
  return data_part;
}

void ResponseJSONVisitor::send_to_reg() { reg.submit_response(type, id, data); }