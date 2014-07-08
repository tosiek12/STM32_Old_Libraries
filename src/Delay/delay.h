#ifndef DELAY_H_
#define DELAY_H_

void delay_ms(uint32_t timeInMs);
void delay_us(uint32_t timeInUs);
void SysTick_Conf(void);
void SysTick_DelayAction();

__IO uint32_t delayCnt;

#endif



