#ifndef __TDS_H
#define __TDS_H

#include "stm32f4xx_hal.h"

extern uint16_t tds_raw_adc;
extern float    tds_voltage;

void TDS_Init(void);

uint16_t TDS_ReadADC(void);

float TDS_ReadVoltage(void);

float TDS_ReadPPM(void);
float TDS_ReadPPM_T(float temperature);  /* 带温度补偿, 避免重复读DS18B20 */

#endif
