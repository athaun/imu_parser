#!/bin/bash

mkdir -p build
cd build

cmake .. || exit 1
cmake --build . || exit 1

case "$1" in
    run)
        ./imu_parser
        ;;
    test)
        ctest --output-on-failure
        ;;
    help|*)
        echo "Usage: $0 [run|test|help]"
        echo "  run   - build and run the imu_parser executable"
        echo "  test  - run tests with CTest"
        echo "  help  - show this help message"
        ;;
esac

cd ..
