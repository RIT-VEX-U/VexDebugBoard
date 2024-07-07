#ifndef STATUS_H
#define STATUS_H

#include <driver/gpio.h>

typedef enum { OFF, SETUP, IDLE, CONNECTED, FAULTED } status_led_options_t;

/**
 * Initialize the LED on the given GPIO and start a new thread
 *
 * @param _gpio status LED gpio number
 */
void status_led_init(gpio_num_t _gpio);

/**
 * Main task entry point. Controls the current state of the LED by constantly
 * checking cur_opt for changes.
 *
 * @param ptr Null
 */
void status_led_task_main();

/**
 * Set the status LED to a new state. Possible options are
 * OFF, SETUP, IDLE, CONNECTED, FAULTED
 *
 * @param _opt new state
 */
void status_led_set(status_led_options_t _opt);

/**
 * Signal the status led that there's a new wifi connection. The LED will blink
 * twice and then continue with it's last state.
 */
void status_led_signal_wifi_conn();

#endif