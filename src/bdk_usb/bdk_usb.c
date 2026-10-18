#include "bdk_usb.h"
#include "bdk_usb_ll.h"

#include "bdk_rcc.h"
#include "bdk_util.h"

#include "stm32f4xx.h"

/*
 * Register offsets from CMSIS (stm32f407xx.h), base = USB_OTG_FS:
 *   USB_OTG_GLOBAL_BASE, USB_OTG_DEVICE_BASE, USB_OTG_IN/OUT_ENDPOINT_BASE,
 *   USB_OTG_EP_REG_SIZE, USB_OTG_PCGCCTL_BASE, USB_OTG_FIFO_BASE/SIZE.
 *
 * Clock: 48 MHz on OTG_FS from RCC PLLQ (app configures PLL before bdk_usb_init).
 * Pins:  PA11/PA12 DM/DP via GPIO AF OTG_FS (app configures before init).
 */

#define BDK_USB_RESET_LOOPS       100000U
#define BDK_USB_MODE_WAIT_LOOPS   20000U
#define BDK_USB_OTG_SPEED_FULL    3U

#define BDK_USB_OTG_REG(usb, offset) \
    ((uintptr_t)(usb) + USB_OTG_GLOBAL_BASE + (offset))

#define BDK_USB_PCGCCTL(usb) \
    (*(__IO uint32_t *)BDK_USB_OTG_REG(usb, USB_OTG_PCGCCTL_BASE))

/*
 * Packet SRAM is shared: first GRXFSIZ words = RX FIFO; IN TX FIFOs follow in RAM.
 * Sizes are in 32-bit words (4 bytes each).
 */
/* FS OTG packet RAM is 1.25 KiB (320 x 32-bit words). Match ST HAL FS VCP layout. */
#define BDK_USB_FS_RX_FIFO_WORDS   128U
#define BDK_USB_FS_EP0_TX_WORDS    64U
#define BDK_USB_FS_EP1_TX_WORDS    128U
#define BDK_USB_FS_DEV_EPS         4U
#define BDK_USB_EP0_MPS            64U
#define BDK_USB_PKTSTS_SETUP_UPDT  6U
#define BDK_USB_PKTSTS_SETUP_COMP  4U
#define BDK_USB_PKTSTS_OUT_COMP    3U
#define BDK_USB_PKTSTS_DATA_UPDT   2U

#define BDK_USB_EP0_STAGE_IDLE       0U
#define BDK_USB_EP0_STAGE_DATA_IN    1U
#define BDK_USB_EP0_STAGE_STATUS_OUT 2U
#define BDK_USB_EP0_STAGE_DATA_OUT   3U
#define BDK_USB_EP0_STAGE_STATUS_IN  4U
static bdk_usb_events_t usb_events;
static bdk_usb_ep0_stack_t usb_ep0_stack;

static uint8_t usb_setup_pkt[8];
static uint8_t usb_ep0_rx[BDK_USB_EP0_MPS];
static volatile uint8_t usb_setup_pending;
static uint8_t usb_ep0_stage;
static const uint8_t *usb_ep0_in_buf;
static uint16_t       usb_ep0_in_rem;
static uint8_t       *usb_ep0_out_buf;
static uint8_t       *usb_ep0_out_app;
static uint16_t       usb_ep0_out_len;
static uint16_t       usb_ep0_out_pos;
static uint16_t       usb_ep0_out_arm_xfer;
static uint16_t       usb_ep0_in_last_chunk;
static const uint8_t *usb_ep0_tx_ptr;
static uint16_t       usb_ep0_tx_remaining;

static bdk_usb_irq_stats_t usb_irq_stats;
static uint8_t             usb_in_busy[BDK_USB_FS_DEV_EPS];
static const uint8_t      *usb_in_ptr[BDK_USB_FS_DEV_EPS];
static uint16_t            usb_in_len[BDK_USB_FS_DEV_EPS];


static USB_OTG_INEndpointTypeDef *usb_in_ep(USB_OTG_GlobalTypeDef *usb, uint32_t num)
{
    return (USB_OTG_INEndpointTypeDef *)(BDK_USB_OTG_REG(usb, USB_OTG_IN_ENDPOINT_BASE) +
                                         (num * USB_OTG_EP_REG_SIZE));
}

static USB_OTG_OUTEndpointTypeDef *usb_out_ep(USB_OTG_GlobalTypeDef *usb, uint32_t num)
{
    return (USB_OTG_OUTEndpointTypeDef *)(BDK_USB_OTG_REG(usb, USB_OTG_OUT_ENDPOINT_BASE) +
                                          (num * USB_OTG_EP_REG_SIZE));
}

static void usb_device_ep0_prepare(USB_OTG_GlobalTypeDef *usb);
static void usb_ep0_activate(USB_OTG_GlobalTypeDef *usb);
static void usb_ep0_out_start(USB_OTG_GlobalTypeDef *usb);
static void usb_ep0_out_arm_status(USB_OTG_GlobalTypeDef *usb);
static void usb_ep0_clear_stall(USB_OTG_GlobalTypeDef *usb);
static void usb_ep0_try_dispatch_setup(void);
static void usb_ep0_run_pending(void);
static void usb_ep0_in_submit_chunk(USB_OTG_GlobalTypeDef *usb);
static void usb_ep0_in_on_xfrc(USB_OTG_GlobalTypeDef *usb);
static void usb_ep0_out_on_xfrc(USB_OTG_GlobalTypeDef *usb);
static void usb_ep0_out_data_complete(USB_OTG_GlobalTypeDef *usb);
static void usb_ep0_arm_control_out(USB_OTG_GlobalTypeDef *usb, uint16_t len);
static void usb_ep0_abort_status_in(USB_OTG_GlobalTypeDef *usb);
static void usb_ep0_status_complete(USB_OTG_GlobalTypeDef *usb);
static void usb_ep0_return_idle(USB_OTG_GlobalTypeDef *usb);
static bdk_status_t usb_flush_tx_fifo(USB_OTG_GlobalTypeDef *usb, uint32_t fifo_num);
static void usb_flush_rx_fifo(USB_OTG_GlobalTypeDef *usb);
static bdk_status_t usb_set_device_mode(USB_OTG_GlobalTypeDef *usb);
static void usb_set_turnaround_time(USB_OTG_GlobalTypeDef *usb, uint32_t hclk_hz);

static USB_OTG_GlobalTypeDef *usb_global(bdk_usb_id_t id)
{
    if (id != BDK_USB_OTG_FS) {
        return NULL;
    }
    return USB_OTG_FS;
}

static USB_OTG_DeviceTypeDef *usb_device(USB_OTG_GlobalTypeDef *usb)
{
    return (USB_OTG_DeviceTypeDef *)BDK_USB_OTG_REG(usb, USB_OTG_DEVICE_BASE);
}

static bdk_status_t usb_wait_grstctl(USB_OTG_GlobalTypeDef *usb, uint32_t mask, int want_set)
{
    uint32_t i;

    for (i = 0; i < BDK_USB_RESET_LOOPS; i++) {
        if (want_set) {
            if (READ_BIT(usb->GRSTCTL, mask) != 0U) {
                return BDK_OK;
            }
        } else if (READ_BIT(usb->GRSTCTL, mask) == 0U) {
            return BDK_OK;
        }
    }
    return BDK_ERR_TIMEOUT;
}

