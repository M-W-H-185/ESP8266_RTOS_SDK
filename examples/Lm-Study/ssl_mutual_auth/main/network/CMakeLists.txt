idf_component_register(SRCS "network_mqtt_task.c" "network_http.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp-mqtt)
                    