# STM32F407-specific flags, linker script, and helper functions.
# F407-only: other F4 parts differ in flash/RAM size and in the clock tree.

if (NOT BDK_SDK_PATH)
    get_filename_component(BDK_SDK_PATH "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

set(BDK_CHIP_CPU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

set(BDK_LINKER_SCRIPT "${BDK_SDK_PATH}/linker/stm32f407.ld")

# Emit .bin/.hex, a .map, and print size.
# The link launcher folds ld --print-memory-usage, objcopy, and size into one
# command so parallel builds print one complete program report at a time.
function(bdk_add_extra_outputs TARGET)
    target_link_options(${TARGET} PRIVATE
        "LINKER:-Map=$<TARGET_FILE_DIR:${TARGET}>/${TARGET}.map"
    )

    set(_bdk_wrap "${CMAKE_CURRENT_BINARY_DIR}/bdk-wrap-${TARGET}.sh")
    file(WRITE "${_bdk_wrap}"
"#!/bin/sh
export BDK_TARGET_NAME=${TARGET}
export BDK_OBJCOPY=${CMAKE_OBJCOPY}
export BDK_SIZE=${CMAKE_SIZE}
exec /bin/sh \"${BDK_SDK_PATH}/cmake/bdk_link_and_report.sh\" \"\$@\"
")

    set_property(TARGET ${TARGET} PROPERTY RULE_LAUNCH_LINK
        "/bin/sh \"${_bdk_wrap}\"")
endfunction()
