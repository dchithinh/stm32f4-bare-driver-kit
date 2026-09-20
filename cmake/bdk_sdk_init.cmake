# Included at the end of the application's project() via CMAKE_PROJECT_INCLUDE.
# Brings the SDK in as a subdirectory so target bdk_stm32f4 exists.

if (NOT TARGET bdk_stm32f4)
    if (NOT BDK_SDK_PATH)
        message(FATAL_ERROR "BDK_SDK_PATH is not set; include bdk_sdk_import.cmake first")
    endif()
    add_subdirectory("${BDK_SDK_PATH}" "${CMAKE_BINARY_DIR}/bdk-sdk")
endif()
