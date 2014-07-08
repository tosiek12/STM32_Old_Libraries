#ifndef TIMER_H_
#define TIMER_H_

extern volatile uint32_t micSamplesReady;	//From timer2ISR (adc.c)

void InitTimerWithIRQ();
void TIM6_Config();
void InitPWM();

#endif
