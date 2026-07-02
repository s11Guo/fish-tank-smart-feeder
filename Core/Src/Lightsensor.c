#include "lightsensor.h"
#include "adc.h"

void LightSensor_Init(ADC_HandleTypeDef* hadc)
{
    (void)hadc;
}

/* DMA 后台连续转换, Rank1=CH10=PC0 */
uint16_t LightSensor_Read(void)
{
    return adc_dma_buf[0];
}

