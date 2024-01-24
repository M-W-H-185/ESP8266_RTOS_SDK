# The following four lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# 添加组件
set(EXTRA_COMPONENT_DIRS  $ENV{IDF_PATH}/examples/Lm-Study/ssl_mutual_auth/main/device)
list(APPEND EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/Lm-Study/ssl_mutual_auth/main/network)
list(APPEND EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/Lm-Study/ssl_mutual_auth/main/tuya)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(mqtt_ssl_mutual_auth)
