#include "esp_http_server.h"

void http_log_raw(const char * str);


httpd_handle_t http_log_start(uint16_t port);
void http_log_stop(httpd_handle_t server);