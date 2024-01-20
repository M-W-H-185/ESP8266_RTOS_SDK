set(srcs "station_example_main.c","device_wifi.c")

idf_component_register(
    SRCS ${srcs}   
    INCLUDE_DIRS "."
)