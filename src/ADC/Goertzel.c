#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <arm_math.h>
#include "stm32f4xx.h"

#include "adc.h"
#include "../Delay/delay.h"
#include "../HD44780/hd44780.h"

#define OVERSAMPLING 10
#define TIMER_CLK 10000
#define SAMPLING_RATE (TIMER_CLK/OVERSAMPLING)
#define MIC_ADC_BUFFER_SIZE 512

#define FREQ_RANGE 13
#define NUMBER_OF_ANALISED_FREQ (FREQ_RANGE*2+1)
#define THRESHOLD_GOERTZEL 1E8	//1*10^8 tyle wykrywalo w matlabie max.
//56 552 056 - stala dla 100Hz

inline void setServoDirection(uint16_t wantedFreq, uint16_t freqForCenterOfWeight) {
    const uint8_t toleration = 1;
	const uint8_t TURNING_TIME_IN_MS = 10;
    volatile int16_t delta = wantedFreq-freqForCenterOfWeight;
	extern volatile uint8_t ServoDirection;

    if(delta < -toleration) {
    	//Too high, go lower
		//100-105 = -5<-1 =>Za wysoko
		//100-101 = -1~<-1 =>ok
		ServoDirection = 18;	//N
		TIM4 ->CCR2 = ServoDirection; //Change duty cycle
		LCD_WriteTextXY("DOWN", 12, 1);

		if(delta>-5) {
			delay_ms(TURNING_TIME_IN_MS*(-delta)/2);
		} else {
			delay_ms(TURNING_TIME_IN_MS*5/2);
		}
		ServoDirection = 19;	//STOP
		TIM4 ->CCR2 = ServoDirection; //Change duty cycle
    } else if(delta > toleration) {
    	//Too low, go higher!
		//100-95 = 5>1 =>Za nisko
		//100-99 = 1~>1 =>ok
    	ServoDirection = 20;	//W
    	TIM4 ->CCR2 = ServoDirection; //Change duty cycle
		LCD_WriteTextXY(" UP ", 12, 1);

		if(delta<5) {
			delay_ms(TURNING_TIME_IN_MS*delta);
		} else {
			delay_ms(TURNING_TIME_IN_MS*5*4);
		}
		ServoDirection = 19;	//STOP
		TIM4 ->CCR2 = ServoDirection; //Change duty cycle
    } else {
		//Thats it
    	LCD_WriteTextXY(" OK ", 12, 1);
		ServoDirection = 19;	//STOP
		TIM4 ->CCR2 = ServoDirection; //Change duty cycle
    }
}

inline void calcCoeffs(float32_t *pCoeffs, const uint32_t* pfreqs ) {
	uint8_t n;
	for (n = 0; n < NUMBER_OF_ANALISED_FREQ; n++) {
		pCoeffs[n] = 2.0 * arm_cos_f32(2.0 * PI * (0.5 + (float32_t)MIC_ADC_BUFFER_SIZE * pfreqs[n] / SAMPLING_RATE) /MIC_ADC_BUFFER_SIZE);
	}
}

