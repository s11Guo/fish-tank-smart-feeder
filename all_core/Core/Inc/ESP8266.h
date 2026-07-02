#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* ========== AP 模式 (ESP8266 开热点，手机连 ESP) ========== */
#define WIFI_SSID        "FishTank_STM32"
#define WIFI_PASSWORD    "12345678"
#define WIFI_STA_SSID    "ZTE-c6Ts6b"
#define WIFI_STA_PASSWORD "20070319qq"
#define TCP_SERVER_PORT  9000
#define TCP_PUB_INTERVAL 1000

/* ========== 公共 API ========== */
extern uint8_t esp_ready;
/* 非阻塞初始化 (替代阻塞式 ESP8266_Init) */
void    ESP8266_InitStart(void);
uint8_t ESP8266_InitPoll(void);   /* 0=忙, 1=成功, 2=失败 */

void    ESP8266_SetPassthrough(uint8_t en);
void    ESP8266_Process(void);
void    ESP8266_Send(const char *data, int len);
void    ESP8266_RxHandler(uint8_t byte);
void    ESP8266_ForcePublish(void);  /* 立即发布一次 JSON */

/* ========== 传感器数据 ========== */
extern float    g_esp_temp;
extern float    g_esp_tds;
extern uint16_t g_esp_light;
extern uint8_t  g_esp_lmode;
extern uint8_t  g_esp_heater;
extern uint8_t  g_esp_pump;
extern uint8_t  g_esp_run;
extern char     g_esp_page[20];
extern uint8_t  g_esp_action;
extern uint8_t  g_esp_sub;

/* ========== 命令通道 (App → STM32) ========== */
uint8_t ESP8266_CmdAvailable(void);
char    ESP8266_GetCmd(void);     /* 0=队列空 */

#endif
