/*
 * Copyright (c) 2025 Kota UCHIDA
 *
 * CH32V003 の TIM2 でロータリーエンコーダーを読むテスト。
 */
#include "ch32fun.h"
#include <stdio.h>

/*
 * TIM2 をエンコーダ計数モードに設定
 */
void TIM2_InitForEncoder() {
  // TIM2 を有効化
  RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

  // TIM2 をリセット
  RCC->APB1PRSTR |= RCC_APB1Periph_TIM2;
  RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM2;

  // 両エッジカウントモードに設定
  TIM2->SMCFGR = TIM_EncoderMode_TI1;
  // TI12 => 両エッジカウントモード（1 クリックで 4 進む）

  // カウンタを有効化
  TIM2->CTLR1 |= TIM_CEN;
}

int main() {
  SystemInit();
  funGpioInitAll();
  funPinMode(PD3, GPIO_CFGLR_IN_PUPD);
  funPinMode(PD4, GPIO_CFGLR_IN_PUPD);
  funDigitalWrite(PD3, 1); // pull-up
  funDigitalWrite(PD4, 1); // pull-up
  TIM2_InitForEncoder();

  uint16_t prev_cnt = 0;
  int prev_dir = 0;
  while (1) {
    uint16_t cnt = TIM2->CNT;
    int dir = cnt - prev_cnt;
    if (prev_cnt != cnt) {
      printf("TIM2 CNT changed: %d", cnt);
      prev_cnt = cnt;

      if (prev_dir == 0) {
        prev_dir = dir;
      } else if (prev_dir * dir < 0) {
        printf(" dir changed to %d", dir);
        prev_dir = dir;
      }

      printf("\n");
    }
    Delay_Ms(10);
  }
}
