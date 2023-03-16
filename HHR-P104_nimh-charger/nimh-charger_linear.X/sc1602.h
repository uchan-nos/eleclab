#pragma once

#include <stdint.h>
#include <pic16f1786.h>

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
void lcd_out8(bool rs, uint8_t val);
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

/** 固定小数点数を "2.75" のような文字列に整形する。
 * 
 * @param s  変換結果を格納する配列（最後にヌル文字は書かれない）
 * @param v  固定小数点数（"2.75" であれば 275 を渡す）
 * @param n  小数点を含めた文字数（"2.75" であれば 3）
 * @param dp 小数点の位置（"2.75" であれば 1）
 */
void format_dec(char* s, long v, int n, int dp);
