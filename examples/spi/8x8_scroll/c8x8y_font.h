#ifndef C8X8Y_FONT_H
#define C8X8Y_FONT_H

#include <stdint.h>

/** Glyph table from MikroElektronika 8x8_Y_click (ascii_matrix). */
extern const uint8_t c8x8y_ascii_matrix[][10];

#define C8X8Y_ASCII_FIRST 32U
#define C8X8Y_GLYPH_ROWS  8U
#define C8X8Y_GLYPH_ROW0  2U
#define C8X8Y_GLYPH_WIDTH  8U
/** Entries in @c c8x8y_ascii_matrix (ASCII space through 'z'). */
#define C8X8Y_ASCII_LAST   122U
#define C8X8Y_ASCII_COUNT  (C8X8Y_ASCII_LAST - C8X8Y_ASCII_FIRST + 1U)

#endif /* C8X8Y_FONT_H */
