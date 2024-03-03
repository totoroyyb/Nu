#!/bin/bash

mkdir -p /tmp
pushd /tmp
git clone https://github.com/google/benchmark.git
pushd benchmark
git clone https://github.com/google/googletest.git googletest
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ../
make
sudo make install
popd
rm -rf ./benchmark