static bdk_status_t usb_core_reset(USB_OTG_GlobalTypeDef *usb)
{
    bdk_status_t st;

    /* GRSTCTL AHBIDL: wait until AHB master is idle before reset. */
    st = usb_wait_grstctl(usb, USB_OTG_GRSTCTL_AHBIDL, 1);
    if (st != BDK_OK) {
        return st;
    }

    /* GRSTCTL CSRST: core soft reset — clears most OTG logic; PLL 48 MHz must be running. */
    SET_BIT(usb->GRSTCTL, USB_OTG_GRSTCTL_CSRST);
    st = usb_wait_grstctl(usb, USB_OTG_GRSTCTL_CSRST, 0);
    if (st != BDK_OK) {
        return st;
    }

    /* GRSTCTL RXFFLSH: flush receive FIFO after reset. */
    SET_BIT(usb->GRSTCTL, USB_OTG_GRSTCTL_RXFFLSH);
    st = usb_wait_grstctl(usb, USB_OTG_GRSTCTL_RXFFLSH, 0);
    return st;
}

static void usb_flush_rx_fifo(USB_OTG_GlobalTypeDef *usb)
{
    SET_BIT(usb->GRSTCTL, USB_OTG_GRSTCTL_RXFFLSH);
    (void)usb_wait_grstctl(usb, USB_OTG_GRSTCTL_RXFFLSH, 0);
}

static bdk_status_t usb_set_device_mode(USB_OTG_GlobalTypeDef *usb)
{
    uint32_t i;

    MODIFY_REG(usb->GUSBCFG,
               USB_OTG_GUSBCFG_FHMOD | USB_OTG_GUSBCFG_FDMOD,
               USB_OTG_GUSBCFG_FDMOD);

    for (i = 0U; i < BDK_USB_MODE_WAIT_LOOPS; i++) {
        if (READ_BIT(usb->GINTSTS, 1U) == 0U) {
            return BDK_OK;
        }
    }
    return BDK_ERR_TIMEOUT;
}

static void usb_set_turnaround_time(USB_OTG_GlobalTypeDef *usb, uint32_t hclk_hz)
{
    uint32_t trdt = 6U;

    if (hclk_hz < 32000000U) {
        trdt = 5U;
    }
    MODIFY_REG(usb->GUSBCFG,
               USB_OTG_GUSBCFG_TRDT,
               (trdt << 10) & USB_OTG_GUSBCFG_TRDT);
}

static void usb_fifo_config(USB_OTG_GlobalTypeDef *usb)
{
    /*
     * GRXFSIZ: depth of shared RX FIFO (words). All OUT traffic + setup packets land here.
     */
    usb->GRXFSIZ = BDK_USB_FS_RX_FIFO_WORDS;

    /*
     * DIEPTXF0_HNPTXFSIZ: EP0 IN / non-periodic TX FIFO.
     *   [15:0]  IN endpoint 0 TX FIFO start address in packet RAM (= end of RX region)
     *   [31:16] IN endpoint 0 TX FIFO depth (words)
     */
    usb->DIEPTXF0_HNPTXFSIZ =
        (BDK_USB_FS_EP0_TX_WORDS << 16) | BDK_USB_FS_RX_FIFO_WORDS;

    /* DIEPTXF[0] = TX FIFO 1 (bulk IN). Same as Cube CDC: Rx 128, TX0 64, TX1 128. */
    usb->DIEPTXF[0] = (BDK_USB_FS_EP1_TX_WORDS << 16) |
                      (BDK_USB_FS_RX_FIFO_WORDS + BDK_USB_FS_EP0_TX_WORDS);
}

static bdk_status_t usb_device_mode_init(USB_OTG_GlobalTypeDef *usb,
                                         const bdk_usb_config_t *config)
{
    USB_OTG_DeviceTypeDef *dev = usb_device(usb);
    uint32_t gccfg;
    uint32_t i;

    if (usb_set_device_mode(usb) != BDK_OK) {
        return BDK_ERR_TIMEOUT;
    }

    /*
     * GCCFG PWRDWN = 1: activate embedded FS transceiver (RM0090 §35.15.1).
     * NOVBUSSENS: treat VBUS as valid when PA9 sense is unused (Discovery CN5).
     */
    gccfg = usb->GCCFG;
    SET_BIT(gccfg, USB_OTG_GCCFG_PWRDWN);
    if (config != NULL && config->ignore_vbus_sense != 0U) {
        SET_BIT(gccfg, USB_OTG_GCCFG_NOVBUSSENS);
        CLEAR_BIT(gccfg, USB_OTG_GCCFG_VBUSBSEN | USB_OTG_GCCFG_VBUSASEN);
    } else {
        CLEAR_BIT(gccfg, USB_OTG_GCCFG_NOVBUSSENS);
        SET_BIT(gccfg, USB_OTG_GCCFG_VBUSBSEN);
        CLEAR_BIT(gccfg, USB_OTG_GCCFG_VBUSASEN);
    }
    usb->GCCFG = gccfg;

    BDK_USB_PCGCCTL(usb) = 0U;

    for (i = 0U; i < 15U; i++) {
        usb->DIEPTXF[i] = 0U;
    }

    MODIFY_REG(dev->DCFG, USB_OTG_DCFG_DSPD, BDK_USB_OTG_SPEED_FULL);

    if (usb_flush_tx_fifo(usb, 0x10U) != BDK_OK) {
        return BDK_ERR_TIMEOUT;
    }
    usb_flush_rx_fifo(usb);

    dev->DIEPMSK  = 0U;
    dev->DOEPMSK  = 0U;
    dev->DAINTMSK = 0U;

    usb_fifo_config(usb);
    (void)usb_flush_tx_fifo(usb, 0x10U);

    SET_BIT(dev->DCTL, USB_OTG_DCTL_SDIS);

    usb->GINTMSK = 0U;
    usb->GINTSTS = 0xBFFFFFFFU;
    usb->GINTMSK = USB_OTG_GINTMSK_USBRST | USB_OTG_GINTMSK_ENUMDNEM |
                   USB_OTG_GINTMSK_RXFLVLM | USB_OTG_GINTMSK_OEPINT |
                   USB_OTG_GINTMSK_IEPINT | USB_OTG_GINTMSK_USBSUSPM |
                   USB_OTG_GINTMSK_WUIM;

    SET_BIT(usb->GAHBCFG, USB_OTG_GAHBCFG_GINT | USB_OTG_GAHBCFG_TXFELVL);

    usb_device_ep0_prepare(usb);

    return BDK_OK;
}

static volatile uint32_t *usb_dfifo(USB_OTG_GlobalTypeDef *usb, uint32_t epnum)
{
    return (volatile uint32_t *)(BDK_USB_OTG_REG(usb, USB_OTG_FIFO_BASE) +
                                 (epnum * USB_OTG_FIFO_SIZE));
}

static void usb_read_fifo(USB_OTG_GlobalTypeDef *usb, uint8_t *dest, uint16_t len)
{
    volatile uint32_t *fifo = usb_dfifo(usb, 0U);
    uint32_t i;
    uint32_t words = (uint32_t)len / 4U;
    uint16_t rem   = (uint16_t)(len % 4U);

    for (i = 0U; i < words; i++) {
        uint32_t w = fifo[0];
        if (dest != NULL) {
            dest[0] = (uint8_t)(w);
            dest[1] = (uint8_t)(w >> 8);
            dest[2] = (uint8_t)(w >> 16);
            dest[3] = (uint8_t)(w >> 24);
            dest += 4;
        }
    }
    if (rem != 0U) {
        uint32_t w = fifo[0];
        for (i = 0U; i < rem; i++) {
            if (dest != NULL) {
                dest[i] = (uint8_t)(w >> (8U * i));
            }
        }
    }
}

static void usb_write_fifo(USB_OTG_GlobalTypeDef *usb, uint32_t epnum,
                           const uint8_t *src, uint16_t len)
{
    volatile uint32_t *fifo = usb_dfifo(usb, epnum);
    uint32_t           i;
    uint32_t           words = ((uint32_t)len + 3U) / 4U;
    uint32_t           pos   = 0U;

    for (i = 0U; i < words; i++) {
        uint32_t w = 0U;
        uint32_t j;

        for (j = 0U; j < 4U && pos < (uint32_t)len; j++) {
            w |= (uint32_t)src[pos++] << (8U * j);
        }
        *fifo = w;
    }
}

