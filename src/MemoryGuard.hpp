#ifndef MEMORY_GUARD_HPP
#define MEMORY_GUARD_HPP

#include <sigsegv.h>
#include <csetjmp>
#include <csignal>
#include <stdexcept>
#include <exception>
#include <functional>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace MemoryGuard {

// Custom exception for invalid memory accesses
class InvalidMemoryAccessException : public std::exception {
private:
    std::string message;

public:
    InvalidMemoryAccessException(const std::string& msg = "Invalid memory access detected") 
        : message(msg) {}

    virtual const char* what() const noexcept override {
        return message.c_str();
    }
};

// Structure to store thread-specific information
struct ThreadContext {
    sigjmp_buf jmpbuf;
    bool active = false;
    std::function<int(void*, int)> handler;
};

// Global map to store thread-specific handlers
// Note: This requires synchronization as multiple threads may access it
inline std::unordered_map<std::thread::id, ThreadContext*>& getThreadHandlers() {
    static std::unordered_map<std::thread::id, ThreadContext*> handlers;
    return handlers;
}

inline std::mutex& getHandlersMutex() {
    static std::mutex mutex;
    return mutex;
}

// Thread-local variables for context and fault address
thread_local static ThreadContext* currentThreadContext = nullptr;
thread_local static void* currentFaultAddress = nullptr;

// Thread-specific handler
inline int threadSegvHandler(void* addr, int serious)
{
    // Store the fault address for later use
    currentFaultAddress = addr;
    
    // We assume that currentThreadContext is not nullptr and is active
    siglongjmp(currentThreadContext->jmpbuf, 1);
    return 0; // Never executed, but prevents compiler warnings
}

// Global handler that delegates to the thread-specific handler
inline int globalSegvHandler(void* addr, int serious)
{
    // If we have a context for the current thread, we use its handler
    if (currentThreadContext && currentThreadContext->active)
    {
        return currentThreadContext->handler(addr, serious);
    }
    
    // If we don't have a context, we return 0 to indicate that we don't handle the signal
    return 0;
}

// Registers a handler for the current thread
inline void registerThreadHandler()
{
    std::thread::id this_id = std::this_thread::get_id();
    
    // Create a new context for this thread if it doesn't exist
    if (!currentThreadContext)
    {
        currentThreadContext = new ThreadContext();
        currentThreadContext->handler = threadSegvHandler;
        
        // Register the context in the global map
        std::lock_guard<std::mutex> lock(getHandlersMutex());
        getThreadHandlers()[this_id] = currentThreadContext;
    }
}

// Unregisters the handler for the current thread
inline void unregisterThreadHandler()
{
    if (currentThreadContext)
    {
        std::thread::id this_id = std::this_thread::get_id();
        
        // Remove the context from the global map
        {
            std::lock_guard<std::mutex> lock(getHandlersMutex());
            getThreadHandlers().erase(this_id);
        }
        
        // Free memory
        delete currentThreadContext;
        currentThreadContext = nullptr;
    }
}

// One-time initializer for the global handler
inline void installGlobalHandlerOnce()
{
    static bool installed = false;
    if (!installed)
    {
        if (sigsegv_install_handler(globalSegvHandler) < 0)
        {
            std::cerr << "Failed to install global SIGSEGV handler" << std::endl;
            std::exit(1);
        }
        installed = true;
    }
}

// Internal function that throws an exception if we exit with siglongjmp
inline void segvTryBlock(const std::function<void()> &block)
{
    // Ensure that the global handler is installed
    installGlobalHandlerOnce();
    
    // Register a handler for this thread
    registerThreadHandler();
    
    // Activate the handler for this thread
    currentThreadContext->active = true;
    
    if (sigsetjmp(currentThreadContext->jmpbuf, 1) == 0)
    {
        block(); // Execute the "_try" block
    }
    else
    {
        currentThreadContext->active = false;
        // Create a more detailed error message based on the fault address
        std::stringstream ss;
        //ss << "SIGSEGV caught: invalid memory access at address " << currentFaultAddress;
        
        // Add more specific information if possible
        if (currentFaultAddress == nullptr) {
            ss << "Invalid null pointer access exception";
            //ss << " (null pointer)";
        }else {
            ss << "Invalid memory access exception at address (0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(currentFaultAddress) << ")";
        }
        
        throw InvalidMemoryAccessException(ss.str());
    }
    
    currentThreadContext->active = false;
}

} // namespace MemoryGuard

// _try and _catch macros
#define _try \
    try      \
    { MemoryGuard::segvTryBlock([&]()

#define _catch(type, var)                                                                                                                  \
                                                                                                                                        ); \
    }                                                                                                                                      \
    catch (const type &var)

#endif // MEMORY_GUARD_HPP
