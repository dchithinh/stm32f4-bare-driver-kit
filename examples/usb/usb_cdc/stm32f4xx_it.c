#include "stm32f4xx.h"

#include "bdk_cdc.h"

void OTG_FS_IRQHandler(void)
{
    bdk_cdc_irq_handler(BDK_USB_OTG_FS);
}
