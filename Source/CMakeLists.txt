file(GLOB_RECURSE SOURCES *.cpp ../Include/*.h)

add_library(VulkanHelper STATIC ${SOURCES})

target_compile_options(VulkanHelper PRIVATE -Wall -Wextra -Werror -Wpedantic -Wsign-conversion -Wconversion -Wshadow -Wcast-align -Wstrict-overflow=2 -Wlogical-op -Wmissing-declarations)

# Libraries
target_link_libraries(
    VulkanHelper
    PRIVATE
    Vulkan-Headers
    vulkan
    glfw
    slang
    VulkanMemoryAllocator
)

target_link_libraries(
    VulkanHelper
    PUBLIC
    fmt
    glm
)

# Mark library headers as SYSTEM so I don't get their stupid ass warnings
target_include_directories(VulkanHelper SYSTEM PUBLIC ${glm_SOURCE_DIR})
get_target_property(FMT_INCLUDE_DIRS fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(VulkanHelper SYSTEM PUBLIC ${FMT_INCLUDE_DIRS})

target_include_directories(VulkanHelper SYSTEM PRIVATE "../Libraries/vulkanmemoryallocator-src/include")
target_include_directories(VulkanHelper PRIVATE ${PROJECT_SOURCE_DIR}/Source)

target_include_directories(VulkanHelper PUBLIC ${PROJECT_SOURCE_DIR}/Include)