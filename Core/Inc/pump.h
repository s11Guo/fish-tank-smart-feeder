#ifndef __PUMP_H
#define __PUMP_H

#include "stm32f4xx_hal.h"

void Pump_Init(void);

void DirtyPump_On(void);
void DirtyPump_Off(void);

void CleanPump_On(void);
void CleanPump_Off(void);

void Pump_StopAll(void);

/* 自动换水流程 */
void WaterChange_Start(void);
void WaterChange_Task(void);

#endif