void algorithm(void) {
	extern volatile uint16_t *pBuffer_done;	//With samples

	float32_t prev1[NUMBER_OF_ANALISED_FREQ] = { 0 };
	float32_t prev2[NUMBER_OF_ANALISED_FREQ] = { 0 };
	volatile uint32_t magnitude[NUMBER_OF_ANALISED_FREQ] = { 0.0 };

	uint32_t magnitudeOfMaxFreq = 0;
	uint8_t indexOfMaxFreq=0;

	float32_t coeffs[NUMBER_OF_ANALISED_FREQ] = { 0.0 };
//	const uint32_t freqs[NUMBER_OF_ANALISED_FREQ] = {323, 324, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335, 336, 337};
	uint32_t searchedFreq = 247;
//	const uint32_t freqs[NUMBER_OF_ANALISED_FREQ] = {79, 81, 83, 85, 87, 89, 91, 93, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 107, 109, 111, 113, 115, 117, 119, 121};
//	const uint32_t freqs[NUMBER_OF_ANALISED_FREQ] = {309, 311, 313, 315, 317, 319, 321, 323, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335, 337, 339, 341, 343, 345, 347, 349, 351};
	const uint32_t freqs[NUMBER_OF_ANALISED_FREQ] = {226, 228, 230, 232, 234, 236, 238, 240, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 254, 256, 258, 260, 262, 264, 266, 268};
	//do +-5 krok 1
	//dalej krok 2
	calcCoeffs(coeffs, freqs);

	char buffor[20];
	float32_t val = 0.0;
	uint8_t i = 0;
	uint16_t sampleCount = 0;
	///Initialize the values of prev1 and prev2
	for (i = 0; i < NUMBER_OF_ANALISED_FREQ; i++) {
		prev2[i] = prev1[i] = 0.0;
	}

	for (sampleCount = 0; sampleCount < MIC_ADC_BUFFER_SIZE;
			sampleCount++) {
		//GOERTZEL Algorithm
		for (i = 0; i < NUMBER_OF_ANALISED_FREQ; i++) {
			val = coeffs[i] * prev1[i] - prev2[i]
					+ (pBuffer_done[sampleCount] - 2048);	//2^11 - by byl plus/minus
			prev2[i] = prev1[i];
			prev1[i] = val;
		}
	}

	magnitudeOfMaxFreq = 0;
	indexOfMaxFreq = 0;
	for (i = 0; i < NUMBER_OF_ANALISED_FREQ; i++) {	// compute the amplitudes/magnitudes
		magnitude[i] = (prev1[i] * prev1[i]) + (prev2[i] * prev2[i])
				- (coeffs[i] * prev1[i] * prev2[i]);
		if (magnitude[i] < THRESHOLD_GOERTZEL) {
			magnitude[i] = 0;
		} else if(magnitude[i] > magnitudeOfMaxFreq) {
			magnitudeOfMaxFreq = magnitude[i];
			indexOfMaxFreq = i;
		}
	}

	//Compute center of weight
	uint8_t rangeAroundMaxValue = 3;
	uint64_t sumOfWeights = 0;
	uint64_t sumOfValues = 0;
	uint8_t indeksForCenterOfWeight = 0;
	uint16_t freqForCenterOfWeight = 0;

	int16_t it = 0;
	if(magnitudeOfMaxFreq>0) {
		for (it = (indexOfMaxFreq - rangeAroundMaxValue); it <= (indexOfMaxFreq + rangeAroundMaxValue); it++) {	// compute the amplitudes/magnitudes
			if(it >= 0 && it < NUMBER_OF_ANALISED_FREQ) {
				sumOfWeights += (magnitude[it]/magnitudeOfMaxFreq);
				sumOfValues += (magnitude[it]/magnitudeOfMaxFreq)*(it+1);	//+1 by uwzglednic indeks 0.
			}
		}
		indeksForCenterOfWeight = (sumOfValues)/sumOfWeights;
		freqForCenterOfWeight = freqs[indeksForCenterOfWeight];

//		sprintf(buffor, "M%u|C%u|%u",indexOfMaxFreq, indeksForCenterOfWeight, freqForCenterOfWeight);
		sprintf(buffor, "Freq:%uHz",freqForCenterOfWeight);
		LCD_WriteTextXY("                ", 0, 1);
		LCD_WriteTextXY(buffor, 0, 1);
		//LCD_WriteTextXY("330|331|332|333|",0,0);	// 16 znakow sie miesci
		//mamy 15 pozycji
		uint8_t magnitudeForLCD = 0;
		LCD_GoTo(0,0);
		for(uint8_t poz = (FREQ_RANGE-6-1);poz<(NUMBER_OF_ANALISED_FREQ-(FREQ_RANGE-6));poz++ ) {
			magnitudeForLCD = (uint8_t) 7*(magnitude[poz]/(float32_t) magnitudeOfMaxFreq);
			LCD_WriteData(magnitudeForLCD);
		}
		//Set proper direction for servo
		setServoDirection(searchedFreq, freqForCenterOfWeight);
	} else {
		LCD_GoTo(0,0);
		for(uint8_t poz = (FREQ_RANGE-6-1);poz<(NUMBER_OF_ANALISED_FREQ-(FREQ_RANGE-6));poz++ ) {
			LCD_WriteData(0);
		}
		setServoDirection(0, 0);//Set servo in stop state.
	}


}


