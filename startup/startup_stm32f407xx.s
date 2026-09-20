/**
 * @file startup_stm32f407xx.s
 * @brief Minimal Cortex-M4 reset path and STM32F407 vector table.
 *
 * Reset_Handler copies .data, zeros .bss, calls SystemInit, then main().
 * Clock configuration is not done here — that belongs in bdk_rcc (RM0090).
 */

    .syntax unified
    .cpu cortex-m4
    .fpu fpv4-sp-d16
    .thumb

    .global g_pfnVectors
    .global Default_Handler
    .global Reset_Handler

    .section .text.Reset_Handler, "ax", %progbits
    .type Reset_Handler, %function
Reset_Handler:
    ldr   r0, =_estack
    mov   sp, r0

    /* Copy .data from flash (_sidata) to RAM (_sdata .. _edata). */
    ldr   r0, =_sdata
    ldr   r1, =_edata
    ldr   r2, =_sidata
    movs  r3, #0
    b     2f
1:
    ldr   r4, [r2, r3]
    str   r4, [r0, r3]
    adds  r3, r3, #4
2:
    adds  r4, r0, r3
    cmp   r4, r1
    bcc   1b

    /* Zero .bss (_sbss .. _ebss). */
    ldr   r0, =_sbss
    ldr   r1, =_ebss
    movs  r2, #0
    b     4f
3:
    str   r2, [r0]
    adds  r0, r0, #4
4:
    cmp   r0, r1
    bcc   3b

    bl    SystemInit
    bl    __libc_init_array
    bl    main

    /* main() is not supposed to return; park if it does. */
5:
    b     5b
    .size Reset_Handler, .-Reset_Handler

    .section .text.Default_Handler, "ax", %progbits
    .type Default_Handler, %function
Default_Handler:
    b     Default_Handler
    .size Default_Handler, .-Default_Handler

    .section .isr_vector, "a", %progbits
    .type g_pfnVectors, %object
