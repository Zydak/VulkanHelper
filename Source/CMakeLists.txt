find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SOURCES *.cpp ../Include/*.h)

add_library(VulkanHelper STATIC ${SOURCES})

# Libraries
target_link_libraries(
    VulkanHelper
    PRIVATE
    Vulkan::Vulkan
    glfw
)
target_link_libraries(
    VulkanHelper
    PUBLIC
    fmt
)

target_include_directories(VulkanHelper PUBLIC ${PROJECT_SOURCE_DIR}/Include)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(VulkanHelper PRIVATE
        -fsanitize=address,undefined -g
    )
    target_link_options(VulkanHelper PRIVATE
        -fsanitize=address,undefined
    )
endif()