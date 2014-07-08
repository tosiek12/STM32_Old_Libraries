#include "stm32f4_discovery.h"
#include "gpio.h"
//**Buttons GPIO**//
__IO enum state buttons[3] = { noPush, noPush, noPush };
const uint32_t LONG_TIME_IN_MS = 1000;
const uint32_t MEDIUM_TIME_IN_MS = 1000-400;	//wait for 100ms

/*
 * In main function is needed:
 * enum state tempButtons[3] = { noPush, noPush, noPush };
 * and its values must be refreshed at begging of the infinite loop
 * //strncpy((char*) tempButtons, (char*) buttons, 3 * sizeof(enum state)); //dont coppy whole table. ;(
 * tempButtons[0] = buttons[0];
 * tempButtons[1] = buttons[1];
 * tempButtons[2] = buttons[2];
 * To do proper action:
 * if (tempButtons[0] == shortPush) {
		//short push action
	} else if (tempButtons[0] == longPush) {
		//Long push action
	}
 */

__IO uint32_t buttonsTime[3] = { 0 };

/* LED5 - PD14,
 * LED6 - PD15
 * LED4 - PD12
 * LED3 - PD13
 */

/**
 * Configure LED5, LED6 as output,
 * LED3, LED4 as AF - for PWM from TIM4
 */
void InitLeds() {
	GPIO_InitTypeDef GPIO_InitStructure;
	/* Periph clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE); // The LEDs are on GPIOD

//	// Configure PD12, PD13, PD14 and PD15 in output pushpull mode
//	GPIO_InitStructure.GPIO_Pin = LED5_PIN | LED6_PIN | LED4_PIN | LED3_PIN;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
//	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
//	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
//	GPIO_Init(GPIOD, &GPIO_InitStructure);

	/* Configure PD14 (LED5) and PD15(LED6) in output pushpull mode */
	GPIO_InitStructure.GPIO_Pin = LED5_PIN | LED6_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(LED3_GPIO_PORT, &GPIO_InitStructure);

	// GPIOD, Pin_12 - LED4, Pin_13 - LED3, - for PWM.
	GPIO_InitStructure.GPIO_Pin = LED4_PIN | LED3_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(LED3_GPIO_PORT, &GPIO_InitStructure);
}

/*
 * Initialize PB5,PB6,PB7 as PuPd NOPULL input.
 */
void InitButtons() {
	GPIO_InitTypeDef GPIO_InitStructure;
	/* Periph clock enable */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	/* Configure pins*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void SysTick_ButtonAction() {
	if (buttonsTime[0] > 0) {
		buttonsTime[0]--;
		if (buttonsTime[0] == MEDIUM_TIME_IN_MS) {
			buttons[0] = mediumPush;
		} else if (buttonsTime[0] == 0) {
			buttons[0] = longPush;
		}
	}
	if (buttonsTime[1] > 0) {
		buttonsTime[1]--;
		if (buttonsTime[1] == MEDIUM_TIME_IN_MS) {
			buttons[1] = mediumPush;
		} else if (buttonsTime[1] == 0) {
			buttons[1] = longPush;
		}
	}
	if (buttonsTime[2] > 0) {
		buttonsTime[2]--;
		if (buttonsTime[2] == MEDIUM_TIME_IN_MS) {
			buttons[2] = mediumPush;
		} else if (buttonsTime[2] == 0) {
			buttons[2] = longPush;
		}
	}
}

//**end**//
