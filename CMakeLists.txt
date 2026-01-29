cmake_minimum_required(VERSION 3.15)
project(VirtualSwitch VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(vswitch_core
    src/core/ethernet_frame.cpp
    src/network/tap_device.cpp
    src/switch/virtual_switch.cpp
)

target_include_directories(vswitch_core PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

add_executable(vswitch
    src/main.cpp
)

target_link_libraries(vswitch PRIVATE vswitch_core)