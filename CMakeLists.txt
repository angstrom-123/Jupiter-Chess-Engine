cmake_minimum_required(VERSION 3.12)
project(jupiter VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package(Python3 COMPONENTS Development REQUIRED)

set(SOURCES
    src/bitboard.cpp 
    src/board.cpp 
    src/move.cpp
    src/core.cpp 
    src/searcher.cpp
    src/evaluator.cpp
    src/attackTable.cpp
    src/boardState.cpp
    src/movegen.cpp
    src/pieceSquareTable.cpp
    src/zobrist.cpp
    src/rng.cpp
    src/transpositionTable.cpp
    src/history.cpp
    src/pythonModule.cpp
)

add_library(jupiter SHARED ${SOURCES})

target_include_directories(jupiter 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${Python3_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link in stack trace library per compiler
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_link_libraries(jupiter PRIVATE stdc++_libbacktrace)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(jupiter PRIVATE -stdlib=libstdc++)
    target_link_options(jupiter PRIVATE -stdlib=libstdc++)
    target_link_libraries(jupiter PRIVATE stdc++_libbacktrace dl backtrace)
elseif (MSVC)
    target_link_libraries(jupiter PRIVATE dbghelp)
endif()

target_link_libraries(jupiter PRIVATE ${Python3_LIBRARIES})

target_compile_options(jupiter PRIVATE -msse4.2 -mpopcnt)

if(WIN32)
    set_target_properties(jupiter PROPERTIES SUFFIX ".pyd")
else()
    set_target_properties(jupiter PROPERTIES SUFFIX ".so")
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
    def has_error(self) -> bool: ...
    def get_error(self) -> str: ...
    def go(self, moveMs: int) -> str: ...
    def make_move(self, move: str) -> None: ...
    def set_time_control(self, seconds: int, increment: int) -> None: ...")
endif()

message(STATUS "Python module will be built as: $<TARGET_FILE_NAME:jupiter>")
