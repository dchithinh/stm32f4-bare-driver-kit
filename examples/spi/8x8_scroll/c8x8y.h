#ifndef C8X8Y_H
#define C8X8Y_H

/**
 * @file c8x8y.h
 * @brief 8x8Y click on BDK — API aligned with MikroSDK click v2.
 * @see https://github.com/MikroElektronika/mikrosdk_click_v2/tree/master/clicks/8x8y
 */

#include <stdint.h>

#include "bdk_gpio.h"
#include "bdk_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define C8X8Y_MAX_CS 3U

/** Scroll strip buffer (8 column-bytes per ASCII char in @ref c8x8y_scroll_prepare). */
#define C8X8Y_SCROLL_BUF_SZ 512U

#define C8X8Y_DECODE_MODE_REG  0x09U
#define C8X8Y_INTENSITY_REG    0x0AU
#define C8X8Y_SCAN_LIMIT_REG   0x0BU
#define C8X8Y_SHUTDOWN_REG     0x0CU
#define C8X8Y_DISPLAY_TEST_REG  0x0FU

#define C8X8Y_NO_DECODE              0x00U
#define C8X8Y_INTENSITY_15           0x0FU
#define C8X8Y_DISPLAY_DIGIT_0_7      0x07U
#define C8X8Y_NORMAL_OPERATION       0x01U

#define C8X8Y_SPEED_SLOW    1U
#define C8X8Y_SPEED_MEDIUM 2U
#define C8X8Y_SPEED_FAST    3U

#define C8X8Y_WIRING_DAISY_DIN   0U
#define C8X8Y_WIRING_PARALLEL_CS 1U

typedef struct {
    bdk_spi_id_t    spi;
    bdk_gpio_port_t cs_port[C8X8Y_MAX_CS];
    uint8_t         cs_pin[C8X8Y_MAX_CS];
    uint8_t         cs_count;
    uint8_t         chain_len;
    uint8_t         wiring;
    uint8_t         speed_scroll;
} c8x8y_t;

void c8x8y_bind_spi(c8x8y_t *ctx, bdk_spi_id_t spi);

int c8x8y_cs_add(c8x8y_t *ctx, bdk_gpio_port_t cs_port, uint8_t cs_pin);

void c8x8y_set_wiring(c8x8y_t *ctx, uint8_t wiring);

void c8x8y_set_chain_len(c8x8y_t *ctx, uint8_t chain_len);

void c8x8y_default_cfg(c8x8y_t *ctx);

void c8x8y_default_cfg_panel(c8x8y_t *ctx, uint8_t panel);

void c8x8y_write_cmd(c8x8y_t *ctx, uint8_t cmd, uint8_t data);

void c8x8y_write_cmd_panel(c8x8y_t *ctx, uint8_t panel, uint8_t cmd, uint8_t data);

void c8x8y_display_refresh(c8x8y_t *ctx);

void c8x8y_set_speed_scroll(c8x8y_t *ctx, uint8_t speed);

void c8x8y_display_byte(c8x8y_t *ctx, char tx_byte);

void c8x8y_display_byte_panel(c8x8y_t *ctx, uint8_t panel, char ch);

void c8x8y_display_image(c8x8y_t *ctx, const uint8_t p_image[8]);

/** Eight column bytes (same layout as font indices 2–9) on one panel. */
void c8x8y_display_columns_panel(c8x8y_t *ctx, uint8_t panel, const uint8_t cols[8]);

/** Twenty-four column bytes across panels 0..2 (8 bytes each). */
void c8x8y_display_wide_columns(c8x8y_t *ctx, const uint8_t cols24[24]);

void c8x8y_display_string(c8x8y_t *ctx, const char *p_array);

void c8x8y_scroll_prepare(c8x8y_t *ctx, const char *msg);

/**
 * @return 0 idle; 1 advanced one column; 2 advanced and wrapped to strip start.
 */
int c8x8y_scroll_tick(c8x8y_t *ctx);

void c8x8y_scroll_delay(const c8x8y_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* C8X8Y_H */
