cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES **.cpp **.h)

add_executable(ExampleProject ${SOURCES})
target_link_libraries(ExampleProject PRIVATE VulkanHelper)

if (UNIX)
target_compile_options(ExampleProject PRIVATE -Wall -Wextra -pedantic)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(ExampleProject PRIVATE
        -fsanitize=address,undefined -g
    )
    target_link_options(ExampleProject PRIVATE
        -fsanitize=address,undefined
    )
endif()
