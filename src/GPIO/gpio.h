#ifndef GPIO_H_
#define GPIO_H_

enum state {
	noPush = 0, shortPush = 1, mediumPush = 2, longPush = 3
};
extern __IO enum state buttons[3];
extern __IO uint32_t buttonsTime[3];
extern const uint32_t LONG_TIME_IN_MS;
extern const uint32_t MEDIUM_TIME_IN_MS;

void SysTick_ButtonAction();
void InitLeds();
void InitButtons();

#endif
