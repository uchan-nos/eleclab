/*
 * Copyright (c) 2025 Kota UCHIDA
 *
 * 画面出力＆ユーザ入力
 */
#include "main.h"

#include <assert.h>
#include <stdio.h>

#define LONG_PRESS_TICK (500 / TICK_MS)

enum Input {
  IN_CW,  // Encoder rotation CW
  IN_CCW, // Encoder rotation CCW
  IN_PR,  // Press
  IN_LPR, // Long press
  IN_REL, // Release
};

const static enum DispMode kOpModes[3] = {
  DM_MULTI_CC, DM_SINGLE_CC, DM_GLOBAL_CC
};
static uint8_t op_mode = 0;
static enum DispMode disp_mode;
static uint8_t led = 0;
static uint8_t long_pressed = 0;
static uint32_t press_tick = 0;
static uint8_t rotated_while_pressed = 0;

static void FormatMA(char *buf, uint16_t ua) {
  char raw[6]; // 20mA == 20000uA なので、NUL 文字含め 6 文字分で足りる
  if (ua >= 10000) {
    // 10mA 以上なので、小数点以下は 1 桁
    sprintf(raw, "%04u", ua + 50);
  } else {
    // 10mA 未満は小数点以下 2 桁
    sprintf(raw, "%04u", ua + 5);
  }

  char *p = buf;
  char *r = raw;
  *p++ = *r++;
  if (ua >= 10000) {
    *p++ = *r++; // 10mA 以上（整数部が 2 桁）
  }
  *p++ = '.';
  *p++ = *r++;
  if (ua < 10000) {
    *p++ = *r++;
  }
  *p++ = 'm';
  *p++ = 'A';
  while (p < buf + 6) {
    *p++ = ' ';
  }
}

static void DispWholeArea(const char *line1, const char *line2) {
  LCD_HideCursor();
  LCD_MoveCursor(0, 0);
  LCD_PutString(line1, 16);
  LCD_MoveCursor(0, 1);
  LCD_PutString(line2, 16);
  LCD_MoveCursor(0, 0);
  LCD_ShowCursor();
}

static void DispModeselect() {
  disp_mode = DM_MODESELECT;
  DispWholeArea("MULTI   SINGLE  ",
                "GLOBAL          ");
}

static void MoveCursorMultiLED(uint8_t led) {
  LCD_MoveCursor((led & 2) * 4 + (led & 1) * 2, led & 1);
}

static void UpdateContentMultiCC(uint8_t led) {
  char buf[6];
  FormatMA(buf, GetGoalCurrent(led));
  LCD_HideCursor();
  MoveCursorMultiLED(led);
  LCD_PutString(buf, 6);
  MoveCursorMultiLED(led);
  LCD_ShowCursor();
}

static void DispMultiCC() {
  char buf[6];
  disp_mode = op_mode;
  LCD_HideCursor();
  for (int i = 0; i < LED_NUM; ++i) {
    FormatMA(buf, GetGoalCurrent(i));
    MoveCursorMultiLED(i);
    LCD_PutString(buf, 6);
  }
  MoveCursorMultiLED(led);
  LCD_ShowCursor();
}

static void HandleInput_ModeSelect(enum Input in) {
  switch (in) {
  case IN_CW:
    ++op_mode;
    if (op_mode >= 3) {
      op_mode = 0;
    }
    break;
  case IN_CCW:
    --op_mode;
    if (op_mode >= 3) {
      op_mode = 2;
    }
    break;
  }
  LCD_MoveCursor((op_mode & 1) * 8, (op_mode & 2) >> 1);
}

static void HandleInput_MultiCC(enum Input in) {
  if (press_tick > 0) {
    switch (in) {
    case IN_CW:
      ++led;
      if (led >= LED_NUM) {
        led = 0;
      }
      break;
    case IN_CCW:
      --led;
      if (led >= LED_NUM) {
        led = 3;
      }
      break;
    case IN_REL:
      if (!rotated_while_pressed) {
        DispModeselect();
        return;
      }
    }
    MoveCursorMultiLED(led);
  } else {
    switch (in) {
    case IN_CW:
      IncGoalCurrent(led, 10);
      break;
    case IN_CCW:
      IncGoalCurrent(led, -10);
      break;
    }
    UpdateContentMultiCC(led);
  }
}

static void DefaultHandler(enum Input in) {
  printf("default handler: in=%d\n", in);
}

typedef void (*HandlerType)(enum Input);
static HandlerType GetHandler(void) {
  switch (disp_mode) {
  case DM_MODESELECT:
    return HandleInput_ModeSelect;
  case DM_MULTI_CC:
    return HandleInput_MultiCC;
  }
  return DefaultHandler;
}

void DialRotatedCW(void) {
  GetHandler()(IN_CW);
  if (press_tick > 0) {
    rotated_while_pressed = 1;
  }
}

void DialRotatedCCW(void) {
  GetHandler()(IN_CCW);
  if (press_tick > 0) {
    rotated_while_pressed = 1;
  }
}

void ButtonPressed(uint32_t tick) {
  GetHandler()(IN_PR);
  if (press_tick == 0) {
    press_tick = tick;
  }
}

static void ButtonLongPressed(uint32_t tick) {
  GetHandler()(IN_LPR);
}

void ButtonReleased(uint32_t tick) {
  GetHandler()(IN_REL);
  long_pressed = 0;
  press_tick = 0;
  rotated_while_pressed = 0;
}

void InitUI() {
  disp_mode = op_mode = DM_MULTI_CC;
  DispMultiCC();
}

void UpdateUI(uint32_t tick) {
  if (press_tick > 0 && !long_pressed && press_tick + LONG_PRESS_TICK <= tick) {
    // 長押し
    long_pressed = 1;
    ButtonLongPressed(tick);
  }
}
