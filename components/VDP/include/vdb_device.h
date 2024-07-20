#pragma once
#include "driver/uart.h"
#include "esp_err.h"

namespace VDB {
esp_err_t init_serial(uart_port_t uart_num, int tx_num, int rx_num, int rts_num,
                      int baud);

} // namespace VDB
