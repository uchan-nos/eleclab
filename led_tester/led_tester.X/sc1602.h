#pragma once

#include <stdint.h>
#include <pic16f18323.h>

#define LCD_DB4 LATC0
#define LCD_DB5 LATC1
#define LCD_DB6 LATC2
#define LCD_DB7 LATC3
#define LCD_RS  LATC4
#define LCD_E   LATC5

/********************
 ***** 基本関数 *****
 ********************/

// LCD にコマンドあるいはデータを送信する
void lcd_out8(uint8_t rs, uint8_t val);
// cmd をコマンドとして送信する
void lcd_exec(uint8_t cmd);
// c をデータとして送信する
void lcd_putc(char c);

/********************
 ** ユーティリティ **
 ********************/

// LCD を初期化する
void lcd_init();
// 文字列を表示する
void lcd_puts(const char* s);
// カーソルを指定した位置に動かす
void lcd_cursor_at(uint8_t x, uint8_t y);
