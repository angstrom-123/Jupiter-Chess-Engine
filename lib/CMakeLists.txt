cmake_minimum_required(VERSION 3.12)
project(jupiter VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Don't use set because apparently apple doesn't like it
string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra")
string(APPEND CMAKE_CXX_FLAGS_DEBUG " -O3 -g -DDEBUG")
string(APPEND CMAKE_CXX_FLAGS_RELEASE " -O3 -DNOTRACE")
string(APPEND CMAKE_CXX_FLAGS_PERF " -O3 -g -DNOTRACE")
string(APPEND CMAKE_CXX_FLAGS_PROFILE " -O3 -DPROFILE -DNOTRACE")

find_package(Python3 COMPONENTS Development REQUIRED)

set(SOURCES
    src/core.cpp 
    src/pythonModule.cpp
    src/board/bitboard.cpp 
    src/board/board.cpp 
    src/board/zobrist.cpp
    src/board/history.cpp
    src/evaluation/pieceSquareTable.cpp
    src/evaluation/distanceTable.cpp
    src/evaluation/evaluator.cpp
    src/movegen/move.cpp
    src/movegen/attackTable.cpp
    src/movegen/movegen.cpp
    src/movegen/moveStream.cpp
    src/search/searcher.cpp
    src/search/transpositionTable.cpp
    src/search/openingBook.cpp
    src/util/rng.cpp
    src/util/executionTimer.cpp
)

# Use module instead of shared so it compiles nicely on mac and linux
add_library(jupiter MODULE ${SOURCES})

target_include_directories(jupiter 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${Python3_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(jupiter PRIVATE ${Python3_LIBRARIES})

# Try to force compilers to emit efficient intrinsics
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    if(APPLE)
        target_compile_options(jupiter PRIVATE -msse4.2 -mpopcnt -mbmi)
    else()
        target_compile_options(jupiter PRIVATE -march=native)
    endif()
endif()

# Windows doesn't do this for us
if(WIN32)
    set_target_properties(jupiter PROPERTIES SUFFIX ".pyd")
endif()

set(PACKAGE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/libjupiter")
file(MAKE_DIRECTORY ${PACKAGE_DIR})

set_target_properties(jupiter PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY ${PACKAGE_DIR}
    RUNTIME_OUTPUT_DIRECTORY ${PACKAGE_DIR}
)

if(NOT EXISTS "${PACKAGE_DIR}/__init__.py")
    file(WRITE "${PACKAGE_DIR}/__init__.py" "from .libjupiter import *")
endif()

if(NOT EXISTS "${PACKAGE_DIR}/__init__.pyi")
    file(WRITE "${PACKAGE_DIR}/__init__.pyi" "class Board:
    def __init__(self, fen: str | None = None) -> None: ...
    def go(self, moveMs: int) -> str | None: ...
    def make_move(self, move: str) -> None: ...
    def set_time_control(self, seconds: int, increment: int) -> None: ...
    def get_telemetry(self) -> str: ...
    def get_metrics(self) -> str: ...")
endif()

message(STATUS "Python module will be built as: $<TARGET_FILE_NAME:jupiter>")
