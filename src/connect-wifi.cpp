#include "connect-wifi.h"

#include <stdio.h>

#include "WiFi.h"
#include "WiFiType.h"
#include "esp32-hal.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char* TAG = "RIT Vex Debug Board";

static int s_retry_num;

static int s_max_retries;

static EventGroupHandle_t s_wifi_event_group;

void connect_to_wifi(const char* ssid, const char* pass) {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  WiFi.setTxPower(WIFI_POWER_5dBm);
  printf("Connecting to Wi-Fi: %s\n", ssid);
  WiFi.begin(ssid, pass);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    printf("Waiting for Wi-Fi (status %d)\n", WiFi.status());
    delay(1000);
    retries++;
  }
  WiFi.setAutoConnect(true);
  if (WiFi.status() != WL_CONNECTED) {
    printf("Failed to connect to %s\n", ssid);
  } else {
    printf("Connected to %s\n", ssid);
    printf("IP Address %s\n", WiFi.localIP().toString().c_str());
    printf("MAC Address %s\n", WiFi.macAddress().c_str());
  }
}