static void usb_read_fifo_discard(USB_OTG_GlobalTypeDef *usb, uint32_t byte_count)
{
    usb_read_fifo(usb, NULL, (uint16_t)byte_count);
}

static void usb_in_fill_tx_fifo(USB_OTG_GlobalTypeDef *usb, uint32_t epnum)
{
    USB_OTG_DeviceTypeDef *dev = usb_device(usb);
    uint16_t len;

    len = usb_in_len[epnum];
    if (len == 0U || usb_in_ptr[epnum] == NULL) {
        CLEAR_BIT(dev->DIEPEMPMSK, 1U << epnum);
        return;
    }

    /* Same as EP0: push the packet; do not skip on DTXFSTS (that left EP1 NAK'ing). */
    usb_write_fifo(usb, epnum, usb_in_ptr[epnum], len);
    usb_in_len[epnum] = 0U;
    usb_in_ptr[epnum] = NULL;
    CLEAR_BIT(dev->DIEPEMPMSK, 1U << epnum);
}

static void usb_ep0_fill_tx_fifo(USB_OTG_GlobalTypeDef *usb)
{
    USB_OTG_DeviceTypeDef *dev = usb_device(usb);
    uint16_t chunk;

    if (usb_ep0_tx_remaining == 0U || usb_ep0_tx_ptr == NULL) {
        CLEAR_BIT(dev->DIEPEMPMSK, 1U << 0);
        return;
    }

    chunk = usb_ep0_tx_remaining;
    if (chunk > BDK_USB_EP0_MPS) {
        chunk = BDK_USB_EP0_MPS;
    }
    usb_write_fifo(usb, 0U, usb_ep0_tx_ptr, chunk);
    usb_ep0_tx_ptr += chunk;
    usb_ep0_tx_remaining -= chunk;
    CLEAR_BIT(dev->DIEPEMPMSK, 1U << 0);
}

static void usb_ep0_in_submit_chunk(USB_OTG_GlobalTypeDef *usb)
{
    USB_OTG_INEndpointTypeDef *in0 = usb_in_ep(usb, 0U);
    uint16_t chunk = usb_ep0_in_rem;

    if (chunk > BDK_USB_EP0_MPS) {
        chunk = BDK_USB_EP0_MPS;
    }
    usb_ep0_in_last_chunk = chunk;

    usb_ep0_tx_ptr       = usb_ep0_in_buf;
    usb_ep0_tx_remaining = chunk;

    in0->DIEPTSIZ = 0U;
    CLEAR_BIT(in0->DIEPTSIZ, USB_OTG_DIEPTSIZ_PKTCNT);
    CLEAR_BIT(in0->DIEPTSIZ, USB_OTG_DIEPTSIZ_XFRSIZ);
    SET_BIT(in0->DIEPTSIZ, USB_OTG_DIEPTSIZ_PKTCNT & (1U << 19));
    if (chunk > 0U) {
        SET_BIT(in0->DIEPTSIZ, USB_OTG_DIEPTSIZ_XFRSIZ & (uint32_t)chunk);
    }

    CLEAR_BIT(in0->DIEPCTL, USB_OTG_DIEPCTL_STALL);
    SET_BIT(in0->DIEPCTL, USB_OTG_DIEPCTL_CNAK | USB_OTG_DIEPCTL_EPENA);

    if (chunk > 0U && usb_ep0_in_buf != NULL) {
        usb_ep0_fill_tx_fifo(usb);
    }
}

static void usb_ep0_in_start(USB_OTG_GlobalTypeDef *usb, const uint8_t *data, uint16_t len)
{
    usb_ep0_in_buf = data;
    usb_ep0_in_rem = len;
    usb_ep0_stage  = (len > 0U) ? BDK_USB_EP0_STAGE_DATA_IN : BDK_USB_EP0_STAGE_STATUS_IN;
    usb_irq_stats.ep0_in_start++;
    usb_ep0_in_submit_chunk(usb);
}

static void usb_ep0_enable_out_data(USB_OTG_GlobalTypeDef *usb)
{
    USB_OTG_OUTEndpointTypeDef *out0 = usb_out_ep(usb, 0U);

    /* HAL USB_EPStartXfer EP0 OUT: keep STUPCNT, XFRSIZ = MPS, PKTCNT = 1. */
    CLEAR_BIT(out0->DOEPCTL, USB_OTG_DOEPCTL_STALL);
    out0->DOEPTSIZ &= ~(USB_OTG_DOEPTSIZ_XFRSIZ | USB_OTG_DOEPTSIZ_PKTCNT);
    out0->DOEPTSIZ |= (1U << 19) | (BDK_USB_EP0_MPS & USB_OTG_DOEPTSIZ_XFRSIZ);
    SET_BIT(out0->DOEPCTL, USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA);
}

static void usb_ep0_arm_control_out(USB_OTG_GlobalTypeDef *usb, uint16_t len)
{
    if (len == 0U || len > BDK_USB_EP0_MPS) {
        return;
    }
    usb_ep0_out_buf      = usb_ep0_rx;
    usb_ep0_out_len      = len;
    usb_ep0_out_pos      = 0U;
    usb_ep0_out_arm_xfer = BDK_USB_EP0_MPS;
    usb_ep0_stage        = BDK_USB_EP0_STAGE_DATA_OUT;
    usb_ep0_enable_out_data(usb);
}

static void usb_ep0_out_data_complete(USB_OTG_GlobalTypeDef *usb)
{
    uint16_t i;
    uint16_t n;

    if (usb_ep0_stage != BDK_USB_EP0_STAGE_DATA_OUT) {
        return;
    }
    if (usb_ep0_out_app != NULL && usb_ep0_out_pos > 0U) {
        n = usb_ep0_out_pos;
        if (n > usb_ep0_out_len) {
            n = usb_ep0_out_len;
        }
        for (i = 0U; i < n; i++) {
            usb_ep0_out_app[i] = usb_ep0_rx[i];
        }
    }
    usb_ep0_out_buf      = NULL;
    usb_ep0_out_app      = NULL;
    usb_ep0_out_len      = 0U;
    usb_ep0_out_pos      = 0U;
    usb_ep0_out_arm_xfer = 0U;
    usb_ep0_in_start(usb, NULL, 0U);
}

static void usb_ep0_abort_status_in(USB_OTG_GlobalTypeDef *usb)
{
    USB_OTG_INEndpointTypeDef *in0 = usb_in_ep(usb, 0U);

    if (usb_ep0_stage != BDK_USB_EP0_STAGE_STATUS_IN) {
        return;
    }
    usb_ep0_in_buf = NULL;
    usb_ep0_in_rem = 0U;
    CLEAR_BIT(in0->DIEPCTL, USB_OTG_DIEPCTL_EPENA);
    usb_ep0_stage = BDK_USB_EP0_STAGE_IDLE;
}

static void usb_ep0_out_on_xfrc(USB_OTG_GlobalTypeDef *usb)
{
    if (usb_ep0_stage != BDK_USB_EP0_STAGE_DATA_OUT) {
        return;
    }
    /* SETUP also raises OUT XFRC. Only finish when the class payload is in. */
    if (usb_ep0_out_pos >= usb_ep0_out_len) {
        usb_ep0_out_data_complete(usb);
    }
}

static void usb_ep0_in_on_xfrc(USB_OTG_GlobalTypeDef *usb)
{
    if (usb_ep0_stage == BDK_USB_EP0_STAGE_DATA_IN) {
        usb_ep0_in_buf += usb_ep0_in_last_chunk;
        usb_ep0_in_rem -= usb_ep0_in_last_chunk;
        if (usb_ep0_in_rem > 0U) {
            usb_irq_stats.ep0_in_cont++;
            usb_ep0_in_submit_chunk(usb);
            return;
        }
        usb_ep0_out_arm_status(usb);
        return;
    }
    if (usb_ep0_stage == BDK_USB_EP0_STAGE_STATUS_IN) {
        usb_ep0_return_idle(usb);
    }
}

