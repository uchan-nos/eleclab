/*
 * Copyright (c) 2025 Kota UCHIDA
 *
 * LED テスター 2 に搭載の CH32V003 を試験するプログラム。
 * PD2 (T1CH1) を有効化するとマイコンが機能停止する疑惑を試験する。
 * 単体の CH32V003F4P6 で動作確認済み。
 */
#include "ch32fun.h"
#include <stdio.h>

/*
 * @param psc  プリスケーラの設定値（0 => 1:1）
 * @param period  タイマ周期
 * @param channels  有効化するチャンネル（ビットマップ）
 * @param default_high  1 なら無信号時の出力を 1 にする
 */
void TIM1_InitForPWM(uint16_t psc, uint16_t period, uint8_t channels, int default_high) {
  // TIM1 を有効化
  RCC->APB2PCENR |= RCC_APB2Periph_TIM1;

  // TIM1 をリセット
  RCC->APB2PRSTR |= RCC_APB2Periph_TIM1;
  RCC->APB2PRSTR &= ~RCC_APB2Periph_TIM1;

  // TIM1 の周期
  TIM1->PSC = psc;
  TIM1->ATRLR = period;

  // CCxS=0 => Compare Capture Channel x は出力
  // CCxM=6/7 => Compare Capture Channel x は PWM モード
  //   CCxM=6 の場合、CNT <= CHxCVR で出力 1、CHxCVR < CNT で出力 0
  //   CCxM=7 の場合、CNT <= CHxCVR で出力 0、CHxCVR < CNT で出力 1
  uint16_t ocmode = TIM_OCMode_PWM1 | TIM_OCPreload_Enable | TIM_OCFast_Disable;
  uint16_t ccconf = TIM_CC1E | (default_high ? TIM_CC1P : 0);
  uint16_t chctlr = 0;
  uint16_t ccer = 0;

  if (channels & 1) {
    chctlr |= ocmode;
    ccer |= ccconf;
  }
  if (channels & 2) {
    chctlr |= ocmode << 8;
    ccer |= ccconf << 4;
  }
  TIM1->CHCTLR1 = chctlr;

  chctlr = 0;
  if (channels & 4) {
    chctlr |= ocmode;
    ccer |= ccconf << 8;
  }
  if (channels & 8) {
    chctlr |= ocmode << 8;
    ccer |= ccconf << 12;
  }
  TIM1->CHCTLR2 = chctlr;
  TIM1->CCER = ccer;

  // 出力を有効化
  TIM1->BDTR |= TIM_MOE;

  //       CNT の変化と PWM 出力
  //   ATRLR|...............__
  //        |            __|  |
  //        |         __|     |
  //  CHxCVR|......__|        |
  //        |   __|  :        |
  //       0|__|_____:________|__
  //         <------>|<------>|
  //         ________          __
  // OCxREF |   1    |___0____|   CCxM=6 の場合。CCxM=7 では 1/0 が反転。

  // CHCTLR の設定後に auto-reload preload を有効化
  TIM1->CTLR1 |= TIM_ARPE;
}

void TIM1_Start() {
  // アップデートイベント（UG）を発生させ、プリロードを行う
  TIM1->SWEVGR |= TIM_UG;

  // TIM1 をスタートする
  TIM1->CTLR1 |= TIM_CEN;
}

int main() {
  SystemInit();

  funGpioInitAll();

  funPinMode(PD2, GPIO_CFGLR_OUT_50Mhz_AF_PP); // T1CH1
  funPinMode(PA1, GPIO_CFGLR_OUT_50Mhz_AF_PP); // T1CH2
  funPinMode(PC3, GPIO_CFGLR_OUT_50Mhz_AF_PP); // T1CH3
  funPinMode(PC4, GPIO_CFGLR_OUT_50Mhz_AF_PP); // T1CH4

  TIM1_InitForPWM(0, 65535, 15, 0);
  TIM1->CH1CVR = 100;
  TIM1->CH2CVR = 1;
  TIM1->CH3CVR = 2;
  TIM1->CH4CVR = 4;
  TIM1_Start();

  while (1) {
    Delay_Ms(500);
    printf("hello\n");
  }
}
