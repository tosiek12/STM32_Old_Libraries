#ifndef ADC_H_
#define ADC_H_

//ADC3_BASE->DR		//((((uint32_t)0x40000000) + 0x00010000) + 0x2200)
//ADC3_BASE             (APB2PERIPH_BASE + 0x2200)
//APB2PERIPH_BASE (PERIPH_BASE + 0x00010000)
//PERIPH_BASE           ((uint32_t)0x40000000)
//#define ADC3_DR_ADDRESS     ((uint32_t)0x4001224C)	//ADC3->Memory DMA address

void ADC3_CH12_DMA_Config(void);
// Conversion must be manually triggered:
// ADC_SoftwareStartConv(ADC3 ); // Start ADC3 Software Conversion
extern volatile uint16_t *pBuffer_done;
extern volatile uint16_t *pBuffer_temp;
#define MIC_ADC_BUFFER_SIZE 512
extern volatile uint16_t ADC3ConvertedValue;

#endif
