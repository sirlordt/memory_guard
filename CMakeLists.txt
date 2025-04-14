cmake_minimum_required(VERSION 3.15)
project(cpp_project VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find dependencies (Conan 2.x approach)
find_package(fmt REQUIRED)

# Add executable
add_executable(${PROJECT_NAME} src/main.cpp)

# Link with dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt sigsegv pthread)
