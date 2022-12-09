#include "status_led.h"
#include "defines.h"

#define MUX_TIMEOUT (500/portTICK_PERIOD_MS)

status_led_options_t cur_opt = SETUP;
bool initialized = false;
bool new_connection = false;

SemaphoreHandle_t mux = NULL;
TaskHandle_t task_h = NULL;
gpio_num_t pin = 0;

/**
 * Initialize the LED on the given GPIO and start a new thread
 * 
 * @param _gpio status LED gpio number
 */
void status_led_init(gpio_num_t _gpio)
{
    pin = _gpio;
    gpio_config_t pin_cfg = {
        .pin_bit_mask = bit_mask(pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&pin_cfg);

    mux = xSemaphoreCreateMutex();
    
    xTaskCreate(status_led_task_main, "task_status_led", 1024, NULL, tskIDLE_PRIORITY, &task_h);
    initialized = true;
}

/**
 * Toggle the LED on for a given time, then off for a given time, in milliseconds.
 * 
 * @param ms on/off pulse. Total blocking time is ms*2
 */
void pulse(int ms)
{
    if(!initialized)
        return;

    gpio_set_level(pin, 1);
    delay(ms);
    gpio_set_level(pin, 0);
    delay(ms);    
}

/**
 * Main task entry point. Controls the current state of the LED by constantly checking cur_opt for changes.
 * 
 * @param ptr Null
 */
void status_led_task_main(void *ptr)
{
    status_led_options_t option = OFF;
    bool wifi_conn = false;
    while(true)
    {
        if(xSemaphoreTake(mux, MUX_TIMEOUT))
        {
            option = cur_opt;
            wifi_conn = new_connection;
            xSemaphoreGive(mux);
        }

        if(wifi_conn)
        {
            pulse(250);
            pulse(250);
            if(xSemaphoreTake(mux, MUX_TIMEOUT))
            {
                wifi_conn = false;
                new_connection = false;
                xSemaphoreGive(mux);
            }
            continue;
        }

        switch(option)
        {
            case SETUP: // long pulse
                pulse(1000);
                continue;
            case IDLE: // setup but disconnected - quick pulse
                pulse(250);
                continue;
            case CONNECTED: // Solid
                gpio_set_level(pin, 1);
                delay(100);
                continue;
            case FAULTED: // triple tap
                pulse(250);
                pulse(250);
                pulse(250);
                delay(1000);
                continue;
            case OFF:
                gpio_set_level(pin, 0);
                delay(100);
                continue;
        }
    }
}

/**
 * Set the status LED to a new state. Possible options are
 * OFF, SETUP, IDLE, CONNECTED, FAULTED
 * 
 * @param _opt new state
 */
void status_led_set(status_led_options_t _opt)
{
    if(xSemaphoreTake(mux, MUX_TIMEOUT))
    {
        cur_opt = _opt;
        xSemaphoreGive(mux);
    }
}

/**
 * Signal the status led that there's a new wifi connection. The LED will blink twice
 * and then continue with it's last state. 
 */
void status_led_signal_wifi_conn()
{
    if(xSemaphoreTake(mux, MUX_TIMEOUT))
    {
        new_connection = true;
        xSemaphoreGive(mux);
    }
}