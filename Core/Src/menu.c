#include "menu.h"
#include "ui.h"
#include "font.h"
#include "oled.h"

void Menu_UpdateFrame(const uint8_t anim[5][128], uint8_t *frame_index){
    OLED_DrawBMP(48,16,79,47,anim[*frame_index]);
    *frame_index = (*frame_index +1)%5;
}
