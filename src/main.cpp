#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include "memory_guard.hpp"

// Global mutex for console output synchronization
std::mutex console_mutex;

// Helper function for synchronized console output
template<typename... Args>
void synchronized_print(Args&&... args) {
    std::lock_guard<std::mutex> lock(console_mutex);
    (std::cout << ... << args) << std::endl;
}

// Helper function for synchronized error output
template<typename... Args>
void synchronized_error(Args&&... args) {
    std::lock_guard<std::mutex> lock(console_mutex);
    (std::cerr << ... << args) << std::endl;
}

void thread_function(int id)
{
    try
    {
        _try
        {
            synchronized_print("Thread ", id, ": Starting execution");
            synchronized_print("Thread ", id, ": Inside _try block");
            
            if (id % 3 == 0) // Threads with ID divisible by 3 (thread 0, 3, 6...)
            {
                // Access a null pointer
                int *ptr = nullptr;
                synchronized_print("Thread ", id, ": Attempting to access null pointer");
                *ptr = 10; // This will generate SIGSEGV
            }
            else if (id % 3 == 1) // Threads with remainder 1 (thread 1, 4, 7...)
            {
                // Safe execution path
                synchronized_print("Thread ", id, ": Safely avoiding invalid memory access");
            }
            else if (id % 3 == 2) // Threads with remainder 2 (thread 2, 5, 8...)
            {
                // Access an invalid memory address (not null)
                int *ptr = reinterpret_cast<int*>(0xDEADBEEF); // Invalid address
                synchronized_print("Thread ", id, ": Attempting to access invalid memory address (0xDEADBEEF)");
                *ptr = 20; // This will also generate SIGSEGV
            }
            
            synchronized_print("Thread ", id, ": Successfully completed _try block");
        }
        _catch(memory_guard::InvalidMemoryAccessException, e)
        {
            synchronized_error("Thread ", id, ": Exception caught: ", e.what());
        }
    }
    catch (...)
    {
        synchronized_error("Thread ", id, ": Unexpected exception caught");
    }
    
    synchronized_print("Thread ", id, ": Terminating");
    
    // Unregister the handler when the thread terminates
    memory_guard::unregisterThreadHandler();
}

// Function to demonstrate nested try blocks with exception in outer block
void nested_try_blocks_example()
{
    std::cout << "Starting nested _try blocks example..." << std::endl;
    
    try
    {
        _try
        {
            std::cout << "Outer _try block: Starting execution" << std::endl;
            
            // Inner _try block that doesn't generate exceptions
            _try
            {
                std::cout << "Inner _try block: Starting execution" << std::endl;
                
               int* ptr = nullptr;
               *ptr = 10; // This will generate SIGSEGV
                
            }
            _catch(memory_guard::InvalidMemoryAccessException, innerException)
            {
                // This should not be executed
                std::cerr << "Inner _catch block: Exception caught (unexpected): " << innerException.what() << std::endl;
            }
            
            std::cout << "Outer _try block: After inner _try block" << std::endl;
            
            // Now generate an exception in the outer block
            std::cout << "Outer _try block: Attempting to access null pointer" << std::endl;
            int* ptr = nullptr;
            *ptr = 10; // This will generate SIGSEGV
            
            std::cout << "Outer _try block: This line should not be executed" << std::endl;
        }
        _catch(memory_guard::InvalidMemoryAccessException, outerException)
        {
            std::cerr << "Outer _catch block: Exception caught: " << outerException.what() << std::endl;
        }
    }
    catch (...)
    {
        std::cerr << "Standard catch block: Unexpected exception caught" << std::endl;
    }
    
    std::cout << "Example completed successfully!" << std::endl;
    
    // Clean up resources
    memory_guard::unregisterThreadHandler();
}

// Function to demonstrate multi-threaded usage
void multi_threaded_example()
{
    const int num_threads = 4;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i)
    {
        threads.emplace_back(thread_function, i);
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    synchronized_print("Main: All threads have terminated successfully");
}

int main()
{
    // Run the nested try blocks example
    nested_try_blocks_example();
    
    // Uncomment to run the multi-threaded example
    // multi_threaded_example();
    
    return 0;
}
