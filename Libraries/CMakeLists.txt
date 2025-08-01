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
    GIT_TAG 3.4
)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.2.0
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)

FetchContent_Declare(
    slang
    GIT_REPOSITORY https://github.com/shader-slang/slang.git
    GIT_TAG v2025.13.2
)

FetchContent_MakeAvailable(glfw)
suppress_warnings(glfw)

FetchContent_MakeAvailable(fmt)
suppress_warnings(fmt)

FetchContent_MakeAvailable(glm)
suppress_warnings(glm)

set(SLANG_ENABLE_TESTS OFF CACHE STRING "")
set(SLANG_ENABLE_EXAMPLES OFF CACHE STRING "")
set(SLANG_ENABLE_GFX OFF CACHE STRING "")
set(SLANG_ENABLE_CUDA OFF CACHE STRING "")
set(SLANG_ENABLE_OPTIX OFF CACHE STRING "")
set(SLANG_ENABLE_NVAPI OFF CACHE STRING "")
set(SLANG_ENABLE_AFTERMATH OFF CACHE STRING "")
set(SLANG_ENABLE_XLIB OFF CACHE STRING "")
set(SLANG_LIB_TYPE STATIC CACHE STRING "")
FetchContent_MakeAvailable(slang)
suppress_warnings(slang)
