#include <stdio.h>
#include <string.h>

#include "../main.h"

uint16_t goals_ua[LED_NUM];

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

#define TP(fmt, ...) \
  printf("%s (%s:%d): " fmt, __func__, __FILE__, __LINE__, __VA_ARGS__)
#define T(expected) \
  if (!((expected) && ++n_passed) && ++n_failed)
int n_passed, n_failed;

void test_screen(int e_cx, int e_cy, char *e_scr0, char *e_scr1) {
  T (e_cx == cx) {
    TP("cx: got %d, want %d\n", cx, e_cx);
  }
  T (e_cy == cy) {
    TP("cy: got %d, want %d\n", cy, e_cy);
  }
  T (memcmp(screen[0], e_scr0, LCD_WIDTH) == 0) {
    TP("screen[0]: got %.*s, want %.*s\n",
       LCD_WIDTH, screen[0], LCD_WIDTH, e_scr0);
  }
  T (memcmp(screen[1], e_scr1, LCD_WIDTH) == 0) {
    TP("screen[1]: got %.*s, want %.*s\n",
       LCD_WIDTH, screen[1], LCD_WIDTH, e_scr1);
  }
}

int main() {
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
  ButtonPressed(10);
  DialRotatedCW();
  test_screen(2, 1, "0.01mA  0.00mA  ", "  0.00mA  0.00mA");
  ButtonReleased(14);

  // LED2 の電流を調整
  DialRotatedCW();
  DialRotatedCW();
  test_screen(2, 1, "0.01mA  0.00mA  ", "  0.02mA  0.00mA");

  // ボタン長押しでメインメニューへ
  ButtonPressed(20);
  ButtonReleased(1000);
  test_screen(0, 0, "MULTI   SINGLE  ", "GLOBAL          ");
  // メインメニューのカーソル移
  DialRotatedCW();
  DialRotatedCW();
  test_screen(0, 1, "MULTI   SINGLE  ", "GLOBAL          ");
  DialRotatedCCW();
  test_screen(8, 0, "MULTI   SINGLE  ", "GLOBAL          ");

  // SINGLE でクリック
  ButtonPressed(1010);
  ButtonReleased(1011);
  test_screen(0, 0, "D1  0.01mA 1.64V", "R5= 589 R33= 291");

  printf("%d passed, %d failed\n", n_passed, n_failed);
  if (n_failed == 0) {
    printf("all passed!\n");
  }
}
