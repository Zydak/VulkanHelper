function(suppress_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE /w)
    else()
        target_compile_options(${target} PRIVATE -w)
    endif()
endfunction()

include(FetchContent)

set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/Libraries")

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
)

FetchContent_MakeAvailable(glfw)
suppress_warnings(glfw)

FetchContent_MakeAvailable(fmt)
suppress_warnings(fmt)
