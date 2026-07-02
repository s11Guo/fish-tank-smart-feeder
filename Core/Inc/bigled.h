#ifndef __BIGLED_H
#define __BIGLED_H

#include "stm32f4xx_hal.h"

void BigLED_Init(TIM_HandleTypeDef* htim);
void BigLED_SetBrightness(uint16_t brightness);
void BigLED_On(void);
void BigLED_Off(void);

#endif
