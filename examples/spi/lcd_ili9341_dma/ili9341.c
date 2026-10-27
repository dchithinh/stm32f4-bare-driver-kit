/**
 * @file ili9341.c
 * @brief ILI9341 panel driver — commands via @ref bdk_spi_write; pixels polling or DMA.
 */

#include "ili9341.h"

#include "ili9341_board.h"
#include "ili9341_regs.h"

#include "bdk_rcc.h"
#include "bdk_spi.h"

#if (ILI9341_PIXEL_SPI_DMA != 0U)
#include "ili9341_board_dma.h"
#endif

extern void ili9341_example_delay_ms(unsigned ms);

static void cs_low(const ili9341_t *lcd)
{
    bdk_gpio_clear(lcd->cs_port, lcd->cs_pin);
}

static void cs_high(const ili9341_t *lcd)
{
    bdk_gpio_set(lcd->cs_port, lcd->cs_pin);
}

static void dc_cmd(const ili9341_t *lcd)
{
    bdk_gpio_clear(lcd->dc_port, lcd->dc_pin);
}

static void dc_data(const ili9341_t *lcd)
{
    bdk_gpio_set(lcd->dc_port, lcd->dc_pin);
}

static void bus_recover_on_fail(ili9341_t *lcd, bdk_status_t st)
{
    if (st != BDK_OK) {
        bdk_spi_recover(lcd->spi);
    }
}

/** LCD command byte (8-bit @ref bdk_spi_write, CS + D/C in app). */
static void bus_write_command(ili9341_t *lcd, uint8_t cmd)
{
    bdk_status_t st;

    (void)bdk_spi_set_width(lcd->spi, BDK_SPI_WIDTH_8);
    cs_low(lcd);
    dc_cmd(lcd);
    st = bdk_spi_write(lcd->spi, &cmd, 1U);
    dc_data(lcd);
    cs_high(lcd);
    bus_recover_on_fail(lcd, st);
}

static void bus_write_data(ili9341_t *lcd, const uint8_t *buffer, uint32_t length)
{
    bdk_status_t st;

    if (length == 0U) {
        return;
    }

    (void)bdk_spi_set_width(lcd->spi, BDK_SPI_WIDTH_8);
    dc_data(lcd);
    cs_low(lcd);
    st = bdk_spi_write(lcd->spi, buffer, (size_t)length);
    cs_high(lcd);
    bus_recover_on_fail(lcd, st);
}

/** After 0x2C: CS stays low; each scanline is another TX burst. */
static void bus_pixels_frame_begin(ili9341_t *lcd)
{
#if (ILI9341_PIXEL_SPI_DMA != 0U)
    (void)bdk_spi_set_width(lcd->spi, BDK_SPI_WIDTH_8);
#elif (ILI9341_PIXEL_SPI_16BIT != 0U)
    (void)bdk_spi_set_width(lcd->spi, BDK_SPI_WIDTH_16);
#else
    (void)bdk_spi_set_width(lcd->spi, BDK_SPI_WIDTH_8);
#endif
    dc_data(lcd);
    cs_low(lcd);
}

#if (ILI9341_PIXEL_SPI_16BIT != 0U)
static void bus_pixels_frame_push(ili9341_t *lcd, const uint16_t *pixels,
                                  uint32_t npix)
{
#if (ILI9341_PIXEL_SPI_DMA != 0U)
    bus_recover_on_fail(lcd,
                        ili9341_board_dma_write(lcd->spi, pixels,
                                                (size_t)npix * 2U));
#else
    bus_recover_on_fail(lcd, bdk_spi_write(lcd->spi, pixels, (size_t)npix));
#endif
}
#else
static void bus_pixels_frame_push(ili9341_t *lcd, const uint8_t *buffer,
                                  uint32_t nbytes)
{
#if (ILI9341_PIXEL_SPI_DMA != 0U)
    bus_recover_on_fail(lcd,
                        ili9341_board_dma_write(lcd->spi, buffer,
                                                (size_t)nbytes));
#else
    bus_recover_on_fail(lcd, bdk_spi_write(lcd->spi, buffer, (size_t)nbytes));
#endif
}
#endif

static void bus_pixels_frame_end(ili9341_t *lcd)
{
    cs_high(lcd);
    (void)bdk_spi_set_width(lcd->spi, BDK_SPI_WIDTH_8);
}

