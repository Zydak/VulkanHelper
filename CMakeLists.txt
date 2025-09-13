cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# VCPKG initialization and bootstrapping
set(VCPKG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/vcpkg")

# Bootstrap vcpkg if needed
if(WIN32)
    set(VCPKG_EXECUTABLE "${VCPKG_ROOT}/vcpkg.exe")
    set(VCPKG_BOOTSTRAP "${VCPKG_ROOT}/bootstrap-vcpkg.bat")
else()
    set(VCPKG_EXECUTABLE "${VCPKG_ROOT}/vcpkg")
    set(VCPKG_BOOTSTRAP "${VCPKG_ROOT}/bootstrap-vcpkg.sh")
endif()

if(NOT EXISTS "${VCPKG_EXECUTABLE}")
    message(STATUS "Bootstrapping vcpkg...")
    execute_process(
        COMMAND ${VCPKG_BOOTSTRAP}
        WORKING_DIRECTORY "${VCPKG_ROOT}"
        RESULT_VARIABLE bootstrap_result
    )
    if(NOT bootstrap_result EQUAL "0")
        message(FATAL_ERROR "Failed to bootstrap vcpkg")
    endif()
endif()

project(VulkanHelper)

add_subdirectory("Libraries")

add_subdirectory("Source")
