#include "stm32f4_discovery.h"
#include "delay.h"

inline void delay_ms(uint32_t timeInMs) {
	delayCnt = timeInMs*1000;
	while (delayCnt) {
	}
}

inline void delay_us(uint32_t timeInUs) {
	delayCnt = timeInUs;
	while (delayCnt) {
	}
}

inline void SysTick_DelayAction() {
	if (delayCnt != 0) {
		delayCnt--;
	}
}


/**
 * Sets interrupt form SysTick for 1us
 */
inline void SysTick_Conf(void) {
	  /* Setup SysTick Timer for 1 usec interrupts.
	     ------------------------------------------
	    1. The SysTick_Config() function is a CMSIS function which configure:
	       - The SysTick Reload register with value passed as function parameter.
	       - Configure the SysTick IRQ priority to the lowest value (0x0F).
	       - Reset the SysTick Counter register.
	       - Configure the SysTick Counter clock source to be Core Clock Source (HCLK).
	       - Enable the SysTick Interrupt.
	       - Start the SysTick Counter.

	    2. You can change the SysTick Clock source to be HCLK_Div8 by calling the
	       SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8) just after the
	       SysTick_Config() function call. The SysTick_CLKSourceConfig() is defined
	       inside the misc.c file.

	    3. You can change the SysTick IRQ priority by calling the
	       NVIC_SetPriority(SysTick_IRQn,...) just after the SysTick_Config() function
	       call. The NVIC_SetPriority() is defined inside the core_cm4.h file.

	    4. To adjust the SysTick time base, use the following formula:

	         Reload Value = SysTick Counter Clock (Hz) x  Desired Time base (s)

	       - Reload Value is the parameter to be passed for SysTick_Config() function
	       - Reload Value should not exceed 0xFFFFFF
	   */
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK );
	//168/8 = 21MHz
	//21MHz/21=1MHz => T=1ms;
	if (SysTick_Config(SystemCoreClock/1000000)) {
		//Capture error
		while (1) {
		}
	}

}
