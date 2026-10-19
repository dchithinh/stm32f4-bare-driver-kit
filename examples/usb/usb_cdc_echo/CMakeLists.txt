cmake_minimum_required(VERSION 3.20)

if (NOT TARGET bdk_stm32f4)
    include(${CMAKE_CURRENT_LIST_DIR}/../../../bdk_sdk_import.cmake)
endif()

project(usb_cdc_echo C ASM)

set(BDK_USB_CDC_ECHO_LOGGER "${CMAKE_CURRENT_LIST_DIR}/../../logger")
set(BDK_USB_CDC_ECHO_SDK "${CMAKE_CURRENT_LIST_DIR}/../../..")

add_executable(usb_cdc_echo
    main.c
    stm32f4xx_it.c
    "${BDK_USB_CDC_ECHO_LOGGER}/log.c"
    "${BDK_USB_CDC_ECHO_SDK}/startup/syscalls.c"
)

target_include_directories(usb_cdc_echo PRIVATE "${BDK_USB_CDC_ECHO_LOGGER}")
target_link_libraries(usb_cdc_echo bdk_stm32f4)
bdk_add_extra_outputs(usb_cdc_echo)
