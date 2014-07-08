#include "stm32f4_discovery.h"
#include "accelerometer.h"
#include "../Delay/delay.h"
#include "stm32f4_discovery_LIS3DSH.h"
#include "../HD44780/hd44780.h"
#include "../NokiaLCD/nokiaLCD.h"
#include "math.h"
#include "arm_math.h"
#include "stdio.h"

__IO int16_t xZero = 0, yZero = 0, zZero = 0;
__IO int16_t xActual= 0, yActual = 0, zActual = 0;

/*
 * Before, sysTick must be initialized.
 * It use delay_ms function!
 */
void InitAccelerometer() {
	LIS3DSH_Init();
	delay_ms(5); 	//wait for boot procedure to be completed.

	//Check ID:
	uint8_t temp = LIS3DSH_ReadByte(LIS3DSH_WHO_AM_I_ADDR);
	/* Check device identification register, this register should contains
	the device identifier that for LIS3DSH is set to 0x3F */
	if (temp != _I_AM_LIS3DSH) {
		while(1) { }//Fail_Handler();
	}
	//BDU by default is 0 - output not updated until MSB and LSB reading.
	LIS3DSH_SetOutputDataRate(LIS3DSH_ODR_800_HZ);	//Power-On
	LIS3DSH_AxesEnable(LIS3DSH_XYZ_ENABLE);	//Enable X,Y,Z Axis
	LIS3DSH_SetFilterBandwidth(_LIS3DSH_BW_50HZ);	//Set bandwidth
	LIS3DSH_SetMeasuredRange(_LIS3DSH_FSCALE_2G);	//Set range of measurement

	/*it have to be in values before scaling by scaling factor
	 * (X_OFFSET/32)/LIS3DSH_2G_FACTOR */
#define X_OFFSET -128	//jest wiecej, wiec musze programowo juz to zalatwic
#define Y_OFFSET 20
#define Z_OFFSET 32
	LIS3DSH_SetOffsets(X_OFFSET, Y_OFFSET, Z_OFFSET);

	/* Required delay for the MEMS Accelerometer: Turn-on time = 3/Output data Rate
	 = 3/100 = 30ms */
	delay_ms(300);

	LIS3DSH_OutXYZTypeDef measurment;
	LIS3DSH_ReadAxes(&measurment);

	xZero = (measurment.x);	//zapisz offset dla osi.
	yZero = (measurment.y);
	zZero = (measurment.z);
}

/*
 * Performed in SysTick interrupt service every 1ms
 * Update actual value
 *
 */
void SysTick_UpdateAccelerometer() {
	LIS3DSH_OutXYZTypeDef measurment;
	static uint8_t Counter = 0;
	static uint8_t numberOfSamples = 0;
	const uint8_t OVERSAMPLING = 20;

	static int32_t xSum = 0;
	static int32_t ySum = 0;
	static int32_t zSum = 0;

	static int16_t xmin = 0;
	static int16_t xmax = 0;
	static int16_t ymin = 0;
	static int16_t ymax = 0;
	static int16_t zmin = 0;
	static int16_t zmax = 0;

	if (++Counter == 2)	{	//Do it after 2ms = 500hz		LIS3DSH_ReadAxes(&measurment);
		if(measurment.x>xmax) {
			xmax = measurment.x;
		} else if (measurment.x < xmin) {
			xmin = measurment.x;
		}
		if(measurment.y>ymax) {
			ymax = measurment.y;
		} else if (measurment.y < ymin) {
			ymin = measurment.y;
		}
		if(measurment.z>zmax) {
			zmax = measurment.z;
		} else if (measurment.z < zmin) {
			zmin = measurment.z;
		}

		xSum += (measurment.x);
		ySum += (measurment.y);
		zSum += (measurment.z);

		if(++numberOfSamples == OVERSAMPLING) {
			xActual = xSum/OVERSAMPLING/10;
			yActual = ySum/OVERSAMPLING/10;
			zActual = zSum/OVERSAMPLING/10;

			xSum = 0;
			ySum = 0;
			zSum = 0;
			numberOfSamples = 0;
		}
		Counter = 0x00;
	}
}

void Main_AccelerometerAction() {
	const uint8_t THRESHOLD = 150;
	__IO int16_t XRollAngle;	//Range -180,180
	__IO int16_t YPitchAngle;	//Range -90,90
	__IO int16_t TiltAngle;		//Range 0,180	//odchylenie od pionu (grawitacji)
	float32_t sqrt_argument;
	float32_t sqrt_result;
	char buf[24];

	XRollAngle = atan2f((float)(yActual), (float)(zActual))*180/PI;	//zgodne z teori�

	sqrt_argument = 0;
	sqrt_argument +=(float)(zActual)*(float)(zActual);
	sqrt_argument +=(float)(yActual)*(float)(yActual);
	arm_sqrt_f32(sqrt_argument,&sqrt_result);
	YPitchAngle = atan2f(-(xActual),sqrt_result)*180/PI;

	sqrt_argument = 0;
	sqrt_argument +=(float)(zActual)*(float)(zActual);
	sqrt_argument +=(float)(xActual)*(float)(xActual);
	sqrt_argument +=(float)(yActual)*(float)(yActual);
	arm_sqrt_f32(sqrt_argument,&sqrt_result);
	TiltAngle = acosf((zActual)/sqrt_result)*180/PI;

	LCD5110_ClearLine(3);
	sprintf(buf,"XRoll=%d",XRollAngle);
	LCD5110_WriteTextXY(buf,0,3);

	LCD5110_ClearLine(4);
	sprintf(buf,"YPitch=%d",YPitchAngle);
	LCD5110_WriteTextXY(buf,0,4);

	LCD5110_ClearLine(5);
	sprintf(buf,"Tilt=%d",TiltAngle);
	LCD5110_WriteTextXY(buf,0,5);

	if (xActual != 0 || yActual != 0 || zActual != 0) {
		LCD5110_ClearLine(0);
		sprintf(buf,"X=%d",(xActual));
		LCD5110_WriteTextXY(buf,0,0);

		LCD5110_ClearLine(1);
		sprintf(buf,"Y=%d",(yActual));
		LCD5110_WriteTextXY(buf,0,1);

		LCD5110_ClearLine(2);
		sprintf(buf,"Z=%d",(zActual));
		LCD5110_WriteTextXY(buf,0,2);

//				if (xActual > THRESHOLD) {
//					LCD5110_WriteTextXY("X>30",6,0);
//				} else if (xActual < -THRESHOLD) {
//					LCD5110_WriteTextXY("X<-30",1,0);
//				} else {
//					LCD5110_ClearLine(0);
//				}
//
//				if (yActual > THRESHOLD) {
//					LCD5110_WriteTextXY("Y>30",6,1);
//				} else if (yActual < -THRESHOLD) {
//					LCD5110_WriteTextXY("Y<-30",1,1);
//				} else {
//					LCD5110_ClearLine(1);
//				}
			}
}

/**
 * @brief  MEMS accelerometre management of the timeout situation.
 * @param  None.
 * @retval None.
 */
uint32_t LIS302DL_TIMEOUT_UserCallback(void) {
	/* MEMS Accelerometer Timeout error occured */
	while (1) {
	}
}
