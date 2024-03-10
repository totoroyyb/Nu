#!/bin/bash

cp CMakeLists.txt ../opentelemetry-cpp
pushd ../opentelemetry-cpp
mkdir -p build
cd build
rm -rf *
cmake -DBUILD_TESTING=OFF -DWITH_BENCHMARK=OFF ..
cmake --build .
popd
