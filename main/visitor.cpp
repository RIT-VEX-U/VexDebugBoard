#include "visitor.hpp"
#include "cJSON_Utils.h"

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

ReceiveVisitor::ReceiveVisitor(std::string json_str,
                               VDP::RegistryListener<std::mutex> &reg)
    : reg(reg) {
  input_json = cJSON_Parse(json_str.c_str());
}

ReceiveVisitor::ReceiveVisitor(const cJSON *input_json,
                               VDP::RegistryListener<std::mutex> &reg)
    : input_json(input_json), reg(reg) {}

void ReceiveVisitor::VisitRecord(VDP::Record *record) {
  std::vector<VDP::PartPtr> record_parts;
  printf("At Recieve Visitor: Input Json name: %s, Input Json Data:\n%s\n\n", input_json->string, cJSON_Print(input_json));
  printf("At Recieve Visitor: Record name: %s.\n", record->get_name().c_str());
  const cJSON *record_json;
  if(std::string(input_json->string) == record->get_name()){
    printf("input json name is the same as the record name, using raw input\n");
    record_json = input_json;
  }
  else{
    printf("setting record json to item in input json\n");
    record_json = cJSON_GetObjectItem(input_json, record->get_name().c_str());
  }
  
  for (int i = 0; i < record->get_fields().size(); i++) {
    printf("getting array item: %s, at index: %d, from json %s\n",record->get_fields().at(i).get()->get_name().c_str(), i, cJSON_Print(record_json));
    cJSON* buffer_json = cJSON_GetArrayItem(record_json, i);
    printf("creating visitor for json: \n%s\n", buffer_json->string);
    ReceiveVisitor new_RV(buffer_json, this->reg);
    printf("visiting record item: \n%s\n\n", record->get_fields().at(i).get()->get_name().c_str());
    record->get_fields().at(i)->Visit(&new_RV);
  }
}

void ReceiveVisitor::VisitString(VDP::String *str) {
  if(std::string(input_json->valuestring) == "N/A"){
    printf("found N/A value for string part: %s, setting part to null\n", str->get_name().c_str());
    str->set_value("null");
  }
  else{
    printf("found value: %s, for string part: %s\n", input_json->valuestring, str->get_name().c_str());
    str->set_value(input_json->valuestring);
  }
}
void ReceiveVisitor::VisitFloat(VDP::Float *float_part) {
  printf("visiting float\n");
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
    printf("found N/A value for float part: %s, setting part to null\n", float_part->get_name().c_str());
    float_part->set_value(NULL);
  }
  }
  else{
    printf("found value: %f, for float part: %s\n", input_json->valuedouble, float_part->get_name().c_str());
  float_part->set_value((float)input_json->valuedouble);
  }
}
void ReceiveVisitor::VisitDouble(VDP::Double *double_part) {\
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
      printf("found N/A value for double part: %s, setting part to null\n", double_part->get_name().c_str());
      double_part->set_value(NULL);
    }
  }
  else{
    printf("found value: %f, for double part: %s\n", input_json->valuedouble, double_part->get_name().c_str());
    double_part->set_value(input_json->valuedouble);
  }
}
void ReceiveVisitor::VisitInt64(VDP::Int64 *int64_part) {
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
      printf("found N/A value for int64 part: %s, setting part to null\n", int64_part->get_name().c_str());
      int64_part->set_value(NULL);
    }
  }
  else{
    printf("found value: %d, for int64 part: %s\n", input_json->valueint, int64_part->get_name().c_str());
    int64_part->set_value((int64_t)input_json->valueint);
  }
}
void ReceiveVisitor::VisitInt32(VDP::Int32 *int32_part) {
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
      printf("found N/A value for int32 part: %s, setting part to null\n", int32_part->get_name().c_str());
      int32_part->set_value(NULL);
    }
  }
  else{
    printf("found value: %d, for int32 part: %s\n", input_json->valueint, int32_part->get_name().c_str());
    int32_part->set_value((int32_t)input_json->valueint);
  }
}
void ReceiveVisitor::VisitInt16(VDP::Int16 *int16_part) {
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
      printf("found N/A value for int16 part: %s, setting part to null\n", int16_part->get_name().c_str());
      int16_part->set_value(NULL);
    }
  }
  else{
    printf("found value: %d, for int16 part: %s\n", input_json->valueint, int16_part->get_name().c_str());
    int16_part->set_value((int16_t)input_json->valueint);
  }
}
void ReceiveVisitor::VisitInt8(VDP::Int8 *int8_part) {
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
      printf("found N/A value for int8 part: %s, setting part to null\n", int8_part->get_name().c_str());
      int8_part->set_value(NULL);
    }
  }
  else{
    printf("found value: %d, for int8 part: %s\n", input_json->valueint, int8_part->get_name().c_str());
    int8_part->set_value((int8_t)input_json->valueint);
  }
}

