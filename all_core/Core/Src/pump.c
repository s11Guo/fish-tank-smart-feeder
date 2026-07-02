#include "pump.h"

static uint8_t waterchange_running = 0;
static uint8_t waterchange_step = 0;

static uint32_t step_tick = 0;

void Pump_Init(void)
{
    Pump_StopAll();
}

void DirtyPump_On(void)
{
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_SET);
}

void DirtyPump_Off(void)
{
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);
}

void CleanPump_On(void)
{
    HAL_GPIO_WritePin(GPIOC,GPIO_PIN_5,GPIO_PIN_SET);
}

void CleanPump_Off(void)
{
    HAL_GPIO_WritePin(GPIOC,GPIO_PIN_5,GPIO_PIN_RESET);
}

void Pump_StopAll(void)
{
    DirtyPump_Off();
    CleanPump_Off();
}
