#include "bigled.h"
#include "stm32f4xx_hal.h"

static TIM_HandleTypeDef* led_tim;

// 关闭
void BigLED_Off(void)
{
    __HAL_TIM_SET_COMPARE(led_tim, TIM_CHANNEL_3, 0);
}

// 初始化
void BigLED_Init(TIM_HandleTypeDef* htim)
{
    led_tim = htim;
    HAL_TIM_PWM_Start(led_tim, TIM_CHANNEL_3);  // PC8 → TIM3_CH3
    BigLED_Off();  // 默认关闭
}

// 设置亮度 (0~1000，对应0%~100%)
void BigLED_SetBrightness(uint16_t brightness)
{
    if(brightness > 1000) brightness = 1000;
    __HAL_TIM_SET_COMPARE(led_tim, TIM_CHANNEL_3, brightness);
}

// 开启(最大亮度)
void BigLED_On(void)
{
    __HAL_TIM_SET_COMPARE(led_tim, TIM_CHANNEL_3, 1000);
}

