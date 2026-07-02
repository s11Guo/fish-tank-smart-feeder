#include "heater.h"
#include "stm32f4xx_hal.h"

void Heater_On(void)
{
    HAL_GPIO_WritePin(GPIOC,GPIO_PIN_9,GPIO_PIN_SET);
}

void Heater_Off(void)
{
    HAL_GPIO_WritePin(GPIOC,GPIO_PIN_9,GPIO_PIN_RESET);
}

