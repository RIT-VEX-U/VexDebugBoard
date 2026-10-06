#pragma once
#include <WebSocketsServer.h>

#include <cstdint>
#include <queue>

#include "led.h"

/// queue of incoming bytes from the UI to send to the vex brain over serial
extern std::queue<uint8_t> inbound_queue;

/// the websocket connection the debug board will use to communicate with a UI
extern WebSocketsServer websocket;

static void websocket_event(uint8_t num, WStype_t type, uint8_t* payload, size_t length);

/**
 * initializes the RS485 serial connection and websocket
 * @param RS485_TX_PIN the transmit pin for the RS485
 * @param RS485_RX_PIN the recieve pin for the RS485
 * @param status_ked the led to blink to show the status of the websocket
 */
void init_router(
    int RS485_TX_PIN, int RS485_RX_PIN, int RS485_DE_PIN, LED& status_led, LED& debug_led_
);

/// loop for routing data between the smart port and the websocket
void route_data();
