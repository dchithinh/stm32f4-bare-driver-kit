/**
 * @file c8x8y.c
 * @brief Port of MikroSDK v2 8x8Y click driver (BDK SPI/GPIO).
 * @see https://github.com/MikroElektronika/mikrosdk_click_v2/tree/master/clicks/8x8y
 */

#include "c8x8y.h"

#include "c8x8y_board.h"
#include "c8x8y_font.h"

#include <string.h>

#ifndef C8X8Y_SCROLL_WIDE_PANELS
#define C8X8Y_SCROLL_WIDE_PANELS 1
#endif

#ifndef C8X8Y_INTENSITY_LEVEL
#define C8X8Y_INTENSITY_LEVEL C8X8Y_INTENSITY_15
#endif

#ifndef C8X8Y_SPEED_SLOW_MS
#define C8X8Y_SPEED_SLOW_MS 90U
#endif

#if C8X8Y_DEBUG_LOG
#include "log.h"
#define C8X8Y_LOG(...) log_info(__VA_ARGS__)
#else
#define C8X8Y_LOG(...) ((void)0)
#endif

extern void c8x8y_example_delay_ms(unsigned ms);

static uint8_t  s_str_word[256];
static uint16_t s_global_cnt;
static uint16_t s_position;

static void cs_all_high(const c8x8y_t *ctx)
{
    for (uint8_t i = 0U; i < ctx->cs_count; i++) {
        bdk_gpio_set(ctx->cs_port[i], ctx->cs_pin[i]);
    }
}

static void cs_all_low(const c8x8y_t *ctx)
{
    for (uint8_t i = 0U; i < ctx->cs_count; i++) {
        bdk_gpio_clear(ctx->cs_port[i], ctx->cs_pin[i]);
    }
}

static void cs_low_dev(const c8x8y_t *ctx, uint8_t dev)
{
    bdk_gpio_clear(ctx->cs_port[dev], ctx->cs_pin[dev]);
}

static void cs_high_dev(const c8x8y_t *ctx, uint8_t dev)
{
    bdk_gpio_set(ctx->cs_port[dev], ctx->cs_pin[dev]);
}

static void cs_settle(void)
{
    for (volatile uint32_t i = 0U; i < 16U; i++) {
    }
}

static void cs_gpio_enable(bdk_gpio_port_t port, uint8_t pin)
{
    bdk_gpio_config_t cfg = {
        .port  = port,
        .pin   = pin,
        .mode  = BDK_GPIO_MODE_OUTPUT,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_UP,
        .speed = BDK_GPIO_SPEED_HIGH,
    };

    (void)bdk_gpio_init(&cfg);
    bdk_gpio_set(port, pin);
}

static void spi_write_cmd(c8x8y_t *ctx, const uint8_t tx_buf[2])
{
    bdk_status_t st = bdk_spi_write(ctx->spi, tx_buf, 2U);

    if (st != BDK_OK) {
        C8X8Y_LOG("spi err reg=0x%02x data=0x%02x st=%d",
                  (unsigned)tx_buf[0], (unsigned)tx_buf[1], (int)st);
        bdk_spi_recover(ctx->spi);
    }
}

void c8x8y_bind_spi(c8x8y_t *ctx, bdk_spi_id_t spi)
{
    ctx->spi          = spi;
    ctx->cs_count     = 0U;
    ctx->chain_len    = 1U;
    ctx->wiring       = C8X8Y_WIRING_DAISY_DIN;
    ctx->speed_scroll = C8X8Y_SPEED_MEDIUM;
}

void c8x8y_set_wiring(c8x8y_t *ctx, uint8_t wiring)
{
    ctx->wiring = wiring;
}

void c8x8y_set_chain_len(c8x8y_t *ctx, uint8_t chain_len)
{
    if (chain_len == 0U) {
        chain_len = 1U;
    }
    if (chain_len > C8X8Y_MAX_CS) {
        chain_len = C8X8Y_MAX_CS;
    }
    ctx->chain_len = chain_len;
}

int c8x8y_cs_add(c8x8y_t *ctx, bdk_gpio_port_t cs_port, uint8_t cs_pin)
{
    if (ctx->cs_count >= C8X8Y_MAX_CS) {
        return -1;
    }

    cs_gpio_enable(cs_port, cs_pin);

    ctx->cs_port[ctx->cs_count] = cs_port;
    ctx->cs_pin[ctx->cs_count]  = cs_pin;
    ctx->cs_count++;

    return 0;
}

