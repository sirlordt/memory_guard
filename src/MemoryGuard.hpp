#ifndef MEMORY_GUARD_HPP
#define MEMORY_GUARD_HPP

// Removed sigsegv.h dependency
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
#include <vector>
#include <cstring> // For memcpy

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

// Simple stack for jmp_buf
struct JmpBufStack {
    static const int MAX_DEPTH = 10; // Maximum nesting depth
    jmp_buf buffers[MAX_DEPTH];
    int top = -1; // -1 means empty
    
    bool empty() const {
        return top < 0;
    }
    
    void push(const jmp_buf& buf) {
        if (top < MAX_DEPTH - 1) {
            top++;
            memcpy(buffers[top], buf, sizeof(jmp_buf));
        }
    }
    
    jmp_buf& back() {
        return buffers[top];
    }
    
    void pop() {
        if (!empty()) {
            top--;
        }
    }
    
    // Return the current size of the stack (number of elements)
    size_t size() const {
        return top + 1; // top is 0-based, so size is top + 1
    }
};

// Structure to store thread-specific information
struct ThreadContext {
    JmpBufStack jmpbuf_stack; // Stack of jump buffers for nested try blocks
    bool active = false;
    std::function<int(void*, int)> handler;
};

// Global map to store thread-specific handlers
// Note: This requires synchronization as multiple threads may access it
inline std::unordered_map<std::thread::id, std::shared_ptr<ThreadContext>>& getThreadHandlers() {
    static std::unordered_map<std::thread::id, std::shared_ptr<ThreadContext>> handlers;
    return handlers;
}

inline std::mutex& getHandlersMutex() {
    static std::mutex mutex;
    return mutex;
}

// Thread-local variables for context and fault address
thread_local static std::shared_ptr<ThreadContext> currentThreadContext = nullptr;
thread_local static void* currentFaultAddress = nullptr;

// Thread-specific handler
inline int threadSegvHandler(void* addr, int serious)
{
    // Store the fault address for later use
    currentFaultAddress = addr;
    
    // We assume that currentThreadContext is not nullptr and is active
    // and that jmpbuf_stack is not empty
    if (!currentThreadContext->jmpbuf_stack.empty()) {
        longjmp(currentThreadContext->jmpbuf_stack.back(), 1);
    }
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
        // Create a new ThreadContext with proper initialization
        auto newContext = std::make_shared<ThreadContext>();
        newContext->active = false;
        newContext->handler = threadSegvHandler;
        
        // Assign to currentThreadContext
        currentThreadContext = newContext;
        
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
        
        // No need to delete manually, shared_ptr will handle it
        currentThreadContext = nullptr;
    }
}

// One-time initializer for the global handler
inline void installGlobalHandlerOnce()
{
    static bool installed = false;
    if (!installed)
    {
        // Use standard signal handling instead of sigsegv library
        if (std::signal(SIGSEGV, [](int sig) {
            // Call our global handler with nullptr as we don't have the exact address
            globalSegvHandler(nullptr, 1);
        }) == SIG_ERR)
        {
            std::cerr << "Failed to install global SIGSEGV handler" << std::endl;
            std::exit(1);
        }
        installed = true;
    }
}

// Internal function that throws an exception if we exit with longjmp
inline void segvTryBlock(const std::function<void()> &block)
{
    // Ensure that the global handler is installed
    installGlobalHandlerOnce();
    
    // Register a handler for this thread
    registerThreadHandler();
    
    // Activate the handler for this thread
    currentThreadContext->active = true;
    
    // Create a new jump buffer for this try block
    jmp_buf jmpbuf;
    
    // Push the jump buffer onto the stack
    currentThreadContext->jmpbuf_stack.push(jmpbuf);
    
    if (setjmp(currentThreadContext->jmpbuf_stack.back()) == 0)
    {
        block(); // Execute the "_try" block
    }
    else
    {
        currentThreadContext->active = false;
        
        // Pop the jump buffer from the stack
        currentThreadContext->jmpbuf_stack.pop();
        
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
    
    // Pop the jump buffer from the stack
    if (!currentThreadContext->jmpbuf_stack.empty()) {
        currentThreadContext->jmpbuf_stack.pop();
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
