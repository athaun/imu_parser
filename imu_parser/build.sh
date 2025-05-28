#!/bin/bash

mkdir -p build
cd build

cmake .. || exit 1
cmake --build . || exit 1

if [ "$1" == "run" ]; then
    ./imu_parser
fi

cd ..