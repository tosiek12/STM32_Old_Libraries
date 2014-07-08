//
// USART.C
//

#include <stdbool.h>

#include "stm32f4xx_conf.h"
#include "stm32f4_discovery.h"
#include "usart.h"

void ust_cpy(LINE_CODING* plc2, const LINE_CODING* plc1) {
	plc2->bitrate = plc1->bitrate;
	plc2->format = plc1->format;
	plc2->paritytype = plc1->paritytype;
	plc2->datatype = plc1->datatype;
}

//Configure the usart based on the LINE_CODING structure settings
//Note - hw flow control on USART 1-3 and 6 only
bool ust_config(USART_TypeDef* pUsart, LINE_CODING* lc) {
	//RSW - HACK, need to disable USART while doing this?

	/* DISCOVERY_COM default configuration */
	/* DISCOVERY_COM configured as follow:
	 - BaudRate = 115200 baud
	 - Word Length = 8 Bits
	 - One Stop Bit
	 - Parity Odd
	 - Hardware flow control disabled
	 - Receive and transmit enabled
	 USART_InitStructure.USART_BaudRate = 115200;
	 USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	 USART_InitStructure.USART_StopBits = USART_StopBits_1;
	 USART_InitStructure.USART_Parity = USART_Parity_Odd;
	 USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	 USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	 */

	//RSW - Sets bit rate, number of stop bits, parity, and number of data bits
	USART_InitTypeDef USART_InitStructure;

	//Set parity
	switch (lc->paritytype) {
	case 0:
		USART_InitStructure.USART_Parity = USART_Parity_No;
		break;
	case 1:
		USART_InitStructure.USART_Parity = USART_Parity_Even;
		break;
	case 2:
		USART_InitStructure.USART_Parity = USART_Parity_Odd;
		break;
	case 3:    //MARK, not supported
	case 4:    //SPACE, not supported
	default:
		return (false);
	}

	//Set word length (STM includes possible parity bit in data bit count, CDC spec doesn't)
	switch (lc->datatype) {
	case 0x07:
		/* With this configuration a parity (Even or Odd) should be set */
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		break;
	case 0x08:
		if (USART_InitStructure.USART_Parity == USART_Parity_No ) {
			USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		} else {
			USART_InitStructure.USART_WordLength = USART_WordLength_9b;
		}
		break;
	case 0x05:      //Not supported
	case 0x06:      //Not supported
	case 0x16:     //Not supported
	default:
		return (false);
	}

	//Set stop bits
	switch (lc->format) {
	case 0:
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		break;
	case 1:
		USART_InitStructure.USART_StopBits = USART_StopBits_1_5;
		break;
	case 2:
		USART_InitStructure.USART_StopBits = USART_StopBits_2;
		break;
	default:
		return (false);
	}

	//Set baudrate
	USART_InitStructure.USART_BaudRate = lc->bitrate;
	//Set up the rest
	USART_InitStructure.USART_HardwareFlowControl =
			USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(pUsart, &USART_InitStructure);
	USART_ITConfig(pUsart, USART_IT_RXNE, ENABLE);
	USART_Cmd(pUsart, ENABLE);
	return true;
}

