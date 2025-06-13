/*
 * Copyright (c) 2025 Kota UCHIDA
 *
 * 画面出力＆ユーザ入力
 */
#include "main.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifndef EXPORT_STATIC_FUNC
#define STATIC static
#else
#define STATIC
#endif

#define LONG_PRESS_MS 500
#define LONG_PRESS_TICK (LONG_PRESS_MS / TICK_MS)
#define REFRESH_PERIOD_MS 50
#define REFRESH_PERIOD_TICK (REFRESH_PERIOD_MS / TICK_MS)
#define VF_THRESHOLD_MV 200

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
static uint32_t refresh_tick = 0;
static uint32_t current_tick;

// 与えられた電流値を mA 単位の文字列に変換（きっかり 6 文字 + NUL 終端）
STATIC void FormatMA(char *buf, uint16_t ua) {
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
  *p = '\0';
}

// 与えられた電圧値を V 単位の文字列に変換（きっかり 5 文字 + NUL 終端）
STATIC void FormatV(char *buf, uint16_t mv) {
  char raw[5];
  // 10mV 単位に四捨五入
  assert(mv < 9995);
  sprintf(raw, "%04u", mv + 5);

  char *p = buf;
  char *r = raw;
  *p++ = *r++;
  *p++ = '.';
  *p++ = *r++;
  *p++ = *r++;
  *p++ = 'V';
  *p = '\0';
}


