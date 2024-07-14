#include "common.hpp"

static esp_ip4_addr_t addr = {0};

static char ip_buf[16] = {0}; // enough for XXX.XXX.XXX.XXX[0]
void submit_ip(esp_ip4_addr_t a) {
  addr = a;
  snprintf(ip_buf, 16, IPSTR, IP2STR(&addr));
}

esp_ip4_addr_t get_ip() { return addr; }

const char *get_ip_str() { return ip_buf; }

const char *get_sw_version() { return "0.0.0"; }