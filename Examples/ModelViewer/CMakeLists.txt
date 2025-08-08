add_executable(ModelViewer Main.cpp)

if (NOT MSVC)
    target_compile_options(ModelViewer PRIVATE -Wall -Wextra -Werror -Wpedantic -Wsign-conversion -Wconversion -Wshadow -Wcast-align -Wstrict-overflow=2 -Wlogical-op -Wmissing-declarations)
endif()

include(FetchContent)
FetchContent_Declare(
    VulkanHelper
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../..
)
FetchContent_MakeAvailable(VulkanHelper)

target_link_libraries(ModelViewer PRIVATE VulkanHelper)