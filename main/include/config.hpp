#include <cstdint>
#include <stdbool.h>
#include <string>

struct station_cfg {
  uint8_t ssid[32];
  uint8_t password[32];
  int auth_mode;
};

struct access_point_cfg {
  uint8_t ssid[32];
};

struct config {
  bool use_mdns;
  std::string mdns_hostname;

  bool use_ap;
  station_cfg sta;
  access_point_cfg ap;
};