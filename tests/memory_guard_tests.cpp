// Include Catch2 headers directly
// CATCH_CONFIG_NO_POSIX_SIGNALS is defined in CMakeLists.txt
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include <atomic>
#include <thread>
#include <vector>
#include "MemoryGuard.hpp"

// Test case for null pointer dereference
TEST_CASE("MemoryGuard catches null pointer dereference", "[memory_guard]") {
    bool exception_caught = false;
    
    _try {
        // Attempt to dereference a null pointer
        int* ptr = nullptr;
        *ptr = 10; // This should trigger an exception
        
        // If we get here, the test should fail
        FAIL("Expected exception was not thrown");
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
        // Verify the exception message contains information about null pointer
        std::string error_message = e.what();
        bool has_null_info = error_message.find("null pointer") != std::string::npos;
        bool has_address_info = error_message.find("0x0") != std::string::npos;
        // Use parentheses to avoid issues with Catch2's operator overloading
        REQUIRE((has_null_info || has_address_info));
        exception_caught = true;
    }
    
    // Verify that the exception was caught
    REQUIRE(exception_caught);
    
    // Clean up resources
    MemoryGuard::unregisterThreadHandler();
}

// Test case for resource cleanup
TEST_CASE("MemoryGuard properly cleans up resources", "[memory_guard]") {
    // This test is more of a demonstration since we can't directly verify memory leaks in a unit test
    // In a real scenario, you would run this with a tool like Valgrind
    
    _try {
        // Some code that might cause a segmentation fault
        if (rand() % 10 == 0) { // Low probability to actually trigger the fault
            int* ptr = nullptr;
            *ptr = 10;
        }
    }
    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
        // Handle the exception
    }
    
    // Explicitly clean up resources
    MemoryGuard::unregisterThreadHandler();
    
    // If we run this test with Valgrind and there are no memory leaks reported,
    // then we know that unregisterThreadHandler() is working correctly
    SUCCEED("Resource cleanup test completed");
}

// Now that we've fixed the Catch2 signal handling, we can enable these tests

// Test case for invalid memory address access
TEST_CASE("MemoryGuard catches invalid memory address access", "[memory_guard]") {
    // This test is similar to the null pointer test but with a different description
    // to avoid duplicate test names
    bool exception_caught = false;
    
    // Create a new scope to ensure we have a fresh MemoryGuard context
    {
        _try {
            // Attempt to access an invalid memory address
            int* bad_ptr = nullptr;
            *bad_ptr = 20; // This should trigger an exception
            
            // If we get here, the test should fail
            FAIL("Expected exception was not thrown");
        }
        _catch(MemoryGuard::InvalidMemoryAccessException, e) {
            // Verify the exception message contains information about the invalid address
            std::string error_message = e.what();
            bool has_null_info = error_message.find("null pointer") != std::string::npos;
            bool has_address_info = error_message.find("0x0") != std::string::npos;
            // Use parentheses to avoid issues with Catch2's operator overloading
            REQUIRE((has_null_info || has_address_info));
            exception_caught = true;
        }
        
        // Clean up resources within this scope
        MemoryGuard::unregisterThreadHandler();
    }
    
    // Verify that the exception was caught
    REQUIRE(exception_caught);
}

// Test case for multi-threaded usage
TEST_CASE("MemoryGuard works correctly in multi-threaded environment", "[memory_guard]") {
    const int num_threads = 4;
    std::atomic<int> exceptions_caught(0);
    std::vector<std::thread> threads;
    
    // Create a new scope to ensure we have a fresh MemoryGuard context
    {
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&exceptions_caught, i]() {
                // Create a new scope for each thread
                {
                    _try {
                        // Every other thread will cause a segmentation fault
                        if (i % 2 == 0) {
                            int* ptr = nullptr;
                            *ptr = 10; // This should trigger an exception
                        }
                    }
                    _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                        exceptions_caught++;
                    }
                    
                    // Clean up resources for this thread
                    MemoryGuard::unregisterThreadHandler();
                }
            });
        }
        
        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }
    }
    
    // Verify that the expected number of exceptions were caught
    REQUIRE(exceptions_caught == num_threads / 2);
}

