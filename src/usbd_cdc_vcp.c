/**
 ******************************************************************************
 * @file    usbd_cdc_vcp.c
 * @author  MCD Application Team
 * @version V1.0.0
 * @date    22-July-2011
 * @brief   Generic media access Layer.
 ******************************************************************************
 * @attention
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
 * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
 ******************************************************************************
 */

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED 
#pragma     data_alignment = 4 
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_vcp.h"
#include "stm32f4xx_conf.h"
#include "stm32f4_discovery.h"
#include "usart.h"

LINE_CODING g_lc =
{
  115200, /* baud rate*/
  0x00,   /* stop bits-1*/
  0x00,   /* parity - none*/
  0x08    /* nb. of bits 8*/
};

/* These are external variables imported from CDC core to be used for IN 
 transfer management. */
extern uint8_t APP_Rx_Buffer[]; /* Write CDC received data in this buffer.
 These data will be sent over USB IN endpoint
 in the CDC core functions. */
extern uint32_t APP_Rx_ptr_in; /* Increment this pointer or roll it back to
 start address when writing received data
 in the buffer APP_Rx_Buffer. */

/* Private function prototypes -----------------------------------------------*/
static uint16_t VCP_Init(void);
static uint16_t VCP_DeInit(void);
static uint16_t VCP_Ctrl(uint32_t Cmd, uint8_t* Buf, uint32_t Len);
static uint16_t VCP_DataRx(uint8_t* Buf, uint32_t Len);

CDC_IF_Prop_TypeDef VCP_fops = { VCP_Init, VCP_DeInit, VCP_Ctrl, VCP_DataTx,
		VCP_DataRx };

/* Private functions ---------------------------------------------------------*/
/**
 * @brief  VCP_Init
 *         Initializes the Media on the STM32
 * @param  None
 * @retval Result of the opeartion (USBD_OK in all cases)
 */