static void panel_reset(const ili9341_t *lcd)
{
    bdk_gpio_clear(lcd->rst_port, lcd->rst_pin);
    ili9341_example_delay_ms(50U);
    bdk_gpio_set(lcd->rst_port, lcd->rst_pin);
    ili9341_example_delay_ms(50U);
}

static void chip_config(ili9341_t *lcd)
{
    uint8_t params[15];

    bus_write_command(lcd, ILI9341_SWRESET);

    bus_write_command(lcd, ILI9341_POWERB);
    params[0] = 0x00U;
    params[1] = 0xD9U;
    params[2] = 0x30U;
    bus_write_data(lcd, params, 3U);

    bus_write_command(lcd, ILI9341_POWER_SEQ);
    params[0] = 0x64U;
    params[1] = 0x03U;
    params[2] = 0x12U;
    params[3] = 0x81U;
    bus_write_data(lcd, params, 4U);

    bus_write_command(lcd, ILI9341_DTCA);
    params[0] = 0x85U;
    params[1] = 0x10U;
    params[2] = 0x7AU;
    bus_write_data(lcd, params, 3U);

    bus_write_command(lcd, ILI9341_POWERA);
    params[0] = 0x39U;
    params[1] = 0x2CU;
    params[2] = 0x00U;
    params[3] = 0x34U;
    params[4] = 0x02U;
    bus_write_data(lcd, params, 5U);

    bus_write_command(lcd, ILI9341_PRC);
    params[0] = 0x20U;
    bus_write_data(lcd, params, 1U);

    bus_write_command(lcd, ILI9341_DTCB);
    params[0] = 0x00U;
    params[1] = 0x00U;
    bus_write_data(lcd, params, 2U);

    bus_write_command(lcd, ILI9341_POWER1);
    params[0] = 0x1BU;
    bus_write_data(lcd, params, 1U);

    bus_write_command(lcd, ILI9341_POWER2);
    params[0] = 0x12U;
    bus_write_data(lcd, params, 1U);

    bus_write_command(lcd, ILI9341_VCOM1);
    params[0] = 0x08U;
    params[1] = 0x26U;
    bus_write_data(lcd, params, 2U);

    bus_write_command(lcd, ILI9341_VCOM2);
    params[0] = 0xB7U;
    bus_write_data(lcd, params, 1U);

    bus_write_command(lcd, ILI9341_PIXEL_FORMAT);
    params[0] = 0x55U;
    bus_write_data(lcd, params, 1U);

    bus_write_command(lcd, ILI9341_FRMCTR1);
    params[0] = 0x00U;
    params[1] = 0x1BU;
    bus_write_data(lcd, params, 2U);

    bus_write_command(lcd, ILI9341_DFC);
    params[0] = 0x0AU;
    params[1] = 0xA2U;
    bus_write_data(lcd, params, 2U);

    bus_write_command(lcd, ILI9341_3GAMMA_EN);
    params[0] = 0x02U;
    bus_write_data(lcd, params, 1U);

    bus_write_command(lcd, ILI9341_GAMMA);
    params[0] = 0x01U;
    bus_write_data(lcd, params, 1U);

    bus_write_command(lcd, ILI9341_PGAMMA);
    params[0]  = 0x0FU;
    params[1]  = 0x1DU;
    params[2]  = 0x1AU;
    params[3]  = 0x0AU;
    params[4]  = 0x0DU;
    params[5]  = 0x07U;
    params[6]  = 0x49U;
    params[7]  = 0x66U;
    params[8]  = 0x3BU;
    params[9]  = 0x07U;
    params[10] = 0x11U;
    params[11] = 0x01U;
    params[12] = 0x09U;
    params[13] = 0x05U;
    params[14] = 0x04U;
    bus_write_data(lcd, params, 15U);

    bus_write_command(lcd, ILI9341_NGAMMA);
    params[0]  = 0x00U;
    params[1]  = 0x18U;
    params[2]  = 0x1DU;
    params[3]  = 0x02U;
    params[4]  = 0x0FU;
    params[5]  = 0x04U;
    params[6]  = 0x36U;
    params[7]  = 0x13U;
    params[8]  = 0x4CU;
    params[9]  = 0x07U;
    params[10] = 0x13U;
    params[11] = 0x0FU;
    params[12] = 0x2EU;
    params[13] = 0x2FU;
    params[14] = 0x05U;
    bus_write_data(lcd, params, 15U);

    bus_write_command(lcd, ILI9341_SLEEP_OUT);
    ili9341_example_delay_ms(50U);
    ili9341_example_delay_ms(50U);
    bus_write_command(lcd, ILI9341_DISPLAY_ON);
}

