#include "esp_http_server.h"
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief begin the http log that can be accessed at hostname.local/log
/// @param port what server port to use (should be 80)
/// @return handle to server. use webserver_stop to destroy the server
httpd_handle_t webserver_start(uint16_t port);

/// @brief destroys the specified server and cleans up
/// @param server the server to be destoyed
void webserver_stop(httpd_handle_t server);

esp_err_t send_string_to_ws(const std::string &str);

#ifdef __cplusplus
}
#endif