# Dedicated TCP server. Uses the shared `asio_standalone` target defined at the
# project root (pulled via FetchContent).
add_executable(belote_server
    src/Session.cpp
    src/GameServer.cpp
    src/main.cpp)
target_include_directories(belote_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(belote_server
    PRIVATE cardgame::common cardgame::protocol asio_standalone)
target_compile_features(belote_server PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options(belote_server PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(belote_server PRIVATE -Wall -Wextra)
endif()

# Headless test client — drives a full match over TCP for end-to-end validation.
add_executable(belote_client_headless tools/headless_client.cpp)
target_link_libraries(belote_client_headless
    PRIVATE cardgame::common cardgame::protocol asio_standalone)
target_compile_features(belote_client_headless PRIVATE cxx_std_20)
