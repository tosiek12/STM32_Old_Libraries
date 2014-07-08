#include "stm32f4_discovery.h"
#include "exti.h"

/*
 * Initialize PB5,PB6,PB7 to generate EXTI
 */
void InitButtonsEXTI() {
	NVIC_InitTypeDef NVIC_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;

	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0D;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0D;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Configure Button EXTI line */
	EXTI_InitStructure.EXTI_Line = EXTI_Line5 | EXTI_Line6 | EXTI_Line7;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	/* Connect Button EXTI Line to Button GPIO Pin */
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource5 );
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource6 );
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource7 );
}

//**end**//
