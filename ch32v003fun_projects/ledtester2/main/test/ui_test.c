/*
 * Copyright (c) 2025 Kota UCHIDA
 *
 * 画面出力＆ユーザ入力
 */

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../main.h"

void FormatMA(char *buf, uint16_t ua);
void FormatV(char *buf, uint16_t mv);
void FormatR(char *buf, uint32_t ohm);
int AccelDial(int dir);

uint16_t goals_ua[LED_NUM];
uint16_t vfs_mv[LED_NUM] = {1650, 2131, 3 /* no LED */, 3215};

uint16_t GetGoalCurrent(uint8_t led) {
  return goals_ua[led];
}

void IncGoalCurrent(uint8_t led, int amount_ua) {
  int g = goals_ua[led] + amount_ua;
  if (g < 0) {
    g = 0;
  } else if (g > 20000) {
    g = 20000;
  }
  goals_ua[led] = g;
}

uint16_t GetVF(uint8_t led) {
  return vfs_mv[led];
}

#define LCD_WIDTH 16

char screen[2][LCD_WIDTH];
int overrun_x;
int cx, cy, cshowed;

void LCD_ShowCursor() {
  cshowed = 1;
}

void LCD_HideCursor() {
  cshowed = 0;
}

void LCD_MoveCursor(int x, int y) {
  cx = x;
  cy = y;
}

void LCD_PutString(const char *s, int n) {
  if (cx + n > LCD_WIDTH) {
    overrun_x = 1;
    n = LCD_WIDTH - cx;
  }
  memcpy(screen[cy] + cx, s, n);
  cx += n;
}

void LCD_PutSpaces(int n) {
  memset(screen[cy] + cx, ' ', n);
  cx += n;
}

static int assert_failed;
void __assert_fail(const char *__assertion, const char *__file,
                   unsigned int __line, const char *__function) {
  assert_failed = 1;
}

void addr2line(const char *exe_file, const char *addr) {
  char cmd[256];
  sprintf(cmd, "addr2line -s -e %s %s", exe_file, addr);
  system(cmd);
}

