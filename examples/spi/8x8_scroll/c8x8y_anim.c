/**
 * @file c8x8y_anim.c
 * @brief 24×8 effects for 3× 8x8Y (column-byte layout, Mikro font style).
 */

#include "c8x8y_anim.h"

#include "c8x8y_board.h"
#include "c8x8y_font.h"

#include <string.h>

extern void c8x8y_example_delay_ms(unsigned ms);

static uint16_t s_lfsr = 0xACE1U;

static const uint8_t k_heart_cols[8] = {
    0x00U, 0x66U, 0x99U, 0xFFU, 0xFFU, 0x7EU, 0x3CU, 0x18U,
};

static const uint8_t k_heart_small_cols[8] = {
    0x00U, 0x24U, 0x66U, 0xFFU, 0xFFU, 0x7EU, 0x3CU, 0x10U,
};

static const uint8_t k_blank_cols[8] = { 0 };

static uint8_t lfsr_u8(void)
{
    uint8_t bit = (uint8_t)(s_lfsr & 1U);

    s_lfsr >>= 1U;
    if (bit != 0U) {
        s_lfsr ^= 0xB400U;
    }

    return (uint8_t)(s_lfsr & 0xFFU);
}

static void intensity_all(c8x8y_t *ctx, uint8_t level)
{
    for (uint8_t p = 0U; p < ctx->cs_count; p++) {
        c8x8y_write_cmd_panel(ctx, p, C8X8Y_INTENSITY_REG, level);
    }
}

static void show_frame(c8x8y_t *ctx, const uint8_t wide[24])
{
    c8x8y_display_wide_columns(ctx, wide);
}

static void blank_panel(c8x8y_t *ctx, uint8_t panel)
{
    c8x8y_display_columns_panel(ctx, panel, k_blank_cols);
}

static void blit_char_wide(uint8_t wide[24], int x0, char ch)
{
    if ((uint8_t)ch < C8X8Y_ASCII_FIRST) {
        return;
    }

    uint8_t idx = (uint8_t)ch - C8X8Y_ASCII_FIRST;

    if (idx >= C8X8Y_ASCII_COUNT) {
        return;
    }

    for (uint8_t i = 0U; i < C8X8Y_GLYPH_WIDTH; i++) {
        int x = x0 + (int)i;

        if (x >= 0 && x < 24) {
            wide[x] = c8x8y_ascii_matrix[idx][C8X8Y_GLYPH_ROW0 + i];
        }
    }
}

static void matrix_rain(c8x8y_t *ctx, uint16_t frames, unsigned ms)
{
    uint8_t wide[24];

    memset(wide, 0, sizeof(wide));

    for (uint16_t f = 0U; f < frames; f++) {
        for (uint8_t c = 0U; c < 24U; c++) {
            uint8_t b = wide[c];

            b >>= 1U;
            if ((lfsr_u8() & 0x1FU) == 0U) {
                b |= 0x80U;
            }
            wide[c] = b;
        }

        show_frame(ctx, wide);
        c8x8y_example_delay_ms(ms);
    }
}

static void dual_comet_meet(c8x8y_t *ctx)
{
    uint8_t wide[24];
    const uint8_t tail[4] = { 0xFFU, 0x7EU, 0x3CU, 0x18U };

    for (int pos = 0; pos < 14; pos++) {
        memset(wide, 0, sizeof(wide));

        for (uint8_t t = 0U; t < 4U; t++) {
            int left  = pos - (int)t;
            int right = (23 - pos) + (int)t;

            if (left >= 0 && left < 24) {
                wide[left] |= tail[t];
            }
            if (right >= 0 && right < 24) {
                wide[right] |= tail[t];
            }
        }

        show_frame(ctx, wide);
        c8x8y_example_delay_ms(28U);
    }

    memset(wide, 0xFF, sizeof(wide));
    show_frame(ctx, wide);
    intensity_all(ctx, 15U);
    c8x8y_example_delay_ms(90U);
    intensity_all(ctx, C8X8Y_INTENSITY_LEVEL);
    c8x8y_display_refresh(ctx);
    c8x8y_example_delay_ms(40U);
}

static void center_burst(c8x8y_t *ctx)
{
    uint8_t wide[24];

    for (int r = 0; r < 13; r++) {
        memset(wide, 0, sizeof(wide));

        for (int c = 0; c < 24; c++) {
            int d = c - 11;

            if (d < 0) {
                d = -d;
            }
            if (d == r) {
                wide[c] = 0xFFU;
            } else if (d == r - 1 && r > 0) {
                wide[c] = 0x7EU;
            }
        }

        show_frame(ctx, wide);
        c8x8y_example_delay_ms(32U);
    }

    for (int r = 12; r >= 0; r--) {
        memset(wide, 0, sizeof(wide));

        for (int c = 0; c < 24; c++) {
            int d = c - 11;

            if (d < 0) {
                d = -d;
            }
            if (d == r) {
                wide[c] = 0x3CU;
            }
        }

        show_frame(ctx, wide);
        c8x8y_example_delay_ms(22U);
    }
}