// Test case for nested try blocks with empty blocks (no exceptions)
TEST_CASE("MemoryGuard handles nested try blocks with empty blocks", "[memory_guard]") {
    int outer_executed = 0;
    int inner_executed = 0;
    
    // Create a new scope to ensure we have a fresh MemoryGuard context
    {
        _try {
            // Outer try block
            outer_executed++;
            
            _try {
                // Inner try block - just increment a counter, no exceptions
                inner_executed++;
            }
            _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                // This should not be executed
                FAIL("Unexpected exception caught in inner try block");
            }
            
            // This code should be executed
            REQUIRE(inner_executed == 1);
        }
        _catch(MemoryGuard::InvalidMemoryAccessException, e) {
            // This should not be executed
            FAIL("Unexpected exception caught in outer try block");
        }
        
        // Clean up resources within this scope
        MemoryGuard::unregisterThreadHandler();
    }
    
    // Verify that both blocks were executed
    REQUIRE(outer_executed == 1);
    REQUIRE(inner_executed == 1);
}

// Test case for nested try blocks with exception in inner block only
TEST_CASE("MemoryGuard handles nested try blocks with exception in inner block", "[memory_guard]") {
    int outer_executed = 0;
    int inner_caught = 0;
    
    // Create a new scope to ensure we have a fresh MemoryGuard context
    {
        _try {
            // Outer try block
            outer_executed++;
            
            // Check stack size after entering outer try block (should be 1)
            size_t outer_stack_size = MemoryGuard::currentThreadContext->jmpbuf_stack.size();
            std::cout << "Outer try block stack size: " << outer_stack_size << std::endl;
            REQUIRE(outer_stack_size == 1);
            
            // Store the address of the outer jmpbuf for later comparison
            void* outer_jmpbuf_addr = &(MemoryGuard::currentThreadContext->jmpbuf_stack.top());
            std::cout << "Outer jmpbuf address: " << outer_jmpbuf_addr << std::endl;
            
            _try {
                // Inner try block
                
                // Check stack size after entering inner try block (should be 2)
                size_t inner_stack_size = MemoryGuard::currentThreadContext->jmpbuf_stack.size();
                std::cout << "Inner try block stack size: " << inner_stack_size << std::endl;
                REQUIRE(inner_stack_size == 2);
                
                // Store the address of the inner jmpbuf
                void* inner_jmpbuf_addr = &(MemoryGuard::currentThreadContext->jmpbuf_stack.top());
                std::cout << "Inner jmpbuf address: " << inner_jmpbuf_addr << std::endl;
                
                // Verify that inner and outer jmpbuf addresses are different
                REQUIRE(inner_jmpbuf_addr != outer_jmpbuf_addr);
                
                // Generate an exception
                int* ptr = nullptr;
                *ptr = 10; // This should trigger an exception
                
                // If we get here, the test should fail
                FAIL("Expected exception was not thrown in inner try block");
            }
            _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                // This should be executed
                inner_caught++;
                
                // Check stack size after catching exception in inner try block (should be 1)
                size_t after_inner_stack_size = MemoryGuard::currentThreadContext->jmpbuf_stack.size();
                std::cout << "Stack size after inner catch: " << after_inner_stack_size << std::endl;
                REQUIRE(after_inner_stack_size == 1);
                
                // Verify that the current jmpbuf is the same as the outer jmpbuf
                void* current_jmpbuf_addr = &(MemoryGuard::currentThreadContext->jmpbuf_stack.top());
                std::cout << "Current jmpbuf address after inner catch: " << current_jmpbuf_addr << std::endl;
                REQUIRE(current_jmpbuf_addr == outer_jmpbuf_addr);
                
                // Do not call unregisterThreadHandler() here
                // as it would free the thread context needed by the outer try block
            }
            
            // This code should be executed after the inner exception is caught
            REQUIRE(inner_caught == 1);
            
            // Check stack size after inner try block (should still be 1)
            size_t after_inner_block_stack_size = MemoryGuard::currentThreadContext->jmpbuf_stack.size();
            std::cout << "Stack size after inner block: " << after_inner_block_stack_size << std::endl;
            REQUIRE(after_inner_block_stack_size == 1);
            
            // Verify that the current jmpbuf is still the same as the outer jmpbuf
            void* final_jmpbuf_addr = &(MemoryGuard::currentThreadContext->jmpbuf_stack.top());
            std::cout << "Final jmpbuf address: " << final_jmpbuf_addr << std::endl;
            REQUIRE(final_jmpbuf_addr == outer_jmpbuf_addr);

        }
        _catch(MemoryGuard::InvalidMemoryAccessException, e) {
            // This should not be executed
            FAIL("Unexpected exception caught in outer try block");
        }
        
        // Clean up resources within this scope
        MemoryGuard::unregisterThreadHandler();
    }
    
    // Verify that the outer block was executed and the inner exception was caught
    REQUIRE(outer_executed == 1);
    REQUIRE(inner_caught == 1);
}

