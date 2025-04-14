#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include "MemoryGuard.hpp"

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
        _catch(MemoryGuard::InvalidMemoryAccessException, e)
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
    MemoryGuard::unregisterThreadHandler();
}

int main()
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

    return 0;
}