static void usb_ep0_out_arm_status(USB_OTG_GlobalTypeDef *usb)
{
    USB_OTG_OUTEndpointTypeDef *out0 = usb_out_ep(usb, 0U);
    USB_OTG_INEndpointTypeDef *in0   = usb_in_ep(usb, 0U);

    /* USBD_LL_DataInStage: StallEP(0x80) then CtlReceiveStatus (see usbd_core.c). */
    SET_BIT(in0->DIEPCTL, USB_OTG_DIEPCTL_STALL);
    /* HAL_PCD_EP_SetStall(0x80) also calls USB_EP0_OutStart before status OUT. */
    usb_ep0_out_start(usb);

    out0->DOEPTSIZ = 0U;
    out0->DOEPTSIZ = (1U << 19) | (BDK_USB_EP0_MPS & USB_OTG_DOEPTSIZ_XFRSIZ);
    SET_BIT(out0->DOEPCTL, USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA);
    usb_ep0_stage = BDK_USB_EP0_STAGE_STATUS_OUT;
    usb_irq_stats.ep0_status_arm++;
}

static void usb_ep0_return_idle(USB_OTG_GlobalTypeDef *usb)
{
    usb_ep0_stage = BDK_USB_EP0_STAGE_IDLE;
    usb_ep0_out_start(usb);
    usb_ep0_try_dispatch_setup();
}

static void usb_ep0_clear_stall(USB_OTG_GlobalTypeDef *usb)
{
    USB_OTG_OUTEndpointTypeDef *out0 = usb_out_ep(usb, 0U);
    USB_OTG_INEndpointTypeDef *in0   = usb_in_ep(usb, 0U);

    CLEAR_BIT(in0->DIEPCTL, USB_OTG_DIEPCTL_STALL);
    CLEAR_BIT(out0->DOEPCTL, USB_OTG_DOEPCTL_STALL);
}

static void usb_ep0_status_complete(USB_OTG_GlobalTypeDef *usb)
{
    if (usb_ep0_stage != BDK_USB_EP0_STAGE_STATUS_OUT) {
        return;
    }
    usb_irq_stats.ep0_status_done++;
    usb_ep0_clear_stall(usb);
    usb_ep0_return_idle(usb);
}

static void usb_rx_fifo_service(USB_OTG_GlobalTypeDef *usb)
{
    uint32_t st;
    uint32_t bcnt;
    uint32_t pktsts;

    /* HAL_PCD_IRQHandler: one GRXSTSP pop per RXFLVL so STUP can run before DATA OUT. */
    if (READ_BIT(usb->GINTSTS, USB_OTG_GINTSTS_RXFLVL) == 0U) {
        return;
    }

    st     = usb->GRXSTSP;
    bcnt   = (st & USB_OTG_GRXSTSP_BCNT) >> USB_OTG_GRXSTSP_BCNT_Pos;
    pktsts = (st & USB_OTG_GRXSTSP_PKTSTS) >> USB_OTG_GRXSTSP_PKTSTS_Pos;

        if (usb_ep0_stack.on_setup != NULL && pktsts == BDK_USB_PKTSTS_SETUP_UPDT) {
            /* HAL: USB_ReadPacket(Setup, 8) on STS_SETUP_UPDT (see HAL_PCD_IRQHandler). */
            usb_read_fifo(usb, usb_setup_pkt, 8U);
            usb_ep0_abort_status_in(usb);
            usb_setup_pending = 1U;
            usb_ep0_out_app   = NULL;
            usb_irq_stats.ep0_setup_updt++;
            /* Arm control-OUT data (SET_LINE_CODING) before the host DATA packet. */
            if ((usb_setup_pkt[0] & 0x80U) == 0U) {
                uint16_t wlen = (uint16_t)(usb_setup_pkt[6] | ((uint16_t)usb_setup_pkt[7] << 8));
                if (wlen > 0U) {
                    usb_ep0_arm_control_out(usb, wlen);
                }
            }
        } else if (pktsts == BDK_USB_PKTSTS_SETUP_COMP) {
            USB_OTG_OUTEndpointTypeDef *out0 = usb_out_ep(usb, 0U);

            if (bcnt > 0U) {
                usb_read_fifo_discard(usb, bcnt);
            }
            /* HAL SetupStage is after setup complete; then CtlPrepareRx. */
            usb_ep0_try_dispatch_setup();
            if (usb_ep0_stage == BDK_USB_EP0_STAGE_DATA_OUT && usb_ep0_out_pos == 0U) {
                /* SETUP complete drops EPENA; rewrite TSIZ so the 7-byte DATA lands. */
                usb_ep0_enable_out_data(usb);
            } else {
                SET_BIT(out0->DOEPCTL, USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA);
            }
        } else if (pktsts == BDK_USB_PKTSTS_OUT_COMP) {
            if (bcnt > 0U) {
                usb_read_fifo_discard(usb, bcnt);
            }
        } else if (pktsts == BDK_USB_PKTSTS_DATA_UPDT) {
            uint32_t epnum = st & USB_OTG_GRXSTSP_EPNUM;
            if (epnum == 0U && usb_ep0_stage == BDK_USB_EP0_STAGE_STATUS_OUT) {
                if (bcnt > 0U) {
                    usb_read_fifo_discard(usb, bcnt);
                }
                usb_ep0_status_complete(usb);
            } else if (epnum == 0U && usb_ep0_stage == BDK_USB_EP0_STAGE_DATA_OUT && bcnt > 0U) {
                uint16_t room = (uint16_t)(usb_ep0_out_len - usb_ep0_out_pos);
                uint16_t copy = (bcnt > room) ? room : (uint16_t)bcnt;

                if (usb_ep0_out_buf != NULL && copy > 0U) {
                    usb_read_fifo(usb, usb_ep0_out_buf + usb_ep0_out_pos, copy);
                    usb_ep0_out_pos += copy;
                } else {
                    usb_read_fifo_discard(usb, bcnt);
                }
                if (usb_ep0_out_pos >= usb_ep0_out_len) {
                    usb_ep0_out_data_complete(usb);
                }
            } else if (bcnt > 0U) {
                usb_read_fifo_discard(usb, bcnt);
            }
        } else if (bcnt > 0U) {
            usb_read_fifo_discard(usb, bcnt);
            usb_irq_stats.fifo_bytes_discarded += bcnt;
        }
}

