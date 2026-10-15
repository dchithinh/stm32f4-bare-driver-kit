#include "bdk_usb.h"

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
#define BDK_USB_FS_RX_FIFO_WORDS   128U
#define BDK_USB_FS_EP0_TX_WORDS    64U
#define BDK_USB_FS_DEV_EPS         4U
#define BDK_USB_EP0_MPS            64U
#define BDK_USB_PKTSTS_SETUP_UPDT  6U
#define BDK_USB_PKTSTS_SETUP_COMP  4U
#define BDK_USB_PKTSTS_DATA_UPDT   2U
#define BDK_USB_REQ_SET_ADDRESS    5U

static bdk_usb_events_t usb_events;
static bdk_usb_setup_fn usb_setup_fn;

static uint8_t usb_setup_pkt[8];
static volatile uint8_t usb_setup_pending;
static uint8_t usb_ep0_stage;
static uint8_t usb_pending_addr;

static bdk_usb_irq_stats_t usb_irq_stats;


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
static void usb_ep0_out_start(USB_OTG_GlobalTypeDef *usb);
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

    SET_BIT(dev->DCTL, USB_OTG_DCTL_SDIS);

    usb->GINTMSK = 0U;
    usb->GINTSTS = 0xBFFFFFFFU;
    usb->GINTMSK = USB_OTG_GINTMSK_USBRST | USB_OTG_GINTMSK_ENUMDNEM |
                   USB_OTG_GINTMSK_RXFLVLM | USB_OTG_GINTMSK_OEPINT |
                   USB_OTG_GINTMSK_IEPINT | USB_OTG_GINTMSK_USBSUSPM |
                   USB_OTG_GINTMSK_WUIM;

    SET_BIT(usb->GAHBCFG, USB_OTG_GAHBCFG_GINT);

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
    uint32_t i;
    uint32_t words = ((uint32_t)len + 3U) / 4U;
    uint32_t pos   = 0U;

    for (i = 0U; i < words; i++) {
        uint32_t w = 0U;
        uint32_t j;

        for (j = 0U; j < 4U && pos < len; j++) {
            w |= (uint32_t)src[pos++] << (8U * j);
        }
        fifo[0] = w;
    }
}

static void usb_read_fifo_discard(USB_OTG_GlobalTypeDef *usb, uint32_t byte_count)
{
    usb_read_fifo(usb, NULL, (uint16_t)byte_count);
}

static void usb_ep0_in_start(USB_OTG_GlobalTypeDef *usb, const uint8_t *data, uint16_t len)
{
    USB_OTG_INEndpointTypeDef *in0 = usb_in_ep(usb, 0U);

    if (len > BDK_USB_EP0_MPS) {
        len = BDK_USB_EP0_MPS;
    }

    usb_flush_tx_fifo(usb, 0U);
    in0->DIEPTSIZ = 0U;
    if (len > 0U && data != NULL) {
        usb_write_fifo(usb, 0U, data, len);
    }
    in0->DIEPTSIZ = (1U << 19) | (uint32_t)len;
    SET_BIT(in0->DIEPCTL, USB_OTG_DIEPCTL_CNAK | USB_OTG_DIEPCTL_EPENA);
    usb_ep0_stage = (len > 0U) ? 1U : 0U;
}

static void usb_ep0_out_arm_status(USB_OTG_GlobalTypeDef *usb)
{
    USB_OTG_OUTEndpointTypeDef *out0 = usb_out_ep(usb, 0U);

    out0->DOEPTSIZ = 0U;
    out0->DOEPTSIZ = (1U << 19) | (BDK_USB_EP0_MPS & USB_OTG_DOEPTSIZ_XFRSIZ);
    SET_BIT(out0->DOEPCTL, USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA);
    usb_ep0_stage = 2U;
}

static void usb_ep0_apply_pending_addr(USB_OTG_GlobalTypeDef *usb)
{
    USB_OTG_DeviceTypeDef *dev = usb_device(usb);

    if (usb_pending_addr != 0U) {
        MODIFY_REG(dev->DCFG,
                   USB_OTG_DCFG_DAD,
                   (uint32_t)usb_pending_addr << USB_OTG_DCFG_DAD_Pos);
        usb_pending_addr = 0U;
    }
}

