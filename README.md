# Jupiter Chess Engine

A chess engine written in c++ as a python library.
Written to integrate directly with [Jupiter Client](https://github.com/angstrom-123/Jupiter-Chess-Interface)

- `example.py` shows usage of the library
- `jupiter.py` integrates with [Jupiter Client](https://github.com/angstrom-123/Jupiter-Chess-Interface)
- `jupiterengine/` contains the source c++ library code

## Platforms / Compilers

| Platform | MSVC | LLVM Clang (clang++) | GCC (g++) | Apple Clang (clang++) |
| --- | --- | --- | --- | --- |
| Windows | Working (1) | Untested (2) | - | - |
| Linux | - | Working | Working | - |
| MacOS | - | Working | Untested (3) | Not Supported |

(1) Build only possible in `Release` mode
(2) Theoretically possible, difficuly without Visual Studio installed
(3) Should be perfectly fine, just haven't tried it yet

## Prerequisites

- Python (>=3.12)
- C++ Compiler (>=c++23)
- CMake 

## Build and Run

### Build the Library

#### Linux

```bash
cd jupiterengine
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
cd ../..
```

If CMake didn't detect your preferred compiler, then specify the one to use with `-DCMAKE_CXX_COMPILER=your/preferred/compiler`, replacing the path with your preferred compiler.

#### Mac

You probably already have Apple Clang installed but this is not a supported compiler for this libary. Your options are LLVM's Clang or GCC's g++ (not recommended). This guide will assume you opted for LLVM clang as this is my preferred toolchain. It should be simple enough to substitute in GCC and g++ to these steps if you so desire.

1. Install LLVM (and Clang with it) if you haven't already
   - This can be done via homebrew: `brew install llvm`
   - By default, this should install to `/opt/homebrew/llvm/bin/clang++`
2. Create and enter the build directory as above:
   ```bash
cd jupiterengine
mkdir build
cd build
```
3. Run `cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=path/to/llvm/clang++` replacing the path with the location of your LLVM `clang++` installation
   - If installed via homebrew, the path is likely to be `/opt/homebrew/llvm/bin/clang++`
4. Build the library and exit the directory as above:
   ```bash
cmake --build .
cd ../..
```

#### Windows

On Windows, I strongly suggest using Microsoft's toolchain as it has the best support. LLVM's Clang is also supported but will be difficult to use without Visual Studio's developer terminal and environment. For this reason, I will walk through usage of MSVC for compilation. I will not detail how to compile with clang.

1. Install Visual Studio if you haven't already
   - Community Edition is free and will work fine 
   - I am testing with VS18, older versions may not be supported as I am using some modern c++ features
2. Open a new VS developer terminal and `cd` to this engine's directory.
3. Create and enter a build directory as above:
   ```bash
cd jupiterengine
mkdir build
cd build
```
4. Run `cmake ..` to generate build scripts
5. Run `cmake --build . --config Release` to build in release mode
   - Currently, building in Debug mode is not supported due to issues with linking with Python (feel free to figure this out for yourself)
   - Profiling mode is not supported at all
6. Run `cd ..\..` to return to the engine's root
