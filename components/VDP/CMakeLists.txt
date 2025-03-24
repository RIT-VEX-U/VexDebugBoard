idf_component_register(SRCS "vdb_device.cpp" "protocol.cpp" "registry.cpp" "types.cpp" "crc32.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES driver esp_timer)