static void draw_panel_size(uint8_t orientation, uint16_t *w, uint16_t *h)
{
    if (orientation == ILI9341_LCD_LANDSCAPE) {
        *w = ILI9341_LCD_NATIVE_HEIGHT;
        *h = ILI9341_LCD_NATIVE_WIDTH;
    } else {
        *w = ILI9341_LCD_NATIVE_WIDTH;
        *h = ILI9341_LCD_NATIVE_HEIGHT;
    }
}

static void apply_madctl(ili9341_t *lcd, uint8_t orientation)
{
    uint8_t param;

    if (orientation == ILI9341_LCD_LANDSCAPE) {
        param = (uint8_t)ILI9341_MADCTL_LANDSCAPE;
    } else {
        param = (uint8_t)ILI9341_MADCTL_PORTRAIT;
    }

    bus_write_command(lcd, ILI9341_MAC);
    bus_write_data(lcd, &param, 1U);
}

void ili9341_set_orientation(ili9341_t *lcd, uint8_t orientation)
{
    lcd->orientation = orientation;
    apply_madctl(lcd, orientation);
}

void ili9341_bind(ili9341_t *lcd, bdk_spi_id_t spi,
                  bdk_gpio_port_t cs_port, uint8_t cs_pin,
                  bdk_gpio_port_t dc_port, uint8_t dc_pin,
                  bdk_gpio_port_t rst_port, uint8_t rst_pin)
{
    lcd->spi      = spi;
    lcd->cs_port  = cs_port;
    lcd->cs_pin   = cs_pin;
    lcd->dc_port  = dc_port;
    lcd->dc_pin   = dc_pin;
    lcd->rst_port = rst_port;
    lcd->rst_pin  = rst_pin;
}

void ili9341_gpio_init(const ili9341_t *lcd)
{
    bdk_gpio_config_t out = {
        .mode  = BDK_GPIO_MODE_OUTPUT,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_HIGH,
        .af    = 0,
    };

    out.port = lcd->cs_port;
    out.pin  = lcd->cs_pin;
    BDK_ASSERT_OK(bdk_gpio_init(&out));
    bdk_gpio_set(lcd->cs_port, lcd->cs_pin);

    out.port = lcd->dc_port;
    out.pin  = lcd->dc_pin;
    BDK_ASSERT_OK(bdk_gpio_init(&out));
    bdk_gpio_set(lcd->dc_port, lcd->dc_pin);

    out.port = lcd->rst_port;
    out.pin  = lcd->rst_pin;
    BDK_ASSERT_OK(bdk_gpio_init(&out));
    bdk_gpio_set(lcd->rst_port, lcd->rst_pin);

    bdk_gpio_config_t sck = {
        .port  = ILI9341_BOARD_SCK_PORT,
        .pin   = ILI9341_BOARD_SCK_PIN,
        .mode  = BDK_GPIO_MODE_AF,
        .af    = ILI9341_BOARD_SPI_AF,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_NONE,
        .speed = BDK_GPIO_SPEED_HIGH,
    };
    bdk_gpio_config_t mosi = sck;

    mosi.port = ILI9341_BOARD_MOSI_PORT;
    mosi.pin  = ILI9341_BOARD_MOSI_PIN;
    BDK_ASSERT_OK(bdk_gpio_init(&sck));
    BDK_ASSERT_OK(bdk_gpio_init(&mosi));

    bdk_gpio_config_t miso = {
        .port  = ILI9341_BOARD_MISO_PORT,
        .pin   = ILI9341_BOARD_MISO_PIN,
        .mode  = BDK_GPIO_MODE_AF,
        .af    = ILI9341_BOARD_SPI_AF,
        .otype = BDK_GPIO_OTYPE_PP,
        .pull  = BDK_GPIO_PULL_UP,
        .speed = BDK_GPIO_SPEED_HIGH,
    };
    BDK_ASSERT_OK(bdk_gpio_init(&miso));

    (void)bdk_rcc_spi_clk_enable(BDK_SPI_2);
}

void ili9341_init(ili9341_t *lcd)
{
    panel_reset(lcd);
    chip_config(lcd);
    ili9341_set_orientation(lcd, ILI9341_LCD_ORIENTATION);
}

