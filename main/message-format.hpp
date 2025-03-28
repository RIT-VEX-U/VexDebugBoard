#pragma once
#include "cJSON.h"
#include "vdb/protocol.hpp"
#include "visitor.hpp"
#include <string>
#include <vector>

std::string
send_advertisement_msg(const std::vector<VDP::Channel> &activeChannels);

std::string send_data_msg(const VDP::Channel &channel);