g_pfnVectors:
    .word _estack
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word MemManage_Handler
    .word BusFault_Handler
    .word UsageFault_Handler
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler
    .word DebugMon_Handler
    .word 0
    .word PendSV_Handler
    .word SysTick_Handler
    /* External IRQs, order matches IRQn_Type in stm32f407xx.h */
    .word WWDG_IRQHandler
    .word PVD_IRQHandler
    .word TAMP_STAMP_IRQHandler
    .word RTC_WKUP_IRQHandler
    .word FLASH_IRQHandler
    .word RCC_IRQHandler
    .word EXTI0_IRQHandler
    .word EXTI1_IRQHandler
    .word EXTI2_IRQHandler
    .word EXTI3_IRQHandler
    .word EXTI4_IRQHandler
    .word DMA1_Stream0_IRQHandler
    .word DMA1_Stream1_IRQHandler
    .word DMA1_Stream2_IRQHandler
    .word DMA1_Stream3_IRQHandler
    .word DMA1_Stream4_IRQHandler
    .word DMA1_Stream5_IRQHandler
    .word DMA1_Stream6_IRQHandler
    .word ADC_IRQHandler
    .word CAN1_TX_IRQHandler
    .word CAN1_RX0_IRQHandler
    .word CAN1_RX1_IRQHandler
    .word CAN1_SCE_IRQHandler
    .word EXTI9_5_IRQHandler
    .word TIM1_BRK_TIM9_IRQHandler
    .word TIM1_UP_TIM10_IRQHandler
    .word TIM1_TRG_COM_TIM11_IRQHandler
    .word TIM1_CC_IRQHandler
    .word TIM2_IRQHandler
    .word TIM3_IRQHandler
    .word TIM4_IRQHandler
    .word I2C1_EV_IRQHandler
    .word I2C1_ER_IRQHandler
    .word I2C2_EV_IRQHandler
    .word I2C2_ER_IRQHandler
    .word SPI1_IRQHandler
    .word SPI2_IRQHandler
    .word USART1_IRQHandler
    .word USART2_IRQHandler
    .word USART3_IRQHandler
    .word EXTI15_10_IRQHandler
    .word RTC_Alarm_IRQHandler
    .word OTG_FS_WKUP_IRQHandler
    .word TIM8_BRK_TIM12_IRQHandler
    .word TIM8_UP_TIM13_IRQHandler
    .word TIM8_TRG_COM_TIM14_IRQHandler
    .word TIM8_CC_IRQHandler
    .word DMA1_Stream7_IRQHandler
    .word FSMC_IRQHandler
    .word SDIO_IRQHandler
    .word TIM5_IRQHandler
    .word SPI3_IRQHandler
    .word UART4_IRQHandler
    .word UART5_IRQHandler
    .word TIM6_DAC_IRQHandler
    .word TIM7_IRQHandler
    .word DMA2_Stream0_IRQHandler
    .word DMA2_Stream1_IRQHandler
    .word DMA2_Stream2_IRQHandler
    .word DMA2_Stream3_IRQHandler
    .word DMA2_Stream4_IRQHandler
    .word ETH_IRQHandler
    .word ETH_WKUP_IRQHandler
    .word CAN2_TX_IRQHandler
    .word CAN2_RX0_IRQHandler
    .word CAN2_RX1_IRQHandler
    .word CAN2_SCE_IRQHandler
    .word OTG_FS_IRQHandler
    .word DMA2_Stream5_IRQHandler
    .word DMA2_Stream6_IRQHandler
    .word DMA2_Stream7_IRQHandler
    .word USART6_IRQHandler
    .word I2C3_EV_IRQHandler
    .word I2C3_ER_IRQHandler
    .word OTG_HS_EP1_OUT_IRQHandler
    .word OTG_HS_EP1_IN_IRQHandler
    .word OTG_HS_WKUP_IRQHandler
    .word OTG_HS_IRQHandler
    .word DCMI_IRQHandler
    .word 0                          /* IRQ 79 reserved (no CRYP on F407) */
    .word HASH_RNG_IRQHandler
    .word FPU_IRQHandler
    .size g_pfnVectors, .-g_pfnVectors

    .macro BDK_WEAK name
    .weak \name
    .thumb_set \name, Default_Handler
    .endm

    BDK_WEAK NMI_Handler
    BDK_WEAK HardFault_Handler
    BDK_WEAK MemManage_Handler
    BDK_WEAK BusFault_Handler
    BDK_WEAK UsageFault_Handler
    BDK_WEAK SVC_Handler
    BDK_WEAK DebugMon_Handler
    BDK_WEAK PendSV_Handler
    BDK_WEAK SysTick_Handler
    BDK_WEAK WWDG_IRQHandler
    BDK_WEAK PVD_IRQHandler
    BDK_WEAK TAMP_STAMP_IRQHandler
    BDK_WEAK RTC_WKUP_IRQHandler
    BDK_WEAK FLASH_IRQHandler
    BDK_WEAK RCC_IRQHandler
    BDK_WEAK EXTI0_IRQHandler
    BDK_WEAK EXTI1_IRQHandler
    BDK_WEAK EXTI2_IRQHandler
    BDK_WEAK EXTI3_IRQHandler
    BDK_WEAK EXTI4_IRQHandler
    BDK_WEAK DMA1_Stream0_IRQHandler
    BDK_WEAK DMA1_Stream1_IRQHandler
    BDK_WEAK DMA1_Stream2_IRQHandler
    BDK_WEAK DMA1_Stream3_IRQHandler
    BDK_WEAK DMA1_Stream4_IRQHandler
    BDK_WEAK DMA1_Stream5_IRQHandler
    BDK_WEAK DMA1_Stream6_IRQHandler
    BDK_WEAK ADC_IRQHandler
    BDK_WEAK CAN1_TX_IRQHandler
    BDK_WEAK CAN1_RX0_IRQHandler
    BDK_WEAK CAN1_RX1_IRQHandler
    BDK_WEAK CAN1_SCE_IRQHandler
    BDK_WEAK EXTI9_5_IRQHandler
    BDK_WEAK TIM1_BRK_TIM9_IRQHandler
    BDK_WEAK TIM1_UP_TIM10_IRQHandler
    BDK_WEAK TIM1_TRG_COM_TIM11_IRQHandler
    BDK_WEAK TIM1_CC_IRQHandler
    BDK_WEAK TIM2_IRQHandler
    BDK_WEAK TIM3_IRQHandler
    BDK_WEAK TIM4_IRQHandler
    BDK_WEAK I2C1_EV_IRQHandler
    BDK_WEAK I2C1_ER_IRQHandler
    BDK_WEAK I2C2_EV_IRQHandler
    BDK_WEAK I2C2_ER_IRQHandler
    BDK_WEAK SPI1_IRQHandler
    BDK_WEAK SPI2_IRQHandler
    BDK_WEAK USART1_IRQHandler
    BDK_WEAK USART2_IRQHandler
    BDK_WEAK USART3_IRQHandler
    BDK_WEAK EXTI15_10_IRQHandler
    BDK_WEAK RTC_Alarm_IRQHandler
    BDK_WEAK OTG_FS_WKUP_IRQHandler
    BDK_WEAK TIM8_BRK_TIM12_IRQHandler
    BDK_WEAK TIM8_UP_TIM13_IRQHandler
    BDK_WEAK TIM8_TRG_COM_TIM14_IRQHandler
    BDK_WEAK TIM8_CC_IRQHandler
    BDK_WEAK DMA1_Stream7_IRQHandler
    BDK_WEAK FSMC_IRQHandler
    BDK_WEAK SDIO_IRQHandler
    BDK_WEAK TIM5_IRQHandler
    BDK_WEAK SPI3_IRQHandler
    BDK_WEAK UART4_IRQHandler
    BDK_WEAK UART5_IRQHandler
    BDK_WEAK TIM6_DAC_IRQHandler
    BDK_WEAK TIM7_IRQHandler
    BDK_WEAK DMA2_Stream0_IRQHandler
    BDK_WEAK DMA2_Stream1_IRQHandler
    BDK_WEAK DMA2_Stream2_IRQHandler
    BDK_WEAK DMA2_Stream3_IRQHandler
    BDK_WEAK DMA2_Stream4_IRQHandler
    BDK_WEAK ETH_IRQHandler
    BDK_WEAK ETH_WKUP_IRQHandler
    BDK_WEAK CAN2_TX_IRQHandler
    BDK_WEAK CAN2_RX0_IRQHandler
    BDK_WEAK CAN2_RX1_IRQHandler
    BDK_WEAK CAN2_SCE_IRQHandler
    BDK_WEAK OTG_FS_IRQHandler
    BDK_WEAK DMA2_Stream5_IRQHandler
    BDK_WEAK DMA2_Stream6_IRQHandler
    BDK_WEAK DMA2_Stream7_IRQHandler
    BDK_WEAK USART6_IRQHandler
    BDK_WEAK I2C3_EV_IRQHandler
    BDK_WEAK I2C3_ER_IRQHandler
    BDK_WEAK OTG_HS_EP1_OUT_IRQHandler
    BDK_WEAK OTG_HS_EP1_IN_IRQHandler
    BDK_WEAK OTG_HS_WKUP_IRQHandler
    BDK_WEAK OTG_HS_IRQHandler
    BDK_WEAK DCMI_IRQHandler
    BDK_WEAK HASH_RNG_IRQHandler
    BDK_WEAK FPU_IRQHandler