void ili9341_set_window(ili9341_t *lcd, uint16_t x0, uint16_t x1,
                        uint16_t y0, uint16_t y1)
{
    uint8_t params[4];

    params[0] = (uint8_t)(x0 >> 8);
    params[1] = (uint8_t)(x0 & 0xFFU);
    params[2] = (uint8_t)(x1 >> 8);
    params[3] = (uint8_t)(x1 & 0xFFU);
    bus_write_command(lcd, ILI9341_CASET);
    bus_write_data(lcd, params, 4U);

    params[0] = (uint8_t)(y0 >> 8);
    params[1] = (uint8_t)(y0 & 0xFFU);
    params[2] = (uint8_t)(y1 >> 8);
    params[3] = (uint8_t)(y1 & 0xFFU);
    bus_write_command(lcd, ILI9341_RASET);
    bus_write_data(lcd, params, 4U);
}

#if (ILI9341_PIXEL_SPI_16BIT != 0U)
static void line_rgb565_fill(uint16_t *line, uint16_t w, uint16_t rgb565)
{
    for (uint16_t i = 0U; i < w; i++) {
        line[i] = rgb565;
    }
}

#else
/** Same byte order as Cube `ili9341_test_draw_color_bars` (0xF8, 0x00 for red). */
static void line_rgb565_be(uint8_t *line, uint16_t w, uint16_t rgb565)
{
    const uint8_t hi = (uint8_t)(rgb565 >> 8);
    const uint8_t lo = (uint8_t)(rgb565 & 0xFFU);

    for (uint16_t i = 0U; i < w; i++) {
        line[(size_t)i * 2U]      = hi;
        line[(size_t)i * 2U + 1U] = lo;
    }
}

#endif

void ili9341_fill_rgb565(ili9341_t *lcd, uint16_t color)
{
    uint16_t w;
    uint16_t h;

    draw_panel_size(lcd->orientation, &w, &h);
#if (ILI9341_PIXEL_SPI_16BIT != 0U)
    static uint16_t line[ILI9341_LCD_LINE_PIXELS];

    line_rgb565_fill(line, w, color);
#else
    static uint8_t line[ILI9341_LCD_LINE_PIXELS * 2U];

    line_rgb565_be(line, w, color);
#endif
    ili9341_set_window(lcd, 0U, (uint16_t)(w - 1U), 0U, (uint16_t)(h - 1U));
    bus_write_command(lcd, ILI9341_GRAM);
    bus_pixels_frame_begin(lcd);
    for (uint16_t row = 0U; row < h; row++) {
#if (ILI9341_PIXEL_SPI_16BIT != 0U)
        bus_pixels_frame_push(lcd, line, (uint32_t)w);
#else
        bus_pixels_frame_push(lcd, line, (uint32_t)w * 2U);
#endif
    }
    bus_pixels_frame_end(lcd);
}

void ili9341_draw_color_bars(ili9341_t *lcd)
{
    uint16_t w;
    uint16_t h;

    apply_madctl(lcd, lcd->orientation);
    draw_panel_size(lcd->orientation, &w, &h);
#if (ILI9341_PIXEL_SPI_16BIT != 0U)
    static uint16_t line[ILI9341_LCD_LINE_PIXELS];
#else
    static uint8_t line[ILI9341_LCD_LINE_PIXELS * 2U];
#endif

    ili9341_set_window(lcd, 0U, (uint16_t)(w - 1U), 0U, (uint16_t)(h - 1U));
    bus_write_command(lcd, ILI9341_GRAM);
    bus_pixels_frame_begin(lcd);

    /*
     * Constant color per scanline (split on y). Portrait MADCTL: horizontal bands on
     * glass. Landscape MADCTL (MV): same RAM pattern appears as vertical columns.
     * Do not split on x here — that still looks like horizontal bands on this panel.
     */
    {
        const uint16_t y_split_a = h / 3U;
        const uint16_t y_split_b = (h * 2U) / 3U;

        for (uint16_t y = 0U; y < h; y++) {
            uint16_t color;

            if (y < y_split_a) {
                color = 0xF800U;
            } else if (y < y_split_b) {
                color = 0x07E0U;
            } else {
                color = 0x001FU;
            }

#if (ILI9341_PIXEL_SPI_16BIT != 0U)
            line_rgb565_fill(line, w, color);
            bus_pixels_frame_push(lcd, line, (uint32_t)w);
#else
            line_rgb565_be(line, w, color);
            bus_pixels_frame_push(lcd, line, (uint32_t)w * 2U);
#endif
        }
    }

    bus_pixels_frame_end(lcd);
}
