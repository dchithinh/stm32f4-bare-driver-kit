# Import the stm32f4-bare-driver-kit SDK into an application CMake project.
# One include, then the app only needs add_executable() +
# target_link_libraries(... bdk_stm32f4).
#
# Typical usage (from examples/<driver>/<app>/CMakeLists.txt):
#
#   cmake_minimum_required(VERSION 3.20)
#   include(${CMAKE_CURRENT_LIST_DIR}/../../../bdk_sdk_import.cmake)
#   project(gpio_blink C ASM)
#   add_executable(gpio_blink main.c)
#   target_link_libraries(gpio_blink bdk_stm32f4)
#   bdk_add_extra_outputs(gpio_blink)

if (DEFINED ENV{BDK_SDK_PATH} AND NOT BDK_SDK_PATH)
    set(BDK_SDK_PATH "$ENV{BDK_SDK_PATH}")
    message(STATUS "Using BDK_SDK_PATH from environment ('${BDK_SDK_PATH}')")
endif()

if (NOT BDK_SDK_PATH)
    # This file lives at the SDK root.
    get_filename_component(BDK_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
endif()

get_filename_component(BDK_SDK_PATH "${BDK_SDK_PATH}" REALPATH)
set(BDK_SDK_PATH "${BDK_SDK_PATH}" CACHE PATH "Path to stm32f4-bare-driver-kit" FORCE)

if (NOT EXISTS "${BDK_SDK_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "BDK SDK not found at '${BDK_SDK_PATH}'")
endif()

# Must be set before the application's project() call.
if (NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${BDK_SDK_PATH}/cmake/toolchain-arm-none-eabi.cmake"
        CACHE FILEPATH "ARM GCC toolchain file")
endif()

# project() has not run yet; pull the SDK in at the end of project().
set(CMAKE_PROJECT_INCLUDE "${BDK_SDK_PATH}/cmake/bdk_sdk_init.cmake")
