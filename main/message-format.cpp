#include "message-format.hpp"

std::string
send_advertisement_msg(const std::vector<VDP::Channel> &activeChannels) {
  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "type", "advertisement");

  cJSON *channelArray = cJSON_AddArrayToObject(root, "channels");

  std::vector<ChannelVisitor *> visitors;

  for (VDP::Channel channel : activeChannels) {
    visitors.emplace_back(new ChannelVisitor());
    channel.data->Visit(visitors[visitors.size() - 1]);
    cJSON *subObject = visitors[visitors.size() - 1]->current_node();
    cJSON *newObject = cJSON_CreateObject();

    cJSON_AddItemReferenceToObject(newObject, "schema", subObject);
    cJSON_AddNumberToObject(newObject, "channel_id", channel.getID());

    cJSON_AddItemToArray(channelArray, newObject);
  }

  const char *json_str = cJSON_Print(root);
  std::string str(json_str);
  cJSON_free((void *)json_str);
  for (auto *v : visitors) {
    delete v;
  }
  cJSON_Delete(root);
  // ESP_LOGI(TAG, "size: %d", (int)visitors[0].node_stack.size());
  return str;
}

std::string send_data_msg(const VDP::Channel &channel) {

  cJSON *root = cJSON_CreateObject();
  DataJSONVisitor visitor;
  channel.data->Visit(&visitor);
  cJSON_AddNumberToObject(root, "rec_time", esp_timer_get_time());
  cJSON_AddNumberToObject(root, "channel_id", channel.getID());
  cJSON_AddStringToObject(root, "type", "data");
  cJSON_AddItemReferenceToObject(root, "motor",
                                 visitor.node_stack[visitor.node_stack.size()]);

  const char *json_str = cJSON_Print(root);
  std::string str(json_str);
  cJSON_free((void *)json_str);

  cJSON_Delete(root);
  // ESP_LOGI(TAG, "size: %d", (int)visitors[0].node_stack.size());
  return str;
}