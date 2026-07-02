#include "oled.h"
#include "font.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

extern I2C_HandleTypeDef hi2c1;
uint8_t OLED_Addr;

/* 帧缓冲: 8页 x 128列 */
static uint8_t OLED_Buffer[8][128];

/* ── 底层 I2C ── */

HAL_StatusTypeDef OLED_WriteCmd(uint8_t cmd)
{
    uint8_t data[2] = {0x00, cmd};
    HAL_StatusTypeDef rc = HAL_I2C_Master_Transmit(&hi2c1, OLED_Addr, data, 2, 200);
    if (rc != HAL_OK) printf("[OLED] cmd 0x%02X err=%d\r\n", cmd, rc);
    return rc;
}

static HAL_StatusTypeDef OLED_WriteCmdBurst(uint8_t *pCmd, uint16_t len)
{
    uint8_t buf[16];
    buf[0] = 0x00;  /* Co=0, D/C#=0 → 后续字节为命令 */
    memcpy(buf + 1, pCmd, len);
    HAL_StatusTypeDef rc = HAL_I2C_Master_Transmit(&hi2c1, OLED_Addr, buf, len + 1, 200);
    if (rc != HAL_OK) printf("[OLED] cmd burst err=%d\r\n", rc);
    return rc;
}

static HAL_StatusTypeDef OLED_WriteDataBurst(uint8_t *pData, uint16_t len)
{
    uint8_t buf[129];
    buf[0] = 0x40;  /* Co=0, D/C#=1 → 后续字节为数据 */
    memcpy(buf + 1, pData, len);
    HAL_StatusTypeDef rc = HAL_I2C_Master_Transmit(&hi2c1, OLED_Addr, buf, len + 1, 200);
    if (rc != HAL_OK) printf("[OLED] data burst err=%d\r\n", rc);
    return rc;
}

/* ── 帧缓冲操作 ── */

static void OLED_ClearBuffer(void)
{
    memset(OLED_Buffer, 0, sizeof(OLED_Buffer));
}

/* ── 初始化 ── */

void OLED_Init(void)
{
    HAL_Delay(100);

    OLED_WriteCmd(0xAE);        /* Display OFF */
    OLED_WriteCmd(0xA4);        /* 关全屏点亮 */
    OLED_WriteCmd(0xA6);        /* 正常显示 */
    OLED_WriteCmd(0xC8);        /* COM Scan: remapped */
    OLED_WriteCmd(0x40);        /* Start line 0 */
    OLED_WriteCmd(0x81); OLED_WriteCmd(0xFF);  /* Contrast max */
    OLED_WriteCmd(0xA1);        /* Segment remap */
    OLED_WriteCmd(0xA8); OLED_WriteCmd(0x3F);  /* Mux ratio 1/64 */
    OLED_WriteCmd(0xD3); OLED_WriteCmd(0x00);  /* Display offset 0 */
    OLED_WriteCmd(0xD5); OLED_WriteCmd(0xF0);  /* Clock divide */
    OLED_WriteCmd(0xD9); OLED_WriteCmd(0x22);  /* Pre-charge */
    OLED_WriteCmd(0xDA); OLED_WriteCmd(0x12);  /* COM pins */
    OLED_WriteCmd(0xDB); OLED_WriteCmd(0x35);  /* VCOMH */
    OLED_WriteCmd(0xAD); OLED_WriteCmd(0x8B);  /* DC-DC ON (SH1106) */
    OLED_WriteCmd(0xAF);        /* Display ON */
}

/* ── 清缓冲 ── */

void OLED_Clear(void)
{
    OLED_ClearBuffer();
}

void OLED_SetCursor(uint8_t x, uint8_t y)
{
    (void)x;
    (void)y;
}

/* ── 刷屏 ── */

void OLED_Refresh(void)
{
    uint8_t page;

    /* Keep display ON while refreshing. If I2C fails mid-frame, the panel
       should keep the previous image instead of staying black. */
    for (page = 0; page < 8; page++)
    {
        uint8_t cmds[3] = {0xB0 | page, 0x02, 0x10};
        if (OLED_WriteCmdBurst(cmds, 3) != HAL_OK) return;
        if (OLED_WriteDataBurst(&OLED_Buffer[page][0], 128) != HAL_OK) return;
        HAL_Delay(1);
    }
}

/* ── 绘制字符 ── */

void OLED_ShowChar(uint8_t x, uint8_t y, char chr, uint8_t size)
{
    uint8_t c = (uint8_t)chr - ' ';
    uint8_t i;

    if (size == 8)
    {
        if (y >= 8 || x + 6 > 128) return;
        for (i = 0; i < 6; i++)
            OLED_Buffer[y][x + i] = Font6x8[c][i];
    }
    else if (size == 16)
    {
        if (y + 1 >= 8 || x + 8 > 128) return;
        for (i = 0; i < 8; i++)
        {
            OLED_Buffer[y][x + i]     = Font8x16[c][i];
            OLED_Buffer[y + 1][x + i] = Font8x16[c][i + 8];
        }
    }
}

/* ── 绘制字符串 ── */

void OLED_ShowString(uint8_t x, uint8_t y, char *str, uint8_t size)
{
    uint8_t step = (size == 8) ? 6 : 8;
    uint8_t line_h = (size == 8) ? 1 : 2;

    while (*str)
    {
        if (x + step > 128)
        {
            x = 0;
            y += line_h;
            if (y >= 8) return;
        }
        OLED_ShowChar(x, y, *str, size);
        x += step;
        str++;
    }
}

/* ── 绘制位图 ── */

void OLED_DrawBMP(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                  const uint8_t BMP[])
{
    uint16_t width = x1 - x0 + 1;
    uint16_t height = y1 - y0 + 1;
    uint16_t pages = height / 8;
    uint16_t page;

    for (page = 0; page < pages; page++)
    {
        memcpy(&OLED_Buffer[(y0 / 8) + page][x0], &BMP[page * width], width);
    }
}
