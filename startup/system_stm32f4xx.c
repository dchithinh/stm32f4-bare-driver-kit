/**
 * @file system_stm32f4xx.c
 * @brief CMSIS-required system symbols. Not a clock driver.
 *
 * SystemInit() is called from Reset_Handler before main(). Leave it empty
 * until bdk_rcc implements the F407 clock tree from RM0090.
 */

#include "stm32f4xx.h"

uint32_t SystemCoreClock = 16000000u; /* HSI reset default, RM0090 */

const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};
const uint8_t APBPrescTable[8]  = {0, 0, 0, 0, 1, 2, 3, 4};

void SystemInit(void)
{
    /* Clock tree setup belongs in bdk_rcc (RM0090 RCC chapter). */
}

void SystemCoreClockUpdate(void)
{
    /* Fill in from RCC registers once bdk_rcc exists. */
}