// Test case for nested try blocks with exception in outer block only
// This test is commented out because it causes segmentation faults
// This is a known limitation of the current implementation of MemoryGuard
// The issue is that when a try block is nested inside another try block,
// both try blocks share the same thread context. When an exception is caught
// in the outer try block after the inner try block has completed,
// there seems to be an issue with the thread context.

// Uncommented to see what errors the sanitizers detect
TEST_CASE("MemoryGuard handles nested try blocks with exception in outer block", "[memory_guard]") {
    int inner_executed = 0;
    int outer_caught = 0;
    
    // Create a new scope to ensure we have a fresh MemoryGuard context
    {
        _try {
            // Outer try block
            
            // Check stack size after entering outer try block (should be 1)
            size_t outer_stack_size = MemoryGuard::currentThreadContext->jmpbuf_stack.size();
            std::cout << "Outer try block stack size: " << outer_stack_size << std::endl;
            REQUIRE(outer_stack_size == 1);
            
            // Store the address of the outer jmpbuf for later comparison
            void* outer_jmpbuf_addr = &(MemoryGuard::currentThreadContext->jmpbuf_stack.top());
            std::cout << "Outer jmpbuf address: " << outer_jmpbuf_addr << std::endl;
            
            _try {
                // Inner try block - just increment a counter, no exceptions
                inner_executed++;
                
                // Check stack size after entering inner try block (should be 2)
                size_t inner_stack_size = MemoryGuard::currentThreadContext->jmpbuf_stack.size();
                std::cout << "Inner try block stack size: " << inner_stack_size << std::endl;
                REQUIRE(inner_stack_size == 2);
                
                // Store the address of the inner jmpbuf
                void* inner_jmpbuf_addr = &(MemoryGuard::currentThreadContext->jmpbuf_stack.top());
                std::cout << "Inner jmpbuf address: " << inner_jmpbuf_addr << std::endl;
                
                // Verify that inner and outer jmpbuf addresses are different
                REQUIRE(inner_jmpbuf_addr != outer_jmpbuf_addr);
            }
            _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                // This should not be executed
                FAIL("Unexpected exception caught in inner try block");
            }
            
            // This code should be executed after the inner try block
            REQUIRE(inner_executed == 1);
            
            // Check stack size after inner try block (should be 1)
            size_t after_inner_block_stack_size = MemoryGuard::currentThreadContext->jmpbuf_stack.size();
            std::cout << "Stack size after inner block: " << after_inner_block_stack_size << std::endl;
            REQUIRE(after_inner_block_stack_size == 1);
            
            // Verify that the current jmpbuf is the same as the outer jmpbuf
            void* current_jmpbuf_addr = &(MemoryGuard::currentThreadContext->jmpbuf_stack.top());
            std::cout << "Current jmpbuf address after inner block: " << current_jmpbuf_addr << std::endl;
            REQUIRE(current_jmpbuf_addr == outer_jmpbuf_addr);
            
            // Now generate an exception in the outer try block
            std::cout << "Generating exception in outer try block..." << std::endl;
            int* ptr = nullptr;
            *ptr = 20; // This should trigger an exception
            
            // If we get here, the test should fail
            FAIL("Expected exception was not thrown in outer try block");
        }
        _catch(MemoryGuard::InvalidMemoryAccessException, e) {
            // This should be executed
            outer_caught++;
            std::cout << "Exception caught in outer try block" << std::endl;
            
            // Check stack size after catching exception in outer try block (should be 0)
            size_t after_outer_catch_stack_size = MemoryGuard::currentThreadContext->jmpbuf_stack.size();
            std::cout << "Stack size after outer catch: " << after_outer_catch_stack_size << std::endl;
        }
        
        // Clean up resources within this scope
        MemoryGuard::unregisterThreadHandler();
    }
    
    // Verify that the inner block was executed and the outer exception was caught
    REQUIRE(inner_executed == 1);
    REQUIRE(outer_caught == 1);
}