static void usb_oepint_service(USB_OTG_GlobalTypeDef *usb)
{
    USB_OTG_DeviceTypeDef *dev = usb_device(usb);
    uint32_t ep_intr =
        (dev->DAINT & dev->DAINTMSK & 0xFFFF0000U) >> 16U;
    uint32_t epnum;

    for (epnum = 0U; epnum < BDK_USB_FS_DEV_EPS; epnum++) {
        if ((ep_intr & (1UL << epnum)) == 0U) {
            continue;
        }
        USB_OTG_OUTEndpointTypeDef *out = usb_out_ep(usb, epnum);
        uint32_t raw_epint              = out->DOEPINT;
        uint32_t epint                  = raw_epint & dev->DOEPMSK;

        if ((epint & USB_OTG_DOEPINT_NAK) != 0U) {
            SET_BIT(out->DOEPINT, USB_OTG_DOEPINT_NAK);
        }
        if ((epint & USB_OTG_DOEPINT_XFRC) != 0U) {
            SET_BIT(out->DOEPINT, USB_OTG_DOEPINT_XFRC);
            if (epnum == 0U) {
                if (usb_ep0_stage == BDK_USB_EP0_STAGE_DATA_OUT) {
                    usb_ep0_out_on_xfrc(usb);
                } else if (usb_ep0_stage == BDK_USB_EP0_STAGE_STATUS_OUT) {
                    usb_ep0_status_complete(usb);
                }
            }
        }
        if ((epint & USB_OTG_DOEPINT_OTEPSPR) != 0U) {
            SET_BIT(out->DOEPINT, USB_OTG_DOEPINT_OTEPSPR);
            if (epnum == 0U && usb_ep0_stage == BDK_USB_EP0_STAGE_STATUS_OUT) {
                usb_ep0_status_complete(usb);
            }
        }
        if ((raw_epint & USB_OTG_DOEPINT_STPKTRX) != 0U) {
            SET_BIT(out->DOEPINT, USB_OTG_DOEPINT_STPKTRX);
            if (epnum == 0U) {
                usb_irq_stats.ep0_stpktrx++;
            }
        }
        if ((epint & USB_OTG_DOEPINT_STUP) != 0U) {
            SET_BIT(out->DOEPINT, USB_OTG_DOEPINT_STUP);
            if (epnum == 0U) {
                usb_irq_stats.ep0_stup++;
                usb_ep0_try_dispatch_setup();
            }
        }
        if ((epint & USB_OTG_DOEPINT_OTEPDIS) != 0U) {
            out->DOEPINT = USB_OTG_DOEPINT_OTEPDIS;
        }
        if ((epint & USB_OTG_DOEPINT_EPDISD) != 0U) {
            out->DOEPINT = USB_OTG_DOEPINT_EPDISD;
        }
    }
}

static void usb_iepint_service(USB_OTG_GlobalTypeDef *usb)
{
    USB_OTG_DeviceTypeDef *dev = usb_device(usb);
    uint32_t ep_intr           = dev->DAINT & dev->DAINTMSK & 0xFFFFU;
    uint32_t epnum;

    for (epnum = 0U; epnum < BDK_USB_FS_DEV_EPS; epnum++) {
        if ((ep_intr & (1UL << epnum)) == 0U) {
            continue;
        }
        USB_OTG_INEndpointTypeDef *in = usb_in_ep(usb, epnum);
        /* TXFE is unmasked per-EP via DIEPEMPMSK, not DIEPMSK (HAL USB_ReadDevInEPInterrupt). */
        uint32_t epint                = in->DIEPINT & dev->DIEPMSK;

        if ((dev->DIEPEMPMSK & (1U << epnum)) != 0U) {
            epint |= in->DIEPINT & USB_OTG_DIEPINT_TXFE;
        }

        if ((epint & USB_OTG_DIEPINT_XFRC) != 0U) {
            in->DIEPINT = USB_OTG_DIEPINT_XFRC;
            if (epnum == 0U) {
                usb_irq_stats.ep0_in_xfrc++;
                CLEAR_BIT(dev->DIEPEMPMSK, 1U << 0);
                usb_ep0_tx_remaining = 0U;
                usb_ep0_in_on_xfrc(usb);
            } else {
                usb_in_busy[epnum] = 0U;
                usb_in_ptr[epnum]  = NULL;
                usb_in_len[epnum]  = 0U;
                CLEAR_BIT(dev->DIEPEMPMSK, 1U << epnum);
            }
        }
        if ((epint & USB_OTG_DIEPINT_TXFE) != 0U) {
            in->DIEPINT = USB_OTG_DIEPINT_TXFE;
            if (epnum == 0U) {
                usb_ep0_fill_tx_fifo(usb);
            } else {
                usb_in_fill_tx_fifo(usb, epnum);
            }
        }
        /* DIEPMSK includes EPDM/TOM: must W1C or IEPINT never drops (IRQ spin, UART dies). */
        if ((epint & USB_OTG_DIEPINT_EPDISD) != 0U) {
            in->DIEPINT = USB_OTG_DIEPINT_EPDISD;
            if (epnum != 0U) {
                usb_in_busy[epnum] = 0U;
            }
        }
        if ((epint & USB_OTG_DIEPINT_TOC) != 0U) {
            in->DIEPINT = USB_OTG_DIEPINT_TOC;
        }
        if ((epint & USB_OTG_DIEPINT_ITTXFE) != 0U) {
            in->DIEPINT = USB_OTG_DIEPINT_ITTXFE;
        }
        if ((epint & USB_OTG_DIEPINT_INEPNE) != 0U) {
            in->DIEPINT = USB_OTG_DIEPINT_INEPNE;
        }
    }
}

static bdk_status_t usb_flush_tx_fifo(USB_OTG_GlobalTypeDef *usb, uint32_t fifo_num)
{
    if (usb_wait_grstctl(usb, USB_OTG_GRSTCTL_AHBIDL, 1) != BDK_OK) {
        return BDK_ERR_TIMEOUT;
    }
    usb->GRSTCTL = USB_OTG_GRSTCTL_TXFFLSH | (fifo_num << 6);
    return usb_wait_grstctl(usb, USB_OTG_GRSTCTL_TXFFLSH, 0);
}

static void usb_ep0_out_start(USB_OTG_GlobalTypeDef *usb)
{
    USB_OTG_OUTEndpointTypeDef *out0 = usb_out_ep(usb, 0U);

    out0->DOEPTSIZ = 0U;
    SET_BIT(out0->DOEPTSIZ, USB_OTG_DOEPTSIZ_PKTCNT & (1U << 19));
    SET_BIT(out0->DOEPTSIZ, (3U * 8U));
    SET_BIT(out0->DOEPTSIZ, USB_OTG_DOEPTSIZ_STUPCNT);
}

static void usb_ep0_activate(USB_OTG_GlobalTypeDef *usb)
{
    USB_OTG_DeviceTypeDef *dev = usb_device(usb);
    USB_OTG_INEndpointTypeDef *in0  = usb_in_ep(usb, 0U);
    USB_OTG_OUTEndpointTypeDef *out0 = usb_out_ep(usb, 0U);

    /* USB_ActivateEndpoint() for control EP0 IN/OUT (usbd_core opens EP0 on reset). */
    SET_BIT(dev->DAINTMSK, USB_OTG_DAINTMSK_IEPM & (1UL << 0));
    SET_BIT(dev->DAINTMSK, (USB_OTG_DAINTMSK_OEPM & (1UL << 0)) << 16);

    MODIFY_REG(in0->DIEPCTL,
               USB_OTG_DIEPCTL_MPSIZ | USB_OTG_DIEPCTL_EPTYP | USB_OTG_DIEPCTL_TXFNUM,
               (BDK_USB_EP0_MPS & USB_OTG_DIEPCTL_MPSIZ) | USB_OTG_DIEPCTL_SD0PID_SEVNFRM |
                   USB_OTG_DIEPCTL_USBAEP);

    MODIFY_REG(out0->DOEPCTL,
               USB_OTG_DOEPCTL_MPSIZ | USB_OTG_DOEPCTL_EPTYP,
               (BDK_USB_EP0_MPS & USB_OTG_DOEPCTL_MPSIZ) | USB_OTG_DOEPCTL_SD0PID_SEVNFRM |
                   USB_OTG_DOEPCTL_USBAEP);
}

static void usb_device_ep0_prepare(USB_OTG_GlobalTypeDef *usb)
{
    USB_OTG_DeviceTypeDef *dev = usb_device(usb);

    usb_ep0_activate(usb);

    dev->DOEPMSK = USB_OTG_DOEPMSK_STUPM | USB_OTG_DOEPMSK_XFRCM | USB_OTG_DOEPMSK_EPDM |
                   USB_OTG_DOEPMSK_OTEPSPRM | USB_OTG_DOEPMSK_NAKM;
    dev->DIEPMSK = USB_OTG_DIEPMSK_TOM | USB_OTG_DIEPMSK_XFRCM | USB_OTG_DIEPMSK_EPDM;

    usb_ep0_out_start(usb);
}

