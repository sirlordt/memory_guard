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
#include <memory>  // For std::shared_ptr and std::make_shared
#include <stack>   // For std::stack

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
    //std::stack<jmp_buf> jmpbuf_stack; // Stack of jump buffers for nested try blocks
    std::stack<__jmp_buf_tag> jmpbuf_stack; //__jmp_buf_tag
    bool active = false;
    //std::function<int(void*, int)> handler;
    std::function<void(int, siginfo_t*, void*)> handler;
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

//[[noreturn]] 
void threadSegvHandler( int signal = 0, siginfo_t *signalInfo = nullptr, void *extra = nullptr )
{
    // ********** Very Important ************
    // Unblock the signal to allow to OS send again
    sigset_t sigs;

    memset( &sigs, 0 , sizeof( sigset_t ) );

    sigemptyset( &sigs );
    sigaddset( &sigs, signal );
    sigprocmask( SIG_UNBLOCK, &sigs, NULL );
    // ********** Very Important ************

    // Store the fault address for later use
    currentFaultAddress = nullptr;
    
    // We assume that currentThreadContext is not nullptr and is active
    // and that jmpbuf_stack is not empty
    if (!currentThreadContext->jmpbuf_stack.empty()) {
        // Get a reference to the top jump buffer
        __jmp_buf_tag& jumpBuffer = currentThreadContext->jmpbuf_stack.top();
        
        // Convert to jmp_buf for longjmp
        jmp_buf localJumpBuffer;
        std::memcpy(localJumpBuffer, &jumpBuffer, sizeof(jmp_buf));
        
        // Don't pop the stack here, it will be popped in segvTryBlock after longjmp
        
        // Jump back to the setjmp call
        longjmp(localJumpBuffer, signal ? signal : 1);
    }
}

// Global handler that delegates to the thread-specific handler
//inline int globalSegvHandler(void* addr, int serious)
//[[noreturn]] 
void globalSegvHandler( int signal = 0, siginfo_t *signalInfo = nullptr, void *extra = nullptr )
{
    // If we have a context for the current thread, we use its handler
    if (currentThreadContext && currentThreadContext->active)
    {
        currentThreadContext->handler(signal, signalInfo, extra );
        //return currentThreadContext->handler(addr, serious);
    }
    
    // If we don't have a context, we return 0 to indicate that we don't handle the signal
    //return 0;
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

        /*
        // Use standard signal handling instead of sigsegv library
        if (std::signal(SIGSEGV, [](int sig) {
            // Call our global handler with nullptr as we don't have the exact address
            globalSegvHandler(nullptr, 1);
        }) == SIG_ERR)
        {
            std::cerr << "Failed to install global SIGSEGV handler" << std::endl;
            std::exit(1);
        }
        */

        struct sigaction sa;

        sa.sa_flags = SA_SIGINFO;
          
        sigemptyset( &sa.sa_mask );
          
        sa.sa_sigaction = threadSegvHandler; //(void (*)(int, siginfo_t *, void *))
          
        //sigaction( SIGILL, &sa, NULL );
        //sigaction( SIGFPE, &sa, NULL );
        sigaction( SIGSEGV, &sa, NULL );            
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
    jmp_buf jmpbuf {};
    
    // Push the jump buffer onto the stack
    //currentThreadContext->jmpbuf_stack.push(jmpbuf[0]);
    
    //if (setjmp(currentThreadContext->jmpbuf_stack.top()) == 0)
    if (setjmp(jmpbuf) == 0)
    {
        currentThreadContext->jmpbuf_stack.push(jmpbuf[0]);
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
