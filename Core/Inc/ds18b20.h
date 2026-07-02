#ifndef __DS18B20_H
#define __DS18B20_H

#include "stm32f4xx_hal.h"

uint8_t DS18B20_Init(void);

/* 非阻塞温度测量状态机, 主循环中调用 */
typedef enum {
    DS18_IDLE,
    DS18_WAIT_CONVERT
} ds18_state_t;

void DS18B20_StartSample(void);   /* 启动一次转换, 非阻塞 */
uint8_t DS18B20_Poll(float *out); /* 轮询转换结果: 1=完成, 0=等待中 */

#endif
