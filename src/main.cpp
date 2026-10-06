#include <Arduino.h>

#include "connect-wifi.h"
#include "freertos/task.h"
#include "router.h"

static LED status_led(STAT_LED);
static LED debug_led(DEBUG_LED);

/**
 * Main setup code for the debug board
 */
void setup(void) {
  Serial.begin(115200);

  connect_to_wifi("RIT-WiFi");
  status_led.set_blink();
  printf("Debug Board Started\n");

  init_router(RS485_TX, RS485_RX, RS485_DE, status_led, debug_led);
}

/**
 * Main loop that the debug board runs
 */
void loop() {
  status_led.loop();
  debug_led.loop();
  route_data();
}
