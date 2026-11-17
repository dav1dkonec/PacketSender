cmake_minimum_required(VERSION 3.20...3.31)
project(udp_sender LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# pacing accuracy depends on it: default to Release when no build type is given
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

# txcore: everything except main(), shared with the tests
add_library(txcore STATIC
    src/config.cpp
    src/udp_sender.cpp
    src/stop_signal.cpp
    src/lateness.cpp
    src/pacer.cpp
)
target_include_directories(txcore PUBLIC include)

# Pacer clock, sleep and real-time setup are per platform
if(APPLE)
    target_sources(txcore PRIVATE src/pacer_mac.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(txcore PRIVATE src/pacer_linux.cpp)
else()
    set(NO_PACER ON)
endif()

if(NOT NO_PACER)
    add_executable(udp_sender src/main.cpp)
    target_link_libraries(udp_sender PRIVATE txcore)
else()
    message(STATUS "${CMAKE_SYSTEM_NAME}: building txcore and tests only, udp_sender needs macOS or Linux")
endif()

# tests: run with `ctest --test-dir build`
enable_testing()

add_executable(config_test tests/config_test.cpp)
target_link_libraries(config_test PRIVATE txcore)
add_test(NAME config COMMAND config_test)
