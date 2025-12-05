cmake_minimum_required(VERSION 3.22)
project(saasemu_native)

add_library(saasemu_native SHARED
    native_bridge.cpp
    libretro_loader.cpp
)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(saasemu_native ${log-lib} ${android-lib})
set_target_properties(saasemu_native PROPERTIES
    CXX_STANDARD 17
    C_STANDARD 11
)