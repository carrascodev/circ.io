# ===========================
# Game Client Build Configuration
# ===========================

add_executable(game_client
    main.cpp
    game_client.cpp
    ../common/eastl_allocator.cpp
)

target_include_directories(game_client PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/client
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/yojimbo/include
    ${CMAKE_SOURCE_DIR}/libs/raylib/src
)

target_link_libraries(game_client PRIVATE
    yojimbo
    raylib
    EASTL
)

# Platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(game_client PRIVATE pthread)
elseif(WIN32)
    target_link_libraries(game_client PRIVATE ws2_32 winmm)
endif()

# Installation
install(TARGETS game_client
    RUNTIME DESTINATION bin
)