static void usb_device_on_bus_reset(USB_OTG_GlobalTypeDef *usb)
{
    USB_OTG_DeviceTypeDef *dev = usb_device(usb);
    uint32_t i;

    CLEAR_BIT(dev->DCTL, USB_OTG_DCTL_RWUSIG);
    usb_fifo_config(usb);
    usb_flush_tx_fifo(usb, 0x10U);

    for (i = 0U; i < BDK_USB_FS_DEV_EPS; i++) {
        USB_OTG_INEndpointTypeDef *in  = usb_in_ep(usb, i);
        USB_OTG_OUTEndpointTypeDef *out = usb_out_ep(usb, i);

        /* HAL USB_DevInit: EP0 must not EPDIS; other IN EPs EPDIS only if EPENA. */
        if (READ_BIT(in->DIEPCTL, USB_OTG_DIEPCTL_EPENA) != 0U) {
            if (i == 0U) {
                in->DIEPCTL = USB_OTG_DIEPCTL_SNAK;
            } else {
                in->DIEPCTL = USB_OTG_DIEPCTL_EPDIS | USB_OTG_DIEPCTL_SNAK;
            }
        } else {
            in->DIEPCTL = 0U;
        }
        in->DIEPTSIZ = 0U;
        in->DIEPINT  = 0xFB7FU;
        out->DOEPINT  = 0xFB7FU;
        CLEAR_BIT(out->DOEPCTL, USB_OTG_DOEPCTL_STALL);
        SET_BIT(out->DOEPCTL, USB_OTG_DOEPCTL_SNAK);
    }

    CLEAR_BIT(dev->DCFG, USB_OTG_DCFG_DAD);
    usb_setup_pending = 0U;
    usb_ep0_stage     = 0U;
    usb_ep0_in_rem    = 0U;
    usb_ep0_in_buf    = NULL;
    usb_ep0_out_buf      = NULL;
    usb_ep0_out_len      = 0U;
    usb_ep0_out_pos      = 0U;
    usb_ep0_out_arm_xfer = 0U;
    for (i = 0U; i < BDK_USB_FS_DEV_EPS; i++) {
        usb_in_busy[i] = 0U;
        usb_in_ptr[i]  = NULL;
        usb_in_len[i]  = 0U;
    }
    usb_device_ep0_prepare(usb);
    if (usb_ep0_stack.on_bus_reset != NULL) {
        usb_ep0_stack.on_bus_reset();
    }
}

static void usb_device_on_enum_done(USB_OTG_GlobalTypeDef *usb)
{
    USB_OTG_DeviceTypeDef *dev = usb_device(usb);
    USB_OTG_INEndpointTypeDef *in0 = usb_in_ep(usb, 0U);

    CLEAR_BIT(in0->DIEPCTL, USB_OTG_DIEPCTL_MPSIZ);
    SET_BIT(dev->DCTL, USB_OTG_DCTL_CGINAK);
    usb_set_turnaround_time(usb, bdk_rcc_get_hclk_hz());
}

bdk_status_t bdk_usb_init(bdk_usb_id_t id, const bdk_usb_config_t *config)
{
    USB_OTG_GlobalTypeDef *usb;
    bdk_status_t st;

    usb = usb_global(id);
    if (usb == NULL) {
        return BDK_ERR_RANGE;
    }

    /* RCC AHB2ENR OTGFSEN: clock the OTG FS peripheral (separate from 48 MHz USB PHY clock). */
    st = bdk_rcc_usb_otg_fs_clk_enable();
    if (st != BDK_OK) {
        return st;
    }

    /*
     * Embedded FS PHY: PHYSEL + core reset + transceiver on.
     */
    SET_BIT(usb->GUSBCFG, USB_OTG_GUSBCFG_PHYSEL);
    st = usb_core_reset(usb);
    if (st != BDK_OK) {
        return st;
    }
    SET_BIT(usb->GCCFG, USB_OTG_GCCFG_PWRDWN);

    return usb_device_mode_init(usb, config);
}

void bdk_usb_set_ep0_stack(bdk_usb_id_t id, const bdk_usb_ep0_stack_t *stack)
{
    if (usb_global(id) == NULL) {
        return;
    }
    if (stack == NULL) {
        usb_ep0_stack.on_bus_reset = NULL;
        usb_ep0_stack.on_setup     = NULL;
        return;
    }
    usb_ep0_stack = *stack;
}

void bdk_usb_set_events(bdk_usb_id_t id, const bdk_usb_events_t *events)
{
    if (usb_global(id) == NULL) {
        return;
    }
    if (events == NULL) {
        usb_events.on_bus_reset = NULL;
        usb_events.on_enum_done = NULL;
        return;
    }
    usb_events = *events;
}

bdk_status_t bdk_usb_connect(bdk_usb_id_t id)
{
    USB_OTG_GlobalTypeDef *usb = usb_global(id);
    USB_OTG_DeviceTypeDef *dev;

    if (usb == NULL) {
        return BDK_ERR_RANGE;
    }
    dev = usb_device(usb);
    /* DCTL SDIS = 0: leave soft disconnect — device visible to host (enumeration can start). */
    CLEAR_BIT(dev->DCTL, USB_OTG_DCTL_SDIS);
    return BDK_OK;
}

bdk_status_t bdk_usb_disconnect(bdk_usb_id_t id)
{
    USB_OTG_GlobalTypeDef *usb = usb_global(id);
    USB_OTG_DeviceTypeDef *dev;

    if (usb == NULL) {
        return BDK_ERR_RANGE;
    }
    dev = usb_device(usb);
    /* DCTL SDIS = 1: electrical disconnect from host without powering down the core. */
    SET_BIT(dev->DCTL, USB_OTG_DCTL_SDIS);
    return BDK_OK;
}

bdk_status_t bdk_usb_irq_enable(bdk_usb_id_t id)
{
    if (usb_global(id) == NULL) {
        return BDK_ERR_RANGE;
    }
    /* NVIC OTG_FS_IRQn: CPU delivers interrupts when GAHBCFG GINT and GINTMSK/GINTSTS fire. */
    NVIC_EnableIRQ(OTG_FS_IRQn);
    return BDK_OK;
}

void bdk_usb_irq_stats_get(bdk_usb_id_t id, bdk_usb_irq_stats_t *out)
{
    if (usb_global(id) == NULL || out == NULL) {
        return;
    }
    *out = usb_irq_stats;
}

void bdk_usb_irq_handler(bdk_usb_id_t id)
{
    USB_OTG_GlobalTypeDef *usb = usb_global(id);
    uint32_t gintsts;
    uint32_t loops = 0U;

    if (usb == NULL) {
        return;
    }

    usb_irq_stats.handler_calls++;

    for (;;) {
        gintsts = usb->GINTSTS & usb->GINTMSK;
        if (gintsts == 0U) {
            break;
        }
        loops++;
        if (loops > 64U) {
            /* TXFE/EPDISD leftover must not spin here: UART/main would freeze. */
            break;
        }

        /* Finish EP0 FIFO/endpoint work before USBRST clears transfer state. */
        if (READ_BIT(gintsts, USB_OTG_GINTSTS_RXFLVL) != 0U) {
            usb_irq_stats.rxflvl_batches++;
            CLEAR_BIT(usb->GINTMSK, USB_OTG_GINTMSK_RXFLVLM);
            usb_rx_fifo_service(usb);
            SET_BIT(usb->GINTMSK, USB_OTG_GINTMSK_RXFLVLM);
        }
        if (READ_BIT(gintsts, USB_OTG_GINTSTS_OEPINT) != 0U) {
            usb_oepint_service(usb);
        }
        if (READ_BIT(gintsts, USB_OTG_GINTSTS_IEPINT) != 0U) {
            usb_iepint_service(usb);
        }
        if (READ_BIT(gintsts, USB_OTG_GINTSTS_USBRST) != 0U) {
            usb_irq_stats.usbrst++;
            usb_device_on_bus_reset(usb);
            SET_BIT(usb->GINTSTS, USB_OTG_GINTSTS_USBRST);
            if (usb_events.on_bus_reset != NULL) {
                usb_events.on_bus_reset();
            }
        }
        if (READ_BIT(gintsts, USB_OTG_GINTSTS_ENUMDNE) != 0U) {
            usb_irq_stats.enumdne++;
            usb_device_on_enum_done(usb);
            SET_BIT(usb->GINTSTS, USB_OTG_GINTSTS_ENUMDNE);
            if (usb_events.on_enum_done != NULL) {
                usb_events.on_enum_done();
            }
        }
        if (READ_BIT(gintsts, USB_OTG_GINTSTS_USBSUSP) != 0U) {
            SET_BIT(usb->GINTSTS, USB_OTG_GINTSTS_USBSUSP);
        }
        if (READ_BIT(gintsts, USB_OTG_GINTSTS_WKUINT) != 0U) {
            SET_BIT(usb->GINTSTS, USB_OTG_GINTSTS_WKUINT);
        }
    }
}

