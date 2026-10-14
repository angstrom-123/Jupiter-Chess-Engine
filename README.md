# Jupiter Chess Engine

A chess engine written in c++ as a python library.
Written to integrate directly with [Jupiter Client](https://github.com/angstrom-123/Jupiter-Chess-Interface)

- `jupiter.py` integrates with [Jupiter Client](https://github.com/angstrom-123/Jupiter-Chess-Interface)
- `jupiterengine/` contains the source c++ library code

## Platforms / Compilers

| Platform | MSVC | LLVM Clang (clang++) | GCC (g++) | Apple Clang (clang++) |
| --- | --- | --- | --- | --- |
| Windows | Working (1) | Untested (2) | - | - |
| Linux | - | Working | Working | - |
| MacOS | - | Working | Untested (3) | Not Supported |

1. Build only possible in `Release` mode
2. Theoretically possible, difficuly without Visual Studio installed
3. Should be perfectly fine, just haven't tried it yet

## Prerequisites

- Python (>=3.12)
- C++ Compiler (>=c++23)
- CMake 

## Build and Run

### Linux

#### 1. Create and Enter the Build Directory

```bash
cd jupiterengine
mkdir build
cd build
```

#### 2. Generate Build Scripts

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
```

If cmake didn't detect your preferred compiler specify it with the flag `-DCMAKE_CXX_COMPILER=your-preferred-compiler`

#### 3. Build The Library

```bash
cmake --build .
```

### Mac

I recommend using LLVM Clang for this (not the same as Apple Clang which is unsupported). You could use GCC (g++) but I won't explain how.

#### 1. Install LLVM Clang

```bash
brew install llvm
```

#### 2. Create and Enter the Build Directory

```bash
cd jupiterengine
mkdir build
cd build
```

#### 3. Generate Build Scripts

1. Determine the location of your LLVM clang++ installation
   - If installed via homebrew, it should be `/opt/homebrew/llvm/bin/clang++`
   - If installed another way, you should find the directory yourself
2. Run `cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=path/to/llvm/clang++`
   - Replace the example path with the path you found to the compiler

#### 4. Build The Library

```bash
cmake --build .
```

### Windows

I recommend using MSVC via Visual Studio's developer terminal as this is by far the simplest. You could also figure out how to use LLVM clang for this but I won't explain how here.

#### 1. Install Visual Studio and Open its Terminal

- Community edition is free: https://visualstudio.microsoft.com/downloads/
  - I am using version 18 for testing. Newer versions are fine, older versions may not be supported.
- Once installed, search for 'vs developer terminal' in the start menu and open it up

#### 2. Create and Enter the Build Directory

```bash
cd jupiterengine
mkdir build
cd build
```

#### 3. Generate Build Scripts

```bash
cmake ..
```

This will generate all build types. This engine only supports `Release` mode for now.

#### 4. Build The Library

```bash
cmake --build . --config Release
```

By default, MSVC will also allow for building in `Debug` mode but this is not supported.
`Profiling` mode is also not supported on Windows specifically.

## Validate

The best way to test that everything is behaving as expected it to launch [Jupiter Client](https://github.com/angstrom-123/Jupiter-Chess-Interface) and try playing a game or two against the engine. Follow the instructions there to find out how to set everything up.
