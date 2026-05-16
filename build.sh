#!/bin/bash

echo "Cleaning build directory..."
rm -rf build

echo "Creating build directory..."
mkdir build

cd build

echo "Running CMake..."
cmake ..

echo "Building project..."
make -j$(nproc)

echo "Build completed successfully."

cd ../build
./json_gui