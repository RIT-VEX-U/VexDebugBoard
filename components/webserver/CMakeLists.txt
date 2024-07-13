idf_component_register(SRCS "WebLog.c" "index_html.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_server mdns website)