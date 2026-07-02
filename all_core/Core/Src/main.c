/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "oled.h"
#include "Lightsensor.h"
#include "ds18b20.h"
#include "heater.h"
#include "tds.h"
#include "pump.h"
#include "app.h"
#include "ui.h"
#include "feed.h"
#include "temp.h"
#include "light.h"
#include "ESP8266.h"  /* WiFi 模块 */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
char rx_buf[64];
char rx_line[64];
uint8_t rx_index = 0;
volatile uint8_t cmd_ready = 0;
volatile uint8_t cmd_from_remote = 0;   // 1=来自手机/ESP8266
uint32_t motor_a_tick = 0;
uint32_t motor_b_tick = 0;
uint8_t motor_a_run = 0;
uint8_t motor_b_run = 0;

/* 显示相关变量 */
uint8_t current_page = PAGE_WELCOME;
uint8_t menu_index = 0;

/* MANUAL FEED 状�?? */
uint8_t  feed_cursor = 0;       // 0=FEED Time, 1=START, 2=END
uint8_t  feed_time = 3;         // 默认3分钟
uint8_t  feed_editing = 0;      // 0=普�??, 1=编辑时间
uint8_t  feed_running = 0;      // 电机运行�????????
uint32_t feed_start_tick = 0;
uint8_t  feed_blink = 0;        // 闪烁状�??
uint32_t last_blink_tick = 0;

/* TRAIN MODE 状�?? */
uint8_t  train_stages[7][2];     // [stage][0]=BO, [stage][1]=AO, 半分�????????
uint8_t  train_interval;         // 阶段间隔(分钟)
uint8_t  train_cursor;           // 0=FEED SETTING, 1=START, 2=END
uint8_t  train_stage;            // 当前配置阶段 0-6
uint8_t  train_setting_cursor;   // 0=BO, 1=AO, 2=Interval, 3=CONTINUE
uint8_t  train_editing;          // 0=normal, 1=editing BO, 2=editing AO, 3=editing Interval
uint8_t  train_running;          // 训练执行�????????
uint8_t  train_exec_stage;       // 当前执行阶段
uint8_t  train_exec_motor;       // 0=BO, 1=AO
uint8_t  train_exec_phase;       // 0=BO, 1=AO, 2=等待间隔
uint8_t  train_motor_active;     // 当前电机运行�????????
uint32_t train_exec_tick;
uint8_t  train_bo_done;          // BO已完�????????
uint8_t  train_ao_done;          // AO已完�????????
uint8_t  train_blink;            // 闪烁状�??

/* AUTO MODE 状�?? */
uint8_t  auto_interval;          // 投喂间隔(分钟)
uint8_t  auto_ao_time;           // AO运行时间(分钟)
uint8_t  auto_cursor;            // 0=Interval, 1=AO Time, 2=START, 3=END
uint8_t  auto_editing;           // 0=normal, 1=editing interval, 2=editing AO
uint8_t  auto_running;           // 自动模式运行�????????
uint8_t  auto_phase;             // 0=等待间隔, 1=运行AO
uint32_t auto_tick;
uint8_t  auto_blink;             // 闪烁状�??

/* LIGHT state */
uint8_t  light_now;              // ambient light 0-100% (internal)
uint8_t  light_led;              // LED PWM 0-100%
uint8_t  light_cursor;           // 0=LED, 1=Status, 2=Mode
uint8_t  light_mode;             // 0=Auto, 1=Manual
uint8_t  light_edit;             // 1=editing LED (Manual)
uint8_t  light_blink;            // blink state

/* TEMP 状�?? */
float    temp_target_low;        // 温度下限 °C
float    temp_target_high;       // 温度上限 °C
float    temp_current;           // 当前温度 °C (10s 均�??)
uint8_t  heater_on;              // 加热器状�???????? 0/1
uint8_t  temp_editing;           // 0=normal, 1=editing low, 2=editing high
float    temp_sum;               // 10s 累加�????????
uint8_t  temp_cnt;               // 10s 采样计�??
uint32_t temp_window_start;      // 10s 窗口起�??
uint32_t temp_sample_tick;       // 下次采样时�??
uint8_t  temp_blink;             // 闪烁状�??

