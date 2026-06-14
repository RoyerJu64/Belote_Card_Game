# Shared, game-agnostic card engine. Pure C++20, zero external dependencies —
# so it builds anywhere and is trivially unit-testable.
add_library(cardgame_common
    src/cards/Suit.cpp
    src/cards/Card.cpp
    src/cards/Deck.cpp
    src/engine/Player.cpp
    src/engine/Trick.cpp
    src/engine/RoundEngine.cpp
    src/engine/RandomController.cpp
    src/engine/GameTable.cpp
    src/games/belote/BeloteRules.cpp
    src/games/belote/BeloteScoring.cpp
    src/games/belote/BeloteBidding.cpp
    src/games/belote/BeloteGame.cpp
    src/games/belote/BeloteTable.cpp
    src/games/coinche/CoincheBidding.cpp
    src/games/coinche/CoincheScoring.cpp
    src/games/coinche/CoincheTable.cpp
)
add_library(cardgame::common ALIAS cardgame_common)

target_include_directories(cardgame_common
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(cardgame_common PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(cardgame_common PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(cardgame_common PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(CARDGAMES_BUILD_TESTS)
    add_executable(common_smoke tests/smoke_test.cpp)
    target_link_libraries(common_smoke PRIVATE cardgame::common)
    add_test(NAME common_smoke COMMAND common_smoke)

    add_executable(belote_sim tests/belote_sim.cpp)
    target_link_libraries(belote_sim PRIVATE cardgame::common)
    add_test(NAME belote_sim COMMAND belote_sim)

    add_executable(gametable_test tests/gametable_test.cpp)
    target_link_libraries(gametable_test PRIVATE cardgame::common)
    add_test(NAME gametable_test COMMAND gametable_test)

    add_executable(bidding_test tests/bidding_test.cpp)
    target_link_libraries(bidding_test PRIVATE cardgame::common)
    add_test(NAME bidding_test COMMAND bidding_test)

    add_executable(coinche_test tests/coinche_test.cpp)
    target_link_libraries(coinche_test PRIVATE cardgame::common)
    add_test(NAME coinche_test COMMAND coinche_test)
endif()
