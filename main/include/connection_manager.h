#include <string>

void init_wifi_ap(std::string wifi_name = "RIT-VDB", std::string wifi_pass = "");
void init_wifi_sta(std::string wifi_name = "RIT-WiFi", std::string wifi_pass = "", bool fallback_on_ap = false);
void init_mdns(void);