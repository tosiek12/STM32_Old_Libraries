#include "stm32f4_discovery.h"
#include "USB.h"

#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"



__ALIGN_BEGIN USB_OTG_CORE_HANDLE USB_OTG_dev __ALIGN_END;

//Configure pins and clocks
void USB_init() {
	GPIO_InitTypeDef GPIO_InitStructure;
	// ---------- GPIO -------- //
	// GPIOD Periph clock enable,
	// Need to enable GPIOA because that's where the UART pins are.
	// (Some of the USB is also on that port, and usb modules turn it on later...
	//  but anyway, UART started working correctly when I turned clock on first)
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	// ------------- USART -------------- //
	RCC_APB1PeriphClockCmd(DISCOVERY_COM_CLK, ENABLE); //USART1+6=APB2, 2-5=APB1

	/* Configure USART Tx+Rx as alternate function  */
	GPIO_InitStructure.GPIO_Pin = DISCOVERY_COM_TX_PIN | DISCOVERY_COM_RX_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);    //Both signals are on port GPIOA

	/* Connect PXx to USARTx_Tx* + Rx*/
	GPIO_PinAFConfig(DISCOVERY_COM_TX_GPIO_PORT, DISCOVERY_COM_TX_SOURCE,
			DISCOVERY_COM_TX_AF );
	GPIO_PinAFConfig(DISCOVERY_COM_RX_GPIO_PORT, DISCOVERY_COM_RX_SOURCE,
			DISCOVERY_COM_RX_AF );

	// ------------- USB -------------- //
#ifdef USE_USB_OTG_HS
	USBD_Init(&USB_OTG_dev,USB_OTG_HS_CORE_ID,&USR_desc, &USBD_CDC_cb, &USR_cb);
#else
	USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb,
			&USR_cb);
#endif
}
