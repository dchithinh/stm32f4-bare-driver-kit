/**
 * @file c8x8y_board.h
 * @brief 3× 8x8Y wiring (matches Cube spi_8x8 / Discovery lab).
 *
 * Option B: PA7 → SDI on each board; PA5 SCK; CS per board below.
 */
#ifndef C8X8Y_BOARD_H
#define C8X8Y_BOARD_H

#include "bdk_gpio.h"
#include "bdk_uart.h"

#define C8X8Y_MODULE_COUNT 3U

/** MAX7219 intensity register 0..15 (lower = dimmer). */
#define C8X8Y_INTENSITY_LEVEL 3U

/** Scroll step delay (ms) for @ref C8X8Y_SPEED_SLOW. */
#define C8X8Y_SPEED_SLOW_MS 90U

/** Hold time per panel during boot demo (show '1', then '2', then '3'). */
#define C8X8Y_BOOT_DIGIT_MS 1000U

#define C8X8Y_CS0_PORT BDK_GPIO_PORT_A
#define C8X8Y_CS0_PIN  4U

#define C8X8Y_CS1_PORT BDK_GPIO_PORT_B
#define C8X8Y_CS1_PIN  0U

#define C8X8Y_CS2_PORT BDK_GPIO_PORT_B
#define C8X8Y_CS2_PIN  1U

/** 1 = USART2 driver logs (disable during scroll — UART is slow). */
#define C8X8Y_DEBUG_LOG 0

/** 1 = one 24×8 ticker across 3 panels; 0 = same 8×8 mirrored on each. */
#define C8X8Y_SCROLL_WIDE_PANELS 1

/** Match your terminal (default 9600 = same as `uart_tx` lab). */
#ifndef C8X8Y_DEBUG_UART_BAUD
#define C8X8Y_DEBUG_UART_BAUD BDK_UART_BAUD_9600
#endif

#endif /* C8X8Y_BOARD_H */
