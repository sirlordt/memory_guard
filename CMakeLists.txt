cmake_minimum_required(VERSION 3.15)
project(cpp_project VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable testing
enable_testing()

# Include FetchContent module
include(FetchContent)

# Download and build Catch2 from GitHub
FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.3.2
)
FetchContent_MakeAvailable(catch2)

# Run the script to modify Catch2's signal handling
execute_process(
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/modify_catch2.sh
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  RESULT_VARIABLE SCRIPT_RESULT
)

if(NOT SCRIPT_RESULT EQUAL 0)
  message(WARNING "Failed to modify Catch2's signal handling. Tests may not work correctly.")
endif()

# Find dependencies (Conan 2.x approach)
find_package(fmt REQUIRED)

# Add executable
add_executable(${PROJECT_NAME} src/main.cpp)

# Link with dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt pthread)

# Add tests directory
add_subdirectory(tests)
