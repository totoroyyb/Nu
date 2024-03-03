#!/bin/bash

cd ../opentelemetry-cpp
mkdir -p build
cd build
rm -rf *
cmake -DBUILD_TESTING=OFF ..
cmake --build .
