cmake_minimum_required(VERSION 3.5)

project(SharedRecursiveMutex LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Boost libraries
find_package(Boost COMPONENTS unit_test_framework REQUIRED)

set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS_INIT} ${ADDRESSSANITIZERFLAGS} ")

add_executable(SharedRecursiveMutex main.cpp
    shared_recursive_mutex.hpp)


add_executable(tests unit_test.cpp shared_recursive_mutex.hpp)
target_include_directories(tests PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(tests ${Boost_LIBRARIES})
