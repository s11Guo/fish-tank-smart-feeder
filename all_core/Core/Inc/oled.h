#ifndef __OLED_H
#define __OLED_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* 底层接口 */
HAL_StatusTypeDef OLED_WriteCmd(uint8_t cmd);
void OLED_Init(void);

/* 显示功能 */
void OLED_Clear(void);
void OLED_SetCursor(uint8_t x, uint8_t y);
void OLED_Refresh(void);

/* 绘制功能（写入帧缓冲，需调 OLED_Refresh 刷屏） */
void OLED_ShowChar(uint8_t x, uint8_t y, char chr, uint8_t size);
void OLED_ShowString(uint8_t x, uint8_t y, char *str, uint8_t size);
void OLED_DrawBMP(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, const uint8_t BMP[]);

/* 全局变量 */
extern uint8_t OLED_Addr;

#endif
