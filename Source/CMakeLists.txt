cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Vulkan REQUIRED)

file(GLOB SOURCES **.cpp)

add_library(VulkanHelper STATIC ${SOURCES})

# Libraries
target_link_libraries(VulkanHelper PRIVATE Vulkan::Vulkan)

target_include_directories(VulkanHelper PUBLIC ${PROJECT_SOURCE_DIR}/Include)
target_compile_options(VulkanHelper PRIVATE -Wall -Wextra -pedantic)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(VulkanHelper PRIVATE
        -fsanitize=address,undefined -g
    )
    target_link_options(VulkanHelper PRIVATE
        -fsanitize=address,undefined
    )
endif()