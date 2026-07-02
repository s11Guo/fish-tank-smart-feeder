#ifndef __LIGHTSENSOR_H
#define __LIGHTSENSOR_H

#include "stm32f4xx_hal.h"

void LightSensor_Init(ADC_HandleTypeDef* hadc);
uint16_t LightSensor_Read(void);

#endif
