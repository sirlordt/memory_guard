# MemoryGuard Library

## Overview

MemoryGuard is a C++ library designed to catch and handle segmentation faults (SIGSEGV) in a controlled manner. It provides a try-catch-like mechanism for handling invalid memory accesses, such as null pointer dereferences or accessing invalid memory addresses, without crashing the application.

## Features

- Catch and handle segmentation faults using a familiar try-catch syntax
- Thread-safe implementation for multi-threaded applications
- Detailed error messages that include the type of memory access violation
- Support for both null pointer dereferences and other invalid memory accesses
- Support for nested try blocks with proper exception propagation
- Minimal performance overhead when no exceptions occur

## How It Works

MemoryGuard uses standard C++ signal handling to install a custom signal handler for SIGSEGV signals. When an invalid memory access occurs within a `_try` block, the signal handler captures the fault, records information about the fault address, and uses `longjmp` to return control to the `_try` block. The library then throws a custom `InvalidMemoryAccessException` that can be caught using the `_catch` macro.

The implementation uses a stack of jump buffers to support nested try blocks, allowing exceptions to be caught at the appropriate level. Thread-local storage ensures that each thread has its own context, making the library thread-safe.

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

### Using with Standard C++ Exceptions

MemoryGuard is compatible with the standard C++ exception mechanism. You can throw and catch standard or custom exceptions within `_try`/`_catch` blocks. This allows you to combine protection against segmentation faults with normal C++ exception handling.

```cpp
#include <iostream>
#include <stdexcept>
#include "MemoryGuard.hpp"

class MyCustomException : public std::exception {
public:
    const char* what() const noexcept override {
        return "This is my custom exception";
    }
};

void mixed_exceptions_example() {
    _try {
        std::cout << "Inside _try block..." << std::endl;
        
        // You can throw standard C++ exceptions inside a _try block
        if (rand() % 3 == 0) {
            throw std::runtime_error("Standard runtime error");
        }
        else if (rand() % 3 == 1) {
            throw MyCustomException();
        }
        else {
            // Or cause a segmentation fault
            int* ptr = nullptr;
            *ptr = 10; // This will generate SIGSEGV
        }
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
        // This catches only memory access exceptions
        std::cerr << "MemoryGuard exception caught: " << e.what() << std::endl;
    }
    
    // You can use standard catch blocks to catch other exceptions
    catch(const MyCustomException& e) {
        std::cerr << "Custom exception caught: " << e.what() << std::endl;
    }
    catch(const std::exception& e) {
        std::cerr << "Standard exception caught: " << e.what() << std::endl;
    }
    catch(...) {
        std::cerr << "Unknown exception caught" << std::endl;
    }
    
    // Clean up resources
    MemoryGuard::unregisterThreadHandler();
}

int main() {
    srand(time(nullptr));
    mixed_exceptions_example();
    return 0;
}
```

You can also nest standard try-catch blocks inside `_try`/`_catch` blocks:

```cpp
#include <iostream>
#include <stdexcept>
#include "MemoryGuard.hpp"

void nested_example() {
    _try {
        std::cout << "Outer _try block..." << std::endl;
        
        try {
            std::cout << "Inner standard try block..." << std::endl;
            throw std::runtime_error("Error inside standard try");
        }
        catch(const std::exception& e) {
            std::cerr << "Caught in standard catch: " << e.what() << std::endl;
            
            // We can cause a segmentation fault inside a standard catch
            // and it will be caught by the outer _catch
            int* ptr = nullptr;
            *ptr = 10;
        }
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
        std::cerr << "Caught in _catch: " << e.what() << std::endl;
    }
    
    MemoryGuard::unregisterThreadHandler();
}
```

### Thread Safety

MemoryGuard is designed to be thread-safe. Each thread registers its own handler, and the library maintains thread-specific contexts to ensure that segmentation faults are properly handled in multi-threaded applications.

```cpp
#include "MemoryGuard.hpp"
#include <thread>
#include <mutex>

// Global mutex for console output synchronization
std::mutex console_mutex;

// Helper function for synchronized console output
template<typename... Args>
void synchronized_print(Args&&... args) {
    std::lock_guard<std::mutex> lock(console_mutex);
    (std::cout << ... << args) << std::endl;
}

void thread_function(int id) {
    _try {
        // Code that might cause a segmentation fault
        if (id % 2 == 0) {
            int* ptr = nullptr;
            synchronized_print("Thread ", id, ": Attempting to access null pointer");
            *ptr = 10; // This would normally crash the program
        }
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
        synchronized_print("Thread ", id, " caught exception: ", e.what());
    }
    
    // Important: Unregister the handler when the thread terminates
    MemoryGuard::unregisterThreadHandler();
}

int main() {
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(thread_function, i);
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::cout << "All threads have terminated successfully" << std::endl;
    
    return 0;
}
```

### Nested Try Blocks

MemoryGuard supports nested try blocks, allowing for more complex error handling scenarios:

```cpp
#include <iostream>
#include "MemoryGuard.hpp"

void nested_try_blocks_example() {
    _try {
        std::cout << "Outer _try block: Starting execution" << std::endl;
        
        // Inner _try block
        _try {
            std::cout << "Inner _try block: Starting execution" << std::endl;
            
            // Generate an exception in the inner block
            int* ptr = nullptr;
            *ptr = 10; // This will generate SIGSEGV
            
            std::cout << "Inner _try block: This line should not be executed" << std::endl;
        }
        _catch(MemoryGuard::InvalidMemoryAccessException, innerException) {
            std::cerr << "Inner _catch block: Exception caught: " << innerException.what() << std::endl;
        }
        
        std::cout << "Outer _try block: After inner _try block" << std::endl;
        
        // Now generate an exception in the outer block
        int* ptr = nullptr;
        *ptr = 20; // This will generate SIGSEGV
        
        std::cout << "Outer _try block: This line should not be executed" << std::endl;
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, outerException) {
        std::cerr << "Outer _catch block: Exception caught: " << outerException.what() << std::endl;
    }
    
    std::cout << "Example completed successfully!" << std::endl;
    
    // Clean up resources
    MemoryGuard::unregisterThreadHandler();
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

## Implementation Details

### Signal Handling

MemoryGuard uses the standard C++ signal handling facilities to install a custom signal handler for SIGSEGV signals. The signal handler is installed once per process using the `installGlobalHandlerOnce()` function, which sets up a signal action using `sigaction()`.

### Thread-Specific Context

Each thread that uses MemoryGuard has its own context, stored in a thread-local variable (`currentThreadContext`). This context includes:

- A stack of jump buffers for nested try blocks
- A flag indicating whether the handler is active
- A function pointer to the thread-specific signal handler

### Jump Buffer Stack

To support nested try blocks, MemoryGuard maintains a stack of jump buffers. When a `_try` block is entered, a new jump buffer is pushed onto the stack. When a segmentation fault occurs, the signal handler uses the top jump buffer to return control to the most recent `_try` block.

### Exception Propagation

When a segmentation fault is caught, MemoryGuard throws a custom `InvalidMemoryAccessException` with a detailed error message. This exception can be caught using the `_catch` macro, which works similarly to a standard C++ catch block.

## Important Notes

1. In multi-threaded applications, always unregister thread handlers when threads terminate using `MemoryGuard::unregisterThreadHandler()`.
2. In single-threaded applications, unregistering is optional but recommended if MemoryGuard is only used in specific sections.
3. The `_try` and `_catch` macros must be used together, similar to standard try-catch blocks.
4. MemoryGuard is designed for development and debugging purposes. In production environments, it's generally better to fix the underlying memory access issues rather than relying on catching segmentation faults.
5. The library has a small performance overhead due to the signal handling mechanism.

## Testing and Sanitizers

When running the tests, you may notice unusual memory access errors displayed on the screen. This is because the test suite is configured with various sanitizers enabled in the `tests/CMakeLists.txt` file:

```cmake
# Enable Address Sanitizer and Undefined Behavior Sanitizer
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
```

These sanitizers include:
- **AddressSanitizer (ASan)**: Detects memory errors such as buffer overflows, use-after-free, and memory leaks
- **UndefinedBehaviorSanitizer (UBSan)**: Detects undefined behavior like integer overflow, null pointer dereference, etc.
- **LeakSanitizer**: Detects memory leaks

The test environment is also configured with special environment variables to allow MemoryGuard to handle segmentation faults while the sanitizers are active:

```cmake
set_tests_properties(memory_guard_tests PROPERTIES
    ENVIRONMENT "ASAN_OPTIONS=handle_segv=0:allow_user_segv_handler=1:detect_leaks=0"
)
```

### Example Test Output

When running the tests, you'll see numerous runtime error messages like the following:

```
Randomness seeded to: 4243962057
.../tests/memory_guard_tests.cpp:17:14: runtime error: store to null pointer of type 'int'
.../tests/memory_guard_tests.cpp:76:22: runtime error: store to null pointer of type 'int'
.../tests/memory_guard_tests.cpp:115:34: runtime error: store to null pointer of type 'int'
Outer try block stack size: 1
Outer jmpbuf address: 0x514000000c40
Inner try block stack size: 2
Inner jmpbuf address: 0x514000000d08
.../tests/memory_guard_tests.cpp:212:22: runtime error: store to null pointer of type 'int'
Stack size after inner catch: 1
Current jmpbuf address after inner catch: 0x514000000c40
Stack size after inner block: 1
Final jmpbuf address: 0x514000000c40
...
Throwing a standard C++ exception inside _try block...
Caught standard exception: Standard C++ exception
Throwing a custom exception inside _try block...
Caught custom exception: Custom exception message
Testing exception type 0...
.../tests/memory_guard_tests.cpp:856:26: runtime error: store to null pointer of type 'int'
Caught memory access exception: Invalid null pointer access exception
Testing exception type 1...
Caught standard exception: Standard C++ exception
Testing exception type 2...
Caught custom exception: Custom exception message
...
===============================================================================
All tests passed (73 assertions in 18 test cases)
```

**Important**: These "runtime error" messages are expected and do not indicate actual problems with the code. They are generated by the UndefinedBehaviorSanitizer when it detects null pointer dereferences, which is exactly what many of our tests are intentionally doing to verify that MemoryGuard correctly catches these issues.

The final line "All tests passed" confirms that MemoryGuard is working correctly, successfully catching and handling all the intentional memory access violations.

These sanitizers are valuable development tools that help identify potential issues, but they can produce verbose output that might look like errors even when the tests are passing successfully. The errors you see are often intentional test cases that verify MemoryGuard's ability to catch and handle memory access violations.

## Limitations

- **Nested Try Blocks**: While the library supports nested try blocks, there are some edge cases that may not work correctly, particularly when an exception occurs in the outer block after the inner block has completed. This is a known limitation of the current implementation.
- **Platform Compatibility**: The library is primarily designed for Linux/Unix systems and may not work correctly on all platforms.
- **Recovery Limitations**: MemoryGuard cannot recover from all types of memory access violations. Some severe memory corruptions may still cause the program to crash.
- **Signal Handler Conflicts**: The library may conflict with other libraries that install their own SIGSEGV signal handlers. The included `modify_catch2.sh` script addresses this for the Catch2 testing framework.
- **Performance Overhead**: There is a small performance overhead due to the signal handling mechanism, especially in multi-threaded applications.
