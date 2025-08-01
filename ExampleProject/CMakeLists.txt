file(GLOB_RECURSE SOURCES *.cpp *.h)

add_executable(ExampleProject ${SOURCES})

target_compile_options(ExampleProject PRIVATE -Wall -Wextra -Werror -Wpedantic -Wsign-conversion -Wconversion -Wshadow -Wcast-align -Wstrict-overflow=2 -Wlogical-op -Wmissing-declarations)

target_link_libraries(ExampleProject PRIVATE VulkanHelper)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(ExampleProject PRIVATE
        -fsanitize=address,undefined -g
    )
    target_link_options(ExampleProject PRIVATE
        -fsanitize=address,undefined
    )
endif()
