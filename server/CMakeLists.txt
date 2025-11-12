# ===========================
# Game Server Build Configuration
# ===========================

add_executable(game_server
    main.cpp
    game_server.cpp
    ../common/eastl_allocator.cpp
)

target_include_directories(game_server PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/server
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libs/yojimbo/include
)

target_link_libraries(game_server PRIVATE
    yojimbo
    EASTL
)

# Platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(game_server PRIVATE pthread)
elseif(WIN32)
    target_link_libraries(game_server PRIVATE ws2_32 winmm)
endif()

# Installation
install(TARGETS game_server
    RUNTIME DESTINATION bin
)
