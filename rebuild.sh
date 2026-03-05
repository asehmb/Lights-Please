#!/bin/bash

# Exit immediately if a command fails
set -e

# 1. Clean up
echo "Wiping the build directory..."
rm -rf build

# 2. Configure with Ninja
echo "Configuring with Ninja (Debug mode)..."
# -G Ninja: Explicitly tells CMake to use the Ninja generator
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug
# 3. Build
echo "Building with Ninja..."
# Ninja automatically uses all CPU cores, but --parallel ensures it
cmake --build build --parallel

echo "Done!"
