#include "app.h"
#include "temp.h"
#include "ds18b20.h"
#include "ui.h"
#include <stdio.h>

void Temp_Poll(void)
{
    /* 10s 窗口: 取平均值更新 temp_current */
    if (HAL_GetTick() - temp_window_start >= 10000)
    {
        if (temp_cnt > 0)
            temp_current = temp_sum / (float)temp_cnt;
        temp_sum = 0.0f;
        temp_cnt = 0;
        temp_window_start = HAL_GetTick();

        if (current_page == PAGE_TEMP && !anim_intro)
            OLED_ShowPage();
    }

    /* 每 2s 启动一次转换 (非阻塞) */
    if (HAL_GetTick() - temp_sample_tick >= 2000)
    {
        temp_sample_tick = HAL_GetTick();
            DS18B20_StartSample();
    }

    /* 轮询转换结果, 累加 */
    {
        float val;
        if (DS18B20_Poll(&val))
        {
            temp_sum += val;
            temp_cnt++;
        }
    }

    /* TEMP 页面调试打印 */
    if (current_page == PAGE_TEMP && !anim_intro)
    {
        static uint32_t temp_printf_tick = 0;
        if (HAL_GetTick() - temp_printf_tick >= 2000)
        {
            temp_printf_tick = HAL_GetTick();
            //printf("Temp: Now=%.1f Low=%.1f High=%.1f Heater=%s\r\n",
            //       temp_current, temp_target_low, temp_target_high,
            //       heater_on ? "ON" : "OFF");
        }
    }
}
