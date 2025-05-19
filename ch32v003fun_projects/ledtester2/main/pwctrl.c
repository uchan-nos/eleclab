/*
 * Copyright (c) 2025 Kota UCHIDA
 *
 * PWM パルス幅の制御プログラム
 */

#include "main.h"

#include <assert.h>

static uint16_t pw_fix_tick;
static uint32_t pw_sum;
static uint16_t err_ua_max;

static uint16_t prev_pw[LED_NUM];
static uint16_t goals_ua[LED_NUM]; // 制御目標の電流値（μA）
static int16_t errors_ua[LED_NUM]; // 目標と現在の電流値の差
static uint8_t pw_fixed;

uint16_t GetGoalCurrent(uint8_t led) {
  assert(led < LED_NUM);
  return goals_ua[led];
}

void SetGoalCurrent(uint8_t led, uint16_t goal_ua) {
  assert(led < LED_NUM);
  if (goals_ua[led] != goal_ua) {
    goals_ua[led] = goal_ua;
    pw_fixed = 0;
  }
}

uint16_t NextPW(uint8_t led, uint16_t if_ua) {
  uint16_t goal_ua = goals_ua[led];
  int16_t err_ua = goal_ua - if_ua;
  int16_t prev_err_ua = errors_ua[led];
  int16_t d_err_ua = err_ua - prev_err_ua;
  errors_ua[led] = err_ua;

  uint16_t pw = prev_pw[led];

  /*
   * pw += 0.35 * err_ua + 0.45 * d_err_ua;
   * Memory region         Used Size  Region Size  %age Used
   *            FLASH:        6448 B        16 KB     39.36%
   *              RAM:          32 B         2 KB      1.56%
   */
  //pw += 0.35 * err_ua + 0.45 * d_err_ua;

  /*
   * pw += 35 * (uint32_t)err_ua / 100 + 45 * (uint32_t)d_err_ua / 100;
   * Memory region         Used Size  Region Size  %age Used
   *            FLASH:        3032 B        16 KB     18.51%
   *              RAM:          24 B         2 KB      1.17%
   */
  //pw += 35 * (uint32_t)err_ua / 100 + 45 * (uint32_t)d_err_ua / 100;

  /*
   * pw += (350 * (uint32_t)err_ua >> 10) + (450 * (uint32_t)d_err_ua >> 10);
   * Memory region         Used Size  Region Size  %age Used
   *            FLASH:        3008 B        16 KB     18.36%
   *              RAM:          24 B         2 KB      1.17%
   */

  int16_t pw_diff = 0;
  if (goal_ua == 0) {
    pw = 5000;
  } else {
    uint32_t kp = 20000 / if_ua + 10;
    if (if_ua <= 40) {
      kp = 500;
    }
    uint32_t kd = 2 * kp;
    pw_diff = (kp * (int32_t)err_ua >> 10) + (kd * (int32_t)d_err_ua >> 10);
    if (pw_diff == 0) {
      pw_diff = err_ua > 0 ? 1 : -1;
    }
  }
  pw += pw_diff;

  if (pw > 30000) {
    pw = 0;
  }

  if (led == 0 && !pw_fixed) {
    if ((pw_fix_tick & 0x3f) == 0) {
      //if_ua_min = if_ua;
      //if_ua_max = if_ua;
      err_ua_max = 0;
      pw_sum = 0;
    } else {
      //if (if_ua_min > if_ua) {
      //  if_ua_min = if_ua;
      //} else if (if_ua_max < if_ua) {
      //  if_ua_max = if_ua;
      //}
      if (err_ua_max < abs(err_ua)) {
        err_ua_max = abs(err_ua);
      }
    }
    pw_sum += pw;
    ++pw_fix_tick;

    // 0x40 ticks => 4ms * 0x40 == 256ms
    if (pw_fix_tick >= 0x40 * 40) {
      pw_fixed = 1;
    } else if ((pw_fix_tick & 0x3f) == 0) {
      uint16_t tolerance = goals_ua[led] >> 7;
      if (tolerance < 4) {
        tolerance = 4;
      }
      if (err_ua_max <= tolerance) {
        pw_fixed = 1;
      }
    }

    if (pw_fixed) {
      pw = pw_sum >> 6;
      //printf("pw fixed @%u: %u (%u %u)\n", pw_fix_tick, pw, if_ua_min, if_ua_max);
      printf("pw fixed @%u: %u (%u)\n", pw_fix_tick, pw, err_ua_max);
      pw_fix_tick = 0;
    }
  }

  prev_pw[led] = pw;
  return pw;
}
