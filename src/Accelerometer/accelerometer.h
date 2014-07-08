#ifndef ACCELEROMETER_H_
#define ACCELEROMETER_H_

/* Exported macro ------------------------------------------------------------*/
#define ABS(x)                           ((x < 0) ? (-x) : x)
#define MAX(a,b)                         ((a < b) ? (b) : a)

extern __IO int16_t xZero, yZero, zZero;
extern __IO int16_t xActual, yActual, zActual;

void SysTick_UpdateAccelerometer();
void InitAccelerometer();
void Main_AccelerometerAction();


#endif



