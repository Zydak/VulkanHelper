function(suppress_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE /w)
    else()
        target_compile_options(${target} PRIVATE -w)
    endif()
endfunction()

include(FetchContent)
include(ExternalProject)

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
    Vulkan-Headers
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG v1.4.321
)

FetchContent_Declare(
    Vulkan-Loader
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader.git
    GIT_TAG v1.4.321
)

FetchContent_MakeAvailable(glfw)
suppress_warnings(glfw)

FetchContent_MakeAvailable(fmt)
suppress_warnings(fmt)

FetchContent_MakeAvailable(glm)
suppress_warnings(glm)

FetchContent_MakeAvailable(Vulkan-Headers)
FetchContent_MakeAvailable(Vulkan-Loader)

# Clone prebuild slang
set(SLANG_URL "https://github.com/shader-slang/slang/releases/download/v2025.13.2/slang-2025.13.2-linux-x86_64.zip")
set(SLANG_ZIP "${CMAKE_SOURCE_DIR}/Libraries/slang.zip")
set(SLANG_DIR "${CMAKE_SOURCE_DIR}/Libraries/slang")

# Download only if not already downloaded
if(NOT EXISTS ${SLANG_ZIP})
    file(DOWNLOAD
        ${SLANG_URL}
        ${SLANG_ZIP}
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
        EXPECTED_HASH SHA256=ca46bacc50b9c426f131928253280512c77d4e9570bc6fb6c2fa60417b130968
    )
endif()

# Extract only if not already extracted
if(NOT EXISTS ${SLANG_DIR})
    file(MAKE_DIRECTORY ${SLANG_DIR})
    file(ARCHIVE_EXTRACT
        INPUT ${SLANG_ZIP}
        DESTINATION ${SLANG_DIR}
    )
endif()

add_library(slang INTERFACE)
target_include_directories(slang SYSTEM INTERFACE "${SLANG_DIR}/include/")
target_link_directories(slang SYSTEM INTERFACE "${SLANG_DIR}/lib/")
target_link_libraries(slang INTERFACE libslang.so)