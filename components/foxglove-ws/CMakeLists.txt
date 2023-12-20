idf_component_register(SRCS "foxglove-ws.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_server nlohmann_json websocketer)

target_compile_options(${COMPONENT_LIB} PRIVATE -Os)