static void usb_ep0_try_dispatch_setup(void)
{
    if (usb_setup_pending == 0U || usb_ep0_stack.on_setup == NULL) {
        return;
    }
    if (usb_ep0_stage != BDK_USB_EP0_STAGE_IDLE &&
        usb_ep0_stage != BDK_USB_EP0_STAGE_DATA_OUT) {
        return;
    }
    usb_ep0_run_pending();
}

static void usb_ep0_run_pending(void)
{
    if (usb_setup_pending == 0U || usb_ep0_stack.on_setup == NULL) {
        return;
    }

    usb_setup_pending = 0U;
    usb_irq_stats.ep0_setup_dispatch++;
    usb_ep0_stack.on_setup(usb_setup_pkt);
}

static void usb_ep0_stall(USB_OTG_GlobalTypeDef *usb)
{
    USB_OTG_OUTEndpointTypeDef *out0 = usb_out_ep(usb, 0U);
    USB_OTG_INEndpointTypeDef *in0   = usb_in_ep(usb, 0U);

    SET_BIT(out0->DOEPCTL, USB_OTG_DOEPCTL_STALL);
    SET_BIT(in0->DIEPCTL, USB_OTG_DIEPCTL_STALL);
}

void bdk_usb_ll_ep0_in(bdk_usb_id_t id, const uint8_t *data, uint16_t len)
{
    USB_OTG_GlobalTypeDef *usb = usb_global(id);

    if (usb == NULL) {
        return;
    }
    usb_ep0_in_start(usb, data, len);
}

void bdk_usb_ll_ep0_out(bdk_usb_id_t id, uint8_t *data, uint16_t len)
{
    USB_OTG_GlobalTypeDef *usb = usb_global(id);
    uint16_t               i;
    uint16_t               n;

    if (usb == NULL || data == NULL || len == 0U) {
        return;
    }

    usb_ep0_out_app = data;
    if (len < usb_ep0_out_len || usb_ep0_out_len == 0U) {
        usb_ep0_out_len = len;
    }

    if (usb_ep0_stage == BDK_USB_EP0_STAGE_DATA_OUT) {
        if (usb_ep0_out_pos >= usb_ep0_out_len) {
            usb_ep0_out_data_complete(usb);
        }
        return;
    }

    if (usb_ep0_stage == BDK_USB_EP0_STAGE_STATUS_IN ||
        usb_ep0_stage == BDK_USB_EP0_STAGE_DATA_IN) {
        n = len;
        if (n > BDK_USB_EP0_MPS) {
            n = BDK_USB_EP0_MPS;
        }
        for (i = 0U; i < n; i++) {
            data[i] = usb_ep0_rx[i];
        }
        usb_ep0_out_app = NULL;
        return;
    }

    usb_ep0_arm_control_out(usb, len);
    usb_ep0_out_app = data;
}

void bdk_usb_ll_ep0_stall(bdk_usb_id_t id)
{
    USB_OTG_GlobalTypeDef *usb = usb_global(id);

    if (usb == NULL) {
        return;
    }
    usb_ep0_stall(usb);
}

void bdk_usb_ll_ep0_set_address(bdk_usb_id_t id, uint8_t addr)
{
    USB_OTG_GlobalTypeDef *usb = usb_global(id);

    if (usb == NULL) {
        return;
    }
    MODIFY_REG(usb_device(usb)->DCFG,
               USB_OTG_DCFG_DAD,
               ((uint32_t)addr & 0x7FU) << USB_OTG_DCFG_DAD_Pos);
}

void bdk_usb_ll_ep0_set_address_deferred(bdk_usb_id_t id, uint8_t addr)
{
    bdk_usb_ll_ep0_set_address(id, addr);
}

void bdk_usb_ll_ep0_poll(bdk_usb_id_t id)
{
    USB_OTG_GlobalTypeDef *usb = usb_global(id);
    uint32_t epnum;

    usb_ep0_try_dispatch_setup();
    if (usb == NULL) {
        return;
    }
    for (epnum = 1U; epnum < BDK_USB_FS_DEV_EPS; epnum++) {
        if (usb_in_busy[epnum] != 0U && usb_in_ptr[epnum] != NULL) {
            usb_in_fill_tx_fifo(usb, epnum);
        }
    }
}

static void usb_ep_deactivate(USB_OTG_GlobalTypeDef *usb, uint8_t ep_addr)
{
    uint32_t epnum = (uint32_t)(ep_addr & 0x7FU);
    USB_OTG_DeviceTypeDef *dev = usb_device(usb);

    if ((ep_addr & 0x80U) != 0U) {
        USB_OTG_INEndpointTypeDef *in = usb_in_ep(usb, epnum);

        if (READ_BIT(in->DIEPCTL, USB_OTG_DIEPCTL_EPENA) != 0U) {
            SET_BIT(in->DIEPCTL, USB_OTG_DIEPCTL_SNAK | USB_OTG_DIEPCTL_EPDIS);
        }
        CLEAR_BIT(dev->DAINTMSK, USB_OTG_DAINTMSK_IEPM & (1UL << epnum));
        CLEAR_BIT(in->DIEPCTL,
                  USB_OTG_DIEPCTL_USBAEP | USB_OTG_DIEPCTL_MPSIZ | USB_OTG_DIEPCTL_TXFNUM |
                      USB_OTG_DIEPCTL_SD0PID_SEVNFRM | USB_OTG_DIEPCTL_EPTYP);
    } else {
        USB_OTG_OUTEndpointTypeDef *out = usb_out_ep(usb, epnum);

        if (READ_BIT(out->DOEPCTL, USB_OTG_DOEPCTL_EPENA) != 0U) {
            SET_BIT(out->DOEPCTL, USB_OTG_DOEPCTL_SNAK | USB_OTG_DOEPCTL_EPDIS);
        }
        CLEAR_BIT(dev->DAINTMSK, (USB_OTG_DAINTMSK_OEPM & (1UL << epnum)) << 16);
        CLEAR_BIT(out->DOEPCTL,
                  USB_OTG_DOEPCTL_USBAEP | USB_OTG_DOEPCTL_MPSIZ |
                      USB_OTG_DOEPCTL_SD0PID_SEVNFRM | USB_OTG_DOEPCTL_EPTYP);
    }
}

