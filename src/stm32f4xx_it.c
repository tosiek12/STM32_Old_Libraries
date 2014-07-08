#include "stm32f4xx_it.h"
#include "stm32f4xx_conf.h"
#include "main.h"
#include "stm32f4_discovery.h"

#include "usb_core.h"
#include "usbd_core.h"
#include "usbd_cdc_core.h"
#include "usbd_cdc_vcp.h"

#include "stm32f4xx.h"

#include "Delay/delay.h"
#include "GPIO/gpio.h"
#include "HD44780/hd44780.h"
#include "Accelerometer/accelerometer.h"
/* Private variables ---------------------------------------------------------*/
extern uint32_t USBD_OTG_ISR_Handler(USB_OTG_CORE_HANDLE *pdev);
extern USB_OTG_CORE_HANDLE USB_OTG_dev;

#ifdef USB_OTG_HS_DEDICATED_EP1_ENABLED
extern uint32_t USBD_OTG_EP1IN_ISR_Handler (USB_OTG_CORE_HANDLE *pdev);
extern uint32_t USBD_OTG_EP1OUT_ISR_Handler (USB_OTG_CORE_HANDLE *pdev);
#endif

/**
 * @brief   This function handles NMI exception.
 * @param  None
 * @retval None
 */
void NMI_Handler(void) {
}

/**
 * @brief  This function handles Hard Fault exception.
 * @param  None
 * @retval None
 */
void HardFault_Handler(void) {
	/* Go to infinite loop when Hard Fault exception occurs */
	while (1) {
	}
}

/**
 * @brief  This function handles Memory Manage exception.
 * @param  None
 * @retval None
 */
void MemManage_Handler(void) {
	/* Go to infinite loop when Memory Manage exception occurs */
	while (1) {
	}
}

/**
 * @brief  This function handles Bus Fault exception.
 * @param  None
 * @retval None
 */
void BusFault_Handler(void) {
	/* Go to infinite loop when Bus Fault exception occurs */
	while (1) {
	}
}

/**
 * @brief  This function handles Usage Fault exception.
 * @param  None
 * @retval None
 */
void UsageFault_Handler(void) {
	/* Go to infinite loop when Usage Fault exception occurs */
	while (1) {
	}
}

/**
 * @brief  This function handles SVCall exception.
 * @param  None
 * @retval None
 */
void SVC_Handler(void) {
}

/**
 * @brief  This function handles Debug Monitor exception.
 * @param  None
 * @retval None
 */
void DebugMon_Handler(void) {
}

/**
 * @brief  This function handles PendSVC exception.
 * @param  None
 * @retval None
 */
void PendSV_Handler(void) {
}

/**
 * @brief  This function handles SysTick Handler.
 * @param  None
 * @retval None
 */
void SysTick_Handler(void) {
	static volatile uint16_t i = 0;
	SysTick_DelayAction();
	++i;
	if(i == 1000) {	//Do it at every 1 ms
		SysTick_ButtonAction();
		if(acc_initialized == 1) {
			SysTick_UpdateAccelerometer();
		}

		i = 1;
	}

}

/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/

void EXTI9_5_IRQHandler(void) {
	if (EXTI_GetITStatus(EXTI_Line5 ) != RESET) {
		EXTI_ClearITPendingBit(EXTI_Line5 );
		if ((GPIOB ->IDR & GPIO_Pin_5 )== (uint32_t) Bit_RESET) {
			buttons[0] = shortPush;
			buttonsTime[0] = LONG_TIME_IN_MS;
		} else {
			buttons[0] = noPush;
//			LCD_WriteTextXY("N",0,1);
			buttonsTime[0] = 0;
		}
	}

	if (EXTI_GetITStatus(EXTI_Line6) != RESET) {
		EXTI_ClearITPendingBit(EXTI_Line6);
		if ((GPIOB ->IDR & GPIO_Pin_6 ) == (uint32_t) Bit_RESET) {
			buttons[1] = shortPush;
			buttonsTime[1] = LONG_TIME_IN_MS;
		} else {
			buttons[1] = noPush;
//			LCD_WriteTextXY("N",1,1);
			buttonsTime[1] = 0;
		}
	}

	if (EXTI_GetITStatus(EXTI_Line7 ) != RESET) {
		EXTI_ClearITPendingBit(EXTI_Line7 );
		if ((GPIOB ->IDR & GPIO_Pin_7 ) == (uint32_t) Bit_RESET) {
			buttons[2] = shortPush;
			buttonsTime[2] = LONG_TIME_IN_MS;
		} else {
			buttons[2] = noPush;
//			LCD_WriteTextXY("N",2,1);
			buttonsTime[2] = 0;
		}
	}
}

/////////////////////////////////USB//////////////////////////////////
#ifdef USE_USB_OTG_FS
void OTG_FS_WKUP_IRQHandler(void) {
	if (USB_OTG_dev.cfg.low_power) {
		*(uint32_t *) (0xE000ED10) &= 0xFFFFFFF9;
		SystemInit();
		USB_OTG_UngateClock(&USB_OTG_dev);
	}
	EXTI_ClearITPendingBit(EXTI_Line18 );
}
#endif

/**
 * @brief  This function handles EXTI15_10_IRQ Handler.
 * @param  None
 * @retval None
 */
#ifdef USE_USB_OTG_HS
void OTG_HS_WKUP_IRQHandler(void)
{
	if(USB_OTG_dev.cfg.low_power)
	{
		*(uint32_t *)(0xE000ED10) &= 0xFFFFFFF9;
		SystemInit();
		USB_OTG_UngateClock(&USB_OTG_dev);
	}
	EXTI_ClearITPendingBit(EXTI_Line20);
}
#endif

/**
 * @brief  This function handles OTG_HS Handler.
 * @param  None
 * @retval None
 */
#ifdef USE_USB_OTG_HS
void OTG_HS_IRQHandler(void)
#else
void OTG_FS_IRQHandler(void)
#endif
{
	USBD_OTG_ISR_Handler(&USB_OTG_dev);
}

#ifdef USB_OTG_HS_DEDICATED_EP1_ENABLED
/**
 * @brief  This function handles EP1_IN Handler.
 * @param  None
 * @retval None
 */
void OTG_HS_EP1_IN_IRQHandler(void)
{
	USBD_OTG_EP1IN_ISR_Handler (&USB_OTG_dev);
}

/**
 * @brief  This function handles EP1_OUT Handler.
 * @param  None
 * @retval None
 */
void OTG_HS_EP1_OUT_IRQHandler(void)
{
	USBD_OTG_EP1OUT_ISR_Handler (&USB_OTG_dev);
}
#endif


//DISCOVERY_COM_TX_PIN //PA.2
//DISCOVERY_COM_RX_PIN // PA.3
void DISCOVERY_COM_IRQHandler(void) {
	GPIO_ToggleBits(LED4_GPIO_PORT, LED4_PIN );
	// Send the received data to the PC Host
	if (USART_GetITStatus(DISCOVERY_COM, USART_IT_RXNE ) != RESET) {
		VCP_DataTx(0, 0);    //Copies RS232 data to USB
	}
	/* If overrun condition occurs, clear the ORE flag and recover communication */
	if (USART_GetFlagStatus(DISCOVERY_COM, USART_FLAG_ORE ) != RESET) {
		(void) USART_ReceiveData(DISCOVERY_COM );
	}

}