// 与えられた抵抗値を文字列に変換（きっかり 5 文字 + NUL 終端）
STATIC void FormatR(char *buf, uint32_t ohm) {
  if (ohm >= 10000000 - 500) {
    // 10M 以上の場合、>=10M と表示
    strcpy(buf, ">=10M");
  } else if (ohm >= 100000) {
    // 100K 以上の場合、1KΩ 単位に丸める
    sprintf(buf, "%4uK", (ohm + 500) / 1000);
  } else {
    // 100K 未満なら 1Ω 単位で表示
    sprintf(buf, "%5u", ohm);
    return;
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
  op_mode = 0;
  DispWholeArea("MULTI   SINGLE  ",
                "GLOBAL          ");
}

static void MoveCursorMultiLED(uint8_t led) {
  LCD_MoveCursor((led & 2) * 4 + (led & 1) * 2, led & 1);
}

static void RefreshContentMultiCC(uint8_t led) {
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
  disp_mode = DM_MULTI_CC;
  LCD_HideCursor();
  for (int i = 0; i < LED_NUM; ++i) {
    FormatMA(buf, GetGoalCurrent(i));
    MoveCursorMultiLED(i);
    LCD_PutString(buf, 6);
  }
  MoveCursorMultiLED(led);
  LCD_ShowCursor();
}

static void DispSingleCC() {
  char buf[6];
  disp_mode = DM_SINGLE_CC;
  LCD_HideCursor();
  LCD_MoveCursor(0, 0);

  sprintf(buf, "D%d", led + 1);
  LCD_PutString(buf, 2);
  LCD_PutSpaces(2);

  uint16_t goal_ua = GetGoalCurrent(led);
  FormatMA(buf, goal_ua);
  LCD_PutString(buf, 6);
  LCD_PutSpaces(1);

  uint16_t vf_mv = GetVF(led);
  FormatV(buf, vf_mv);
  LCD_PutString(buf, 5);

  LCD_MoveCursor(0, 1);
  if (vf_mv < VF_THRESHOLD_MV) {
    // LED ヲ サシテクダサイ
    LCD_PutString("LED \xa6 \xbb\xbc\xc3 \xb8\xc0\xde\xbb\xb2 ", 16);
  } else if (goal_ua == 0) {
    LCD_PutString("5:>=10M 33:>=10M", 16);
  } else {
    uint32_t r5 = (uint32_t)(5000 - vf_mv) * 1000 / goal_ua;
    FormatR(buf, r5);
    LCD_PutString("5:", 2);
    LCD_PutString(buf, 5);
    LCD_PutSpaces(1);

    uint32_t r3 = (uint32_t)(3300 - vf_mv) * 1000 / goal_ua;
    FormatR(buf, r3);
    LCD_PutString("33:", 3);
    LCD_PutString(buf, 5);
  }

  LCD_MoveCursor(4, 0);
  LCD_ShowCursor();
}

static void DispGlobalCC() {
  disp_mode = DM_GLOBAL_CC;
}

static void HandleInput_ModeSelect(enum Input in, int arg) {
  switch (in) {
  case IN_CW:
    INC_MOD(op_mode, 3);
    LCD_MoveCursor((op_mode & 1) * 8, (op_mode & 2) >> 1);
    break;
  case IN_CCW:
    DEC_MOD(op_mode, 3);
    LCD_MoveCursor((op_mode & 1) * 8, (op_mode & 2) >> 1);
    break;
  case IN_REL:
    switch (kOpModes[op_mode]) {
    case DM_MULTI_CC: DispMultiCC(); break;
    case DM_SINGLE_CC: DispSingleCC(); break;
    case DM_GLOBAL_CC: DispGlobalCC(); break;
    default: assert(0);
    }
    break;
  }
}

static void HandleInput_MultiCC(enum Input in, int arg) {
  if (press_tick > 0) {
    switch (in) {
    case IN_CW:
      INC_MOD(led, LED_NUM);
      break;
    case IN_CCW:
      DEC_MOD(led, LED_NUM);
      break;
    case IN_REL:
      if (long_pressed && !rotated_while_pressed) {
        DispModeselect();
        return;
      }
    }
    MoveCursorMultiLED(led);
  } else {
    switch (in) {
    case IN_CW:
      IncGoalCurrent(led, 10 * arg);
      break;
    case IN_CCW:
      IncGoalCurrent(led, 10 * arg);
      break;
    }
    RefreshContentMultiCC(led);
  }
}

static void HandleInput_SingleCC(enum Input in, int arg) {
  if (press_tick > 0) {
    switch (in) {
    case IN_CW:
      INC_MOD(led, LED_NUM);
      break;
    case IN_CCW:
      DEC_MOD(led, LED_NUM);
      break;
    case IN_REL:
      if (long_pressed && !rotated_while_pressed) {
        DispModeselect();
        return;
      }
    }
    DispSingleCC();
  } else {
    switch (in) {
    case IN_CW:
      IncGoalCurrent(led, 10 * arg);
      break;
    case IN_CCW:
      IncGoalCurrent(led, 10 * arg);
      break;
    }
    DispSingleCC();
  }
}

static void DefaultHandler(enum Input in, int arg) {
  printf("default handler: in=%d arg=%d\n", in, arg);
}

typedef void (*HandlerType)(enum Input, int);
static HandlerType GetHandler(void) {
  switch (disp_mode) {
  case DM_MODESELECT:
    return HandleInput_ModeSelect;
  case DM_MULTI_CC:
    return HandleInput_MultiCC;
  case DM_SINGLE_CC:
    return HandleInput_SingleCC;
  }
  return DefaultHandler;
}

/*
 * 加速を考慮したダイアル回転数の計算
 *
 * @param dir  ダイアル回転方向（CW=1, CCW=-1）
 * @return 加速済み回転数
 */
STATIC int AccelDial(int dir) {
  static int lastdir = 0; // 最後に観測した回転方向
  static unsigned int rotcnt = 0; // 回転数（tick が変化するたびにリセット）
  static uint32_t lastrot_tick = 0; // 最後に回転したときの時刻

  int v = 0;
  if (lastdir != dir) {
    // 逆回転が早すぎたら誤動作として無視する
    // ゆっくりした逆回転は意図した動作だとして受け入れる
    if (lastrot_tick + (100 / TICK_MS) < current_tick) {
      v = dir;
      lastdir = dir;
      rotcnt = 1;
      lastrot_tick = current_tick;
    }
  } else if (lastrot_tick == current_tick) {
    ++rotcnt;
  } else if (lastrot_tick + 2 <= current_tick && rotcnt == 1) {
    v = dir;
  } else {
    ++rotcnt;
    int tick_diff = current_tick - lastrot_tick;
    int rot2 = rotcnt * rotcnt;
    v = (1 << (rotcnt << 2)) / tick_diff;
    if (v == 0) {
      // 最低でも ±1 は動くようにする
      v = dir;
    } else if (dir < 0) {
      v = -v;
    }

    lastrot_tick = current_tick;
    rotcnt = 1;
  }

  return v;
}

static void DialRotatedCW(void) {
  GetHandler()(IN_CW, AccelDial(1));
  if (press_tick > 0) {
    rotated_while_pressed = 1;
  }
}

static void DialRotatedCCW(void) {
  GetHandler()(IN_CCW, AccelDial(-1));
  if (press_tick > 0) {
    rotated_while_pressed = 1;
  }
}

static void ButtonLongPressed(uint32_t tick) {
  GetHandler()(IN_LPR, 0);
}

// 変化する現在値に応じて表示を更新する
static void PeriodicRefresh(void) {
  switch (disp_mode) {
  case DM_SINGLE_CC:
    DispSingleCC();
    break;
  }
}

void InitUI() {
  disp_mode = op_mode = DM_MULTI_CC;
  DispMultiCC();
}

static void UpdateUI(uint32_t tick) {
  current_tick = tick;
  if (press_tick > 0 && !long_pressed && press_tick + LONG_PRESS_TICK <= tick) {
    // 長押し
    long_pressed = 1;
    ButtonLongPressed(tick);
  }

  if (refresh_tick + REFRESH_PERIOD_TICK <= tick) {
    refresh_tick += REFRESH_PERIOD_TICK;
    PeriodicRefresh();
  }
}

void HandleUIEvent(uint32_t tick, MessageType msg) {
  switch (msg) {
  case MSG_TICK:
    UpdateUI(tick);
    break;
  case MSG_CW:
    DialRotatedCW();
    break;
  case MSG_CCW:
    DialRotatedCCW();
    break;
  case MSG_PRS_MODE:
    GetHandler()(IN_PR, 0);
    if (press_tick == 0) {
      press_tick = tick;
    }
    break;
  case MSG_REL_MODE:
    GetHandler()(IN_REL, 0);
    long_pressed = 0;
    press_tick = 0;
    rotated_while_pressed = 0;
    break;
  case MSG_PRS_LED:
  case MSG_REL_LED:
  }
}
