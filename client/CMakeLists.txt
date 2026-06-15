# Raylib graphical client. Raylib is pulled via FetchContent (needs the X11 dev
# packages on Linux: libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev
# libxi-dev libgl1-mesa-dev). Uses the shared `asio_standalone` target from root.
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(raylib)

add_executable(belote_client
    src/NetworkClient.cpp
    src/ClientState.cpp
    src/Ui.cpp
    src/CardRenderer.cpp
    src/scenes/MainMenuScene.cpp
    src/scenes/LobbyScene.cpp
    src/scenes/GameScene.cpp
    src/main.cpp)

target_include_directories(belote_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(belote_client
    PRIVATE cardgame::common cardgame::protocol asio_standalone raylib)
target_compile_features(belote_client PRIVATE cxx_std_20)

# Warnings on our own code only — raylib headers are included as system headers.
if(MSVC)
    # /utf-8 : les sources contiennent des commentaires accentués (UTF-8).
    target_compile_options(belote_client PRIVATE /utf-8)
else()
    target_compile_options(belote_client PRIVATE -Wall -Wextra)
endif()

# Sous Windows, Asio inclut <windows.h>, dont les macros/fonctions Win32
# (Rectangle de GDI, DrawText→DrawTextA et CloseWindow de USER, etc.) entrent en
# collision avec l'API raylib. Nos unités de compilation n'utilisent ni GDI ni
# USER (seulement Winsock via Asio et le rendu via raylib), on les neutralise.
# Ciblé sur belote_client : n'affecte pas la compilation de raylib/GLFW.
if(WIN32)
    target_compile_definitions(belote_client PRIVATE WIN32_LEAN_AND_MEAN NOGDI NOUSER)
endif()
