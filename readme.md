[![Build Status](https://travis-ci.org/0xd34d10cc/shar.svg?branch=master)](https://travis-ci.org/0xd34d10cc/shar) [![Build status](https://ci.appveyor.com/api/projects/status/914cejeivquyjjw9?svg=true)](https://ci.appveyor.com/project/0xd34d10cc/shar)

# Prerequisites
- conan
- cmake
- C++17 compliant compiler

# Build
```bash
$ git clone --recurse-submodules https://github.com/0xd34d10cc/shar
$ mkdir build && cd build
$ conan install .. --build=missing
$ cmake ..
$ cmake --build .
```
