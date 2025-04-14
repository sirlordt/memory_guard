#!/bin/bash
set -e  # Exit on error

# Print commands before executing them
set -x

# Check if Conan profile exists and create one if it doesn't
if ! conan profile list | grep -q default; then
    echo "No default Conan profile found. Creating one..."
    conan profile detect
fi

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Install dependencies with Conan and generate CMake files (Conan 2.x approach)
conan install .. --output-folder=. --build=missing

# Configure and build with CMake
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Return to project root
cd ..

# Run the application
echo -e "\nRunning the application:"
./build/cpp_project
