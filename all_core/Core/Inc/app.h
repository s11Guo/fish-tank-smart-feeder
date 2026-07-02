#ifndef __APP_H
#define __APP_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* ===== 页面定义 ===== */
#define PAGE_WELCOME       0
#define PAGE_MENU          1
#define PAGE_FEED          2
#define PAGE_LIGHT         3
#define PAGE_TEMP          4
#define PAGE_PUMP          5
#define PAGE_FEED_SUBMENU  7
#define PAGE_FEED_TRAIN    8
#define PAGE_FEED_MANUAL   9
#define PAGE_TRAIN_SETTING 10
#define PAGE_AUTO          11
#define PAGE_PUMP_SUB      12

/* ===== 电机 ===== */
#define MOTOR_A_TIME   10000U
#define MOTOR_B_TIME   10000U
#define MOTOR_PWM_DUTY 800U
#define PULSE_ON_MS    500U    /* 转 0.5 秒 */
#define PULSE_OFF_MS   400U    /* 停 0.4 秒 */

/* ===== 光敏 ===== */
/* 光敏电阻: 暗→高ADC(~2400), 亮→低ADC(~500) */
#define LIGHT_ADC_MIN  500U
#define LIGHT_ADC_MAX  2500U
#define LIGHT_STEP     5U

/* ===== 阈值 ===== */
/* ── ASR 语音回复码 ── */
#define ASR_RSP_FEED_DONE      0x0F
#define ASR_RSP_TRAIN_DONE     0x11

#define TDS_GOOD      300.0f
#define TDS_FAIR      600.0f
#define TDS_DIRTY    1000.0f
#define TEMP_LOW       18.0f
#define TEMP_HIGH      35.0f

/* ===== 共享硬件句柄 ===== */
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;

extern volatile uint8_t asr_cmd_ready;
void ASR_Init(void);
void ASR_SendResponse(uint8_t code);
void ASR_ProcessCommand(void);

/* ===== 命令/串口 ===== */
extern char rx_buf[64];
extern volatile uint8_t cmd_ready;

/* ===== 电机状态 ===== */
extern uint32_t motor_a_tick;
extern uint32_t motor_b_tick;
extern uint8_t motor_a_run;
extern uint8_t motor_b_run;

/* ===== 显示 ===== */
extern uint8_t current_page;
extern uint8_t menu_index;

/* ===== MANUAL FEED ===== */
extern uint8_t  feed_cursor;
extern uint8_t  feed_time;
extern uint8_t  feed_editing;
extern uint8_t  feed_running;
extern uint32_t feed_start_tick;
extern uint8_t  feed_blink;

/* ===== TRAIN MODE ===== */
extern uint8_t  train_stages[7][2];
extern uint8_t  train_interval;
extern uint8_t  train_cursor;
extern uint8_t  train_stage;
extern uint8_t  train_setting_cursor;
extern uint8_t  train_editing;
extern uint8_t  train_running;
extern uint8_t  train_exec_stage;
extern uint8_t  train_exec_phase;
extern uint8_t  train_motor_active;
extern uint32_t train_exec_tick;
extern uint8_t  train_bo_done;
extern uint8_t  train_ao_done;
extern uint8_t  train_blink;

/* ===== AUTO MODE ===== */
extern uint8_t  auto_interval;
extern uint8_t  auto_ao_time;
extern uint8_t  auto_cursor;
extern uint8_t  auto_editing;
extern uint8_t  auto_running;
extern uint8_t  auto_phase;
extern uint32_t auto_tick;
extern uint8_t  auto_blink;

/* ===== LIGHT ===== */
#define LIGHT_AUTO     0
#define LIGHT_MANUAL   1
extern uint8_t  light_now;
extern uint8_t  light_led;
extern uint8_t  light_cursor;
extern uint8_t  light_mode;
extern uint8_t  light_edit;
extern uint8_t  light_blink;

/* ===== TEMP ===== */
extern float    temp_target_low;
extern float    temp_target_high;
extern float    temp_current;
extern uint8_t  heater_on;
extern uint8_t  temp_editing;
extern float    temp_sum;
extern uint8_t  temp_cnt;
extern uint32_t temp_window_start;
extern uint32_t temp_sample_tick;
extern uint8_t  temp_blink;

/* ===== PUMP ===== */
extern uint8_t  pump_sub;
extern uint8_t  pump_cursor;
extern uint8_t  pump_running;
extern uint8_t  pump_blink;
extern float    pump_tds;

/* ===== 动画 ===== */
extern uint8_t  anim_frame;
extern uint32_t anim_tick;
extern uint8_t  anim_intro;
extern uint32_t anim_intro_tick;
extern uint32_t oled_watchdog;

/* ===== 光照 ADC ===== */
extern uint16_t light_raw;

#endif