/* PUMP 状�?? */
uint8_t  pump_sub;               // 0=主页�????????, 1=Setting子页�????????
uint8_t  pump_cursor;            // �????????:0=Condition,1=Setting; �????????:0=P1out,1=P2into,2=End
uint8_t  pump_running;           // 0=�????????, 1=P1out(DirtyPump), 2=P2into(CleanPump)
uint8_t  pump_blink;             // 闪烁状�??
float    pump_tds;               // 当前TDS�????????

/* 动画状�?? */
uint8_t  anim_frame;              // 当前�???????? 0-4
uint32_t anim_tick;               // 帧计�????????
uint8_t  anim_intro;              // 1=播放入场动画
uint32_t anim_intro_tick;         // 入场�????????始时�????????
uint32_t oled_watchdog;             // OLED 强制刷新看门�???????

	uint16_t light_raw = 0;			// light ADC value
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */

  printf("Boot OK, starting OLED...\r\n");

  /* OLED 初始�???????? (地址 0x3C) */
  HAL_Delay(200);  /* OLED 上电延时, 等驱动芯片稳�??? */

  OLED_Addr = (0x3C << 1);
  OLED_Init();
  HAL_Delay(50);

  current_page = PAGE_WELCOME;
  OLED_ShowPage();
  HAL_Delay(50);
  printf("CR1=0x%08lX SR=0x%08lX\r\n",
         USART1->CR1, USART1->SR);
  printf("System Ready\r\n");
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);   /* 使能 RXNE, ISR 直接处理 */

  /* 启动PWM */
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);   // 通道A PA6
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);   // 通道B PA7
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);   // 通道C PB0 (LED补光)

  /* 电机初始停�??, STBY 默认�???????? */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
  motor_a_run = 0;
  motor_b_run = 0;

  /* TRAIN 阶段默认�???????? (半分钟单�????????) */
  {
      uint8_t i;
      for (i = 0; i < 7; i++)
      {
          train_stages[i][0] = 7 - i;   // BO: 7->1 (x0.5min)
          train_stages[i][1] = i + 1;   // AO: 1->7 (x0.5min)
      }
      train_interval = 1;               // interval 1 min
      train_cursor = 0;
      train_stage = 0;
      train_setting_cursor = 0;
      train_editing = 0;
      train_running = 0;
      train_exec_stage = 0;
      train_motor_active = 0;
      train_bo_done = 0;
      train_ao_done = 0;
  }

  /* AUTO 默认�???????? */
  {
      auto_interval = 30;           // 默认30分钟
      auto_ao_time = 3;             // 默认3分钟
      auto_cursor = 0;
      auto_editing = 0;
      auto_running = 0;
  }

  /* 光敏传感器初始化 */
  LightSensor_Init(&hadc1);

  /* LIGHT defaults */
  {
      light_mode   = LIGHT_AUTO;
      light_now    = 0;
      light_led    = 0;
      light_cursor = 2;           // default on Mode row
      light_edit   = 0;
  }

  /* TEMP defaults */
  {
      temp_target_low = 24.0f;
      temp_target_high = 30.0f;
      temp_current = 0.0f;   /* 0=未读到传感器, 保持初�?? */
      heater_on = 0;
      temp_editing = 0;
      temp_sum = 0.0f;
      temp_cnt = 0;
      temp_window_start = HAL_GetTick();
      temp_sample_tick  = 0;
  }

    /* PUMP defaults */
    {
        pump_sub = 0;
        pump_cursor = 0;
        pump_running = 0;
        pump_tds = 0.0f;
    }

  ASR_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* �???�??? heartbeat PC13 �???�??? */
    {
        static uint32_t hb_tick = 0;
        if (HAL_GetTick() - hb_tick >= 500)
        {
            hb_tick = HAL_GetTick();
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
    }

    /* 物理按键非阻塞扫�?????? (30ms 时间消抖, 松手触发) */
    {
        static uint8_t  k_prev   = 0;
        static uint8_t  k_armed  = 0;
        static uint32_t k_stable = 0;

        uint8_t k_raw = 0;
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_RESET) k_raw |= 0x01;  /* PA15=SW1 */
        if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_11) == GPIO_PIN_RESET) k_raw |= 0x02;  /* PC11=SW2 */
        if (HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_2)  == GPIO_PIN_RESET) k_raw |= 0x04;  /* PD2=SW3 */
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4)  == GPIO_PIN_RESET) k_raw |= 0x08;  /* PB4=SW4 */

        if (k_raw != k_prev)
        {
            k_prev   = k_raw;
            k_stable = HAL_GetTick();
        }

        if (HAL_GetTick() - k_stable >= 30)   /* 30ms 稳定 �?????? 消抖完成 */
        {
            if (k_raw && !k_armed)
            {
                k_armed = 1;
                if      (k_raw & 0x01) { rx_buf[0] = 'u'; rx_buf[1] = '\0'; cmd_ready = 1; printf("[BTN] u\r\n"); }
                else if (k_raw & 0x02) { rx_buf[0] = 'd'; rx_buf[1] = '\0'; cmd_ready = 1; printf("[BTN] d\r\n"); }
                else if (k_raw & 0x04) { rx_buf[0] = 'e'; rx_buf[1] = '\0'; cmd_ready = 1; printf("[BTN] e\r\n"); }
                else if (k_raw & 0x08) { rx_buf[0] = 'b'; rx_buf[1] = '\0'; cmd_ready = 1; printf("[BTN] b\r\n"); }
            }
            else if (!k_raw)
            {
                k_armed = 0;
            }
        }
    }

	    /* sensor data: per-page read */

    /* ---- ESP8266 AP 模式 ---- */
    {
        static uint32_t esp_retry = 0;
        static uint8_t esp_started_once = 0;

        if (!esp_ready)
        {
            if (!esp_started_once || HAL_GetTick() - esp_retry >= 10000)
            {
                esp_started_once = 1;
                esp_retry = HAL_GetTick();
                ESP8266_InitStart();
            }
            {
                uint8_t esp_r = ESP8266_InitPoll();
                if (esp_r == 1) esp_ready = 1;
            }
        }

        if (esp_ready)
        {
            static const char *pages[] = {
                "Welcome","Menu","Feed","Light","Temp","Pump","",
                "FeedSub","FeedTrain","ManualFeed","TrainSet","Auto"
            };
            g_esp_temp  = temp_current;
            g_esp_tds   = pump_tds;
            g_esp_light = light_led;
            g_esp_lmode = light_mode;
            g_esp_heater= heater_on;
			g_esp_run   = (feed_running || train_running || auto_running || pump_running) ? 1 : 0;
            g_esp_pump  = pump_running;
            if (current_page < sizeof(pages)/sizeof(pages[0]))
                strncpy(g_esp_page, pages[current_page], sizeof(g_esp_page)-1);
            ESP8266_Process();

            if (ESP8266_CmdAvailable() && !cmd_ready) {
                char c = ESP8266_GetCmd();
                /* printf("[FWD] c=%c to rx_buf\r\n", c); */
                rx_buf[0] = c;
                rx_buf[1] = '\0';
                cmd_ready = 1;
                cmd_from_remote = 1;
            }
        }
    }

    /* 电机/喂食/训练/自动 状�?�机 */
    Feed_Poll();


    /* 语音命令处理 */
    if (asr_cmd_ready)
    {
        asr_cmd_ready = 0;
        ASR_ProcessCommand();
    }

    /* 200ms animation frame */
    {
        if (HAL_GetTick() - anim_tick >= 200)
        {
            anim_tick = HAL_GetTick();
            anim_frame = (anim_frame + 1) % 5;
        }
    }

    /* Intro: after 3s show real page */
    if (anim_intro && HAL_GetTick() - anim_intro_tick >= 3000)
    {
        anim_intro = 0;
        temp_sample_tick = HAL_GetTick();   // 重置温度读取计时，让用户先操�????????
        OLED_ShowPage();
    }

    /* Refresh during intro */
    if (anim_intro)
    {
        static uint32_t intro_refresh = 0;
        if (HAL_GetTick() - intro_refresh >= 200)
        {
            intro_refresh = HAL_GetTick();
            OLED_ShowPage();
        }
    }

    /* OLED 看门�????????: 超过5秒未刷新则强制刷�???????? */
    if (HAL_GetTick() - oled_watchdog >= 5000)
    {
        oled_watchdog = HAL_GetTick();
        OLED_ShowPage();
    }

    /* 闪烁计时 (500ms 翻转) */
    {
        static uint32_t blink_tick = 0;
        if (HAL_GetTick() - blink_tick >= 500)
        {
            blink_tick = HAL_GetTick();
            feed_blink = !feed_blink;
            train_blink = !train_blink;
            auto_blink  = !auto_blink;
            light_blink = !light_blink;
            temp_blink  = !temp_blink;
            pump_blink  = !pump_blink;
            if ((current_page == PAGE_FEED_MANUAL && feed_editing) ||
                (current_page == PAGE_TRAIN_SETTING && train_editing) ||
                (current_page == PAGE_AUTO && auto_editing) ||
                (current_page == PAGE_LIGHT && light_edit) ||
                (current_page == PAGE_TEMP && temp_editing) ||
                (current_page == PAGE_PUMP && pump_sub && pump_running) ||
                (current_page == PAGE_FEED_SUBMENU && (train_running || feed_running || auto_running)))
                OLED_ShowPage();
        }
    }


    /* LIGHT 自动补光控制 */
    Light_Poll();

    /* TEMP: 非阻�?????? DS18B20 采样 + 10s 均�?? */
    Temp_Poll();

    /* TDS 定期采样 (�?????? 5s) */
    {
        static uint32_t tds_tick = 0;
        if (HAL_GetTick() - tds_tick >= 5000)
        {
            tds_tick = HAL_GetTick();
                pump_tds = TDS_ReadPPM_T(temp_current);
        }
    }

    /* 传感器串口打�??? */
    {
        static uint32_t sensor_printf_tick = 0;
        if (HAL_GetTick() - sensor_printf_tick >= 5000)
        {
            sensor_printf_tick = HAL_GetTick();
            printf("[Sensor] TDS=%.1f ppm (ADC=%u,%.2fV) Temp=%.1f C Light=%u(%u%%)\r\n",
                   pump_tds, tds_raw_adc, tds_voltage, temp_current,
                   LightSensor_Read(), light_now);
        }
    }

    /* 串口命令处理: e=enter u=up d=down b=back */
    /* 动画播放期间仅允�???????? b=back 中止 */
    if (cmd_ready)
    {
        char c = rx_buf[0];
        uint8_t was_remote = cmd_from_remote;   /* save before cleared */

        printf("[CMD] c=%c pg=%u intro=%u rem=%u\r\n", c, current_page, anim_intro, was_remote);

        /* 动画�????????: �???????? b 有效, 中止动画 */
        if (anim_intro)
        {
            cmd_ready = 0;
            if (c == 'b')
            {
                anim_intro = 0;
                temp_sample_tick = HAL_GetTick();
                /* �????????回菜�???????? */
                if (current_page == PAGE_FEED_SUBMENU ||
                    current_page == PAGE_LIGHT ||
                    current_page == PAGE_TEMP ||
                    current_page == PAGE_PUMP)
                {
                    if (current_page == PAGE_PUMP)
                    {
                        if (pump_running == 1) DirtyPump_Off();
                        if (pump_running == 2) CleanPump_Off();
                        pump_running = 0;
                        pump_sub = 0;
                    }
                    current_page = PAGE_MENU;
                    menu_index = 0;
                }
                OLED_ShowPage();
                //printf("OK:BACK\r\n");
            }
            if (c == 'b') {
                cmd_from_remote = 0;
                continue;   /* 物理/远程 b 均已处理, 跳过后续 */
            }
            if (cmd_from_remote) {
                anim_intro = 0;
                cmd_from_remote = 0;
            } else {
                cmd_from_remote = 0;
                continue;   /* 物理按键: 动画期间忽略 */
            }
        }
        cmd_from_remote = 0;

        cmd_ready = 0;

        /* 多字符行 (暂时禁用 ESP8266 转发)
        if (strlen(rx_buf) > 1)
        {
            //ESP8266_Send(rx_buf, strlen(rx_buf));
            //ESP8266_Send("\r\n", 2);
            continue;
        }
        */

        /* 强制刷新 (App 手动刷新按钮) */
        if (c == 'r') {
            ESP8266_ForcePublish();
        }

        /* UP */
        if (c == 'u')
        {
            if (feed_editing)
                { feed_time++; OLED_ShowPage(); }
            else if (train_editing == 1)
                { train_stages[train_stage][0]++; OLED_ShowPage(); }
            else if (train_editing == 2)
                { train_stages[train_stage][1]++; OLED_ShowPage(); }
            else if (train_editing == 3)
                { train_interval++; OLED_ShowPage(); }
            else if (auto_editing == 1)
                { auto_interval++; OLED_ShowPage(); }
            else if (auto_editing == 2)
                { auto_ao_time++; OLED_ShowPage(); }
            else if (light_edit)
                { if (light_led < 100) { light_led++; __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, (uint16_t)light_led * 10U); } OLED_ShowPage(); }
            else if (current_page == PAGE_LIGHT)
                { if (light_cursor > 0) light_cursor--; else light_cursor = 2; OLED_ShowPage(); }
            else if (temp_editing == 1)
                { if (temp_target_low < 40.0f) temp_target_low += 0.5f; OLED_ShowPage(); }
            else if (temp_editing == 2)
                { if (temp_target_high < 40.0f) temp_target_high += 0.5f; OLED_ShowPage(); }
            else if (current_page == PAGE_FEED_MANUAL)
                { if (feed_cursor > 0) { feed_cursor--; OLED_ShowPage(); } }
            else if (current_page == PAGE_TRAIN_SETTING)
                { if (train_setting_cursor > 0) { train_setting_cursor--; OLED_ShowPage(); } }
            else if (current_page == PAGE_FEED_TRAIN)
                { if (train_cursor > 0) { train_cursor--; OLED_ShowPage(); } }
            else if (current_page == PAGE_AUTO)
                { if (auto_cursor > 0) { auto_cursor--; OLED_ShowPage(); } }
            else if (current_page == PAGE_PUMP)
                { if (pump_cursor > 0) { pump_cursor--; OLED_ShowPage(); } }
            else if (menu_index > 0 &&
                     (current_page == PAGE_MENU || current_page == PAGE_FEED_SUBMENU))
                { menu_index--; OLED_ShowPage(); }
            //printf("OK:UP\r\n");
        }
        /* �????????�???????? DOWN �????????�???????? */
        else if (c == 'd')
        {
            if (feed_editing)
                { if (feed_time > 0) feed_time--; OLED_ShowPage(); }
            else if (train_editing == 1)
                { if (train_stages[train_stage][0] > 0) train_stages[train_stage][0]--; OLED_ShowPage(); }
            else if (train_editing == 2)
                { if (train_stages[train_stage][1] > 0) train_stages[train_stage][1]--; OLED_ShowPage(); }
            else if (train_editing == 3)
                { if (train_interval > 0) train_interval--; OLED_ShowPage(); }
            else if (auto_editing == 1)
                { if (auto_interval > 0) auto_interval--; OLED_ShowPage(); }
            else if (auto_editing == 2)
                { if (auto_ao_time > 0) auto_ao_time--; OLED_ShowPage(); }
            else if (light_edit)
                { if (light_led > 0) { light_led--; __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, (uint16_t)light_led * 10U); } OLED_ShowPage(); }
            else if (current_page == PAGE_LIGHT)
                { if (light_cursor < 2) light_cursor++; else light_cursor = 0; OLED_ShowPage(); }
            else if (temp_editing == 1)
                { if (temp_target_low > 10.0f) temp_target_low -= 0.5f; OLED_ShowPage(); }
            else if (temp_editing == 2)
                { if (temp_target_high > 10.0f) temp_target_high -= 0.5f; OLED_ShowPage(); }
            else if (current_page == PAGE_FEED_MANUAL)
                { if (feed_cursor < 2) { feed_cursor++; OLED_ShowPage(); } }
            else if (current_page == PAGE_TRAIN_SETTING)
                { if (train_setting_cursor < 3) { train_setting_cursor++; OLED_ShowPage(); } }
            else if (current_page == PAGE_FEED_TRAIN)
                { if (train_cursor < 2) { train_cursor++; OLED_ShowPage(); } }
            else if (current_page == PAGE_AUTO)
                { if (auto_cursor < 3) { auto_cursor++; OLED_ShowPage(); } }
            else if (current_page == PAGE_PUMP)
                {
                    uint8_t max = pump_sub ? 2 : 1;
                    if (pump_cursor < max) { pump_cursor++; OLED_ShowPage(); }
                }
            else
            {
                uint8_t max_idx = (current_page == PAGE_FEED_SUBMENU) ? 2 : 4;
                if ((current_page == PAGE_MENU || current_page == PAGE_FEED_SUBMENU)
                    && menu_index < max_idx)
                    { menu_index++; OLED_ShowPage(); }
            }
            //printf("OK:DOWN\r\n");
        }
        /* �????????�???????? ENTER �????????�???????? */
        else if (c == 'e')
        {
            g_esp_action = 1;
            /* �?????出编�????? */
            if (feed_editing)     { feed_editing = 0; feed_blink = 0; }
            else if (train_editing) { train_editing = 0; train_blink = 0; }
            else if (auto_editing)  { auto_editing = 0; auto_blink = 0; }
            /* MANUAL FEED */
            else if (current_page == PAGE_FEED_MANUAL)
            {
                if (feed_cursor == 0)
                    feed_editing = 1;
                else if (feed_cursor == 1)   /* START */
                {
                    if (feed_time == 0)
                        printf("AO Time=0, ignored\r\n");
                    else
                    {
                        FeedMotorA_Start();
                        feed_start_tick = HAL_GetTick();
                        feed_running = 1;
                        printf("Feed Start: %d min\r\n", feed_time);
                    }
                }
                else if (feed_cursor == 2)   /* END */
                {
                    feed_running = 0;
                    FeedMotorA_Stop();
                    ASR_SendResponse(ASR_RSP_FEED_DONE);
                    printf("Feed Stopped\r\n");
                }
            }
            /* TRAIN MODE 菜单 */
            else if (current_page == PAGE_FEED_TRAIN)
            {
                if (train_cursor == 0)       /* FEED SETTING */
                    {
                        current_page = PAGE_TRAIN_SETTING;
                        train_setting_cursor = 0;
                        train_editing = 0;
                        if (train_exec_stage < 7)
                            train_stage = train_exec_stage;
                    }
                else if (train_cursor == 1)  /* START */
                    { train_running = 1; train_exec_stage = 0; train_motor_active = 0; printf("Training Started\r\n"); }
                else if (train_cursor == 2)  /* END */
                {
                    train_running = 0; train_motor_active = 0;
                    FeedMotorA_Stop();
                    FeedMotorB_Stop();
                    ASR_SendResponse(ASR_RSP_TRAIN_DONE);
                    printf("Training Stopped\r\n");
                }
            }
            /* TRAIN SETTING */
            else if (current_page == PAGE_TRAIN_SETTING)
            {
                if (train_setting_cursor == 0)       /* BO */
                    train_editing = 1;
                else if (train_setting_cursor == 1)  /* AO */
                    train_editing = 2;
                else if (train_setting_cursor == 2)  /* Interval */
                    train_editing = 3;
                else if (train_setting_cursor == 3)  /* CONTINUE */
                {
                    if (train_stage < 6)
                        { train_stage++; train_setting_cursor = 0; printf("Stage %d/7\r\n", train_stage + 1); }
                    else
                        { current_page = PAGE_FEED_TRAIN; train_cursor = 0; printf("Training Setting Complete\r\n"); }
                }
            }
            /* AUTO MODE */
            else if (current_page == PAGE_AUTO)
            {
                if (auto_cursor == 0)        /* Interval */
                    auto_editing = 1;
                else if (auto_cursor == 1)   /* AO Time */
                    auto_editing = 2;
                else if (auto_cursor == 2)   /* START */
                {
                    auto_running = 1;
                    auto_phase = 0;
                    auto_tick = 0;             /* 立刻触发首次投喂 */
                    printf("Auto Mode Started\r\n");
                }
                else if (auto_cursor == 3)   /* END */
                {
                    auto_running = 0;
                    FeedMotorA_Stop();
                    printf("Auto Stopped\r\n");
                }
            }
            /* LIGHT */
            else if (current_page == PAGE_LIGHT)
            {
                if (light_cursor == 2)   /* Mode row */
                {
                    light_mode = !light_mode;
                    if (light_mode == LIGHT_AUTO)
                        Light_ReSample();
                    else
                        light_edit = 0;
                }
                else if (light_cursor == 0 && light_mode == LIGHT_MANUAL)   /* LED row */
                {
                    light_edit = !light_edit;
                    if (!light_edit)
                        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, (uint16_t)light_led * 10U);
                }
                OLED_ShowPage();
            }
            /* TEMP */
            else if (current_page == PAGE_TEMP)
            {
                if (temp_editing == 0)
                {
                    /* 手动切换加热�?????? */
                    if (heater_on) { Heater_Off(); heater_on = 0; }
                    else           { Heater_On();  heater_on = 1; }
                }
                else if (temp_editing == 1)
                    temp_editing = 2;
                else
                    temp_editing = 0;
                OLED_ShowPage();
            }
            /* PUMP */
            else if (current_page == PAGE_PUMP)
            {
                if (pump_sub == 0)
                {
                    if (pump_cursor == 1)
                    {
                        pump_sub = 1;
                        pump_cursor = 0;
                    }
                }
                else
                {
                    if (pump_cursor == 0)       /* P1out */
                    {
                        if (pump_running == 1)
                        {
                            DirtyPump_Off();
                            pump_running = 0;
                        }
                        else
                        {
                            if (pump_running == 2) { CleanPump_Off(); }
                            DirtyPump_On();
                            pump_running = 1;
                            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
                        }
                    }
                    else if (pump_cursor == 1)  /* P2into */
                    {
                        if (pump_running == 2)
                        {
                            CleanPump_Off();
                            pump_running = 0;
                        }
                        else
                        {
                            if (pump_running == 1) { DirtyPump_Off(); }
                            CleanPump_On();
                            pump_running = 2;
                            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
                        }
                    }
                    else                        /* End */
                    {
                        if (pump_running == 1) DirtyPump_Off();
                        if (pump_running == 2) CleanPump_Off();
                        pump_running = 0;
                        if (!motor_a_run && !motor_b_run && !feed_running && !train_motor_active
                            && !(auto_running && auto_phase == 1))
                            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
                    }
                }
                OLED_ShowPage();
            }
            else if (current_page == PAGE_WELCOME)
                { current_page = PAGE_MENU; menu_index = 0; printf("OK:MENU\r\n"); }
            else if (current_page == PAGE_MENU)
            {
                switch (menu_index)
                {
                    case 0: current_page = PAGE_WELCOME;      break;
                    case 1: current_page = PAGE_FEED_SUBMENU;
                        anim_intro = 1; anim_intro_tick = HAL_GetTick();
                        anim_frame = 0;                break;
                    case 2: current_page = PAGE_LIGHT;
                            anim_intro = 1; anim_intro_tick = HAL_GetTick();
                            anim_frame = 0;                break;
                    case 3: current_page = PAGE_TEMP;
                            anim_intro = 1; anim_intro_tick = HAL_GetTick();
                            anim_frame = 0;                break;
                    case 4: current_page = PAGE_PUMP;
                            pump_sub = 0; pump_cursor = 0;
                            pump_running = 0;
							pump_tds = TDS_ReadPPM_T(temp_current);
                            anim_intro = 1; anim_intro_tick = HAL_GetTick();
                            anim_frame = 0;                break;
                }
                menu_index = 0;
            }
            else if (current_page == PAGE_FEED_SUBMENU)
            {
                if (menu_index == 0)
                    { current_page = PAGE_FEED_TRAIN; train_cursor = 0; }
                else if (menu_index == 1)
                    { current_page = PAGE_FEED_MANUAL; feed_cursor = 0; feed_editing = 0; }
                else
                    { current_page = PAGE_AUTO; auto_cursor = 0; auto_editing = 0; }
            }
            OLED_ShowPage();
            //printf("OK:ENTER\r\n");
        }
        /* �????????�???????? BACK �????????�???????? */
        else if (c == 'b')
        {
            if (feed_editing)  { feed_editing = 0; feed_blink = 0; }
            if (train_editing) { train_editing = 0; train_blink = 0; }
            if (auto_editing)  { auto_editing = 0; auto_blink = 0; }
            if (light_edit)    { light_edit = 0;    light_blink = 0; }
            if (temp_editing)  { temp_editing = 0; temp_blink = 0; }
            if (current_page == PAGE_TRAIN_SETTING)
                { current_page = PAGE_FEED_TRAIN; train_cursor = 0; }
            else if (current_page == PAGE_FEED_TRAIN ||
                     current_page == PAGE_FEED_MANUAL ||
                     current_page == PAGE_AUTO)
                { current_page = PAGE_FEED_SUBMENU; menu_index = 0; }
            else if (current_page == PAGE_LIGHT)
                {
                    current_page = PAGE_MENU; menu_index = 0;
                }
            else if (current_page == PAGE_TEMP)
                {
                    current_page = PAGE_MENU; menu_index = 0;
                }
            else if (current_page == PAGE_PUMP)
                {
                    if (pump_sub)
                    {
                        pump_sub = 0;
                        pump_cursor = 1;
                    }
                    else
                    {
                        if (pump_running == 1) DirtyPump_Off();
                        if (pump_running == 2) CleanPump_Off();
                        pump_running = 0;
                        if (!motor_a_run && !motor_b_run && !feed_running && !train_motor_active
                            && !(auto_running && auto_phase == 1))
                            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
                        current_page = PAGE_MENU; menu_index = 0;
                    }
                }
            else
                { current_page = PAGE_MENU; menu_index = 0; }
            OLED_ShowPage();
            //printf("OK:BACK\r\n");
        }
        /* else
        {
            //ESP8266_Send(rx_buf, strlen(rx_buf));
            //ESP8266_Send("\r\n", 2);
        }
        */
        if (esp_ready) {
            ESP8266_ForcePublish();
            ESP8266_Process();              /* push JSON now */
        }
    }

    /* 主循环限�??????: 不低�?????? 10ms 间隔, 但用非阻塞方�?????? */
    {
        static uint32_t next_loop = 0;
        if ((int32_t)(HAL_GetTick() - next_loop) < 0) continue;
        next_loop = HAL_GetTick() + 10;
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* �???�??? ASRPRO 语音模块 �???�??? */
volatile uint8_t asr_cmd_ready = 0;
static  uint8_t asr_cmd_buf;

void USART3_IRQHandler(void)
{
    uint32_t sr = USART3->SR;
    if (sr & USART_SR_RXNE)
    {
        asr_cmd_buf = (uint8_t)USART3->DR;
        asr_cmd_ready = 1;
    }
    if (sr & (USART_SR_ORE | USART_SR_FE | USART_SR_NE))
        (void)USART3->DR;
}

void ASR_SendResponse(uint8_t code)
{
    uint8_t buf[2] = {0xAA, code};
    HAL_UART_Transmit(&huart3, buf, 2, 100);
}

void ASR_Init(void)
{
    SET_BIT(USART3->CR1, USART_CR1_RXNEIE);
    HAL_NVIC_SetPriority(USART3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
}

void ASR_ProcessCommand(void)
{
    uint8_t cmd = asr_cmd_buf;
    switch (cmd)
    {
    case 0x01: /* CHECK_LIGHT -- �???????? OLED 显示�???????? */
    {
        if (light_mode == LIGHT_MANUAL)
        {
            ASR_SendResponse(0x06);   /* 手动模�?????: 直接回复正�?? */
        }
        else if (light_now <= 75)     /* Bright(<25) or OK(25~75) */
        {
            ASR_SendResponse(0x06);
        }
        else                          /* Low(>75) �??? �???????? */
        {
            ASR_SendResponse(0x07);
            HAL_Delay(2000);          /* �????????播完第一条再发第二�?? */
            light_led = 100;
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, (uint16_t)light_led * 10U);
            ASR_SendResponse(0x08);
        }
        break;
    }
    case 0x02: /* CHECK_WATER -- �???????? OLED Condition */
    {
        if (pump_tds <= TDS_FAIR)     /* Good(<300) or Fair(300~600) �??? 0x09 */
            ASR_SendResponse(0x09);
        else                          /* Dirty(600~1000) or Poor(>1000) �??? 0x0A */
            ASR_SendResponse(0x0A);
        break;
    }
    case 0x03: /* CHECK_TEMP -- �???????? OLED Status */
    {
        if (temp_current >= TEMP_LOW && temp_current <= TEMP_HIGH)   /* OK */
        {
            ASR_SendResponse(0x0B);
        }
        else if (temp_current > TEMP_HIGH)   /* High �??? 当�?????处理 */
        {
            ASR_SendResponse(0x0B);
        }
        else   /* Low �??? �???????? */
        {
            ASR_SendResponse(0x0C);
            HAL_Delay(2000);          /* �????????播完第一条再发第二�?? */
            Heater_On();
            heater_on = 1;
            ASR_SendResponse(0x0D);
        }
        break;
    }
    case 0x04: /* TRAIN_FEED */
        ASR_SendResponse(0x10);
        HAL_Delay(5000);             /* 5�????????自动启动训饵 */
        train_running      = 1;
        train_exec_stage   = 0;
        train_motor_active = 0;
        break;
    case 0x05: /* NORMAL_FEED */
        ASR_SendResponse(0x0E);
        feed_time       = 1;         /* 1分钟 */
        FeedMotorA_Start();
        feed_running    = 1;
        feed_start_tick = HAL_GetTick();
        break;
    default:
        break;
    }
}

/* USART1 接收回调 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    (void)huart;
}

/* printf 重定�??????? */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
    while(!__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TXE));
    huart1.Instance->DR = (uint8_t)ch;
    return ch;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
