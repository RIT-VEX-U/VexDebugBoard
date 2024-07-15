#include "common.hpp"
#include "esp_check.h"
#include <esp_log.h>
#include <nvs_flash.h>

static const char *TAG = "common";

static esp_ip4_addr_t addr = {0};

static char ip_buf[16] = {0}; // enough for XXX.XXX.XXX.XXX[0]
void submit_ip(esp_ip4_addr_t a) {
  addr = a;
  snprintf(ip_buf, 16, IPSTR, IP2STR(&addr));
}

esp_ip4_addr_t get_ip() { return addr; }

const char *get_ip_str() { return ip_buf; }

const char *get_sw_version() { return "0.0.0"; }

static uint32_t bootcount = 0;

esp_err_t init_nvs() {
  ESP_LOGI(TAG, "Initializing NVS...");
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  if (err != ESP_OK) {
    return err;
  }

  nvs_handle_t my_handle;
  ESP_RETURN_ON_ERROR(nvs_open("storage", NVS_READWRITE, &my_handle), TAG,
                      "Failed to open NVS");
  err = nvs_get_u32(my_handle, "bootcount", &bootcount);
  if (err == ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGI(TAG, "Bootcount not yet written. initializing now");
  } else if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to read bootcount");
    return err;
  }

  bootcount++;
  ESP_RETURN_ON_ERROR(nvs_set_u32(my_handle, "bootcount", bootcount), TAG,
                      "Failed to store bootcount");

  ESP_LOGI(TAG, "Bootcount %ld", bootcount);
  return ESP_OK;
}

uint32_t get_bootcount() { return bootcount; }