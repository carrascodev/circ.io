# ===========================
# Unit Tests Build Configuration
# ===========================

# Test executable
add_executable(run_tests
    test_main.cpp
    test_connection.cpp
    test_messages.cpp
)

target_include_directories(run_tests PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/tests
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/server
    ${CMAKE_SOURCE_DIR}/client
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/yojimbo/include
    ${CMAKE_SOURCE_DIR}/libs/raylib/src
)

# Link against game server and client code
# We need to compile server and client sources for tests
add_library(game_server_lib STATIC
    ${CMAKE_SOURCE_DIR}/server/game_server.cpp
    ${CMAKE_SOURCE_DIR}/common/eastl_allocator.cpp
)

target_include_directories(game_server_lib PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/server
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/yojimbo/include
    $<TARGET_PROPERTY:EASTL,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(game_server_lib PUBLIC
    yojimbo
    EASTL
)

add_library(game_client_lib STATIC
    ${CMAKE_SOURCE_DIR}/client/game_client.cpp
    ${CMAKE_SOURCE_DIR}/common/eastl_allocator.cpp
)

target_include_directories(game_client_lib PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/client
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/yojimbo/include
    ${CMAKE_SOURCE_DIR}/libs/raylib/src
    $<TARGET_PROPERTY:EASTL,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(game_client_lib PUBLIC
    yojimbo
    raylib
    EASTL
)

# Link test executable
target_link_libraries(run_tests PRIVATE
    game_server_lib
    game_client_lib
    yojimbo
    raylib
    EASTL
)

# Platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(run_tests PRIVATE pthread)
elseif(WIN32)
    target_link_libraries(run_tests PRIVATE ws2_32 winmm)
endif()

# Enable testing
enable_testing()
add_test(NAME ConnectionTests COMMAND run_tests "[connection]")
add_test(NAME MessageTests COMMAND run_tests "[messages]")
add_test(NAME AllTests COMMAND run_tests)