static uint16_t VCP_Init(void) {
	NVIC_InitTypeDef NVIC_InitStructure;

	//RSW HACK, don't set up the port here, because we don't know what the OS wants for speed
	//settings yet...  do this in VCP_Ctrl instead..

	/* Enable USART Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = DISCOVERY_COM_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	return USBD_OK;
}

/**
 * @brief  VCP_DeInit
 *         DeInitializes the Media on the STM32
 * @param  None
 * @retval Result of the opeartion (USBD_OK in all cases)
 */
static uint16_t VCP_DeInit(void) {
	USART_ITConfig(DISCOVERY_COM, USART_IT_RXNE, DISABLE);
	USART_Cmd(DISCOVERY_COM, DISABLE);
	return USBD_OK;
}

/**
 * @brief  VCP_Ctrl
 *         Manage the CDC class requests
 * @param  Cmd: Command code
 * @param  Buf: Buffer containing command data (request parameters)
 * @param  Len: Number of data to be sent (in bytes)
 * @retval Result of the opeartion (USBD_OK in all cases)
 */
static uint16_t VCP_Ctrl(uint32_t Cmd, uint8_t* Buf, uint32_t Len) {
	LINE_CODING* plc = (LINE_CODING*) Buf;

	assert_param(Len>=sizeof(LINE_CODING));

	switch (Cmd) {
	/* Not  needed for this driver, AT modem commands */
	case SEND_ENCAPSULATED_COMMAND:
		/* Not  needed for this driver */
		break;

	case GET_ENCAPSULATED_RESPONSE:
		/* Not  needed for this driver */
		break;

	case SET_COMM_FEATURE:
		/* Not  needed for this driver */
		break;

	case GET_COMM_FEATURE:
		/* Not  needed for this driver */
		break;

	case CLEAR_COMM_FEATURE:
		/* Not  needed for this driver */
		break;

		//Note - hw flow control on UART 1-3 and 6 only
	case SET_LINE_CODING:
		if (!ust_config(DISCOVERY_COM, plc))
			return USBD_FAIL;

		ust_cpy(&g_lc, plc);           //Copy into structure to save for later
		break;

	case GET_LINE_CODING:
		ust_cpy(plc, &g_lc);
		break;

	case SET_CONTROL_LINE_STATE:
		/* Not  needed for this driver */
		//RSW - This tells how to set RTS and DTR
		break;

	case SEND_BREAK:
		/* Not  needed for this driver */
		break;

	default:
		break;
	}

	return USBD_OK;
}

//- OUT transfers (from Host to Device):
//    When a packet is received from the host on the OUT pipe (EP3), the Endpoint
//    callback function is called when the transfer is complete and all the received
//    data are then sent through USART peripheral in polling mode. This allows all
//    incoming OUT packets to be NAKed till the current packet is completely transferred
//    through the USART interface.
//
//- IN transfers (from Device to Host):
//    For IN data, a large circular buffer is used. USART and USB respectively write
//    and read to/from this buffer independently.
//    USART RXNE interrupt is used to write data into the buffer. This interrupt
//    has the highest priority, which allows to avoid overrun and data loss conditions.
//    USB IN endpoint (EP1) is written with the received data into the SOF interrupt
//    callback.
//    Into this callback, a frame counter counts the number of passed frames since
//    the last time IN endpoint was written. This allows to write IN endpoint at
//    separated frames when enough data are present in the data buffer.
//    To modify the number of frame between IN write operations, modify the value
//    of the define "VCOMPORT_IN_FRAME_INTERVAL" in "usb_endp.c" file.
//    To allow high data rate performance, SOF interrupt is managed with highest
//    priority (thus SOF interrupt is checked before all other interrupts).

/**
 * @brief  VCP_DataTx
 *         CDC received data to be send over USB IN endpoint are managed in
 *         this function.
 * @param  Buf: Buffer of data to be sent
 * @param  Len: Number of data to be sent (in bytes)
 * @retval Result of the opeartion: USBD_OK if all operations are OK else VCP_FAIL
 */
uint16_t VCP_DataTx(uint8_t* Buf, uint32_t Len) {
	uint32_t i = 0;

	GPIO_ToggleBits(LED3_GPIO_PORT, LED3_PIN );

	//If no buffer, we're supposed to receive USART, send USB
	//(add to the circular buffer)
	if (Buf == NULL) {
		if (g_lc.datatype == 7)
			APP_Rx_Buffer[APP_Rx_ptr_in] = USART_ReceiveData(DISCOVERY_COM )
					& 0x7F;
		else if (g_lc.datatype == 8)
			APP_Rx_Buffer[APP_Rx_ptr_in] = USART_ReceiveData(DISCOVERY_COM );

		APP_Rx_ptr_in++;

		/* To avoid buffer overflow */
		if (APP_Rx_ptr_in == APP_RX_DATA_SIZE)
			APP_Rx_ptr_in = 0;
	} else {      //If we were passed a buffer, transmit that
		while (i < Len) {
			APP_Rx_Buffer[APP_Rx_ptr_in] = *(Buf + i);
			APP_Rx_ptr_in++;
			i++;

			/* To avoid buffer overflow */
			if (APP_Rx_ptr_in == APP_RX_DATA_SIZE)
				APP_Rx_ptr_in = 0;
		}
	}

	return USBD_OK;
}

/**
 * @brief  VCP_DataRx
 *         Data received over USB OUT endpoint are sent over CDC interface
 *         through this function.
 *
 *         @note
 *         This function will block any OUT packet reception on USB endpoint
 *         until exiting this function. If you exit this function before transfer
 *         is complete on CDC interface (ie. using DMA controller) it will result
 *         in receiving more data while previous ones are still not sent.
 *
 * @param  Buf: Buffer of data to be received
 * @param  Len: Number of data received (in bytes)
 * @retval Result of the opeartion: USBD_OK if all operations are OK else VCP_FAIL
 */
static uint16_t VCP_DataRx(uint8_t* Buf, uint32_t Len) {
	//RSW - Receive USB, send USART
	//Receive via USB, do action on uC.
	while (Len-- > 0) {
		extern volatile uint8_t ServoDirection;
		if (*Buf == 'a' || *Buf == 'A') {
			LED6_GPIO_PORT ->ODR |= LED6_PIN;
		} else if (*Buf == 's') {
			LED6_GPIO_PORT ->ODR &= ~(LED6_PIN );
		} else if (*Buf == 'd') {
			extern volatile uint8_t DataSendRequest;
			DataSendRequest = 1;
		} else if (*Buf == 'W') {
			ServoDirection = 18;
		} else if (*Buf == 'N') {
			ServoDirection = 20;
		} else if (*Buf == 'S') {
			ServoDirection = 19;
		}

		//Send further via USART
		USART_SendData(DISCOVERY_COM, *Buf);
		while (USART_GetFlagStatus(DISCOVERY_COM, USART_FLAG_TXE ) == RESET);
		Buf++;
	}
	GPIO_ToggleBits(LED4_GPIO_PORT, LED4_PIN );
	return USBD_OK;
}
