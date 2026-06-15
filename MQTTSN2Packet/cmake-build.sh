#!/bin/bash

set -e

rm -rf build.paho
mkdir build.paho
cd build.paho
cmake ..
cmake --build .
ctest -VV --timeout 600
