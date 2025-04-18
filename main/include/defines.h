#ifndef DEFINES_H
#define DEFINES_H

#define bit_mask(gpio) (1ull << gpio)
#define delay(ms) vTaskDelay(ms / portTICK_PERIOD_MS)

#define NETWORK_NAME "V5 Debug Board"
#define NETWORK_PASS ""
#define MDNS_NAME "debug"

#endif