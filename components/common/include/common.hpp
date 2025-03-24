#include "config.hpp"
#include <array>
#include <cstdint>
#include <esp_netif_types.h>
/**
 * Submit the ip address that we were assigned by our own network or by whatever
 * network we connected to
 */
void submit_ip(esp_ip4_addr_t addr);

// Get the ip we were assigned or 0.0.0.0 if we havent got one yet
esp_ip4_addr_t get_ip();
const char *get_ip_str();

const char *get_sw_version();

// NVS stuff

// Load bootcount and settings
esp_err_t init_nvs();
uint32_t get_bootcount();

VDBConfig get_config();