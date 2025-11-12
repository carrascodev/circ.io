cmake_minimum_required(VERSION 3.10)
project(CircIO VERSION 1.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Set vcpkg toolchain if available
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# Enable verbose output for debugging
set(CMAKE_VERBOSE_MAKEFILE ON)

# Set build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
endif()

# Platform-specific settings
if(UNIX AND NOT APPLE)
    # Linux
    add_definitions(-DPLATFORM_LINUX=1)
elseif(WIN32)
    # Windows
    add_definitions(-DPLATFORM_WINDOWS=1)
elseif(APPLE)
    # macOS
    add_definitions(-DPLATFORM_MAC=1)
endif()

# ===========================
# Build Yojimbo Dependencies (OBJECT libraries for optimization)
# ===========================
# Using OBJECT libraries to compile once and link to both client and server

# Netcode (networking library)
add_library(netcode_obj OBJECT
    libs/yojimbo/netcode/netcode.c
)
target_include_directories(netcode_obj PUBLIC
    libs/yojimbo/netcode
    libs/yojimbo/sodium
)
target_link_libraries(netcode_obj PUBLIC /lib/x86_64-linux-gnu/libsodium.so.23)

# Create static library from object library
add_library(netcode STATIC $<TARGET_OBJECTS:netcode_obj>)
target_include_directories(netcode PUBLIC
    libs/yojimbo/netcode
    libs/yojimbo/sodium
)
target_link_libraries(netcode PUBLIC /lib/x86_64-linux-gnu/libsodium.so.23)

# Reliable (reliable UDP library)
add_library(reliable_obj OBJECT
    libs/yojimbo/reliable/reliable.c
)
target_include_directories(reliable_obj PUBLIC
    libs/yojimbo/reliable
)

# Create static library from object library
add_library(reliable STATIC $<TARGET_OBJECTS:reliable_obj>)
target_include_directories(reliable PUBLIC
    libs/yojimbo/reliable
)

# TLSF (memory allocator)
add_library(tlsf_obj OBJECT
    libs/yojimbo/tlsf/tlsf.c
)
target_include_directories(tlsf_obj PUBLIC
    libs/yojimbo/tlsf
)

# Create static library from object library
add_library(tlsf STATIC $<TARGET_OBJECTS:tlsf_obj>)
target_include_directories(tlsf PUBLIC
    libs/yojimbo/tlsf
)

# ===========================
# Build Yojimbo Library (OBJECT library for optimization)
# ===========================

file(GLOB YOJIMBO_SOURCES
    libs/yojimbo/source/*.cpp
)

# Build yojimbo as OBJECT library to compile once
add_library(yojimbo_obj OBJECT
    ${YOJIMBO_SOURCES}
)

target_include_directories(yojimbo_obj PUBLIC
    libs/yojimbo/include
    libs/yojimbo
    libs/yojimbo/serialize
)

target_link_libraries(yojimbo_obj PUBLIC
    netcode
    reliable
    tlsf
)

# Create static library from object library
add_library(yojimbo STATIC $<TARGET_OBJECTS:yojimbo_obj>)

target_include_directories(yojimbo PUBLIC
    libs/yojimbo/include
    libs/yojimbo
    libs/yojimbo/serialize
)

target_link_libraries(yojimbo PUBLIC
    netcode
    reliable
    tlsf
)

# ===========================
# Find EASTL
# ===========================

find_package(EASTL CONFIG REQUIRED)

# ===========================
# Build Raylib (Client dependency)
# ===========================

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
add_subdirectory(libs/raylib)

# ===========================
# Build Game Server (modular)
# ===========================

add_subdirectory(server)

# ===========================
# Build Game Client (modular)
# ===========================

add_subdirectory(client)

# ===========================
# Build Unit Tests
# ===========================

add_subdirectory(tests)

# ===========================
# Installation
# ===========================
# Note: Individual targets (game_server, game_client) handle their own installation
# in their respective CMakeLists.txt files

# ===========================
# Print Configuration
# ===========================

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
message(STATUS "C flags: ${CMAKE_C_FLAGS}")
