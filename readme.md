[![Build Status](https://travis-ci.org/0xd34d10cc/shar.svg?branch=master)](https://travis-ci.org/0xd34d10cc/shar)

# Prerequisites
- conan
- cmake
- C++17 compliant compiler

# Build
```bash
$ mkdir build && cd build
$ conan remote add d34dpkgs https://api.bintray.com/conan/0xd34d10cc/d34dpkgs
$ conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan
$ conan install .. --build=missing
$ cmake ..
$ cmake --build ..
```