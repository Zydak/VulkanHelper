cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_COMPILER clang-15)
set(CMAKE_CXX_COMPILER clang++-15)

project(VulkanHelper)

add_subdirectory("Source")

add_subdirectory("ExampleProject")
