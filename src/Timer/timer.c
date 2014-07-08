#include "stm32f4_discovery.h"
#include "timer.h"
#include "USB/usb.h"
#include "ADC/adc.h"
#include "main.h"

void TIM6_Config() {
	/* --------------------------------------------------------
	 TIM3 input clock (TIM6CLK) is set to 2 * APB1 clock (PCLK1),
	 since APB1 prescaler is different from 1.
	 TIM6CLK = 2 * PCLK1
	 TIM6CLK = HCLK / 2 = SystemCoreClock /2
	 TIM6 Update event occurs each TIM6CLK/256
	 Note:
	 SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f4xx.c file.
	 Each time the core clock (HCLK) changes, user had to call SystemCoreClockUpdate()
	 function to update SystemCoreClock variable value. Otherwise, any configuration
	 based on this variable will be incorrect.
	 ----------------------------------------------------------- */

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	/* TIM6 Periph clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);

	/* Time base configuration */
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Period = 1;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);

	/* TIM6 TRGO selection */
	TIM_SelectOutputTrigger(TIM6, TIM_TRGOSource_Update );

	/* TIM6 enable counter */
	TIM_Cmd(TIM6, ENABLE);

}

/**
 * Configure TIM2 to generate interruption.
 */
void InitTimerWithIRQ() {
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	#define COUNTER_CLK 100000	//100kHz
	#define OUTPUT_CLK 10000	//20kHz
	//Prescaler = ((SystemCoreClock/2) / TIM3 counter clock) - 1
	const uint16_t PRESCALER = ( ((SystemCoreClock / 2) / COUNTER_CLK)-1);
	//ARR(TIM_Period) = (TIM3 counter clock / TIM3 output clock) - 1
	const uint32_t PERIOD = ((COUNTER_CLK/OUTPUT_CLK)-1);

	/* Init timer*/
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Period = PERIOD;
	TIM_TimeBaseStructure.TIM_Prescaler = PRESCALER;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	TIM_Cmd(TIM2, ENABLE);	//Enable peripherial

	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);	//Enable interupt flag

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0E;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0E;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/**
 * Configure PWM for Leds 3 and 4
 * TIM4 ->CCR1 = newValue; //Change duty cycle
 */
void InitPWM() {
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

void TIM2_IRQHandler() {
	const uint8_t NUMBER_OF_SAMPLES = 10;//Oversampling by 10, and average (low-pass filter??)
	static uint8_t numberOfSummedSamples = 0;
	static uint32_t currentSum = 0;
	static uint16_t numberOfStoredValues = 0;

	if (TIM_GetITStatus(TIM2, TIM_IT_Update ) != RESET) {
		currentSum += ADC3ConvertedValue;
		++numberOfSummedSamples;

		if (numberOfSummedSamples == NUMBER_OF_SAMPLES) {
			pBuffer_temp[numberOfStoredValues] = currentSum / NUMBER_OF_SAMPLES;

			numberOfSummedSamples = 0;
			currentSum = 0;

			++numberOfStoredValues;
			//Store result in memory.
			if (numberOfStoredValues == MIC_ADC_BUFFER_SIZE) {
				//Data vector is ready - change addresses.
				volatile uint16_t *temp = pBuffer_done;
				pBuffer_done = pBuffer_temp;
				pBuffer_temp = temp;

				numberOfStoredValues = 0;
				micSamplesReady = 1;
			}
		}
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update );
	}
}
