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

struct wifi_cfg {
  bool use_ap;
  station_cfg sta;
  access_point_cfg ap;
};

struct config {
  bool use_mdns;
  std::string mdns_hostname;

  bool trying_most_recent; // true if on the next reboot we should try the trial
                           // wifi. If the trial succeeds, then replace
                           // last_known_good_wifi and stop the trial
  wifi_cfg trial_wifi;
  wifi_cfg last_known_good_wifi;
};

/**
 * Load the config from non volatile storage
 */
config load_config_nvs();