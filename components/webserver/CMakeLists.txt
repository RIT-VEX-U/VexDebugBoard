idf_component_register(SRCS "WebLog.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_server mdns website)