void ReceiveVisitor::VisitUint64(VDP::Uint64 *Uint64_part) {
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
      printf("found N/A value for uint64 part: %s, setting part to null\n", Uint64_part->get_name().c_str());
      Uint64_part->set_value(NULL);
    }
  }
  else{
    printf("found value: %d, for uint64 part: %s\n", input_json->valueint, Uint64_part->get_name().c_str());
    Uint64_part->set_value((uint64_t)input_json->valueint);
  }
}
void ReceiveVisitor::VisitUint32(VDP::Uint32 *Uint32_part) {
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
      printf("found N/A value for uint32 part: %s, setting part to null\n", Uint32_part->get_name().c_str());
      Uint32_part->set_value(NULL);
    }
  }
  else{
    printf("found value: %d, for uint32 part: %s\n", input_json->valueint, Uint32_part->get_name().c_str());
    Uint32_part->set_value((uint32_t)input_json->valueint);
  }
}
void ReceiveVisitor::VisitUint16(VDP::Uint16 *Uint16_part) {
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
      printf("found N/A value for uint16 part: %s, setting part to null\n", Uint16_part->get_name().c_str());
      Uint16_part->set_value(NULL);
    }
  }
  else{
    printf("found value: %d, for uint16 part: %s\n", input_json->valueint, Uint16_part->get_name().c_str());
    Uint16_part->set_value((uint16_t)input_json->valueint);
  }
}
void ReceiveVisitor::VisitUint8(VDP::Uint8 *Uint8_part) {
  if(input_json->type == cJSON_String){
    if(std::string(input_json->valuestring) == "N/A"){
      printf("found N/A value for uint8 part: %s, setting part to null\n", Uint8_part->get_name().c_str());
      Uint8_part->set_value(NULL);
    }
  }
  else{
    printf("found value: %d, for uint8 part: %s\n", input_json->valueint, Uint8_part->get_name().c_str());
    Uint8_part->set_value((uint8_t)input_json->valueint);
  }
}

std::string ReceiveVisitor::get_string() {
  const char *json_str = cJSON_Print(input_json);
  std::string str{json_str};
  cJSON_free((void *)json_str);
  return str;
}

void ReceiveVisitor::set_data() {

  id = cJSON_GetObjectItem(input_json, "channel_id")->valueint;
  printf("got channel id: %d from websocket\n", id);

  std::string type_string = cJSON_GetObjectItem(input_json, "type")->valuestring;

  if (type_string == "data") {
    type = VDP::PacketType::Data;
    printf("got type data from websocket\n");
  } else {
    type = VDP::PacketType::Broadcast;
    printf("got type broadcast from websocket\n");
  }

  cJSON *data_json = cJSON_GetObjectItem(input_json, "data");
  printf("got data from websocket: \n%s\n", cJSON_Print(data_json));

  ReceiveVisitor RV(data_json, reg);

  printf("visiting remote schema\n");
  reg.get_remote_schema(id)->Visit(&RV);

  printf("adding to list of data\n");
  data_parts.push_back(reg.get_remote_schema(id));
  printf("found data at id: %d\n", id);
  for (int i = 0; i < cJSON_GetArraySize(data_json); i++) {
    reg.get_remote_schema(id)->Visit(&RV);
    data_parts.push_back(reg.get_remote_schema(id));
    printf("found data at id: %d\n", id);
  }
  data = (std::shared_ptr<VDP::Record>)new VDP::Record("data", data_parts);
}

VDP::ChannelID ReceiveVisitor::get_id() {
  return cJSON_GetObjectItem(input_json, "channel_id")->valueint;
}

VDP::PacketType ReceiveVisitor::get_type() {
  std::string type = cJSON_GetObjectItem(input_json, "type")->valuestring;
  if (type == "data") {
    return VDP::PacketType::Data;
  } else {
    return VDP::PacketType::Broadcast;
  }
}

VDP::PartPtr ReceiveVisitor::get_data() {
  ReceiveVisitor RV(input_json, reg);

  cJSON *data_json = cJSON_GetObjectItem(input_json, "data");
  for (int i = 0; i < cJSON_GetArraySize(data_json); i++) {
    reg.get_remote_schema(id)->Visit(&RV);
    data_parts.push_back(reg.get_remote_schema(id));
  }
  VDP::PartPtr data_part =
      (std::shared_ptr<VDP::Record>)new VDP::Record("data", data_parts);
  return data_part;
}

void ReceiveVisitor::send_to_reg() { reg.submit_response(type, id, data); }