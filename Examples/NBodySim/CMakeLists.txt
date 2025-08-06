file(GLOB_RECURSE SOURCES *.cpp ../Include/*.h)

add_executable(NBodySim ${SOURCES})

target_compile_options(NBodySim PRIVATE -Wall -Wextra -Werror -Wpedantic -Wsign-conversion -Wconversion -Wshadow -Wcast-align -Wstrict-overflow=2 -Wlogical-op -Wmissing-declarations)

include(FetchContent)
FetchContent_Declare(
    VulkanHelper
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../..
)
FetchContent_MakeAvailable(VulkanHelper)

target_link_libraries(NBodySim PRIVATE VulkanHelper)

#if(CMAKE_BUILD_TYPE MATCHES "Debug")
#    target_compile_options(NBodySim PRIVATE
#        -fsanitize=address,undefined -g
#    )
#    target_link_options(NBodySim PRIVATE
#        -fsanitize=address,undefined
#    )
#endif()