static void usb_ep0_on_setup(USB_OTG_GlobalTypeDef *usb)
{
    bdk_usb_setup_t setup;
    const uint8_t *tx     = NULL;
    uint16_t tx_len       = 0U;
    bdk_status_t st;

    if (usb_setup_fn == NULL) {
        return;
    }

    setup.bmRequestType = usb_setup_pkt[0];
    setup.bRequest      = usb_setup_pkt[1];
    setup.wValue        = (uint16_t)(usb_setup_pkt[2] | (usb_setup_pkt[3] << 8));
    setup.wIndex        = (uint16_t)(usb_setup_pkt[4] | (usb_setup_pkt[5] << 8));
    setup.wLength       = (uint16_t)(usb_setup_pkt[6] | (usb_setup_pkt[7] << 8));

    if (setup.bRequest == BDK_USB_REQ_SET_ADDRESS &&
        (setup.bmRequestType & 0x80U) == 0U) {
        usb_pending_addr = (uint8_t)(setup.wValue & 0x7FU);
        usb_ep0_in_start(usb, NULL, 0U);
        if (usb_events.on_ep0_setup != NULL) {
            usb_events.on_ep0_setup();
        }
        return;
    }

    st = usb_setup_fn(&setup, &tx, &tx_len);
    if (st != BDK_OK || (tx_len > 0U && tx == NULL)) {
        USB_OTG_OUTEndpointTypeDef *out0 = usb_out_ep(usb, 0U);
        USB_OTG_INEndpointTypeDef *in0   = usb_in_ep(usb, 0U);

        SET_BIT(out0->DOEPCTL, USB_OTG_DOEPCTL_STALL);
        SET_BIT(in0->DIEPCTL, USB_OTG_DIEPCTL_STALL);
        return;
    }

    if (tx_len > setup.wLength) {
        tx_len = setup.wLength;
    }
    usb_ep0_in_start(usb, tx, tx_len);
    if (usb_events.on_ep0_setup != NULL) {
        usb_events.on_ep0_setup();
    }
}