void c8x8y_write_cmd_panel(c8x8y_t *ctx, uint8_t panel, uint8_t cmd, uint8_t data)
{
    uint8_t tx_buf[2];

    if (ctx->cs_count == 0U || panel >= ctx->cs_count) {
        return;
    }

    tx_buf[0] = cmd;
    tx_buf[1] = data;

    cs_all_high(ctx);
    cs_settle();
    cs_low_dev(ctx, panel);
    cs_settle();
    spi_write_cmd(ctx, tx_buf);
    cs_settle();
    cs_high_dev(ctx, panel);
    cs_all_high(ctx);
}

static void c8x8y_write_cmd_daisy(c8x8y_t *ctx, uint8_t cmd, uint8_t data)
{
    uint8_t tx_buf[2];

    tx_buf[0] = cmd;
    tx_buf[1] = data;

    cs_all_high(ctx);
    cs_settle();
    cs_all_low(ctx);
    cs_settle();

    for (uint8_t n = 0U; n < ctx->chain_len; n++) {
        spi_write_cmd(ctx, tx_buf);
    }

    cs_settle();
    cs_all_high(ctx);
}

void c8x8y_write_cmd(c8x8y_t *ctx, uint8_t cmd, uint8_t data)
{
    if (ctx->cs_count == 0U) {
        return;
    }

    if (ctx->wiring == C8X8Y_WIRING_DAISY_DIN) {
        c8x8y_write_cmd_daisy(ctx, cmd, data);
        return;
    }

    for (uint8_t dev = 0U; dev < ctx->cs_count; dev++) {
        c8x8y_write_cmd_panel(ctx, dev, cmd, data);
    }
}

void c8x8y_default_cfg_panel(c8x8y_t *ctx, uint8_t panel)
{
    C8X8Y_LOG("panel %u default_cfg start", (unsigned)panel);

    c8x8y_write_cmd_panel(ctx, panel, C8X8Y_DECODE_MODE_REG, C8X8Y_NO_DECODE);
    c8x8y_write_cmd_panel(ctx, panel, C8X8Y_INTENSITY_REG, C8X8Y_INTENSITY_LEVEL);
    c8x8y_write_cmd_panel(ctx, panel, C8X8Y_SCAN_LIMIT_REG, C8X8Y_DISPLAY_DIGIT_0_7);
    c8x8y_write_cmd_panel(ctx, panel, C8X8Y_SHUTDOWN_REG, C8X8Y_NORMAL_OPERATION);

    for (uint8_t cnt = 1U; cnt < 9U; cnt++) {
        c8x8y_write_cmd_panel(ctx, panel, cnt, 0x00U);
    }

    C8X8Y_LOG("panel %u default_cfg done", (unsigned)panel);
}

void c8x8y_display_refresh(c8x8y_t *ctx)
{
    for (uint8_t cnt = 1U; cnt < 9U; cnt++) {
        c8x8y_write_cmd(ctx, cnt, 0x00U);
    }
}

void c8x8y_set_speed_scroll(c8x8y_t *ctx, uint8_t speed)
{
    ctx->speed_scroll = speed;
}

void c8x8y_default_cfg(c8x8y_t *ctx)
{
    c8x8y_write_cmd(ctx, C8X8Y_DECODE_MODE_REG, C8X8Y_NO_DECODE);
    c8x8y_write_cmd(ctx, C8X8Y_INTENSITY_REG, C8X8Y_INTENSITY_LEVEL);
    c8x8y_write_cmd(ctx, C8X8Y_SCAN_LIMIT_REG, C8X8Y_DISPLAY_DIGIT_0_7);
    c8x8y_write_cmd(ctx, C8X8Y_SHUTDOWN_REG, C8X8Y_NORMAL_OPERATION);
    c8x8y_set_speed_scroll(ctx, C8X8Y_SPEED_MEDIUM);
    c8x8y_display_refresh(ctx);
}

void c8x8y_scroll_delay(const c8x8y_t *ctx)
{
    switch (ctx->speed_scroll) {
    case C8X8Y_SPEED_SLOW:
        c8x8y_example_delay_ms(C8X8Y_SPEED_SLOW_MS);
        break;
    case C8X8Y_SPEED_FAST:
        c8x8y_example_delay_ms(12U);
        break;
    case C8X8Y_SPEED_MEDIUM:
    default:
        c8x8y_example_delay_ms(25U);
        break;
    }
}