static bdk_status_t usb_ep_activate(USB_OTG_GlobalTypeDef *usb, uint8_t ep_addr,
                                    bdk_usb_ep_type_t type, uint16_t mps)
{
    uint32_t epnum = (uint32_t)(ep_addr & 0x7FU);
    USB_OTG_DeviceTypeDef *dev = usb_device(usb);
    uint32_t eptyp = (uint32_t)type;

    if (epnum == 0U || epnum >= BDK_USB_FS_DEV_EPS) {
        return BDK_ERR_RANGE;
    }

    if ((ep_addr & 0x80U) != 0U) {
        USB_OTG_INEndpointTypeDef *in = usb_in_ep(usb, epnum);

        SET_BIT(dev->DAINTMSK, USB_OTG_DAINTMSK_IEPM & (1UL << epnum));
        /* USB_ActivateEndpoint: OR config bits only (do not rewrite the whole DIEPCTL). */
        if (READ_BIT(in->DIEPCTL, USB_OTG_DIEPCTL_USBAEP) == 0U) {
            in->DIEPCTL |= (mps & USB_OTG_DIEPCTL_MPSIZ) | (eptyp << 18) | (epnum << 22) |
                           USB_OTG_DIEPCTL_SD0PID_SEVNFRM | USB_OTG_DIEPCTL_USBAEP;
        }
    } else {
        USB_OTG_OUTEndpointTypeDef *out = usb_out_ep(usb, epnum);

        SET_BIT(dev->DAINTMSK, (USB_OTG_DAINTMSK_OEPM & (1UL << epnum)) << 16);
        if (READ_BIT(out->DOEPCTL, USB_OTG_DOEPCTL_USBAEP) == 0U) {
            MODIFY_REG(out->DOEPCTL,
                       USB_OTG_DOEPCTL_MPSIZ | USB_OTG_DOEPCTL_EPTYP,
                       (mps & USB_OTG_DOEPCTL_MPSIZ) | (eptyp << 18) |
                           USB_OTG_DOEPCTL_SD0PID_SEVNFRM | USB_OTG_DOEPCTL_USBAEP);
        }
    }

    return BDK_OK;
}

bdk_status_t bdk_usb_ep_open(bdk_usb_id_t id, uint8_t ep_addr, bdk_usb_ep_type_t type,
                             uint16_t mps)
{
    USB_OTG_GlobalTypeDef *usb = usb_global(id);

    if (usb == NULL) {
        return BDK_ERR_RANGE;
    }
    return usb_ep_activate(usb, ep_addr, type, mps);
}

bdk_status_t bdk_usb_ep_close(bdk_usb_id_t id, uint8_t ep_addr)
{
    USB_OTG_GlobalTypeDef *usb = usb_global(id);

    if (usb == NULL) {
        return BDK_ERR_RANGE;
    }
    if ((ep_addr & 0x7FU) == 0U) {
        return BDK_ERR_RANGE;
    }
    usb_ep_deactivate(usb, ep_addr);
    if ((ep_addr & 0x80U) != 0U) {
        usb_in_busy[ep_addr & 0x7FU] = 0U;
    }
    return BDK_OK;
}

bdk_status_t bdk_usb_ep_in_write(bdk_usb_id_t id, uint8_t ep_addr,
                                 const uint8_t *data, uint16_t len)
{
    USB_OTG_GlobalTypeDef *usb = usb_global(id);
    USB_OTG_INEndpointTypeDef *in;
    uint32_t epnum = (uint32_t)(ep_addr & 0x7FU);

    if (usb == NULL || (ep_addr & 0x80U) == 0U || epnum == 0U ||
        epnum >= BDK_USB_FS_DEV_EPS) {
        return BDK_ERR_RANGE;
    }
    if (len > 0U && data == NULL) {
        return BDK_ERR_NULL;
    }
    if (len > BDK_USB_EP0_MPS) {
        return BDK_ERR_RANGE;
    }

    in = usb_in_ep(usb, epnum);
    if (usb_in_busy[epnum] != 0U) {
        return BDK_ERR_BUSY;
    }
    if (READ_BIT(in->DIEPCTL, USB_OTG_DIEPCTL_EPENA) != 0U) {
        in->DIEPCTL = USB_OTG_DIEPCTL_EPDIS | USB_OTG_DIEPCTL_SNAK;
        return BDK_ERR_BUSY;
    }
    if (READ_BIT(in->DIEPCTL, USB_OTG_DIEPCTL_USBAEP) == 0U) {
        return BDK_ERR_STATE;
    }

    usb_in_busy[epnum] = 1U;
    usb_in_ptr[epnum]  = data;
    usb_in_len[epnum]  = len;

    /* USB_EPStartXfer (slave IN): DIEPTSIZ, CNAK|EPENA, then packet into DFIFO. */
    in->DIEPTSIZ = 0U;
    in->DIEPTSIZ = (USB_OTG_DIEPTSIZ_PKTCNT & (1U << 19)) |
                   (USB_OTG_DIEPTSIZ_XFRSIZ & (uint32_t)len);
    CLEAR_BIT(in->DIEPCTL, USB_OTG_DIEPCTL_STALL);
    SET_BIT(in->DIEPCTL, USB_OTG_DIEPCTL_CNAK | USB_OTG_DIEPCTL_EPENA);
    if (len > 0U) {
        usb_write_fifo(usb, epnum, data, len);
        usb_in_len[epnum] = 0U;
        usb_in_ptr[epnum] = NULL;
    }
    return BDK_OK;
}

bdk_status_t bdk_usb_ep_stall(bdk_usb_id_t id, uint8_t ep_addr)
{
    USB_OTG_GlobalTypeDef *usb = usb_global(id);
    uint32_t epnum = (uint32_t)(ep_addr & 0x7FU);

    if (usb == NULL || epnum == 0U || epnum >= BDK_USB_FS_DEV_EPS) {
        return BDK_ERR_RANGE;
    }
    if ((ep_addr & 0x80U) != 0U) {
        SET_BIT(usb_in_ep(usb, epnum)->DIEPCTL, USB_OTG_DIEPCTL_STALL);
    } else {
        SET_BIT(usb_out_ep(usb, epnum)->DOEPCTL, USB_OTG_DOEPCTL_STALL);
    }
    return BDK_OK;
}

bdk_status_t bdk_usb_ep_clear_stall(bdk_usb_id_t id, uint8_t ep_addr)
{
    USB_OTG_GlobalTypeDef *usb = usb_global(id);
    uint32_t epnum = (uint32_t)(ep_addr & 0x7FU);

    if (usb == NULL || epnum == 0U || epnum >= BDK_USB_FS_DEV_EPS) {
        return BDK_ERR_RANGE;
    }
    if ((ep_addr & 0x80U) != 0U) {
        USB_OTG_INEndpointTypeDef *in = usb_in_ep(usb, epnum);

        CLEAR_BIT(in->DIEPCTL, USB_OTG_DIEPCTL_STALL);
        SET_BIT(in->DIEPCTL, USB_OTG_DIEPCTL_SD0PID_SEVNFRM | USB_OTG_DIEPCTL_CNAK);
        usb_in_busy[epnum] = 0U;
        usb_in_ptr[epnum]  = NULL;
        usb_in_len[epnum]  = 0U;
    } else {
        USB_OTG_OUTEndpointTypeDef *out = usb_out_ep(usb, epnum);

        CLEAR_BIT(out->DOEPCTL, USB_OTG_DOEPCTL_STALL);
        SET_BIT(out->DOEPCTL, USB_OTG_DOEPCTL_SD0PID_SEVNFRM | USB_OTG_DOEPCTL_CNAK);
    }
    return BDK_OK;
}

uint8_t bdk_usb_ep_is_stalled(bdk_usb_id_t id, uint8_t ep_addr)
{
    USB_OTG_GlobalTypeDef *usb = usb_global(id);
    uint32_t epnum = (uint32_t)(ep_addr & 0x7FU);

    if (usb == NULL || epnum == 0U || epnum >= BDK_USB_FS_DEV_EPS) {
        return 0U;
    }
    if ((ep_addr & 0x80U) != 0U) {
        return (READ_BIT(usb_in_ep(usb, epnum)->DIEPCTL, USB_OTG_DIEPCTL_STALL) != 0U) ? 1U : 0U;
    }
    return (READ_BIT(usb_out_ep(usb, epnum)->DOEPCTL, USB_OTG_DOEPCTL_STALL) != 0U) ? 1U : 0U;
}
