#! /usr/bin/bash

# NOTE: run from project root

if [ ! -d .git ]; then
    echo "Not in root of repo"
    exit 1
fi

if [ ! -d  ./debug ]; then
    echo "Build directory not found. Generating..."

    mkdir debug
    cd debug
    conan install .. -s build_type=Debug --build=missing || exit 1
    cd ..
fi

if [ ! -f ./debug/compile_commands.json ]; then
    echo "Generating compile_commands.json"
    cd ./debug
    cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug || exit 1
    cd ..
fi

cppfiles=`find ./src -name '*.cpp'`
hppfiles=`find ./src -name '*.hpp'`
checks='bugprone-*,clang-analyzer-*,hicpp-*,modernize-*,performance-*,portability-*,readability-*,-hicpp-no-array-decay,-readability-named-parameter,-hicpp-signed-bitwise'
clang-tidy -p ./debug/compile_commands.json -header-filter=./src -checks="$checks" $cppfiles $hppfile