// Test case for sequential try blocks (not nested) with exceptions in both blocks
TEST_CASE("MemoryGuard handles sequential try blocks with exceptions", "[memory_guard]") {
    int first_caught = 0;
    int second_caught = 0;
    
    // Create a new scope to ensure we have a fresh MemoryGuard context
    {
        // First try block
        _try {
            // Generate an exception in the first try block
            int* ptr = nullptr;
            *ptr = 10; // This should trigger an exception
            
            // If we get here, the test should fail
            FAIL("Expected exception was not thrown in first try block");
        }
        _catch(MemoryGuard::InvalidMemoryAccessException, e) {
            // This should be executed
            first_caught++;
        }
        
        // Second try block (not nested, sequential)
        _try {
            // Generate an exception in the second try block
            int* ptr = nullptr;
            *ptr = 20; // This should trigger an exception
            
            // If we get here, the test should fail
            FAIL("Expected exception was not thrown in second try block");
        }
        _catch(MemoryGuard::InvalidMemoryAccessException, e) {
            // This should be executed
            second_caught++;
        }
        
        // Clean up resources within this scope
        MemoryGuard::unregisterThreadHandler();
    }
    
    // Verify that both exceptions were caught
    REQUIRE(first_caught == 1);
    REQUIRE(second_caught == 1);
}

// Test case for three nested try blocks with exception in the innermost block only
TEST_CASE("MemoryGuard handles three nested try blocks with exception in innermost block", "[memory_guard]") {
    int outer_executed = 0;
    int middle_executed = 0;
    int inner_caught = 0;
    
    // Create a new scope to ensure we have a fresh MemoryGuard context
    {
        _try {
            // Outer try block
            outer_executed++;
            
            _try {
                // Middle try block
                middle_executed++;
                
                _try {
                    // Inner try block - generate an exception
                    int* ptr = nullptr;
                    *ptr = 10; // This should trigger an exception
                    
                    // If we get here, the test should fail
                    FAIL("Expected exception was not thrown in inner try block");
                }
                _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                    // This should be executed
                    inner_caught++;
                }
                
                // This code should be executed after the inner exception is caught
                REQUIRE(inner_caught == 1);
            }
            _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                // This should not be executed
                FAIL("Unexpected exception caught in middle try block");
            }
            
            // This code should be executed
            REQUIRE(middle_executed == 1);
        }
        _catch(MemoryGuard::InvalidMemoryAccessException, e) {
            // This should not be executed
            FAIL("Unexpected exception caught in outer try block");
        }
        
        // Clean up resources within this scope
        MemoryGuard::unregisterThreadHandler();
    }
    
    // Verify that all blocks were executed as expected
    REQUIRE(outer_executed == 1);
    REQUIRE(middle_executed == 1);
    REQUIRE(inner_caught == 1);
}

