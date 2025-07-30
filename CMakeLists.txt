cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

add_compile_options(-Wno-builtin-macro-redefined)

project(VulkanHelper)

add_subdirectory("Libraries")

add_subdirectory("Source")

add_subdirectory("ExampleProject")
