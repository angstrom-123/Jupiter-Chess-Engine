# Jupiter Chess Engine

A chess engine written in c++ as a python library.
Written to integrate directly with [Jupiter Client](https://github.com/angstrom-123/Jupiter-Chess-Interface)

- `example.py` shows usage of the library
- `jupiter.py` integrates with [Jupiter Client](https://github.com/angstrom-123/Jupiter-Chess-Interface)
- `jupiterengine/` contains the source c++ library code

## Prerequisites

- Python (>=3.12)
- C++ Compiler (>=c++23)
- CMake 
- make (or windows equivalent for use with CMake)

## Build and Run

### Build the Library

> [!NOTE]
> Build is not currently working for windows. This is a work in progress. Build 
> is also untested on MacOS but should be working fine.

Compile the library code into a dll:

```bash
cd jupiterengine
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
cd ../..
```

Valid `CMAKE_BUILD_TYPE`'s are: 
- Release (optimised, fastest)
- Debug (no optimisation + debugging symbols + manual tracing)
- Profile (optimised + debugging symbols + trace symbols for perf)
