/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PDM_FILTER_H
#define __PDM_FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
typedef struct {
	/*Defines the frequency output of the filter in Hz.*/
	uint16_t Fs;
	/*Defines the low pass filter cut-off frequency. If 0, the low pass filter is disabled.*/
	float LP_HZ;
	/*Defines the high pass filter cut frequency. If 0, the high pass filter is disabled.*/
	float HP_HZ;
	/*Define the number of microphones in the input stream.
	 * This parameter is used to specify the interlacing of microphones in the input buffer.
	 * The PDM samples are grouped eight by eight in u8 format (8-bit).*/
	uint16_t In_MicChannels;
	/* Defines the number of microphones in the output stream.
	 * This parameter is used to interlace different microphones in the output buffer.
	 * Each sample is a 16-bit value.*/
	uint16_t Out_MicChannels;
	/*Defines a 34 Byte memory used internally by the library during the PDM decimation step.*/
	char InternalFilter[34];
} PDMFilter_InitStruct;

/* Exported constants --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
#define HTONS(A)  ((((u16)(A) & 0xff00) >> 8) | \
                   (((u16)(A) & 0x00ff) << 8))

/* Exported functions ------------------------------------------------------- */
void PDM_Filter_Init(PDMFilter_InitStruct * Filter);
/* @Brief: These functions are used to process a millisecond of PDM data from a single microphone.
 * @Return: number of PCM samples equal to the frequency defined in the filter initialization, divided by 1000 (floor division).
 * @Param:
 * data: is the input buffer containing the PDM; the application must pass to the function the pointer to the first input sample of the microphone that must be processed.
 * dataOut: is the output buffer processed by the PDM_Filter function. The application must pass to the function the pointer to the first sample of the channel to be obtained.
 * MicGain: is a value between 0 and 64 used to specify the microphone gain.
 * Filter: is the structure containing all the filter parameters specified by the user using the PDM_Filter_Init function.
 */

/*
 * PDM_Filter_XX_YYY
 * XX - decimation factor 64/80.
 * YYY - LSB or MSB representation of the input buffer.
 */
int32_t PDM_Filter_64_MSB(uint8_t* data, uint16_t* dataOut, uint16_t MicGain,
		PDMFilter_InitStruct * Filter);
int32_t PDM_Filter_80_MSB(uint8_t* data, uint16_t* dataOut, uint16_t MicGain,
		PDMFilter_InitStruct * Filter);
int32_t PDM_Filter_64_LSB(uint8_t* data, uint16_t* dataOut, uint16_t MicGain,
		PDMFilter_InitStruct * Filter);
int32_t PDM_Filter_80_LSB(uint8_t* data, uint16_t* dataOut, uint16_t MicGain,
		PDMFilter_InitStruct * Filter);

#ifdef __cplusplus
}
#endif

#endif /* __PDM_FILTER_H */