static void knight_rider(c8x8y_t *ctx, uint8_t laps)
{
    uint8_t wide[24];
    const uint8_t seg[5] = { 0x18U, 0x3CU, 0x7EU, 0x3CU, 0x18U };

    for (uint8_t lap = 0U; lap < laps; lap++) {
        for (int pos = 0; pos <= 19; pos++) {
            memset(wide, 0, sizeof(wide));
            for (uint8_t i = 0U; i < 5U; i++) {
                int x = pos + (int)i;

                if (x >= 0 && x < 24) {
                    wide[x] = seg[i];
                }
            }
            show_frame(ctx, wide);
            c8x8y_example_delay_ms(26U);
        }
        for (int pos = 19; pos >= 0; pos--) {
            memset(wide, 0, sizeof(wide));
            for (uint8_t i = 0U; i < 5U; i++) {
                int x = pos + (int)i;

                if (x >= 0 && x < 24) {
                    wide[x] = seg[i];
                }
            }
            show_frame(ctx, wide);
            c8x8y_example_delay_ms(26U);
        }
    }
}

/** Three 8-wide glyphs exactly fill 24 columns (8 x 8 on three panels). */
static void blit_8x8_wide(uint8_t wide[24], int x0)
{
    blit_char_wide(wide, x0, '8');
    blit_char_wide(wide, x0 + 8, 'x');
    blit_char_wide(wide, x0 + 16, '8');
}

static void demo_8x8_slide(c8x8y_t *ctx)
{
    uint8_t wide[24];

    for (int x0 = 16; x0 >= 0; x0--) {
        memset(wide, 0, sizeof(wide));
        blit_8x8_wide(wide, x0);
        show_frame(ctx, wide);
        c8x8y_example_delay_ms(42U);
    }

    for (uint8_t pulse = 0U; pulse < 4U; pulse++) {
        memset(wide, 0, sizeof(wide));
        blit_8x8_wide(wide, 0);
        show_frame(ctx, wide);
        intensity_all(ctx, (pulse & 1U) != 0U ? 15U : 6U);
        c8x8y_example_delay_ms(100U);
    }

    intensity_all(ctx, C8X8Y_INTENSITY_LEVEL);
}

static void panel_march_chars(c8x8y_t *ctx)
{
    static const char k_seq[] = { 'S', 'P', 'I', '1', '!' };
    const uint8_t n = (uint8_t)(sizeof(k_seq) / sizeof(k_seq[0]));

    for (uint8_t i = 0U; i < n; i++) {
        for (uint8_t p = 0U; p < ctx->cs_count; p++) {
            blank_panel(ctx, p);
        }
        c8x8y_display_byte_panel(ctx, (uint8_t)(i % ctx->cs_count), k_seq[i]);
        c8x8y_example_delay_ms(220U);
    }

    c8x8y_display_refresh(ctx);
    c8x8y_example_delay_ms(80U);
}

static void hearts_panel_wave(c8x8y_t *ctx)
{
    for (uint8_t round = 0U; round < 2U; round++) {
        for (uint8_t p = 0U; p < ctx->cs_count; p++) {
            for (uint8_t q = 0U; q < ctx->cs_count; q++) {
                if (q == p) {
                    c8x8y_display_columns_panel(ctx, q, k_heart_cols);
                } else {
                    blank_panel(ctx, q);
                }
            }
            c8x8y_example_delay_ms(140U);
        }
        for (uint8_t p = 0U; p < ctx->cs_count; p++) {
            c8x8y_display_columns_panel(ctx, p, k_heart_small_cols);
        }
        c8x8y_example_delay_ms(80U);
    }

    for (uint8_t p = 0U; p < ctx->cs_count; p++) {
        c8x8y_display_columns_panel(ctx, p, k_heart_cols);
    }
}

static void vertical_wave(c8x8y_t *ctx, uint8_t cycles)
{
    uint8_t wide[24];

    for (uint8_t cy = 0U; cy < cycles; cy++) {
        for (uint8_t phase = 0U; phase < 8U; phase++) {
            memset(wide, 0, sizeof(wide));

            for (uint8_t c = 0U; c < 24U; c++) {
                uint8_t row = (uint8_t)((c + phase) & 7U);

                wide[c] = (uint8_t)(1U << row);
            }

            show_frame(ctx, wide);
            c8x8y_example_delay_ms(30U);
        }
    }
}

static void checker_wipe(c8x8y_t *ctx)
{
    uint8_t wide[24];

    for (uint8_t phase = 0U; phase < 16U; phase++) {
        memset(wide, 0, sizeof(wide));

        for (uint8_t c = 0U; c < 24U; c++) {
            if (((c + phase) & 1U) != 0U) {
                wide[c] = 0x55U;
            } else {
                wide[c] = 0xAAU;
            }
        }

        show_frame(ctx, wide);
        c8x8y_example_delay_ms(45U);
    }
}

