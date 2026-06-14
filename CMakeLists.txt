cmake_minimum_required(VERSION 3.20)
project(CardGames LANGUAGES CXX VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to a debug build with sanitizers handy for a network game.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(CARDGAMES_BUILD_TESTS  "Build unit/smoke tests"      ON)
option(CARDGAMES_BUILD_SERVER "Build the Asio game server"  OFF) # Phase 3
option(CARDGAMES_BUILD_CLIENT "Build the Raylib client"     OFF) # Phase 6

enable_testing()

# Foundation, always built (pure C++, no external dependencies).
add_subdirectory(common)
add_subdirectory(protocol)

# Standalone Asio is shared by both the server and the client. Set it up once,
# only when one of them is being built, so the foundation stays dependency-free.
if(CARDGAMES_BUILD_SERVER OR CARDGAMES_BUILD_CLIENT)
    include(FetchContent)
    FetchContent_Declare(asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-30-2
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(asio)

    # The asio repo has no root CMake target — expose its header-only library.
    add_library(asio_standalone INTERFACE)
    target_include_directories(asio_standalone SYSTEM INTERFACE ${asio_SOURCE_DIR}/asio/include)
    target_compile_definitions(asio_standalone INTERFACE ASIO_STANDALONE ASIO_NO_DEPRECATED)
    find_package(Threads REQUIRED)
    target_link_libraries(asio_standalone INTERFACE Threads::Threads)
endif()

if(CARDGAMES_BUILD_SERVER)
    add_subdirectory(server)
endif()
if(CARDGAMES_BUILD_CLIENT)
    add_subdirectory(client)
endif()