#define TP(fmt, ...) \
  do {\
    printf("%s (%s:%d): " fmt, __func__, __FILE__, __LINE__, ##__VA_ARGS__);\
    void *bt[3];\
    int n = backtrace(bt, 3);\
    if (n >= 2) {\
      char **bt_syms = backtrace_symbols(bt, n);\
      char *lparen = strchr(bt_syms[1], '(');\
      char *rparen = strchr(lparen, ')');\
      *lparen = *rparen = 0;\
      printf("  called at ");\
      fflush(stdout);\
      addr2line(bt_syms[1], lparen+1);\
    }\
  } while (0)
#define T(expected) \
  if (!((expected) && ++n_passed) && ++n_failed)
int n_passed, n_failed;

void test_format_ma(uint16_t ua, const char *expected) {
  char buf[8] = "DEADBEEF";
  FormatMA(buf, ua);
  T (strcmp(buf, expected) == 0) {
    TP("FormatMA for %uuA: got '%s', want '%s'\n", ua, buf, expected);
  }
}

void test_format_v(uint16_t mv, const char *expected) {
  char buf[8] = "DEADBEEF";
  FormatV(buf, mv);
  T (strcmp(buf, expected) == 0) {
    TP("FormatV for %umV: got '%s', want '%s'\n", mv, buf, expected);
  }
}

void test_format_r(uint32_t ohm, const char *expected) {
  char buf[8] = "DEADBEEF";
  FormatR(buf, ohm);
  T (strcmp(buf, expected) == 0) {
    TP("FormatR for %uohms: got '%s', want '%s'\n", ohm, buf, expected);
  }
}

void test_format_funcs() {
  test_format_ma(   10, "0.01mA");
  test_format_ma( 2304, "2.30mA");
  test_format_ma( 2305, "2.31mA");
  test_format_ma(19990, "20.0mA");

  test_format_v(    4, "0.00V");
  test_format_v(    5, "0.01V");
  test_format_v(   94, "0.09V");
  test_format_v(   95, "0.10V");
  test_format_v( 5500, "5.50V");

  char buf[8];
  assert_failed = 0;
  FormatV(buf, 10000);
  T (assert_failed) {
    TP("FormatV doesn't call assert when mv >= 10000\n");
  }

  test_format_r(       1, "    1");
  test_format_r(   99999, "99999");
  test_format_r(  100000, " 100K");
  test_format_r(  100499, " 100K");
  test_format_r(  100500, " 101K");
  test_format_r( 9000000, "9000K");
  test_format_r(10000000, ">=10M");
}

void test_screen(int e_cx, int e_cy, char *e_scr0, char *e_scr1) {
  T (e_cx == cx) {
    TP("cx: got %d, want %d\n", cx, e_cx);
  }
  T (e_cy == cy) {
    TP("cy: got %d, want %d\n", cy, e_cy);
  }
  T (e_scr0 && memcmp(screen[0], e_scr0, LCD_WIDTH) == 0) {
    TP("screen[0]: got %.*s, want %.*s\n",
       LCD_WIDTH, screen[0], LCD_WIDTH, e_scr0);
  }
  T (e_scr1 && memcmp(screen[1], e_scr1, LCD_WIDTH) == 0) {
    TP("screen[1]: got %.*s, want %.*s\n",
       LCD_WIDTH, screen[1], LCD_WIDTH, e_scr1);
  }
}

uint32_t tick = 0;
void StepTick(int step) {
  for (int i = 0; i < step; ++i) {
    ++tick;
    UpdateUI(tick);
  }
}

void test_ui_manip() {
  memset(screen, ' ', LCD_WIDTH * 2);

  InitUI();
  test_screen(0, 0, "0.00mA  0.00mA  ", "  0.00mA  0.00mA");

  // LED1 の電流を調整
  DialRotatedCW();
  test_screen(0, 0, "0.01mA  0.00mA  ", "  0.00mA  0.00mA");
  DialRotatedCW();
  test_screen(0, 0, "0.02mA  0.00mA  ", "  0.00mA  0.00mA");
  DialRotatedCCW();
  test_screen(0, 0, "0.01mA  0.00mA  ", "  0.00mA  0.00mA");

  // ボタンを押しながら回転で LED 変更
  StepTick(10);
  ButtonPressed(tick);
  DialRotatedCW();
  test_screen(2, 1, "0.01mA  0.00mA  ", "  0.00mA  0.00mA");
  StepTick(4);
  ButtonReleased(tick);

  // LED2 の電流を調整
  for (int i = 0; i < 25; ++i) {
    DialRotatedCW();
  }
  test_screen(2, 1, "0.01mA  0.00mA  ", "  0.25mA  0.00mA");

  // ボタン長押しでメインメニューへ
  StepTick(6);
  ButtonPressed(tick);
  StepTick(980);
  ButtonReleased(tick);
  test_screen(0, 0, "MULTI   SINGLE  ", "GLOBAL          ");
  // メインメニューのカーソル移
  StepTick(10);
  DialRotatedCW();
  test_screen(8, 0, "MULTI   SINGLE  ", "GLOBAL          ");
  StepTick(10);
  DialRotatedCW();
  test_screen(0, 1, "MULTI   SINGLE  ", "GLOBAL          ");
  StepTick(10);
  DialRotatedCCW();
  test_screen(8, 0, "MULTI   SINGLE  ", "GLOBAL          ");

  // SINGLE でクリック
  StepTick(10);
  ButtonPressed(tick);
  StepTick(10);
  ButtonReleased(tick);
  test_screen(4, 0, "D2  0.25mA 2.13V", "5:11476 33: 4676");

  // LED3 を選択
  StepTick(10);
  ButtonPressed(tick);
  StepTick(1000);
  DialRotatedCW();
  test_screen(4, 0, "D3  0.00mA 0.00V", "LED ｦ ｻｼﾃ ｸﾀﾞｻｲ ");

  // LED3 を挿入
  vfs_mv[2] = 2600;
  StepTick(100);
  test_screen(4, 0, "D3  0.00mA 2.60V", "5:>=10M 33:>=10M");
  ButtonReleased(tick);
  DialRotatedCW();
  test_screen(4, 0, "D3  0.01mA 2.60V", "5: 240K 33:70000");

  printf("%d passed, %d failed\n", n_passed, n_failed);
  if (n_failed == 0) {
    printf("all passed!\n");
  }
}

int main() {
  test_format_funcs();
  test_ui_manip();
}
