cmake_minimum_required(VERSION 3.20)

if (NOT TARGET bdk_stm32f4)
    include(${CMAKE_CURRENT_LIST_DIR}/../../../bdk_sdk_import.cmake)
endif()

project(usb_example C ASM)

set(BDK_USB_EXAMPLE_LOGGER_DIR "${CMAKE_CURRENT_LIST_DIR}/../../logger")
set(BDK_USB_EXAMPLE_SDK_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")

add_executable(usb_example
    main.c
    "${BDK_USB_EXAMPLE_LOGGER_DIR}/log.c"
    "${BDK_USB_EXAMPLE_SDK_DIR}/startup/syscalls.c"
)
target_include_directories(usb_example PRIVATE "${BDK_USB_EXAMPLE_LOGGER_DIR}")
target_link_libraries(usb_example bdk_stm32f4)

bdk_add_extra_outputs(usb_example)
