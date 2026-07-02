#include "app.h"
#include "ui.h"
#include "oled.h"
#include "img.h"
#include <stdio.h>
#include <string.h>

void OLED_ShowPage(void)
{
    char buf[20];

    OLED_Clear();

    if (anim_intro)
    {
        const uint8_t (*anim)[128] = NULL;
        switch (current_page)
        {
            case PAGE_FEED_SUBMENU:
                anim = feed_anim;
                OLED_ShowString(32, 0, "FEED", 16);
                break;
            case PAGE_LIGHT:
                anim = light_anim;
                OLED_ShowString(32, 0, "LIGHT", 16);
                break;
            case PAGE_TEMP:
                anim = heat_anim;
                OLED_ShowString(32, 0, "HEATER", 16);
                break;
            case PAGE_PUMP:
                anim = pump_anim;
                OLED_ShowString(32, 0, "PUMP", 16);
                break;
        }
        if (anim)
        {
            OLED_DrawBMP(48, 16, 79, 47, anim[anim_frame]);
            OLED_ShowString(40, 7, "Loading...", 8);
        }
        OLED_Refresh();
        oled_watchdog = HAL_GetTick();
        return;
    }

    switch(current_page)
    {
        case PAGE_WELCOME:
            OLED_ShowString(0, 0, "Welcome!", 16);
            OLED_ShowString(0, 2, "Press ENTER", 8);
            break;

        case PAGE_MENU:
        {
            const char* menu_items[] = {
                "1.Welcome", "2.Feed", "3.Light",
                "4.Temp",   "5.Pump"
            };
            int i;

            OLED_ShowString(0, 0, "=== MENU ===", 8);
            for (i = 0; i < 5; i++)
            {
                sprintf(buf, "%c%s",
                    (i == menu_index) ? '>' : ' ', menu_items[i]);
                OLED_ShowString(0, i + 1, buf, 8);
            }
            break;
        }

        case PAGE_FEED_SUBMENU:
        {
            const char* feed_items[] = {
                "TRAIN MODE", "MANUAL FEED", "AUTO MODE"
            };
            uint8_t running[3] = {
                train_running, feed_running, auto_running
            };
            int i;
            OLED_ShowString(0, 0, "=== FEED ===", 8);
            for (i = 0; i < 3; i++)
            {
                if (running[i] && feed_blink)
                    sprintf(buf, "%c          ",
                        (i == menu_index) ? '>' : ' ');
                else
                    sprintf(buf, "%c%s",
                        (i == menu_index) ? '>' : ' ', feed_items[i]);
                OLED_ShowString(0, i + 1, buf, 8);
            }
            break;
        }

        case PAGE_FEED_TRAIN:
        {
            const char* train_items[] = {
                "FEED SETTING", "START", "END"
            };
            int i;
            OLED_ShowString(0, 0, "TRAIN MODE", 16);
            for (i = 0; i < 3; i++)
            {
                sprintf(buf, "%c%s",
                    (i == train_cursor) ? '>' : ' ', train_items[i]);
                OLED_ShowString(0, i * 2 + 2, buf, 8);
            }
            break;
        }

        case PAGE_TRAIN_SETTING:
        {
            char prefix;
            OLED_ShowString(0, 0, "TRAIN FEED", 16);

            prefix = (train_setting_cursor == 0) ? '>' : ' ';
            if (train_editing == 1 && train_blink)
                sprintf(buf, "%cBO Feed(min/2):   ", prefix);
            else
                sprintf(buf, "%cBO Feed(min/2):%d", prefix,
                    train_stages[train_stage][0]);
            OLED_ShowString(0, 2, buf, 8);

            prefix = (train_setting_cursor == 1) ? '>' : ' ';
            if (train_editing == 2 && train_blink)
                sprintf(buf, "%cAO Feed(min/2):   ", prefix);
            else
                sprintf(buf, "%cAO Feed(min/2):%d", prefix,
                    train_stages[train_stage][1]);
            OLED_ShowString(0, 3, buf, 8);

            prefix = (train_setting_cursor == 2) ? '>' : ' ';
            if (train_editing == 3 && train_blink)
                sprintf(buf, "%cInterval(min):   ", prefix);
            else
                sprintf(buf, "%cInterval(min):%d", prefix, train_interval);
            OLED_ShowString(0, 4, buf, 8);

            sprintf(buf, "  Stage : %d/7", train_stage + 1);
            OLED_ShowString(0, 5, buf, 8);

            prefix = (train_setting_cursor == 3) ? '>' : ' ';
            sprintf(buf, "%cCONTINUE", prefix);
            OLED_ShowString(0, 7, buf, 8);
            if (train_stage < train_exec_stage)
                OLED_ShowString(92, 7, "finish", 8);
            break;
        }

        case PAGE_FEED_MANUAL:
        {
            char prefix;
            OLED_ShowString(0, 0, "MANUAL FEED", 16);

            prefix = (feed_cursor == 0) ? '>' : ' ';
            if (feed_editing && feed_blink)
                sprintf(buf, "%cAO Time(min):   ", prefix);
            else
                sprintf(buf, "%cAO Time(min):%d", prefix, feed_time);
            OLED_ShowString(0, 2, buf, 8);

            prefix = (feed_cursor == 1) ? '>' : ' ';
            sprintf(buf, "%cSTART", prefix);
            OLED_ShowString(0, 4, buf, 8);

            prefix = (feed_cursor == 2) ? '>' : ' ';
            sprintf(buf, "%cEND", prefix);
            OLED_ShowString(0, 6, buf, 8);
            break;
        }

        case PAGE_AUTO:
        {
            char prefix;
            OLED_ShowString(0, 0, "AUTO MODE", 16);

            prefix = (auto_cursor == 0) ? '>' : ' ';
            if (auto_editing == 1 && auto_blink)
                sprintf(buf, "%cInterval(min):  ", prefix);
            else
                sprintf(buf, "%cInterval(min):%d", prefix, auto_interval);
            OLED_ShowString(0, 2, buf, 8);

            prefix = (auto_cursor == 1) ? '>' : ' ';
            if (auto_editing == 2 && auto_blink)
                sprintf(buf, "%cAO Time(min):   ", prefix);
            else
                sprintf(buf, "%cAO Time(min):%d", prefix, auto_ao_time);
            OLED_ShowString(0, 3, buf, 8);

            prefix = (auto_cursor == 2) ? '>' : ' ';
            sprintf(buf, "%cSTART", prefix);
            OLED_ShowString(0, 5, buf, 8);

            prefix = (auto_cursor == 3) ? '>' : ' ';
            sprintf(buf, "%cEND", prefix);
            OLED_ShowString(0, 7, buf, 8);
            break;
        }

        case PAGE_FEED:
            OLED_ShowString(0, 0, "Feed Control", 16);
            OLED_ShowString(0, 2, "Running...", 8);
            break;

        case PAGE_LIGHT:
        {
            char *mode_str = light_mode ? "Manual" : "Auto";
            OLED_ShowString(0, 0, "LIGHT", 16);
            OLED_ShowString(64, 0, mode_str, 8);

            /* LED row */
            if (light_mode && light_edit && light_blink)
                OLED_ShowString(0, 2, "> LED    :     %%", 8);
            else {
                sprintf(buf, "%c LED    : %u%%",
                        (light_cursor == 0) ? '>' : ' ', light_led);
                OLED_ShowString(0, 2, buf, 8);
            }

            /* Status row */
            {
                const char *st;
                if (light_mode)
                    st = "Custom";
                else if (light_now < 25)
                    st = "Bright";
                else if (light_now < 75)
                    st = "OK";
                else
                    st = "Low";
                sprintf(buf, "%c Status : %s",
                        (light_cursor == 1) ? '>' : ' ', st);
                OLED_ShowString(0, 4, buf, 8);
            }

            /* Mode row */
            sprintf(buf, "%c Mode   : %s",
                    (light_cursor == 2) ? '>' : ' ', mode_str);
            OLED_ShowString(0, 6, buf, 8);
            break;
        }

        case PAGE_TEMP:
        {
            OLED_ShowString(0, 0, "HEATER", 16);

            if (temp_editing == 1 && temp_blink)
                OLED_ShowString(0, 2, "> Target Low :    ", 8);
            else {
                sprintf(buf, "> Target Low : %.1f", temp_target_low);
                OLED_ShowString(0, 2, buf, 8);
            }

            if (temp_editing == 2 && temp_blink)
                OLED_ShowString(0, 3, "> Target High:    ", 8);
            else {
                sprintf(buf, "> Target High: %.1f", temp_target_high);
                OLED_ShowString(0, 3, buf, 8);
            }

            sprintf(buf, "  Now   (C)  : %.1f", temp_current);
            OLED_ShowString(0, 5, buf, 8);

            {
                const char *tst;
                if (temp_current < TEMP_LOW) tst = "Low";
                else if (temp_current > TEMP_HIGH) tst = "High";
                else tst = "OK";
                sprintf(buf, "  Status     : %s", tst);
                OLED_ShowString(0, 6, buf, 8);
            }

            sprintf(buf, "  Heater     : %s", heater_on ? "ON" : "OFF");
            OLED_ShowString(0, 7, buf, 8);
            break;
        }

        case PAGE_PUMP:
        {
            if (pump_sub == 0)
            {
                const char *cond;
                if (pump_tds < TDS_GOOD) cond = "Good";
                else if (pump_tds <= TDS_FAIR) cond = "Fair";
                else if (pump_tds <= TDS_DIRTY) cond = "Dirty";
                else cond = "Poor";

                OLED_ShowString(0, 0, "PUMP", 16);
			sprintf(buf, "%cCondition: %s", (pump_cursor == 0) ? '>' : ' ', cond);
                OLED_ShowString(0, 2, buf, 8);

                sprintf(buf, "%c Setting",
                    (pump_cursor == 1) ? '>' : ' ');
                OLED_ShowString(0, 4, buf, 8);
            }
            else
            {
                uint8_t i;
                const char *items[] = {"P1out", "P2into", "End"};
                OLED_ShowString(0, 0, "PUMP SETTING", 16);
                for (i = 0; i < 3; i++)
                {
                    if (pump_running == i + 1 && pump_blink)
                        sprintf(buf, "%c       ",
                            (i == pump_cursor) ? '>' : ' ');
                    else
                        sprintf(buf, "%c %s",
                            (i == pump_cursor) ? '>' : ' ', items[i]);
                    OLED_ShowString(0, i * 2 + 2, buf, 8);
                }
            }
            break;
        }

    }

    OLED_Refresh();
    oled_watchdog = HAL_GetTick();
}
