#ifndef USB_CDC_DEBUG_H
#define USB_CDC_DEBUG_H

#include "bdk_usb.h"

/** USART2 trace: SETUP lines + periodic status (pass log to agent). */
void usb_debug_poll(bdk_usb_id_t id);

#endif /* USB_CDC_DEBUG_H */
