#include "led.h"

#include "Arduino.h"
#include "esp32-hal-gpio.h"
#include "esp32-hal.h"

/**
 * Creates an object for controlling an LED
 * @param led_pin the ESP32 pin that the LED is connected to
 */
LED::LED(uint8_t led_pin) : pin_(led_pin) {
  blink_interval_ = 500;
  last_blink_time_ = millis();
  pinMode(led_pin, OUTPUT);
  digitalWrite(pin_, LOW);
};

/**
 * function for controlling what the LED is doing to be placed in the main debug board control
 * loop
 */
void LED::loop() {
  switch (status_) {
    case SOLID_ON:
      digitalWrite(pin_, HIGH);
      break;
    case SOLID_OFF:
      digitalWrite(pin_, LOW);
      break;
    case BLINKING:
      if ((millis() - last_blink_time_) > blink_interval_) {
        if (digitalRead(pin_) == LOW) {
          digitalWrite(pin_, HIGH);
        } else {
          digitalWrite(pin_, LOW);
        }
        last_blink_time_ = millis();
      }
      break;
  }
}

void LED::turn_on() { status_ = SOLID_ON; }

void LED::turn_off() { status_ = SOLID_OFF; }

void LED::toggle() {
  switch (status_) {
    case SOLID_ON:
      status_ = SOLID_OFF;
      break;
    case SOLID_OFF:
      status_ = SOLID_ON;
      break;
    default:
      break;
  }
}

/**
 * sets the LED to blink
 * @param blink_interval the interval in milliseconds that the LED will toggle on
 */
void LED::set_blink(int blink_interval) {
  status_ = BLINKING;
  blink_interval_ = blink_interval;
}
