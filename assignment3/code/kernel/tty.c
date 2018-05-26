
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               tty.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"
#include "keyboard.h"

typedef int Mode;

typedef int Boolean;

#define COMMON 0
#define SEARCH_INPUT 1
#define SEARCH_VIEW 2

#define FALSE 0
#define TRUE 1

#define TEXR_SIZE V_MEM_SIZE / 2
#define WIDTH 80
#define HEIGHT 25
#define TARGET_LIMIT 80 - 1

PRIVATE Mode mode = COMMON;
PRIVATE char inputs[TEXR_SIZE]; // 用户的输入字符串，缓存在内存里
PRIVATE char target[TARGET_LIMIT]; // 用户搜索时候输入的字符串
PRIVATE u8 screen[V_MEM_SIZE]; // 显示器的副本
PRIVATE int input_size = 0;
PRIVATE int target_size = 0;

PRIVATE void show();

PRIVATE void init() 
{
    memset(inputs, 0, TEXR_SIZE);
    input_size = 0;
    mode = COMMON;
    show();
}

/*======================================================================*
                           task_tty
 *======================================================================*/
PUBLIC void task_tty()
{
    init(); 
	while (TRUE) {
		keyboard_read();
	}
}

PRIVATE void set_cursor(unsigned int pos) 
{
    disable_int();
    out_byte(CRTC_ADDR_REG, CURSOR_H);
    out_byte(CRTC_DATA_REG, (pos >> 8) & 0xFF);
    out_byte(CRTC_ADDR_REG, CURSOR_L);
    out_byte(CRTC_DATA_REG, pos & 0xFF);
    enable_int();
}


PRIVATE void show() // 把用户的输入转化成显示到屏幕上的数据
{
    for (int i = 0; i < TEXR_SIZE; i++) {
        screen[i << 1] = 0;
        screen[1 + (i << 1)] = 0x07; // 白色非闪烁
    }
    int row = 0;
    int col = 0;
    for (int i = 0; i < input_size; i++) {
        if (inputs[i] == '\n') { // a new Line
            row = (row + 1) % HEIGHT;
            col= 0;
        } else if (inputs[i] == '\t') {
            col = ((col >> 3) + 1) << 3; 
        } else { // a common char
            screen[(row * WIDTH + col) << 1] = inputs[i];
            if (mode == SEARCH_VIEW) {
                Boolean equal = TRUE;
                for (int j = 0; j < target_size; j++) {
                    if (i + j == input_size || target[j] != inputs[i + j]) {
                        equal = FALSE;
                        break;
                    }
                }
                if (equal) 
                    for (int j = 0; j < target_size; j++) 
                        screen[(row * WIDTH + col + j) * 2 + 1] = 0x0e;
                
            }
            col += 1;
        }
        if (col == WIDTH) {
            row = (row + 1) % HEIGHT;
            col = 0;
        }
    }
    if (mode != COMMON) {
        row = HEIGHT - 1; // 最后一行
        screen[(row * WIDTH) << 1] = ':';
        screen[1 + ((row * WIDTH) << 1)] = 0x0e;
        for (int i = 0; i < target_size; i++) {
            screen[(row * WIDTH + 1 + i) << 1] = target[i];
            screen[((row * WIDTH + 1 + i) << 1) + 1] = 0x0e;
        }
    }
    memcpy(V_MEM_BASE, screen, V_MEM_SIZE);
    if (mode != COMMON) {
        set_cursor(row * WIDTH + target_size + 1);
    } else {
        set_cursor(row * WIDTH + col + target_size);
    }
}

PRIVATE void change_to_common() 
{
    memset(target, 0, TARGET_LIMIT);
    target_size = 0;
    mode = COMMON;
}


/*======================================================================*
				in_process
 *======================================================================*/
PUBLIC void in_process(u32 key)
{
   if (mode == COMMON) {
        if (!(key & FLAG_EXT)) { // 非功能键
            inputs[input_size++] = key & 0xFF;
        } else {
            switch(key & MASK_RAW) {
                case ENTER:
                    inputs[input_size++] = '\n';
                    break;
                case TAB:
                    inputs[input_size++] = '\t';
		    break;
                case BACKSPACE:
                    if (input_size > 0)
                        inputs[--input_size] = 0;
                    break;
                case ESC:
                    mode = SEARCH_INPUT;
                    break;
                default:
                    break;
            }
        }
    } else if (mode == SEARCH_INPUT) {
        if (!(key & FLAG_EXT) && target_size < TARGET_LIMIT - 1) {
            target[target_size++] = key & 0xFF; 
        } else {
            switch(key & MASK_RAW) {
                case ENTER:
                    mode = SEARCH_VIEW;
                    break;
                case BACKSPACE:
                    if (target_size == 0) {
                        change_to_common();
                    } else {
                        target[--target_size] = 0;
                    }
                    break;
                case ESC:
                    change_to_common();
                    break;
                default:
                    break;
            }
        }
    } else if (mode == SEARCH_VIEW && (key & FLAG_EXT) && (key & MASK_RAW) == ESC) {
        change_to_common();
    }
    show();
}


PUBLIC void clear() // 每隔20秒清除屏幕
{
    while(TRUE) {
        milli_delay(20 * 10000);
        if (mode == COMMON) 
            init();
    }
}