// Test case for three nested try blocks with exception in the middle block only
TEST_CASE("MemoryGuard handles three nested try blocks with exception in middle block", "[memory_guard]") {
    int outer_executed = 0;
    int middle_caught = 0;
    int inner_executed = 0;
    
    // Create a new scope to ensure we have a fresh MemoryGuard context
    {
        _try {
            // Outer try block
            outer_executed++;
            
            _try {
                // Middle try block
                
                _try {
                    // Inner try block - no exception
                    inner_executed++;
                }
                _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                    // This should not be executed
                    FAIL("Unexpected exception caught in inner try block");
                }
                
                // This code should be executed
                REQUIRE(inner_executed == 1);
                
                // Now generate an exception in the middle block
                int* ptr = nullptr;
                *ptr = 20; // This should trigger an exception
                
                // If we get here, the test should fail
                FAIL("Expected exception was not thrown in middle try block");
            }
            _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                // This should be executed
                middle_caught++;
            }
            
            // This code should be executed after the middle exception is caught
            REQUIRE(middle_caught == 1);
        }
        _catch(MemoryGuard::InvalidMemoryAccessException, e) {
            // This should not be executed
            FAIL("Unexpected exception caught in outer try block");
        }
        
        // Clean up resources within this scope
        MemoryGuard::unregisterThreadHandler();
    }
    
    // Verify that all blocks were executed as expected
    REQUIRE(outer_executed == 1);
    REQUIRE(middle_caught == 1);
    REQUIRE(inner_executed == 1);
}

// Test case for three nested try blocks with exception in the outermost block only
TEST_CASE("MemoryGuard handles three nested try blocks with exception in outermost block", "[memory_guard]") {
    int outer_caught = 0;
    int middle_executed = 0;
    int inner_executed = 0;
    
    // Create a new scope to ensure we have a fresh MemoryGuard context
    {
        _try {
            // Outer try block
            
            _try {
                // Middle try block
                middle_executed++;
                
                _try {
                    // Inner try block - no exception
                    inner_executed++;
                }
                _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                    // This should not be executed
                    FAIL("Unexpected exception caught in inner try block");
                }
                
                // This code should be executed
                REQUIRE(inner_executed == 1);
            }
            _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                // This should not be executed
                FAIL("Unexpected exception caught in middle try block");
            }
            
            // This code should be executed
            REQUIRE(middle_executed == 1);
            
            // Now generate an exception in the outer block
            int* ptr = nullptr;
            *ptr = 30; // This should trigger an exception
            
            // If we get here, the test should fail
            FAIL("Expected exception was not thrown in outer try block");
        }
        _catch(MemoryGuard::InvalidMemoryAccessException, e) {
            // This should be executed
            outer_caught++;
        }
        
        // Clean up resources within this scope
        MemoryGuard::unregisterThreadHandler();
    }
    
    // Verify that all blocks were executed as expected
    REQUIRE(outer_caught == 1);
    REQUIRE(middle_executed == 1);
    REQUIRE(inner_executed == 1);
}

