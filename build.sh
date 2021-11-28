#!/usr/bin/env bash
set -e

if [[ $1 == "--clean" && -d "build" ]]; then
    rm -r build
fi

if [[ ! -d "build" ]]; then
    mkdir build
fi

cd build

g++ -std=c++17 -o main ../src/main.cpp
