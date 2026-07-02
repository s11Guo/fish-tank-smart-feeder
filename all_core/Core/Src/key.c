#include "key.h"
#include "main.h"

void Key_Init(void)
{

}

/*
 * PB6  = SW1 -> UP
 * PB4  = SW2 -> DOWN
 * PD2  = SW3 -> ENTER
 * PC11 = SW4 -> BACK
 *
 * 按下 = 低电平, 20ms 延时消抖
 */

KeyEvent_t Key_Scan(void)
{
    if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_6)==GPIO_PIN_RESET)
    {
        HAL_Delay(20);

        if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_6)==GPIO_PIN_RESET)
        {
            while(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_6)==GPIO_PIN_RESET);
            return KEY_UP;
        }
    }

    if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_4)==GPIO_PIN_RESET)
    {
        HAL_Delay(20);

        if(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_4)==GPIO_PIN_RESET)
        {
            while(HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_4)==GPIO_PIN_RESET);
            return KEY_DOWN;
        }
    }

    if(HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_2)==GPIO_PIN_RESET)
    {
        HAL_Delay(20);

        if(HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_2)==GPIO_PIN_RESET)
        {
            while(HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_2)==GPIO_PIN_RESET);
            return KEY_ENTER;
        }
    }

    if(HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_11)==GPIO_PIN_RESET)
    {
        HAL_Delay(20);

        if(HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_11)==GPIO_PIN_RESET)
        {
            while(HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_11)==GPIO_PIN_RESET);
            return KEY_BACK;
        }
    }

    return KEY_NONE;
}
