#include "MOTOR.h"
#include "tim.h"  

void Motor_Init(void)
{
    HAL_GPIO_WritePin(STBY_PORT, STBY_PIN, GPIO_PIN_SET);
}

void Motor_Start(void)
{
    HAL_GPIO_WritePin(STBY_PORT, STBY_PIN, GPIO_PIN_SET);
}

void Motor_Stop(void)
{
    HAL_GPIO_WritePin(STBY_PORT, STBY_PIN, GPIO_PIN_RESET);

    MotorA_Stop();
    MotorB_Stop();
}

void MotorA_Run(void)
{
    HAL_GPIO_WritePin(AIN1_PORT, AIN1_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(AIN2_PORT, AIN2_PIN, GPIO_PIN_RESET);
}

void MotorB_Run(void)
{
    HAL_GPIO_WritePin(BIN1_PORT, BIN1_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(BIN2_PORT, BIN2_PIN, GPIO_PIN_RESET);
}

void MotorA_Stop(void)
{
    HAL_GPIO_WritePin(AIN1_PORT, AIN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AIN2_PORT, AIN2_PIN, GPIO_PIN_RESET);
}

void MotorB_Stop(void)
{
    HAL_GPIO_WritePin(BIN1_PORT, BIN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BIN2_PORT, BIN2_PIN, GPIO_PIN_RESET);
}

// PWMËÙ¶È¿ØÖÆ
void Motor_SetSpeed(uint16_t a, uint16_t b)
{
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, a);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, b);
}
