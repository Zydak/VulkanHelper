file(GLOB_RECURSE SOURCES *.cpp ../Include/*.h)

add_executable(RayTracing ${SOURCES})

if (NOT MSVC)
    target_compile_options(RayTracing PRIVATE -Wall -Wextra -Werror -Wpedantic -Wsign-conversion -Wconversion -Wshadow -Wcast-align -Wstrict-overflow=2 -Wlogical-op -Wmissing-declarations)
endif()

include(FetchContent)
FetchContent_Declare(
    VulkanHelper
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../..
)
FetchContent_MakeAvailable(VulkanHelper)

target_link_libraries(RayTracing PRIVATE VulkanHelper)

#if(CMAKE_BUILD_TYPE MATCHES "Debug")
#    target_compile_options(RayTracing PRIVATE
#        -fsanitize=address,undefined -g
#    )
#    target_link_options(RayTracing PRIVATE
#        -fsanitize=address,undefined
#    )
#endif()