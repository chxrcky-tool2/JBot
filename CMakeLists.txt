cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

project(AsmProfiler VERSION 1.0.0 LANGUAGES C CXX ASM)

file(GLOB SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Assembly is only built for Android arm64; every other target uses the C++ fallback in ticks.hpp
if (ANDROID AND (CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64"))
    target_sources(${PROJECT_NAME} PRIVATE src/asm/ticks_arm64.S)
    set_source_files_properties(src/asm/ticks_arm64.S PROPERTIES LANGUAGE ASM)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ASMPROF_HAS_ASM=1)
    message(STATUS "AsmProfiler: using arm64 assembly")
else()
    message(STATUS "AsmProfiler: using C++ fallback")
endif()

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME})
