#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "stm32f4xx_conf.h"
#include "stm32f4xx.h"
#include "main.h"

#include "stm32f4_discovery.h"

#include "arm_math.h"	//For float32_t type
#include "Delay/delay.h"	//For delay_ms() and SysClock initialization
#include "GPIO/gpio.h"	//For initialize Buttons and leds
#include "EXTI/exti.h"	//For configuring buttons as EXTI
#include "USB/usb.h"	//For initialize
#include "ADC/adc.h"	//For ADC3ConvertedValue, initialize
#include "Timer/timer.h"	//For timer initialization
#include "ADC/Goertzel.h"
#include "HD44780/hd44780.h"
#include "Accelerometer/accelerometer.h"
#include "NokiaLCD/nokiaLCD.h"

/* Private variables ---------------------------------------------------------*/
volatile uint8_t ServoDirection = 19;
volatile uint8_t acc_initialized = 0;
/* Private function prototypes -----------------------------------------------*/
int main(void) {
#if (__FPU_USED == 1)//run FPU coprocessor
	SCB ->CPACR |= ((3UL << 10 * 2) | /* set CP10 Full Access */
	(3UL << 11 * 2)); /* set CP11 Full Access */
#endif


	//char buf[50] = "";
	extern volatile uint32_t micSamplesReady;	//From timer2ISR (adc.c)
	extern volatile uint8_t DataSendRequest;//Set in VCP_DataRx (usbd_cdc_vcp.c)
	extern volatile uint16_t *pBuffer_done;

//	ADC3_CH12_DMA_Config();
	SysTick_Conf();
	InitButtons();

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);	//Bylo potrzebne, by dzialaly przyciski (na przerwaniach)
	InitButtonsEXTI();
	InitLeds();
	USB_init();
	InitTimerWithIRQ();
//	LCD_Initialize();
//	LCD_CreateCustomCharsForLevels();
//	LCD5110_WriteTextXY("Prezentacja",0,0);
//	LCD5110_WriteTextXY(" Freescale 2014",0,1);
	InitPWM();	//For Servo
	const uint16_t DOWN_VALUE = 5;	//6 stabilne
	const uint16_t UP_VALUE = 27;	//
	const uint16_t STOP_VALUE = 19;	//
	__IO uint32_t value = DOWN_VALUE;


	LCD5110_Initialize();
	LCD5110_Clear();
	LCD5110_WriteBMP();
	delay_ms(500);
	LCD5110_Clear();

	InitAccelerometer();
	acc_initialized = 1;
	ADC_SoftwareStartConv(ADC3 ); // Start ADC3 Software Conversion

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOC, ENABLE);
	enum state tempButtons[3] = { noPush, noPush, noPush };
	enum state oldButtons[3] = { noPush, noPush, noPush };
	volatile uint8_t doGoertzel = 0;

	while (1) {
		tempButtons[0] = buttons[0];	//Copy buttons states for current loop.
		tempButtons[1] = buttons[1];
		tempButtons[2] = buttons[2];

		delay_ms(50);
		Main_AccelerometerAction();

		if (micSamplesReady == 1) {

			if(doGoertzel == 1) {
				LCD_Clear();
				algorithm();
				micSamplesReady = 0;
				GPIO_ToggleBits(LED5_GPIO_PORT, LED5_PIN );
			}

			if (DataSendRequest == 1) {
				VCP_DataTx((uint8_t *) pBuffer_done, MIC_ADC_BUFFER_SIZE * 2);
				VCP_DataTx((uint8_t *) " ", 1);
				DataSendRequest = 0;
				micSamplesReady = 0;
			}
		}
		if (tempButtons[0] == shortPush && oldButtons[0] != shortPush) {
//			LCD5110_WriteTextXY("S",0,1);
			doGoertzel = 0;
			LCD_Clear();
			LCD5110_WriteTextXY("Stroik gitarowy",0,0);
			TIM4 ->CCR2 = STOP_VALUE;	//zatrzymaj servo
			GPIO_ToggleBits(LED6_GPIO_PORT, LED6_PIN );
		} else if (tempButtons[0] == longPush && oldButtons[0] != longPush) {
//			LCD5110_WriteTextXY("L",0,1);
			doGoertzel = 1;
			LCD5110_WriteTextXY("Start strojenia.",0,0);
			LCD5110_WriteTextXY("Uderzaj w strune!",0,1);
			delay_ms(3000);
			GPIO_ResetBits(LED6_GPIO_PORT, LED6_PIN | LED5_PIN );
		}

		if (tempButtons[1] == shortPush && oldButtons[1] != shortPush) {
//			LCD5110_WriteTextXY("S",1,1);
			VCP_DataTx((uint8_t*) "SHORT",5);
			TIM4 ->CCR2 = STOP_VALUE;
			LCD5110_WriteTextXY("          ",0,1);
			LCD5110_WriteTextXY("Servo Stop",0,1);
		} else if (tempButtons[1] == mediumPush && oldButtons[1] != mediumPush) {
//			LCD5110_WriteTextXY("M",1,1);
			VCP_DataTx((uint8_t*) "MEDIUM",6);
		} else if (tempButtons[1] == longPush && oldButtons[1] != longPush) {
//			LCD5110_WriteTextXY("L",1,1);
			VCP_DataTx((uint8_t*) "LONG",4);
		}

//		if (tempButtons[2] == shortPush && oldButtons[2] != shortPush) {
////			LCD5110_WriteTextXY("S",2,1);
//			GPIO_ToggleBits(LED6_GPIO_PORT, LED6_PIN );
//		} else if (tempButtons[2] == longPush && oldButtons[2] != longPush) {
////			LCD5110_WriteTextXY("L",2,1);
//			GPIO_ResetBits(LED6_GPIO_PORT, LED6_PIN | LED5_PIN );
//		}


		oldButtons[0] = tempButtons[0];	//Copy old buttons states.
		oldButtons[1] = tempButtons[1];
		oldButtons[2] = tempButtons[2];
	}
	return 0;
}
