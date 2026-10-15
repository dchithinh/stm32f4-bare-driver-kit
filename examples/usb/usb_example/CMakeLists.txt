cmake_minimum_required(VERSION 3.20)

if (NOT TARGET bdk_stm32f4)
    include(${CMAKE_CURRENT_LIST_DIR}/../../../bdk_sdk_import.cmake)
endif()

project(usb_example C ASM)

set(BDK_USB_EXAMPLE_LOGGER_DIR "${CMAKE_CURRENT_LIST_DIR}/../../logger")
set(BDK_USB_EXAMPLE_SDK_DIR "${CMAKE_CURRENT_LIST_DIR}/../../..")
set(BDK_USB_VENDOR_DIR "${CMAKE_CURRENT_LIST_DIR}/vendor")
set(BDK_USB_MW "${BDK_USB_VENDOR_DIR}/Middlewares/ST/STM32_USB_Device_Library")

add_executable(usb_example
    main.c
    "${BDK_USB_VENDOR_DIR}/stm32f4xx_hal_msp.c"
    "${BDK_USB_VENDOR_DIR}/stm32f4xx_it.c"
    "${BDK_USB_VENDOR_DIR}/USB_DEVICE/App/Src/usb_device.c"
    "${BDK_USB_VENDOR_DIR}/USB_DEVICE/App/Src/usbd_conf.c"
    "${BDK_USB_VENDOR_DIR}/USB_DEVICE/App/Src/usbd_desc.c"
    "${BDK_USB_VENDOR_DIR}/USB_DEVICE/App/Src/usbd_cdc_if.c"
    "${BDK_USB_MW}/Core/Src/usbd_core.c"
    "${BDK_USB_MW}/Core/Src/usbd_ctlreq.c"
    "${BDK_USB_MW}/Core/Src/usbd_ioreq.c"
    "${BDK_USB_MW}/Class/CDC/Src/usbd_cdc.c"
    "${BDK_USB_VENDOR_DIR}/HAL/Src/stm32f4xx_hal.c"
    "${BDK_USB_VENDOR_DIR}/HAL/Src/stm32f4xx_hal_cortex.c"
    "${BDK_USB_VENDOR_DIR}/HAL/Src/stm32f4xx_hal_gpio.c"
    "${BDK_USB_VENDOR_DIR}/HAL/Src/stm32f4xx_hal_pcd.c"
    "${BDK_USB_VENDOR_DIR}/HAL/Src/stm32f4xx_hal_pcd_ex.c"
    "${BDK_USB_VENDOR_DIR}/HAL/Src/stm32f4xx_hal_rcc.c"
    "${BDK_USB_VENDOR_DIR}/HAL/Src/stm32f4xx_hal_rcc_ex.c"
    "${BDK_USB_VENDOR_DIR}/HAL/Src/stm32f4xx_hal_flash.c"
    "${BDK_USB_VENDOR_DIR}/HAL/Src/stm32f4xx_hal_flash_ex.c"
    "${BDK_USB_VENDOR_DIR}/HAL/Src/stm32f4xx_hal_flash_ramfunc.c"
    "${BDK_USB_VENDOR_DIR}/HAL/Src/stm32f4xx_hal_pwr.c"
    "${BDK_USB_VENDOR_DIR}/HAL/Src/stm32f4xx_hal_pwr_ex.c"
    "${BDK_USB_VENDOR_DIR}/HAL/Src/stm32f4xx_ll_usb.c"
    "${BDK_USB_EXAMPLE_LOGGER_DIR}/log.c"
    "${BDK_USB_EXAMPLE_SDK_DIR}/startup/syscalls.c"
)

target_include_directories(usb_example PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}"
    "${BDK_USB_EXAMPLE_LOGGER_DIR}"
    "${BDK_USB_VENDOR_DIR}/USB_DEVICE/App/Inc"
    "${BDK_USB_VENDOR_DIR}"
    "${BDK_USB_MW}/Core/Inc"
    "${BDK_USB_MW}/Class/CDC/Inc"
    "${BDK_USB_VENDOR_DIR}/HAL/Inc"
    "${BDK_USB_VENDOR_DIR}/HAL/Inc/Legacy"
    "${BDK_USB_VENDOR_DIR}/HAL"
)

target_compile_definitions(usb_example PRIVATE
    USE_HAL_DRIVER
    USE_USB_FS
)

target_link_libraries(usb_example bdk_stm32f4)

bdk_add_extra_outputs(usb_example)
