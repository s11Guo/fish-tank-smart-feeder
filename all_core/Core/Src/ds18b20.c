#include "ds18b20.h"

#define DS18_PORT GPIOA
#define DS18_PIN  GPIO_PIN_12

/* DWT 微秒延时 */
static void Delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (HAL_RCC_GetHCLKFreq() / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}

static void DS18_Output(void)
{
    GPIO_InitTypeDef s = {0};
    s.Pin   = DS18_PIN;
    s.Mode  = GPIO_MODE_OUTPUT_OD;
    s.Pull  = GPIO_NOPULL;
    s.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DS18_PORT, &s);
}

static void DS18_Input(void)
{
    GPIO_InitTypeDef s = {0};
    s.Pin   = DS18_PIN;
    s.Mode  = GPIO_MODE_INPUT;
    s.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(DS18_PORT, &s);
}

/* 复位脉冲: 750us 低, 释放 */
static void DS18_Rst(void)
{
    DS18_Output();
    HAL_GPIO_WritePin(DS18_PORT, DS18_PIN, GPIO_PIN_RESET);
    Delay_us(750);
    HAL_GPIO_WritePin(DS18_PORT, DS18_PIN, GPIO_PIN_SET);
    Delay_us(15);
}

/* 检测存在脉冲: 0=存在, 1=不存在 */
static uint8_t DS18_Check(void)
{
    uint8_t retry = 0;
    DS18_Input();
    while (HAL_GPIO_ReadPin(DS18_PORT, DS18_PIN) && retry < 200)
    {
        retry++;
        Delay_us(1);
    }
    if (retry >= 200) return 1;

    retry = 0;
    while (!HAL_GPIO_ReadPin(DS18_PORT, DS18_PIN) && retry < 240)
    {
        retry++;
        Delay_us(1);
    }
    if (retry >= 240) return 1;

    return 0;
}

/* 复位+检测, 返回 1=成功 0=失败 */
static uint8_t DS18_Reset(void)
{
    DS18_Rst();
    return !DS18_Check();
}

static uint8_t DS18_ReadBit(void)
{
    uint8_t data;
    DS18_Output();
    HAL_GPIO_WritePin(DS18_PORT, DS18_PIN, GPIO_PIN_RESET);
    Delay_us(2);
    HAL_GPIO_WritePin(DS18_PORT, DS18_PIN, GPIO_PIN_SET);
    DS18_Input();
    Delay_us(12);
    data = HAL_GPIO_ReadPin(DS18_PORT, DS18_PIN) ? 1 : 0;
    Delay_us(50);
    return data;
}

static uint8_t DS18_ReadByte(void)
{
    uint8_t i, dat = 0;
    uint8_t j;
    for (i = 0; i < 8; i++)
    {
        j = DS18_ReadBit();
        dat = (j << 7) | (dat >> 1);
    }
    return dat;
}

static void DS18_WriteByte(uint8_t dat)
{
    uint8_t j;
    uint8_t testb;
    DS18_Output();
    for (j = 0; j < 8; j++)
    {
        testb = dat & 0x01;
        dat >>= 1;
        if (testb)
        {
            HAL_GPIO_WritePin(DS18_PORT, DS18_PIN, GPIO_PIN_RESET);
            Delay_us(2);
            HAL_GPIO_WritePin(DS18_PORT, DS18_PIN, GPIO_PIN_SET);
            Delay_us(60);
        }
        else
        {
            HAL_GPIO_WritePin(DS18_PORT, DS18_PIN, GPIO_PIN_RESET);
            Delay_us(60);
            HAL_GPIO_WritePin(DS18_PORT, DS18_PIN, GPIO_PIN_SET);
            Delay_us(2);
        }
    }
}

uint8_t DS18B20_Init(void)
{
    return DS18_Reset();
}

/* ── 非阻塞温度测量状态机 ── */

static ds18_state_t ds18_state = DS18_IDLE;
static uint32_t    ds18_tick   = 0;
static float       last_temp   = 0.0f;

/* 启动一次转换 (非阻塞) */
void DS18B20_StartSample(void)
{
    if (ds18_state != DS18_IDLE) return;

    __disable_irq();
    DS18_Rst();
    if (DS18_Check()) {
        __enable_irq();
        return;
    }
    DS18_WriteByte(0xCC);  /* Skip ROM */
    DS18_WriteByte(0x44);  /* Convert T */
    __enable_irq();
    ds18_state = DS18_WAIT_CONVERT;
    ds18_tick  = HAL_GetTick();
}

/* 轮询转换结果: 1=完成(out 写入温度), 0=等待中 */
uint8_t DS18B20_Poll(float *out)
{
    uint8_t temp_sign;
    uint8_t TL, TH;
    short   raw;

    if (ds18_state != DS18_WAIT_CONVERT) return 0;

    /* DS18B20 12-bit 转换最长 750ms */
    if (HAL_GetTick() - ds18_tick < 750)
        return 0;

    ds18_state = DS18_IDLE;

    __disable_irq();
    DS18_Rst();
    if (DS18_Check()) { __enable_irq(); return 0; }

    DS18_WriteByte(0xCC);  /* Skip ROM */
    DS18_WriteByte(0xBE);  /* Read Scratchpad */

    TL = DS18_ReadByte();
    TH = DS18_ReadByte();
    __enable_irq();

    /* 负温度处理 */
    if (TH > 7)
    {
        TH = ~TH;
        TL = ~TL;
        temp_sign = 0;  /* 负 */
    }
    else
    {
        temp_sign = 1;  /* 正 */
    }

    raw = TH;
    raw <<= 8;
    raw += TL;
    last_temp = raw / 16.0f;
    if (!temp_sign) last_temp = -last_temp;

    if (last_temp > -55.0f && last_temp < 125.0f)
        *out = last_temp;
    else
        *out = 0.0f;

    return 1;
}