// Test case for three nested try blocks with exceptions in innermost and middle blocks
TEST_CASE("MemoryGuard handles three nested try blocks with exceptions in innermost and middle blocks", "[memory_guard]") {
    int outer_executed = 0;
    int middle_caught = 0;
    int inner_caught = 0;
    
    // Create a new scope to ensure we have a fresh MemoryGuard context
    {
        _try {
            // Outer try block
            outer_executed++;
            
            _try {
                // Middle try block
                
                _try {
                    // Inner try block - generate an exception
                    int* ptr = nullptr;
                    *ptr = 10; // This should trigger an exception
                    
                    // If we get here, the test should fail
                    FAIL("Expected exception was not thrown in inner try block");
                }
                _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                    // This should be executed
                    inner_caught++;
                }
                
                // This code should be executed after the inner exception is caught
                REQUIRE(inner_caught == 1);
                
                // Now generate an exception in the middle block
                int* ptr = nullptr;
                *ptr = 20; // This should trigger an exception
                
                // If we get here, the test should fail
                FAIL("Expected exception was not thrown in middle try block");
            }
            _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                // This should be executed
                middle_caught++;
            }
            
            // This code should be executed after the middle exception is caught
            REQUIRE(middle_caught == 1);
        }
        _catch(MemoryGuard::InvalidMemoryAccessException, e) {
            // This should not be executed
            FAIL("Unexpected exception caught in outer try block");
        }
        
        // Clean up resources within this scope
        MemoryGuard::unregisterThreadHandler();
    }
    
    // Verify that all blocks were executed as expected
    REQUIRE(outer_executed == 1);
    REQUIRE(middle_caught == 1);
    REQUIRE(inner_caught == 1);
}

// Test case for three nested try blocks with exceptions in innermost and outermost blocks
TEST_CASE("MemoryGuard handles three nested try blocks with exceptions in innermost and outermost blocks", "[memory_guard]") {
    int outer_caught = 0;
    int middle_executed = 0;
    int inner_caught = 0;
    
    // Create a new scope to ensure we have a fresh MemoryGuard context
    {
        _try {
            // Outer try block
            
            _try {
                // Middle try block
                middle_executed++;
                
                _try {
                    // Inner try block - generate an exception
                    int* ptr = nullptr;
                    *ptr = 10; // This should trigger an exception
                    
                    // If we get here, the test should fail
                    FAIL("Expected exception was not thrown in inner try block");
                }
                _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                    // This should be executed
                    inner_caught++;
                }
                
                // This code should be executed after the inner exception is caught
                REQUIRE(inner_caught == 1);
            }
            _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                // This should not be executed
                FAIL("Unexpected exception caught in middle try block");
            }
            
            // This code should be executed
            REQUIRE(middle_executed == 1);
            
            // Now generate an exception in the outer block
            int* ptr = nullptr;
            *ptr = 30; // This should trigger an exception
            
            // If we get here, the test should fail
            FAIL("Expected exception was not thrown in outer try block");
        }
        _catch(MemoryGuard::InvalidMemoryAccessException, e) {
            // This should be executed
            outer_caught++;
        }
        
        // Clean up resources within this scope
        MemoryGuard::unregisterThreadHandler();
    }
    
    // Verify that all blocks were executed as expected
    REQUIRE(outer_caught == 1);
    REQUIRE(middle_executed == 1);
    REQUIRE(inner_caught == 1);
}

// Test case for three nested try blocks with exceptions in middle and outermost blocks
TEST_CASE("MemoryGuard handles three nested try blocks with exceptions in middle and outermost blocks", "[memory_guard]") {
    int outer_caught = 0;
    int middle_caught = 0;
    int inner_executed = 0;
    
    // Create a new scope to ensure we have a fresh MemoryGuard context
    {
        _try {
            // Outer try block
            
            _try {
                // Middle try block
                
                _try {
                    // Inner try block - no exception
                    inner_executed++;
                }
                _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                    // This should not be executed
                    FAIL("Unexpected exception caught in inner try block");
                }
                
                // This code should be executed
                REQUIRE(inner_executed == 1);
                
                // Now generate an exception in the middle block
                int* ptr = nullptr;
                *ptr = 20; // This should trigger an exception
                
                // If we get here, the test should fail
                FAIL("Expected exception was not thrown in middle try block");
            }
            _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                // This should be executed
                middle_caught++;
            }
            
            // This code should be executed after the middle exception is caught
            REQUIRE(middle_caught == 1);
            
            // Now generate an exception in the outer block
            int* ptr = nullptr;
            *ptr = 30; // This should trigger an exception
            
            // If we get here, the test should fail
            FAIL("Expected exception was not thrown in outer try block");
        }
        _catch(MemoryGuard::InvalidMemoryAccessException, e) {
            // This should be executed
            outer_caught++;
        }
        
        // Clean up resources within this scope
        MemoryGuard::unregisterThreadHandler();
    }
    
    // Verify that all blocks were executed as expected
    REQUIRE(outer_caught == 1);
    REQUIRE(middle_caught == 1);
    REQUIRE(inner_executed == 1);
}

