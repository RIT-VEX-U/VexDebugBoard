idf_component_register(SRCS "rest.cpp" "webserver.cpp" "favicon.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_timer esp_http_server mdns json common)