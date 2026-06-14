# Wire protocol: binary codec + message definitions. Pure C++20, depends only on
# the common card types, so it is shared by both server and (future) client and
# is fully unit-testable without any networking.
add_library(cardgame_protocol
    src/BinaryWriter.cpp
    src/BinaryReader.cpp
    src/Packet.cpp
    src/Messages.cpp
)
add_library(cardgame::protocol ALIAS cardgame_protocol)

target_include_directories(cardgame_protocol
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(cardgame_protocol PUBLIC cardgame::common)
target_compile_features(cardgame_protocol PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(cardgame_protocol PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(cardgame_protocol PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CARDGAMES_BUILD_TESTS)
    add_executable(protocol_test tests/protocol_test.cpp)
    target_link_libraries(protocol_test PRIVATE cardgame::protocol)
    add_test(NAME protocol_test COMMAND protocol_test)
endif()
