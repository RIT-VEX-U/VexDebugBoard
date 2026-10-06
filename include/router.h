#pragma once
#include "led.h"

/**
 * initializes the RS485 serial connection and websocket
 * @param RS485_TX_PIN the transmit pin for the RS485
 * @param RS485_RX_PIN the recieve pin for the RS485
 * @param status_led the led to blink to show the status of the websocket
 */
void init_router(
    int RS485_TX_PIN, int RS485_RX_PIN, int RS485_DE_PIN, LED& status_led, LED& debug_led_
);

/// loop for routing data between the smart port and the websocket
void route_data();
