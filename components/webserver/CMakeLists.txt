idf_component_register(SRCS "rest.cpp" "webserver.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_server mdns website)