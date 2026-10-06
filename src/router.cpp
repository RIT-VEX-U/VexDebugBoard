#include "router.h"

#include <cstdint>
#include <cstdio>

#include "HardwareSerial.h"
#include "WebSocketsServer.h"
#include "WiFi.h"

std::queue<uint8_t> inbound_queue;
WebSocketsServer websocket(8080);
static LED* status_led_ = nullptr;
static LED* debug_led_ = nullptr;
static HardwareSerial RS485_Serial(1);

/**
 * initializes the RS485 serial connection and websocket
 * @param RS485_TX_PIN the transmit pin for the RS485
 * @param RS485_RX_PIN the recieve pin for the RS485
 * @param status_ked the led to blink to show the status of the websocket
 */
void init_router(
    int RS485_TX_PIN, int RS485_RX_PIN, int RS485_DE_PIN, LED& status_led, LED& debug_led
) {
  status_led_ = &status_led;
  debug_led_ = &debug_led;
  // Set an initial GPIO level before the UART takes over direction control.
  pinMode(RS485_DE_PIN, OUTPUT);
  digitalWrite(RS485_DE_PIN, LOW);
  RS485_Serial.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);

  // Half-duplex mode controls DE through RTS; begin() only assigns RX/TX.
  if (!RS485_Serial.setPins(RS485_RX_PIN, RS485_TX_PIN, -1, RS485_DE_PIN) ||
      !RS485_Serial.setMode(UART_MODE_RS485_HALF_DUPLEX)) {
    printf("RS485 direction-control setup failed\n");
    return;
  }
  printf(
      "RS485 initialized: 9600 baud, RX=%d TX=%d DE=%d\n", RS485_RX_PIN, RS485_TX_PIN, RS485_DE_PIN
  );
  if (WiFi.isConnected()) {
    printf("Starting Websocket on ws://%s:8080/\n", WiFi.localIP().toString().c_str());
    websocket.begin();
    websocket.onEvent(websocket_event);
  }
};

/// callback for routing information recieved over the websocket
void websocket_event(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      printf("[%d] Disconnected!\n", num);
      status_led_->set_blink();
      break;
    case WStype_CONNECTED: {
      IPAddress ip = websocket.remoteIP(num);
      printf("[%d] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      websocket.sendTXT(num, "Connected To Debug Board");
      status_led_->turn_on();
      break;
    }
    case WStype_BIN:
      for (size_t i = 0; i < length; ++i) inbound_queue.push(payload[i]);
      break;
  }
}

/// loop for routing data between the smart port and the websocket
void route_data() {
  websocket.loop();
  if (RS485_Serial.available() > 0) {
    debug_led_->turn_on();
    uint8_t byte = RS485_Serial.read();
    websocket.broadcastBIN(&byte, 1);
  } else {
    debug_led_->turn_off();
  }
  if (inbound_queue.size() > 0) {
    RS485_Serial.write(inbound_queue.front());
    inbound_queue.pop();
  }
}
