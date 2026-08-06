# Jupiter Chess Engine

A chess engine written in c++ as a python library.
Written to integrate directly with [Jupiter Client](https://github.com/angstrom-123/Jupiter-Chess-Interface)

- `example.py` shows usage of the library.
- `jupiter.py` integrates with [Jupiter Client](https://github.com/angstrom-123/Jupiter-Chess-Interface) directly (hence the missing import `framework.base_engine` and the import of `.build` instead of `build`)

## Prerequisites

- Python (>=3.12)
- C++ Compiler (>=c++23)
- CMake 
- make (or windows equivalent for use with CMake)

## Build Library

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```
