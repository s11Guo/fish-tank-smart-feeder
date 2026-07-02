#include "app.h"
#include "light.h"
#include "Lightsensor.h"
#include "ui.h"

/*
 * Plan B auto control:
 *   Every 10s, kill LED for 100ms, sample pure ambient light,
 *   then decide LED level.  This avoids LED→sensor feedback loop.
 */

#define LIGHT_CYCLE_MS     10000U
#define LIGHT_OFF_MS       100U
#define LIGHT_ADC_FLOOR    500U
#define LIGHT_ADC_CEIL     2500U

static uint32_t light_tick   = 0;
static uint8_t  light_phase  = 0;       /* 0=LED-on/wait, 1=off/sampling */
static uint32_t sample_tick  = 0;

void Light_Poll(void)
{
    uint32_t now = HAL_GetTick();
    uint8_t display_changed = 0;

    if (light_mode == LIGHT_MANUAL)
    {
        if (!light_edit)
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, (uint16_t)light_led * 10U);
        return;
    }

    /* ── Auto mode ── */
    switch (light_phase)
    {
    case 0:   /* LED on, wait for next cycle */
        if (now - light_tick >= LIGHT_CYCLE_MS)
        {
            light_phase = 1;
            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);   /* kill LED */
            sample_tick = now;
        }
        break;

    case 1:   /* LED off, wait 100ms then sample ambient */
        if (now - sample_tick >= LIGHT_OFF_MS)
        {
            uint16_t raw = LightSensor_Read();
            if (raw < LIGHT_ADC_FLOOR) raw = LIGHT_ADC_FLOOR;
            if (raw > LIGHT_ADC_CEIL)  raw = LIGHT_ADC_CEIL;

            /* Map raw → light_now (0-100%) */
            light_now = (uint8_t)((uint32_t)(raw - LIGHT_ADC_FLOOR) * 100U
                                  / (LIGHT_ADC_CEIL - LIGHT_ADC_FLOOR));

            /* Map ambient to LED: bright→off, mid→proportional, dark→full */
            if (raw < 1000)
                light_led = 0;
            else if (raw < 2000)
                light_led = (uint8_t)((uint32_t)(raw - 1000) * 100U / 1000U);
            else
                light_led = 100;

            __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, (uint16_t)light_led * 10U);
            light_phase = 0;
            light_tick  = now;
            display_changed = 1;
        }
        break;
    }

    if (display_changed && current_page == PAGE_LIGHT)
        OLED_ShowPage();
}

/* Called when switching back to Auto from Manual: force re-sample */
void Light_ReSample(void)
{
    light_phase = 1;
    light_tick  = HAL_GetTick() - LIGHT_CYCLE_MS;   /* force immediate cycle */
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
    sample_tick = HAL_GetTick();
    light_now   = 0;
}
