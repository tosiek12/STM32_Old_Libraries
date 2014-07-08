#include "stm32f4_discovery.h"
#include "stm32f4xx.h"
#include "servo.h"

/**
 * Configure PWM for Leds 3 and 4
 * TIM4 ->CCR1 = newValue; //Change duty cycle
 */
void InitServoViaPWM() {
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	//TIM3CLK=2*PCLK1(APB1 clk) => PCLK1=HCLK/4 => TIM3CLK=HCLK/2=SystemCoreClock/2
	//Prescaler = (TIM3CLK / TIM3 counter clock) - 1
	uint16_t PrescalerValue = ((SystemCoreClock / 2) / 10000) - 1;//to get timer for 0,1ms
	/* Init timer*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	//ARR(TIM_Period) = (TIM3 counter clock / TIM3 output clock) - 1
	TIM_TimeBaseStructure.TIM_Period = 200;
	TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	/* Init PWM mode for channel 1 */
	TIM_OCInitTypeDef outputChannelInit = { 0, };
	outputChannelInit.TIM_OCMode = TIM_OCMode_PWM1;
	outputChannelInit.TIM_Pulse = 10;
	outputChannelInit.TIM_OutputState = TIM_OutputState_Enable;
	outputChannelInit.TIM_OCPolarity = TIM_OCPolarity_High;

	/* Enable channel 1 */
	TIM_OC1Init(TIM4, &outputChannelInit);
	TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable );
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource12, GPIO_AF_TIM4 );

	/* Change saturation for 2nd channel - */
	outputChannelInit.TIM_Pulse =100;
	/* Enable channel 2*/
	TIM_OC2Init(TIM4, &outputChannelInit);
	TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable );
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource13, GPIO_AF_TIM4 );

	TIM_ARRPreloadConfig(TIM4, ENABLE);
	TIM_Cmd(TIM4, ENABLE);	//Run peripherial
	//TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);	//Enable interupt flag
}

