#ifndef BDK_USB_H
#define BDK_USB_H

/**
 * @file bdk_usb.h
 * @brief USB OTG FS device-mode hardware (STM32F4 internal FS PHY).
 *
 * Programs the OTG FS peripheral. Chapter 9 and class drivers use @ref bdk_usbd
 * on top of this layer.
 */

#include <stdint.h>

#include "bdk_status.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB OTG instance (F407 has one FS OTG).
 */
typedef enum {
    BDK_USB_OTG_FS = 0,
} bdk_usb_id_t;

/**
 * @brief Device-mode bring-up options.
 */
typedef struct {
    /**
     * When non-zero, sets @c USB_OTG_GCCFG_NOVBUSSENS: OTG does not use the VBUS
     * sense pin for session valid (Discovery device on PA11/PA12 only).
     */
    uint8_t ignore_vbus_sense;
} bdk_usb_config_t;

/**
 * @brief Optional IRQ callbacks (may be NULL).
 */
typedef struct {
    void (*on_bus_reset)(void);
    void (*on_enum_done)(void);
} bdk_usb_events_t;

/**
 * @brief EP0 stack hooks (registered by @ref bdk_usbd).
 * @note @p on_setup runs from @ref bdk_usb_ll_ep0_poll (main context).
 */
typedef struct {
    void (*on_bus_reset)(void);
    void (*on_setup)(const uint8_t setup[8]);
} bdk_usb_ep0_stack_t;

/**
 * @brief Reset and configure OTG FS for full-speed device mode.
 * @pre App has configured 48 MHz USB clock (PLLQ) via @ref bdk_rcc_clock_init.
 * @pre DM/DP pins are muxed to OTG_FS (e.g. @ref bdk_gpio_init).
 */
bdk_status_t bdk_usb_init(bdk_usb_id_t id, const bdk_usb_config_t *config);

void bdk_usb_set_events(bdk_usb_id_t id, const bdk_usb_events_t *events);

/** @brief Register device stack EP0 handler (@ref bdk_usbd). */
void bdk_usb_set_ep0_stack(bdk_usb_id_t id, const bdk_usb_ep0_stack_t *stack);

bdk_status_t bdk_usb_connect(bdk_usb_id_t id);
bdk_status_t bdk_usb_disconnect(bdk_usb_id_t id);
bdk_status_t bdk_usb_irq_enable(bdk_usb_id_t id);

/** @brief Service OTG FS global interrupt; call from app @c OTG_FS_IRQHandler. */
void bdk_usb_irq_handler(bdk_usb_id_t id);

/** Counters since boot (debug); updated only inside @ref bdk_usb_irq_handler. */
typedef struct {
    uint32_t handler_calls;
    uint32_t usbrst;
    uint32_t enumdne;
    uint32_t rxflvl_batches;
    uint32_t fifo_bytes_discarded;
    uint32_t ep0_stup;
    /** DOEPINT STPKTRX (setup complete on some OTG cores; no DOEPMSK bit). */
    uint32_t ep0_stpktrx;
    /** SETUP dispatched to stack after STUP / STPKTRX / SETUP_COMP. */
    uint32_t ep0_setup_dispatch;
    /** SETUP token payload read from RX FIFO (SETUP_UPDT). */
    uint32_t ep0_setup_updt;
    /** EP0 IN transfers started (@ref bdk_usb_ll_ep0_in). */
    uint32_t ep0_in_start;
    /** EP0 IN continuation packets (descriptor > 64 B). */
    uint32_t ep0_in_cont;
    /** EP0 IN transfer complete (DIEPINT XFRC). */
    uint32_t ep0_in_xfrc;
    /** Status OUT armed after IN data (stall IN + enable EP0 OUT). */
    uint32_t ep0_status_arm;
    /** EP0 OUT status complete (XFRC or OTEPSPR, stage 2). */
    uint32_t ep0_status_done;
} bdk_usb_irq_stats_t;

void bdk_usb_irq_stats_get(bdk_usb_id_t id, bdk_usb_irq_stats_t *out);

/** USB endpoint transfer type (OTG @c EPTYP field). */
typedef enum {
    BDK_USB_EP_TYPE_CONTROL    = 0,
    BDK_USB_EP_TYPE_ISO        = 1,
    BDK_USB_EP_TYPE_BULK       = 2,
    BDK_USB_EP_TYPE_INTERRUPT  = 3,
} bdk_usb_ep_type_t;

/**
 * @brief Activate a non-zero endpoint after @c SET_CONFIGURATION.
 * @pre IN endpoints need a TX FIFO slot configured at init (see @ref bdk_usb_init).
 */
bdk_status_t bdk_usb_ep_open(bdk_usb_id_t id, uint8_t ep_addr,
                             bdk_usb_ep_type_t type, uint16_t mps);

/** @brief Deactivate endpoint @p ep_addr (address bit 0x80 = IN). */
bdk_status_t bdk_usb_ep_close(bdk_usb_id_t id, uint8_t ep_addr);

/**
 * @brief Set STALL on a non-zero endpoint (USB @c SET_FEATURE ENDPOINT_HALT).
 */
bdk_status_t bdk_usb_ep_stall(bdk_usb_id_t id, uint8_t ep_addr);

/**
 * @brief Clear STALL and reset DATA0 on a non-zero endpoint
 *        (USB @c CLEAR_FEATURE ENDPOINT_HALT).
 */
bdk_status_t bdk_usb_ep_clear_stall(bdk_usb_id_t id, uint8_t ep_addr);

/** @brief 1 if the endpoint STALL bit is set. */
uint8_t bdk_usb_ep_is_stalled(bdk_usb_id_t id, uint8_t ep_addr);

/**
 * @brief Start an IN transfer on a non-zero endpoint (one packet, up to MPS).
 * @param data Payload; may be NULL when @p len is 0 (ZLP).
 * @return @c BDK_ERR_BUSY if the endpoint is still transmitting.
 */
bdk_status_t bdk_usb_ep_in_write(bdk_usb_id_t id, uint8_t ep_addr,
                                 const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif /* BDK_USB_H */
