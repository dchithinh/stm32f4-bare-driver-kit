cmake_minimum_required(VERSION 3.20)

if (NOT TARGET bdk_stm32f4)
    include(${CMAKE_CURRENT_LIST_DIR}/../../../bdk_sdk_import.cmake)
endif()

project(usb_cdc C ASM)

set(BDK_USB_CDC_LOGGER "${CMAKE_CURRENT_LIST_DIR}/../../logger")
set(BDK_USB_CDC_SDK "${CMAKE_CURRENT_LIST_DIR}/../../..")

add_executable(usb_cdc
    main.c
    stm32f4xx_it.c
    "${BDK_USB_CDC_LOGGER}/log.c"
    "${BDK_USB_CDC_SDK}/startup/syscalls.c"
)

target_include_directories(usb_cdc PRIVATE "${BDK_USB_CDC_LOGGER}")
target_link_libraries(usb_cdc bdk_stm32f4)
bdk_add_extra_outputs(usb_cdc)
