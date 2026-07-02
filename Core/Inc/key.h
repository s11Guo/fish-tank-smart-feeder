#ifndef __KEY_H
#define __KEY_H

#include "stm32f4xx_hal.h"

typedef enum
{
    KEY_NONE = 0,

    KEY_UP,
    KEY_DOWN,

    KEY_ENTER,
    KEY_BACK

}KeyEvent_t;

void Key_Init(void);

KeyEvent_t Key_Scan(void);

#endif
