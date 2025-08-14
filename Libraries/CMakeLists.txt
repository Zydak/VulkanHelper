function(suppress_warnings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE /w)
    else()
        target_compile_options(${target} PRIVATE -w)
    endif()
endfunction()



include(FetchContent)
include(ExternalProject)

set(FETCHCONTENT_BASE_DIR "${PROJECT_SOURCE_DIR}/Libraries")

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

FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.3.0
)

FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v6.0.2
)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG f58f558c120e9b32c217290b80bad1a0729fbb2c
)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.2b-docking
)

FetchContent_MakeAvailable(glfw)
suppress_warnings(glfw)

FetchContent_MakeAvailable(fmt)
suppress_warnings(fmt)

FetchContent_MakeAvailable(glm)
suppress_warnings(glm)

FetchContent_MakeAvailable(assimp)
suppress_warnings(assimp)

FetchContent_MakeAvailable(stb)

FetchContent_MakeAvailable(imgui)

FetchContent_MakeAvailable(VulkanMemoryAllocator)

FetchContent_MakeAvailable(Vulkan-Headers)
FetchContent_MakeAvailable(Vulkan-Loader)

# Clone prebuilt slang
if (MSVC)
    set(SLANG_URL "https://github.com/shader-slang/slang/releases/download/v2025.13.2/slang-2025.13.2-windows-x86_64.zip")

    set(SLANG_ZIP "${PROJECT_SOURCE_DIR}/Libraries/slang.zip")
    set(SLANG_DIR "${PROJECT_SOURCE_DIR}/Libraries/slang")

    # Download only if not already downloaded
    if(NOT EXISTS ${SLANG_ZIP})
        file(DOWNLOAD
            ${SLANG_URL}
            ${SLANG_ZIP}
            SHOW_PROGRESS
            STATUS DOWNLOAD_STATUS
            EXPECTED_HASH SHA256=049ba6023b13c237f26f6bf27f1909c39e518f2b75b7963df8f0ba26f1582421
        )
    endif()
else()
    set(SLANG_URL "https://github.com/shader-slang/slang/releases/download/v2025.13.2/slang-2025.13.2-linux-x86_64.zip")

    set(SLANG_ZIP "${PROJECT_SOURCE_DIR}/Libraries/slang.zip")
    set(SLANG_DIR "${PROJECT_SOURCE_DIR}/Libraries/slang")

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

if (MSVC)
    target_link_libraries(slang INTERFACE "${SLANG_DIR}/lib/slang.lib")
else()
    target_link_libraries(slang INTERFACE "${SLANG_DIR}/lib/libslang.so")
endif()

add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE "${PROJECT_SOURCE_DIR}/Libraries/stb-src/")

add_library(imgui STATIC)
target_include_directories(imgui PUBLIC 
    "${PROJECT_SOURCE_DIR}/Libraries/imgui-src/"
    "${PROJECT_SOURCE_DIR}/Libraries/imgui-src/backends/"
)
target_sources(imgui PRIVATE
    "${PROJECT_SOURCE_DIR}/Libraries/imgui-src/imgui.cpp"
    "${PROJECT_SOURCE_DIR}/Libraries/imgui-src/imgui_draw.cpp"
    "${PROJECT_SOURCE_DIR}/Libraries/imgui-src/imgui_tables.cpp"
    "${PROJECT_SOURCE_DIR}/Libraries/imgui-src/imgui_widgets.cpp"
    "${PROJECT_SOURCE_DIR}/Libraries/imgui-src/imgui_demo.cpp"
    "${PROJECT_SOURCE_DIR}/Libraries/imgui-src/backends/imgui_impl_glfw.cpp"
    "${PROJECT_SOURCE_DIR}/Libraries/imgui-src/backends/imgui_impl_vulkan.cpp"
)
target_link_libraries(imgui PRIVATE glfw)

suppress_warnings(imgui)
