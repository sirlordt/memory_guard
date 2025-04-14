# MemoryGuard Library

## Overview

MemoryGuard is a C++ library designed to catch and handle segmentation faults (SIGSEGV) in a controlled manner. It provides a try-catch-like mechanism for handling invalid memory accesses, such as null pointer dereferences or accessing invalid memory addresses, without crashing the application.

## Features

- Catch and handle segmentation faults using a familiar try-catch syntax
- Thread-safe implementation for multi-threaded applications
- Detailed error messages that include the type of memory access violation
- Support for both null pointer dereferences and other invalid memory accesses
- Minimal performance overhead when no exceptions occur

## How It Works

MemoryGuard uses the `libsigsegv` library to install a custom signal handler for SIGSEGV signals. When an invalid memory access occurs within a `_try` block, the signal handler captures the fault, records information about the fault address, and uses `siglongjmp` to return control to the `_try` block. The library then throws a custom `InvalidMemoryAccessException` that can be caught using the `_catch` macro.

## Usage

### Basic Usage

```cpp
#include <iostream>
#include "MemoryGuard.hpp"

void example() {
    _try {
        // Code that might cause a segmentation fault
        int* ptr = nullptr;
        *ptr = 10; // This would normally crash the program
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
        // Handle the exception
        std::cerr << "Caught exception: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "Starting basic example..." << std::endl;
    
    example();
    
    std::cout << "Example completed successfully!" << std::endl;
    
    // It's recommended to call unregisterThreadHandler() to avoid memory leaks
    // that would be detected by tools like Valgrind
    MemoryGuard::unregisterThreadHandler();
    
    return 0;
}
```

### Thread Safety

MemoryGuard is designed to be thread-safe. Each thread registers its own handler, and the library maintains thread-specific contexts to ensure that segmentation faults are properly handled in multi-threaded applications.

```cpp
#include "MemoryGuard.hpp"
#include <thread>

void thread_function(int id) {
    _try {
        // Code that might cause a segmentation fault
        if (id % 2 == 0) {
            int* ptr = nullptr;
            *ptr = 10; // This would normally crash the program
        }
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
        std::cerr << "Thread " << id << " caught exception: " << e.what() << std::endl;
    }
    
    // Important: Unregister the handler when the thread terminates
    MemoryGuard::unregisterThreadHandler();
}

int main() {
    std::thread t1(thread_function, 1);
    std::thread t2(thread_function, 2);
    
    t1.join();
    t2.join();
    
    return 0;
}
```

### Different Types of Memory Access Violations

MemoryGuard can handle different types of invalid memory accesses:

```cpp
#include <iostream>
#include "MemoryGuard.hpp"

void test_null_pointer() {
    _try {
        // Null pointer dereference
        std::cout << "Attempting to access null pointer..." << std::endl;
        int* null_ptr = nullptr;
        *null_ptr = 10; // This will throw an InvalidMemoryAccessException
        std::cout << "This line will not be executed" << std::endl;
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
        std::cerr << "Caught null pointer exception: " << e.what() << std::endl;
    }
}

void test_invalid_address() {
    _try {
        // Invalid memory address access
        std::cout << "Attempting to access invalid memory address..." << std::endl;
        int* bad_ptr = reinterpret_cast<int*>(0xDEADBEEF);
        *bad_ptr = 20; // This will also throw an InvalidMemoryAccessException
        std::cout << "This line will not be executed" << std::endl;
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
        std::cerr << "Caught invalid address exception: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "Starting memory violation examples..." << std::endl;
    
    test_null_pointer();
    test_invalid_address();
    
    std::cout << "All examples completed successfully!" << std::endl;
    
    // It's recommended to call unregisterThreadHandler() to avoid memory leaks
    // that would be detected by tools like Valgrind
    MemoryGuard::unregisterThreadHandler();
    
    return 0;
}
```

## When to Use unregisterThreadHandler()

### Single-Threaded Applications

In single-threaded applications, calling `MemoryGuard::unregisterThreadHandler()` at the end of the program is technically optional since the operating system will reclaim all memory when the process terminates. However, it is strongly recommended to call it for the following reasons:

1. **Memory Leak Detection Tools**: Tools like Valgrind will report memory leaks if you don't explicitly free the memory allocated by MemoryGuard before program termination.
2. **Resource Management**: It's good practice to clean up all resources your program uses, even if the OS would reclaim them anyway.
3. **Long-Running Applications**: If you're using MemoryGuard in a long-running application where you only need it for specific sections, you should call `unregisterThreadHandler()` after those sections to free up resources immediately.

```cpp
#include <iostream>
#include "MemoryGuard.hpp"

int main() {
    std::cout << "Starting single-threaded example..." << std::endl;
    
    // Section where we need MemoryGuard
    _try {
        std::cout << "Protected section..." << std::endl;
        // Some code that might cause segmentation faults
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
        std::cerr << "Caught exception: " << e.what() << std::endl;
    }
    
    // If we don't need MemoryGuard anymore, we can unregister the handler
    MemoryGuard::unregisterThreadHandler();
    
    std::cout << "Continuing with the rest of the program..." << std::endl;
    // Rest of the program where we don't need MemoryGuard
    
    return 0;
}
```

### Multi-Threaded Applications

In multi-threaded applications, it's important to call `MemoryGuard::unregisterThreadHandler()` at the end of each thread function to prevent resource leaks. This is especially important for worker threads that may be created and destroyed frequently.

It's crucial to understand that the main function also runs in a thread (the "main thread"). If the main thread uses MemoryGuard directly, it must also call `unregisterThreadHandler()` before terminating to avoid memory leaks that would be detected by tools like Valgrind.

```cpp
#include <iostream>
#include <thread>
#include "MemoryGuard.hpp"

void worker_thread() {
    // Thread code using MemoryGuard
    _try {
        // Some code that might cause segmentation faults
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
        std::cerr << "Worker thread caught exception: " << e.what() << std::endl;
    }
    
    // Clean up thread resources
    MemoryGuard::unregisterThreadHandler();
}

int main() {
    // Create and run worker threads
    std::thread t(worker_thread);
    
    // Main thread also uses MemoryGuard
    _try {
        // Some code in the main thread that might cause segmentation faults
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
        std::cerr << "Main thread caught exception: " << e.what() << std::endl;
    }
    
    // Wait for worker thread to finish
    t.join();
    
    // Clean up main thread resources
    MemoryGuard::unregisterThreadHandler();
    
    return 0;
}
```

## Important Notes

1. In multi-threaded applications, always unregister thread handlers when threads terminate using `MemoryGuard::unregisterThreadHandler()`.
2. In single-threaded applications, unregistering is optional but recommended if MemoryGuard is only used in specific sections.
3. The `_try` and `_catch` macros must be used together, similar to standard try-catch blocks.
4. MemoryGuard is designed for development and debugging purposes. In production environments, it's generally better to fix the underlying memory access issues rather than relying on catching segmentation faults.
5. The library has a small performance overhead due to the signal handling mechanism.

## Dependencies

- `libsigsegv`: A library for handling page faults in user mode

## Limitations

- MemoryGuard cannot recover from all types of memory access violations. Some severe memory corruptions may still cause the program to crash.
- The library is primarily designed for Linux/Unix systems and may not work correctly on all platforms.
- Using MemoryGuard does not eliminate the need for proper memory management practices.
