cmake_minimum_required(VERSION 3.16)
project(FishingBoatColWarning VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --------------------------------------------------------------------------
# Build Options
# --------------------------------------------------------------------------
option(BUILD_VISUALIZATION "Build PC visualization app (requires Qt5)"     ON)
option(BUILD_PI_APP        "Build Raspberry Pi headless application"       ON)
option(BUILD_TESTS         "Build unit tests"                              ON)

# --------------------------------------------------------------------------
# Core shared library (AIS decode + collision avoidance)
# Always built regardless of target platform
# --------------------------------------------------------------------------
add_subdirectory(core)

# --------------------------------------------------------------------------
# PC Visualization  —— only for native (non-cross-compile) builds
# --------------------------------------------------------------------------
if(BUILD_VISUALIZATION AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(visualization)
endif()

# --------------------------------------------------------------------------
# Raspberry Pi headless application
# --------------------------------------------------------------------------
if(BUILD_PI_APP)
    add_subdirectory(pi_app)
endif()

# --------------------------------------------------------------------------
# Unit tests  —— only for native builds
# --------------------------------------------------------------------------
if(BUILD_TESTS AND NOT CMAKE_CROSSCOMPILING)
    enable_testing()
    add_subdirectory(tests)
endif()