static void usb_rx_fifo_service(USB_OTG_GlobalTypeDef *usb)
{
    while (READ_BIT(usb->GINTSTS, USB_OTG_GINTSTS_RXFLVL) != 0U) {
        uint32_t st      = usb->GRXSTSP;
        uint32_t bcnt    = (st & USB_OTG_GRXSTSP_BCNT) >> USB_OTG_GRXSTSP_BCNT_Pos;
        uint32_t pktsts  = (st & USB_OTG_GRXSTSP_PKTSTS) >> USB_OTG_GRXSTSP_PKTSTS_Pos;

        if (usb_setup_fn != NULL && bcnt == 8U &&
            (pktsts == BDK_USB_PKTSTS_SETUP_UPDT ||
             pktsts == BDK_USB_PKTSTS_SETUP_COMP)) {
            usb_read_fifo(usb, usb_setup_pkt, 8U);
            usb_setup_pending = 1U;
        } else if (usb_setup_fn != NULL && pktsts == BDK_USB_PKTSTS_DATA_UPDT &&
                   bcnt > 0U) {
            usb_read_fifo_discard(usb, bcnt);
        } else if (bcnt > 0U) {
            usb_read_fifo_discard(usb, bcnt);
            usb_irq_stats.fifo_bytes_discarded += bcnt;
        }
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
        uint32_t epint                  = out->DOEPINT & dev->DOEPMSK;

        if ((epint & USB_OTG_DOEPINT_STUP) != 0U) {
            SET_BIT(out->DOEPINT, USB_OTG_DOEPINT_STUP);
            if (epnum == 0U) {
                usb_irq_stats.ep0_stup++;
                usb_ep0_out_start(usb);
            }
        }
        if ((epint & USB_OTG_DOEPINT_XFRC) != 0U) {
            SET_BIT(out->DOEPINT, USB_OTG_DOEPINT_XFRC);
            if (epnum == 0U && usb_ep0_stage == 2U) {
                usb_ep0_stage = 0U;
                usb_ep0_out_start(usb);
            }
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
        uint32_t epint                = in->DIEPINT & dev->DIEPMSK;

        if ((epint & USB_OTG_DIEPINT_XFRC) != 0U) {
            SET_BIT(in->DIEPINT, USB_OTG_DIEPINT_XFRC);
            if (epnum == 0U) {
                if (usb_ep0_stage == 1U) {
                    usb_ep0_out_arm_status(usb);
                } else if (usb_pending_addr != 0U) {
                    usb_ep0_apply_pending_addr(usb);
                    usb_ep0_out_start(usb);
                }
            }
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

static void usb_device_ep0_prepare(USB_OTG_GlobalTypeDef *usb)
{
    USB_OTG_DeviceTypeDef *dev = usb_device(usb);
    USB_OTG_INEndpointTypeDef *in0  = usb_in_ep(usb, 0U);
    USB_OTG_OUTEndpointTypeDef *out0 = usb_out_ep(usb, 0U);

    if (READ_BIT(out0->DOEPCTL, USB_OTG_DOEPCTL_USBAEP) == 0U) {
        SET_BIT(out0->DOEPCTL,
                (BDK_USB_EP0_MPS & USB_OTG_DOEPCTL_MPSIZ) | USB_OTG_DOEPCTL_USBAEP);
    }
    if (READ_BIT(in0->DIEPCTL, USB_OTG_DIEPCTL_USBAEP) == 0U) {
        SET_BIT(in0->DIEPCTL,
                (BDK_USB_EP0_MPS & USB_OTG_DIEPCTL_MPSIZ) | USB_OTG_DIEPCTL_USBAEP);
    }

    SET_BIT(dev->DAINTMSK, 0x00010001U);
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
    usb_flush_tx_fifo(usb, 0x10U);

    for (i = 0U; i < BDK_USB_FS_DEV_EPS; i++) {
        USB_OTG_INEndpointTypeDef *in  = usb_in_ep(usb, i);
        USB_OTG_OUTEndpointTypeDef *out = usb_out_ep(usb, i);

        in->DIEPINT  = 0xFB7FU;
        CLEAR_BIT(in->DIEPCTL, USB_OTG_DIEPCTL_STALL);
        out->DOEPINT  = 0xFB7FU;
        CLEAR_BIT(out->DOEPCTL, USB_OTG_DOEPCTL_STALL);
        SET_BIT(out->DOEPCTL, USB_OTG_DOEPCTL_SNAK);
    }

    CLEAR_BIT(dev->DCFG, USB_OTG_DCFG_DAD);
    usb_setup_pending = 0U;
    usb_ep0_stage     = 0U;
    usb_pending_addr = 0U;
    usb_device_ep0_prepare(usb);
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

void bdk_usb_set_setup_handler(bdk_usb_id_t id, bdk_usb_setup_fn fn)
{
    if (usb_global(id) == NULL) {
        return;
    }
    usb_setup_fn = fn;
}

void bdk_usb_set_events(bdk_usb_id_t id, const bdk_usb_events_t *events)
{
    if (usb_global(id) == NULL) {
        return;
    }
    if (events == NULL) {
        usb_events.on_bus_reset  = NULL;
        usb_events.on_enum_done  = NULL;
        usb_events.on_ep0_setup  = NULL;
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

    if (usb == NULL) {
        return;
    }

    usb_irq_stats.handler_calls++;

    for (;;) {
        gintsts = usb->GINTSTS & usb->GINTMSK;
        if (gintsts == 0U) {
            break;
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
        if (READ_BIT(gintsts, USB_OTG_GINTSTS_USBSUSP) != 0U) {
            SET_BIT(usb->GINTSTS, USB_OTG_GINTSTS_USBSUSP);
        }
        if (READ_BIT(gintsts, USB_OTG_GINTSTS_WKUINT) != 0U) {
            SET_BIT(usb->GINTSTS, USB_OTG_GINTSTS_WKUINT);
        }
    }
}