// Test case for standard C++ exceptions within _try blocks
TEST_CASE("MemoryGuard handles standard C++ exceptions within _try blocks", "[memory_guard]") {
    bool standard_exception_caught = false;
    
    // Create a new scope to ensure we have a fresh MemoryGuard context
    {
        _try {
            // Throw a standard C++ exception
            std::cout << "Throwing a standard C++ exception inside _try block..." << std::endl;
            throw std::runtime_error("Standard C++ exception");
            
            // If we get here, the test should fail
            FAIL("Expected standard exception was not thrown");
        }
        _catch(MemoryGuard::InvalidMemoryAccessException, e) {
            // This should not be executed
            FAIL("Unexpected memory access exception caught");
        }
        
        // Standard catch blocks to catch the standard exception
        catch(const std::runtime_error& e) {
            std::string error_message = e.what();
            std::cout << "Caught standard exception: " << error_message << std::endl;
            REQUIRE(error_message == "Standard C++ exception");
            standard_exception_caught = true;
        }
        catch(...) {
            FAIL("Unexpected exception type caught");
        }
        
        // Clean up resources within this scope
        MemoryGuard::unregisterThreadHandler();
    }
    
    // Verify that the standard exception was caught
    REQUIRE(standard_exception_caught);
}

// Test case for custom exceptions within _try blocks
TEST_CASE("MemoryGuard handles custom exceptions within _try blocks", "[memory_guard]") {
    class CustomException : public std::exception {
    public:
        const char* what() const noexcept override {
            return "Custom exception message";
        }
    };
    
    bool custom_exception_caught = false;
    
    // Create a new scope to ensure we have a fresh MemoryGuard context
    {
        _try {
            // Throw a custom exception
            std::cout << "Throwing a custom exception inside _try block..." << std::endl;
            throw CustomException();
            
            // If we get here, the test should fail
            FAIL("Expected custom exception was not thrown");
        }
        _catch(MemoryGuard::InvalidMemoryAccessException, e) {
            // This should not be executed
            FAIL("Unexpected memory access exception caught");
        }
        
        // Standard catch blocks to catch the custom exception
        catch(const CustomException& e) {
            std::string error_message = e.what();
            std::cout << "Caught custom exception: " << error_message << std::endl;
            REQUIRE(error_message == "Custom exception message");
            custom_exception_caught = true;
        }
        catch(...) {
            FAIL("Unexpected exception type caught");
        }
        
        // Clean up resources within this scope
        MemoryGuard::unregisterThreadHandler();
    }
    
    // Verify that the custom exception was caught
    REQUIRE(custom_exception_caught);
}

