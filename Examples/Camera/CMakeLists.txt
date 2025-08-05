add_executable(Camera Main.cpp)

target_compile_options(Camera PRIVATE -Wall -Wextra -Werror -Wpedantic -Wsign-conversion -Wconversion -Wshadow -Wcast-align -Wstrict-overflow=2 -Wlogical-op -Wmissing-declarations)

include(FetchContent)
FetchContent_Declare(
    VulkanHelper
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../..
)
FetchContent_MakeAvailable(VulkanHelper)

target_link_libraries(Camera PRIVATE VulkanHelper)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(Camera PRIVATE
        -fsanitize=address,undefined -g
    )
    target_link_options(Camera PRIVATE
        -fsanitize=address,undefined
    )
endif()