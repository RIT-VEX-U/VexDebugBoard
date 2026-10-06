#include <cstdint>

void init_leds();

enum LED_STATUS { SOLID_ON, SOLID_OFF, BLINKING };

/**
 * class for controlling LEDs on the debug board
 */
class LED {
 public:
  /**
   * Creates an object for controlling an LED
   * @param led_pin the ESP32 pin that the LED is connected to
   */
  LED(uint8_t led_pin);

  /**
   * function for controlling what the LED is doing to be placed in the main debug board control
   * loop
   */
  void loop();

  void turn_on();
  void turn_off();
  void toggle();
  /**
   * sets the LED to blink
   * @param blink_interval the interval in milliseconds that the LED will toggle on
   */
  void set_blink(int blink_interval = 500);

 private:
  LED_STATUS status_;
  uint8_t pin_;
  int blink_interval_;
  int last_blink_time_;
};
