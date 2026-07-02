#include "tds.h"
#include "adc.h"

#define VREF        3.3f
#define ADC_MAX     4095.0f

/* TDS 线性校准因子 (可调)
   0.48V ≈ 250ppm 时因子=520
   调到你的探头实际值即可 */
#define TDS_FACTOR  520.0f

void TDS_Init(void)
{
}

uint16_t tds_raw_adc = 0;
float    tds_voltage  = 0.0f;

/* 从 DMA 缓冲区读取 (DMA 后台连续转换, Rank2=CH1=PA1) */
uint16_t TDS_ReadADC(void)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 50; i++)
    {
        sum += adc_dma_buf[1];
        HAL_Delay(1);
    }
    tds_raw_adc = sum / 50;
    tds_voltage = tds_raw_adc * VREF / ADC_MAX;
    return tds_raw_adc;
}

/* 电压值 */
float TDS_ReadVoltage(void)
{
    return TDS_ReadADC() * VREF / ADC_MAX;
}

/* ppm 计算: DFRobot多项式 + 温度补偿 */
static float calc_ppm(float voltage, float temp)
{
    float coeff = 1.0f + 0.02f * (temp - 25.0f);
    float v_comp = voltage / coeff;
    float ppm = (133.42f * v_comp * v_comp * v_comp
               - 255.86f * v_comp * v_comp
               + 857.39f * v_comp) * 0.5f;

    if (ppm < 0.0f)     ppm = 0.0f;
    if (ppm > 3000.0f)  ppm = 3000.0f;
    return ppm;
}

/* ppm (已废弃) */
float TDS_ReadPPM(void)
{
    return 0.0f;
}

/* ppm (外部传入温度) */
float TDS_ReadPPM_T(float temperature)
{
    return calc_ppm(TDS_ReadVoltage(), temperature);
}
