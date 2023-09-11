#ifndef DEFINES_H
#define DEFINES_H

#define bit_mask(gpio) (1ull << gpio)
#define delay(ms) vTaskDelay(ms/portTICK_PERIOD_MS)

#define NETWORK_NAME "V5 Debug Board"
#define NETWORK_PASS ""
//"ch@ngem3!"
#define MDNS_NAME "debug"

extern bool setup_finished;

#endif