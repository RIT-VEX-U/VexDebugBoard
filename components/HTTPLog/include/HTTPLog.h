#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/// @brief send raw string to http log
/// @param str null terminated string to write to log. user of this function is responsible for deallocating str
void http_log_raw(const char * str);

/// @brief begin the http log that can be accessed at hostname.local/log
/// @param port what server port to use (should be 80)
/// @return handle to server. use http_log_stop to destroy the server
httpd_handle_t http_log_start(uint16_t port);

/// @brief destroys the specified server and cleans up
/// @param server the server to be destoyed
void http_log_stop(httpd_handle_t server);

#ifdef __cplusplus
}
#endif