// Test case for mixed exceptions (memory access and standard exceptions) within _try blocks
TEST_CASE("MemoryGuard handles mixed exceptions within _try blocks", "[memory_guard]") {
    enum class ExceptionType { MEMORY_ACCESS, STANDARD, CUSTOM };
    
    class CustomException : public std::exception {
    public:
        const char* what() const noexcept override {
            return "Custom exception message";
        }
    };
    
    for (int type = 0; type < 3; ++type) {
        ExceptionType exceptionType = static_cast<ExceptionType>(type);
        bool exception_caught = false;
        
        // Create a new scope to ensure we have a fresh MemoryGuard context
        {
            _try {
                std::cout << "Testing exception type " << type << "..." << std::endl;
                
                // Throw different types of exceptions based on the loop iteration
                if (exceptionType == ExceptionType::MEMORY_ACCESS) {
                    // Cause a segmentation fault
                    int* ptr = nullptr;
                    *ptr = 10; // This will generate SIGSEGV
                }
                else if (exceptionType == ExceptionType::STANDARD) {
                    // Throw a standard exception
                    throw std::runtime_error("Standard C++ exception");
                }
                else {
                    // Throw a custom exception
                    throw CustomException();
                }
                
                // If we get here, the test should fail
                FAIL("Expected exception was not thrown");
            }
            _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                // This should only be executed for memory access exceptions
                if (exceptionType == ExceptionType::MEMORY_ACCESS) {
                    std::cout << "Caught memory access exception: " << e.what() << std::endl;
                    exception_caught = true;
                }
                else {
                    FAIL("Unexpected memory access exception caught");
                }
            }
            
            // Standard catch blocks for other exception types
            catch(const std::runtime_error& e) {
                if (exceptionType == ExceptionType::STANDARD) {
                    std::cout << "Caught standard exception: " << e.what() << std::endl;
                    REQUIRE(std::string(e.what()) == "Standard C++ exception");
                    exception_caught = true;
                }
                else {
                    FAIL("Unexpected standard exception caught");
                }
            }
            catch(const CustomException& e) {
                if (exceptionType == ExceptionType::CUSTOM) {
                    std::cout << "Caught custom exception: " << e.what() << std::endl;
                    REQUIRE(std::string(e.what()) == "Custom exception message");
                    exception_caught = true;
                }
                else {
                    FAIL("Unexpected custom exception caught");
                }
            }
            catch(...) {
                FAIL("Unexpected exception type caught");
            }
            
            // Clean up resources within this scope
            MemoryGuard::unregisterThreadHandler();
        }
        
        // Verify that the appropriate exception was caught
        REQUIRE(exception_caught);
    }
}

// Test case for three nested try blocks with exceptions in all three blocks
TEST_CASE("MemoryGuard handles three nested try blocks with exceptions in all blocks", "[memory_guard]") {
    int outer_caught = 0;
    int middle_caught = 0;
    int inner_caught = 0;
    
    // Create a new scope to ensure we have a fresh MemoryGuard context
    {
        _try {
            // Outer try block
            
            _try {
                // Middle try block
                
                _try {
                    // Inner try block - generate an exception
                    int* ptr = nullptr;
                    *ptr = 10; // This should trigger an exception
                    
                    // If we get here, the test should fail
                    FAIL("Expected exception was not thrown in inner try block");
                }
                _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                    // This should be executed
                    inner_caught++;
                }
                
                // This code should be executed after the inner exception is caught
                REQUIRE(inner_caught == 1);
                
                // Now generate an exception in the middle block
                int* ptr = nullptr;
                *ptr = 20; // This should trigger an exception
                
                // If we get here, the test should fail
                FAIL("Expected exception was not thrown in middle try block");
            }
            _catch(MemoryGuard::InvalidMemoryAccessException, e) {
                // This should be executed
                middle_caught++;
            }
            
            // This code should be executed after the middle exception is caught
            REQUIRE(middle_caught == 1);
            
            // Now generate an exception in the outer block
            int* ptr = nullptr;
            *ptr = 30; // This should trigger an exception
            
            // If we get here, the test should fail
            FAIL("Expected exception was not thrown in outer try block");
        }
        _catch(MemoryGuard::InvalidMemoryAccessException, e) {
            // This should be executed
            outer_caught++;
        }
        
        // Clean up resources within this scope
        MemoryGuard::unregisterThreadHandler();
    }
    
    // Verify that all blocks were executed as expected
    REQUIRE(outer_caught == 1);
    REQUIRE(middle_caught == 1);
    REQUIRE(inner_caught == 1);
}
