#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f4xx_hal.h"
#include "tim.h"  
// TB6612 ¿ØÖÆÒý½Å
#define AIN1_PIN   GPIO_PIN_4
#define AIN1_PORT  GPIOA

#define AIN2_PIN   GPIO_PIN_5
#define AIN2_PORT  GPIOA

#define BIN1_PIN   GPIO_PIN_0
#define BIN1_PORT  GPIOB

#define BIN2_PIN   GPIO_PIN_1
#define BIN2_PORT  GPIOB

#define STBY_PIN   GPIO_PIN_0
#define STBY_PORT  GPIOA

void Motor_Init(void);
void Motor_Start(void);
void Motor_Stop(void);

void MotorA_Run(void);
void MotorB_Run(void);

void MotorA_Stop(void);
void MotorB_Stop(void);

void Motor_SetSpeed(uint16_t a, uint16_t b);

#endif

