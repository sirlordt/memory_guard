# TryCatchGuard Library

A C++ library for catching and handling segmentation faults (SIGSEGV) in a controlled manner, providing a try-catch-like mechanism for handling invalid memory accesses without crashing your application.

## Features

- Catch and handle segmentation faults using a familiar try-catch syntax with `_try` and `_catch` macros
- Thread-safe implementation for multi-threaded applications
- Detailed error messages that include the type of memory access violation
- Support for both null pointer dereferences and other invalid memory accesses
- Support for nested try blocks
- Minimal performance overhead when no exceptions occur

## Prerequisites

- C++ compiler with C++17 support (GCC, Clang, etc.)
- CMake (version 3.15 or higher)
- Conan package manager
- pthread library

## Project Structure

```
try_catch_guard/
├── CMakeLists.txt          # Main CMake configuration
├── conanfile.txt           # Conan dependencies
├── build.sh                # Build script
├── modify_catch2.sh        # Script to modify Catch2's signal handling
├── src/
│   ├── main.cpp            # Example usage
│   └── try_catch_guard.hpp     # Main library header
├── tests/
│   ├── CMakeLists.txt      # Test configuration
│   └── try_catch_guard_tests.cpp  # Comprehensive tests
└── DOC.en.md / DOC.es.md   # Documentation in English and Spanish
```

## How It Works

TryCatchGuard uses standard C++ signal handling to install a custom signal handler for SIGSEGV signals. When an invalid memory access occurs within a `_try` block, the signal handler captures the fault, records information about the fault address, and uses `longjmp` to return control to the `_try` block. The library then throws a custom `InvalidMemoryAccessException` that can be caught using the `_catch` macro.

## Basic Usage

```cpp
#include <iostream>
#include "try_catch_guard.hpp"

int main() {
    _try {
        // Code that might cause a segmentation fault
        int* ptr = nullptr;
        *ptr = 10; // This would normally crash the program
    }
    _catch(try_catch_guard::InvalidMemoryAccessException, e) {
        // Handle the exception
        std::cerr << "Caught exception: " << e.what() << std::endl;
    }
    
    // Clean up resources
    try_catch_guard::unregisterThreadHandler();
    
    return 0;
}
```

## Building and Testing

### Using the Build Scripts

For convenience, several build scripts are provided. These scripts will automatically check for and install required dependencies (CMake and Conan) if they're not already present on your system:

```bash
# Make sure the scripts are executable
chmod +x build.sh clean_build.sh build_test.sh run_test.sh run.sh

# Full build and run
./build.sh

# Clean the build directory
./clean_build.sh

# Build only the tests
./build_test.sh

# Run the tests
./run_test.sh

# Run the main application (builds first if needed)
./run.sh
```

Each script includes:
- Automatic detection of CMake and Conan
- Installation of missing dependencies via apt and pip (requires sudo access)
- Creation of a default Conan profile if one doesn't exist

### Manual Build

```bash
# Create a build directory
mkdir -p build && cd build

# Install dependencies with Conan
conan install .. --output-folder=. --build=missing

# Configure and build with CMake
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Run the tests
ctest -V
```

## Thread Safety

MemoryGuard is designed to be thread-safe. Each thread registers its own handler, and the library maintains thread-specific contexts to ensure that segmentation faults are properly handled in multi-threaded applications.

```cpp
#include "try_catch_guard.hpp"
#include <thread>

void thread_function(int id) {
    _try {
        // Code that might cause a segmentation fault
        if (id % 2 == 0) {
            int* ptr = nullptr;
            *ptr = 10;
        }
    }
    _catch(try_catch_guard::InvalidMemoryAccessException, e) {
        std::cerr << "Thread " << id << " caught exception: " << e.what() << std::endl;
    }
    
    // Important: Unregister the handler when the thread terminates
    try_catch_guard::unregisterThreadHandler();
}
```

## Limitations

- **Nested Try Blocks**: The library fully supports nested try blocks up to 3 levels of nesting, which has been thoroughly tested. While deeper nesting levels have not been explicitly tested, the implementation is expected to work correctly with more levels of nesting (with approximately 70% confidence).
- **Platform Compatibility**: The library is primarily designed for Linux/Unix systems and may not work correctly on all platforms.
- **Recovery Limitations**: TryCatchGuard cannot recover from all types of memory access violations. Some severe memory corruptions may still cause the program to crash.
- **Signal Handler Conflicts**: The library may conflict with other libraries that install their own SIGSEGV signal handlers. The included `modify_catch2.sh` script addresses this for Catch2 testing framework.
- **Performance Overhead**: There is a small performance overhead due to the signal handling mechanism, especially in multi-threaded applications.

## Important Notes

1. Always call `try_catch_guard::unregisterThreadHandler()` when a thread that uses TryCatchGuard terminates to avoid memory leaks.
2. The `_try` and `_catch` macros must be used together, similar to standard try-catch blocks.
3. TryCatchGuard is designed for development and debugging purposes. In production environments, it's generally better to fix the underlying memory access issues rather than relying on catching segmentation faults.

## License

This project is open source and available under the MIT License.

## Further Documentation

For more detailed information, see the documentation files:
- [English Documentation](DOC.en.md)
- [Spanish Documentation](DOC.es.md)
