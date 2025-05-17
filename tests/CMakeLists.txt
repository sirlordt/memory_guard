# Enable Address Sanitizer and Undefined Behavior Sanitizer
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")

# Create test executable
add_executable(try_catch_guard_tests try_catch_guard_tests.cpp)

# Link with Catch2 and required libraries
target_link_libraries(try_catch_guard_tests PRIVATE
    Catch2::Catch2WithMain
    pthread
)

# Include directories
target_include_directories(try_catch_guard_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Define CATCH_CONFIG_NO_POSIX_SIGNALS as an extra safety measure
target_compile_definitions(try_catch_guard_tests PRIVATE
    CATCH_CONFIG_NO_POSIX_SIGNALS
)

# Register test with CTest and set environment variables for Address Sanitizer
add_test(NAME try_catch_guard_tests COMMAND try_catch_guard_tests)
set_tests_properties(try_catch_guard_tests PROPERTIES
    ENVIRONMENT "ASAN_OPTIONS=handle_segv=0:allow_user_segv_handler=1:detect_leaks=0"
)

# Configure Catch2 integration with CTest
include(${CMAKE_BINARY_DIR}/_deps/catch2-src/extras/Catch.cmake)
catch_discover_tests(try_catch_guard_tests
    TEST_PREFIX ""
    PROPERTIES ENVIRONMENT "ASAN_OPTIONS=handle_segv=0:allow_user_segv_handler=1:detect_leaks=0"
)