void c8x8y_display_byte_panel(c8x8y_t *ctx, uint8_t panel, char ch)
{
    uint8_t char_ascii;
    uint8_t position = 8U;

    if ((uint8_t)ch < C8X8Y_ASCII_FIRST) {
        return;
    }

    char_ascii = (uint8_t)ch - C8X8Y_ASCII_FIRST;
    if (char_ascii >= C8X8Y_ASCII_COUNT) {
        return;
    }

    for (uint8_t cnt = 2U; cnt < 10U; cnt++) {
        c8x8y_write_cmd_panel(ctx, panel, position,
                               c8x8y_ascii_matrix[char_ascii][cnt]);
        position--;
    }
}

void c8x8y_display_byte(c8x8y_t *ctx, char tx_byte)
{
    uint8_t char_ascii;
    uint8_t position = 8U;

    if (ctx->cs_count == 0U) {
        return;
    }

    if ((uint8_t)tx_byte < C8X8Y_ASCII_FIRST) {
        return;
    }

    char_ascii = (uint8_t)tx_byte - C8X8Y_ASCII_FIRST;
    if (char_ascii >= C8X8Y_ASCII_COUNT) {
        return;
    }

    if (ctx->wiring == C8X8Y_WIRING_PARALLEL_CS) {
        for (uint8_t panel = 0U; panel < ctx->cs_count; panel++) {
            c8x8y_display_byte_panel(ctx, panel, tx_byte);
        }
        return;
    }

    for (uint8_t cnt = 2U; cnt < 10U; cnt++) {
        uint8_t wr_chr = c8x8y_ascii_matrix[char_ascii][cnt];

        c8x8y_write_cmd(ctx, position, wr_chr);
        position--;
    }
}

void c8x8y_display_image(c8x8y_t *ctx, const uint8_t p_image[8])
{
    uint8_t position = 8U;

    for (uint8_t cnt = 0U; cnt < 8U; cnt++) {
        c8x8y_write_cmd(ctx, position, p_image[cnt]);
        position--;
    }
}

void c8x8y_scroll_prepare(c8x8y_t *ctx, const char *msg)
{
    size_t len = strlen(msg);

    (void)ctx;

    s_global_cnt = 0U;
    s_position   = 0U;

    for (size_t i = 0; i < len; i++) {
        uint8_t ch = (uint8_t)msg[i];

        if (ch < C8X8Y_ASCII_FIRST) {
            continue;
        }

        uint8_t char_ascii = ch - C8X8Y_ASCII_FIRST;

        if (char_ascii >= C8X8Y_ASCII_COUNT) {
            continue;
        }

        if (s_global_cnt > (uint16_t)(sizeof(s_str_word) - 10U)) {
            break;
        }

        for (uint8_t cnt = 0U; cnt < 10U; cnt++) {
            s_str_word[s_global_cnt++] = c8x8y_ascii_matrix[char_ascii][cnt];
        }
    }
}

int c8x8y_scroll_tick(c8x8y_t *ctx)
{
    uint16_t limit;
    uint8_t  window_cols = 8U;

    if (ctx == NULL || ctx->cs_count == 0U) {
        return 0;
    }

#if C8X8Y_SCROLL_WIDE_PANELS
    if (ctx->wiring == C8X8Y_WIRING_PARALLEL_CS && ctx->cs_count > 1U) {
        window_cols = (uint8_t)(8U * ctx->cs_count);
    }
#endif

    if (s_global_cnt <= window_cols) {
        return 0;
    }

    limit = s_global_cnt - window_cols;
    if (s_position >= limit) {
        s_position = 0U;
    }

#if C8X8Y_SCROLL_WIDE_PANELS
    if (ctx->wiring == C8X8Y_WIRING_PARALLEL_CS && ctx->cs_count > 1U) {
        for (uint8_t panel = 0U; panel < ctx->cs_count; panel++) {
            uint8_t        col  = 8U;
            const uint16_t base = s_position + (uint16_t)panel * 8U;

            for (uint8_t i = 0U; i < 8U; i++) {
                const uint16_t idx  = base + i;
                const uint8_t  data = (idx < s_global_cnt) ? s_str_word[idx] : 0U;

                c8x8y_write_cmd_panel(ctx, panel, col, data);
                col--;
            }
        }
    } else
#endif
    {
        uint8_t col = 8U;

        for (uint16_t cnt = s_position; cnt < (s_position + 8U); cnt++) {
            c8x8y_write_cmd(ctx, col, s_str_word[cnt]);
            col--;
        }
    }

    s_position++;
    if (s_position >= limit) {
        s_position = 0U;
    }

    return 1;
}

void c8x8y_display_string(c8x8y_t *ctx, const char *p_array)
{
    c8x8y_scroll_prepare(ctx, p_array);

    while (c8x8y_scroll_tick(ctx) != 0) {
        c8x8y_scroll_delay(ctx);
    }
}
