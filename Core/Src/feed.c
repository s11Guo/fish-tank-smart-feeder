#include "app.h"
#include "feed.h"
#include "ui.h"
#include "pump.h"
#include <stdio.h>

/* 直接发送 ASR 语音回复 */
static void feed_asr_send(uint8_t code)
{
    extern UART_HandleTypeDef huart3;
    uint8_t buf[2] = {0xAA, code};
    HAL_UART_Transmit(&huart3, buf, 2, 100);
}

/* ── 脉冲状态 ── */
static uint8_t  pulse_a_active = 0;
static uint8_t  pulse_a_on     = 0;
static uint32_t pulse_a_tick   = 0;

static uint8_t  pulse_b_active = 0;
static uint8_t  pulse_b_on     = 0;
static uint32_t pulse_b_tick   = 0;

/* ── 脉冲轮询 ── */
static void pulse_poll(void)
{
    uint32_t now = HAL_GetTick();

    if (pulse_a_active)
    {
        uint32_t e = now - pulse_a_tick;
        if (pulse_a_on)
        {
            if (e >= PULSE_ON_MS)
            {
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
                pulse_a_on   = 0;
                pulse_a_tick = now;
            }
        }
        else
        {
            if (e >= PULSE_OFF_MS)
            {
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, MOTOR_PWM_DUTY);
                pulse_a_on   = 1;
                pulse_a_tick = now;
            }
        }
    }

    if (pulse_b_active)
    {
        uint32_t e = now - pulse_b_tick;
        if (pulse_b_on)
        {
            if (e >= PULSE_ON_MS)
            {
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
                pulse_b_on   = 0;
                pulse_b_tick = now;
            }
        }
        else
        {
            if (e >= PULSE_OFF_MS)
            {
                __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, MOTOR_PWM_DUTY);
                pulse_b_on   = 1;
                pulse_b_tick = now;
            }
        }
    }
}

/* ── 启停 ── */
void FeedMotorA_Start(void)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, MOTOR_PWM_DUTY);
    motor_a_run    = 1;
    motor_a_tick   = HAL_GetTick();
    pulse_a_active = 1;
    pulse_a_on     = 1;
    pulse_a_tick   = motor_a_tick;
}

void FeedMotorB_Start(void)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, MOTOR_PWM_DUTY);
    motor_b_run    = 1;
    motor_b_tick   = HAL_GetTick();
    pulse_b_active = 1;
    pulse_b_on     = 1;
    pulse_b_tick   = motor_b_tick;
}

void FeedMotorA_Stop(void)
{
    pulse_a_active = 0;
    pulse_a_on     = 0;
    motor_a_run    = 0;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0);
}

void FeedMotorB_Stop(void)
{
    pulse_b_active = 0;
    pulse_b_on     = 0;
    motor_b_run    = 0;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, 0);
}

void Feed_Poll(void)
{
    /* 脉冲轮询 (每个循环都跑) */
    pulse_poll();

    /* 所有电机和水泵都停了才关 STBY */
    if (!pulse_a_active && !pulse_b_active && !feed_running && !pump_running
        && !train_motor_active && !(auto_running && auto_phase == 1))
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
    }

    /* MANUAL FEED */
    if (feed_running && (HAL_GetTick() - feed_start_tick >= (uint32_t)feed_time * 60000))
    {
        feed_running = 0;
        FeedMotorA_Stop();
        feed_asr_send(0x0F);   /* FEED_DONE */
        OLED_ShowPage();
    }

    /* TRAIN 执行状态机 */
    if (train_running)
    {
        if (!train_motor_active)
        {
            if (train_exec_stage >= 7)
            {
                train_running = 0;
                feed_asr_send(0x11);   /* TRAIN_DONE */
                OLED_ShowPage();
            }
            else
            {
                train_exec_phase   = 0;
                train_bo_done      = 0;
                train_ao_done      = 0;
                train_motor_active = 1;

                if (train_stages[train_exec_stage][0] > 0)
                    FeedMotorB_Start();
                else
                    train_bo_done = 1;

                if (train_stages[train_exec_stage][1] > 0)
                    FeedMotorA_Start();
                else
                    train_ao_done = 1;

                train_exec_tick = HAL_GetTick();
            }
        }
        else
        {
            uint32_t elapsed = HAL_GetTick() - train_exec_tick;

            if (train_exec_phase == 0)
            {
                if (!train_bo_done)
                {
                    if (elapsed >= (uint32_t)train_stages[train_exec_stage][0] * 30000)
                    {
                        FeedMotorB_Stop();
                        train_bo_done = 1;
                    }
                }
                if (!train_ao_done)
                {
                    if (elapsed >= (uint32_t)train_stages[train_exec_stage][1] * 30000)
                    {
                        FeedMotorA_Stop();
                        train_ao_done = 1;
                    }
                }
                if (train_bo_done && train_ao_done)
                {
                    train_exec_phase = 1;
                    train_exec_tick  = HAL_GetTick();
                }
            }
            else
            {
                if (elapsed >= (uint32_t)train_interval * 60000)
                {
                    train_exec_stage++;
                    train_motor_active = 0;
                }
            }
        }
    }

    /* AUTO 执行状态机 */
    if (auto_running)
    {
        if (auto_phase == 0)
        {
            if (HAL_GetTick() - auto_tick >= (uint32_t)auto_interval * 60000)
            {
                if (auto_ao_time > 0)
                {
                    FeedMotorA_Start();
                }
                auto_phase = 1;
                auto_tick  = HAL_GetTick();
            }
        }
        else
        {
            if (HAL_GetTick() - auto_tick >= (uint32_t)auto_ao_time * 60000)
            {
                FeedMotorA_Stop();
                auto_phase = 0;
                auto_tick  = HAL_GetTick();
                OLED_ShowPage();
            }
        }
    }
}
