#ifndef USB_EXAMPLE_MAIN_H
#define USB_EXAMPLE_MAIN_H

#include "stm32f4xx_hal.h"

/* Discovery CN5 / OTG FS (same as Cube usb_test) */
#define OTG_FS_DM_Pin          GPIO_PIN_11
#define OTG_FS_DM_GPIO_Port    GPIOA
#define OTG_FS_DP_Pin          GPIO_PIN_12
#define OTG_FS_DP_GPIO_Port    GPIOA
#define OTG_FS_ID_Pin          GPIO_PIN_10
#define OTG_FS_ID_GPIO_Port    GPIOA
#define VBUS_FS_Pin            GPIO_PIN_9
#define VBUS_FS_GPIO_Port      GPIOA

void Error_Handler(void);

#endif
