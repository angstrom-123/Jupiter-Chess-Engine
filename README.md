# Jupiter Chess Engine

A chess engine written in c++ as a python library.
Written to integrate directly with [Jupiter Client](https://github.com/angstrom-123/Jupiter-Chess-Interface)

- `example.py` shows usage of the library
- `jupiter.py` integrates with [Jupiter Client](https://github.com/angstrom-123/Jupiter-Chess-Interface)
- `jupiterengine/` contains the source c++ library code

## Platforms / Compilers

| Platform | MSVC | LLVM Clang (clang++) | GCC (g++) | Apple Clang (clang++) |
| --- | --- | --- | --- | --- |
| Windows | WIP | Untested | Not Native | - |
| Linux | - | Working | Working | - |
| MacOS | - | Working | Untested | Not Supported |

## Prerequisites

- Python (>=3.12)
- C++ Compiler (>=c++23)
- CMake 
- CMake-compatible build system (I am testing with `make`)

## Build and Run

### Build the Library

Compile the library code into a shared object:

```bash
cd jupiterengine
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
cd ../..
```

Valid build types are: 
- `-DCMAKE_BUILD_TYPE=Release` (optimised, fastest)
- `-DCMAKE_BUILD_TYPE=Debug` (no optimisation + debugging symbols + manual tracing)
- `-DCMAKE_BUILD_TYPE=Profile` (optimised + debugging symbols + trace symbols for perf)

#### Mac Users

If you are on MacOS then you probably have apple clang installed by default, this is not the same as 
LLVM clang and will not work for compiling this engine. Follow these steps to build:
1. Install LLVM clang if you haven't already (can be done via homebrew)
2. Clear the `build` directory
3. Run `cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=path/to/llvm/clang++` replacing this path with the actual path to your clang++ installation
4. Run `cmake --build .` as normal (everything should now compile fine)

#### Windows Users

If you are on Windows then wait for me to update the cmake to work with MSVC or try to figure it out yourself.