static void cascade_fill(c8x8y_t *ctx)
{
    uint8_t wide[24];
    const uint8_t rows[8] = { 0x80U, 0xC0U, 0xE0U, 0xF0U, 0xF8U, 0xFCU, 0xFEU, 0xFFU };

    memset(wide, 0, sizeof(wide));

    for (uint8_t r = 0U; r < 8U; r++) {
        for (uint8_t c = 0U; c < 24U; c++) {
            wide[c] = rows[r];
        }
        show_frame(ctx, wide);
        c8x8y_example_delay_ms(35U);
    }

    c8x8y_example_delay_ms(120U);

    for (int r = 7; r >= 0; r--) {
        for (uint8_t c = 0U; c < 24U; c++) {
            wide[c] = rows[r];
        }
        show_frame(ctx, wide);
        c8x8y_example_delay_ms(28U);
    }
}

static void box_pulse(c8x8y_t *ctx)
{
    uint8_t wide[24];

    for (int inset = 12; inset >= 0; inset--) {
        memset(wide, 0, sizeof(wide));

        for (int c = inset; c < (24 - inset); c++) {
            wide[c] = 0xFFU;
        }

        show_frame(ctx, wide);
        c8x8y_example_delay_ms(32U);
    }

    for (int inset = 1; inset <= 12; inset++) {
        memset(wide, 0, sizeof(wide));

        for (int c = inset; c < (24 - inset); c++) {
            wide[c] = 0x7EU;
        }

        show_frame(ctx, wide);
        c8x8y_example_delay_ms(26U);
    }
}

static void heart_storm(c8x8y_t *ctx)
{
    const uint8_t levels[] = { 3U, 6U, 10U, 15U, 10U, 6U, 3U };

    for (uint8_t beat = 0U; beat < 4U; beat++) {
        for (uint8_t p = 0U; p < ctx->cs_count; p++) {
            c8x8y_display_columns_panel(ctx, p,
                                        (beat & 1U) != 0U ? k_heart_cols : k_heart_small_cols);
        }

        for (uint8_t i = 0U; i < sizeof(levels); i++) {
            intensity_all(ctx, levels[i]);
            c8x8y_example_delay_ms(28U);
        }
    }

    intensity_all(ctx, C8X8Y_INTENSITY_LEVEL);
}

static void fireworks(c8x8y_t *ctx, uint8_t shots)
{
    uint8_t wide[24];

    for (uint8_t s = 0U; s < shots; s++) {
        uint8_t cx = (uint8_t)(4U + (lfsr_u8() % 16U));

        for (int r = 0; r < 8; r++) {
            memset(wide, 0, sizeof(wide));

            for (int c = 0; c < 24; c++) {
                int d = (int)c - (int)cx;

                if (d < 0) {
                    d = -d;
                }
                if (d == r) {
                    wide[c] = (uint8_t)(0xFFU >> (uint8_t)r);
                }
            }

            show_frame(ctx, wide);
            c8x8y_example_delay_ms(24U);
        }

        c8x8y_example_delay_ms(50U);
    }
}

static void wipe_out(c8x8y_t *ctx)
{
    uint8_t wide[24];

    for (int inset = 0; inset <= 12; inset++) {
        memset(wide, 0, sizeof(wide));

        for (int c = inset; c < (24 - inset); c++) {
            wide[c] = 0xFFU;
        }

        show_frame(ctx, wide);
        c8x8y_example_delay_ms(28U);
    }
}

void c8x8y_anim_intro(c8x8y_t *ctx)
{
    if (ctx == NULL || ctx->cs_count == 0U) {
        return;
    }

    s_lfsr = 0xACE1U;
    intensity_all(ctx, C8X8Y_INTENSITY_LEVEL);

    matrix_rain(ctx, 45U, 36U);
    dual_comet_meet(ctx);
    center_burst(ctx);
    knight_rider(ctx, 2U);
    vertical_wave(ctx, 2U);
    checker_wipe(ctx);
    cascade_fill(ctx);
    box_pulse(ctx);
    demo_8x8_slide(ctx);
    panel_march_chars(ctx);
    hearts_panel_wave(ctx);
    heart_storm(ctx);
    fireworks(ctx, 3U);
    wipe_out(ctx);
    c8x8y_display_refresh(ctx);
    c8x8y_example_delay_ms(150U);
}

void c8x8y_anim_scroll_break(c8x8y_t *ctx)
{
    uint8_t wide[24];

    if (ctx == NULL || ctx->cs_count == 0U) {
        return;
    }

    for (int pos = 20; pos >= 0; pos--) {
        memset(wide, 0, sizeof(wide));
        wide[pos]     = 0xFFU;
        if (pos > 0) {
            wide[pos - 1] = 0x7EU;
        }
        if (pos > 1) {
            wide[pos - 2] = 0x3CU;
        }
        show_frame(ctx, wide);
        c8x8y_example_delay_ms(18U);
    }

    hearts_panel_wave(ctx);
    intensity_all(ctx, 15U);
    c8x8y_example_delay_ms(100U);
    intensity_all(ctx, C8X8Y_INTENSITY_LEVEL);

    fireworks(ctx, 2U);
    c8x8y_display_refresh(ctx);
}
