# C++ Project with Conan and CMake

This is a simple C++ project that demonstrates the integration of Conan package manager with CMake build system.

## Prerequisites

- CMake (version 3.15 or higher)
- Conan package manager
- C++ compiler with C++17 support (GCC, Clang, MSVC, etc.)

## Project Structure

```
cpp_project/
├── CMakeLists.txt    # CMake configuration
├── conanfile.txt     # Conan dependencies
├── src/
│   └── main.cpp      # Main source file
└── README.md         # This file
```

## Dependencies

This project uses the following external libraries:
- fmt (version 9.1.0) - A modern formatting library

## Building the Project

### 1. Install Dependencies with Conan

First, install the required dependencies using Conan:

```bash
# Navigate to the project directory
cd cpp_project

# Create a build directory
mkdir build && cd build

# Install dependencies (Conan 2.x approach)
conan install .. --output-folder=. --build=missing
```

### 2. Configure and Build with CMake

After installing the dependencies, configure and build the project:

```bash
# Configure the project (while in the build directory)
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release

# Build the project
cmake --build .
```

## Running the Application

After building, you can run the application:

```bash
# From the build directory
./cpp_project
```

The application should output:
```
Hello, World from standard library!
Hello, World from fmt library!
```

## Using the Build Script

For convenience, a build script is provided that automates the entire build and run process:

```bash
# Make sure the script is executable
chmod +x build.sh

# Run the build script
./build.sh
```

This script will:
1. Check if a Conan profile exists and create one if needed
2. Create a build directory if it doesn't exist
3. Install dependencies with Conan
4. Configure and build with CMake
5. Run the application

The script handles the Conan profile setup automatically, which is especially helpful for first-time Conan users.

## Customizing

To add more dependencies:
1. Add them to the `conanfile.txt` file
2. Update the source code to use the new libraries
3. Rebuild the project following the steps above

## License

This project is open source and available under the MIT License.
