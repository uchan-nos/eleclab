/*
 * Copyright (c) 2025 Kota UCHIDA
 *
 * CH32V003 の GPTM による PWM 出力を検証するプログラム。
 */
#include "ch32fun.h"
#include <assert.h>
#include <stdio.h>

/*
 * @param psc  プリスケーラの設定値（0 => 1:1）
 * @param period  タイマ周期
 * @param channels  有効化するチャンネル（ビットマップ）
 * @param default_high  1 なら無信号時の出力を 1 にする
 */
void TIM2_InitForPWM(uint16_t psc, uint16_t period, uint8_t channels, int default_high) {
  // TIM2 を有効化
  RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

  // TIM2 をリセット
  RCC->APB1PRSTR |= RCC_APB1Periph_TIM2;
  RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM2;

  // TIM2 の周期
  TIM2->PSC = psc;
  TIM2->ATRLR = period;

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
  TIM2->CHCTLR1 = chctlr;

  chctlr = 0;
  if (channels & 4) {
    chctlr |= ocmode;
    ccer |= ccconf << 8;
  }
  if (channels & 8) {
    chctlr |= ocmode << 8;
    ccer |= ccconf << 12;
  }
  TIM2->CHCTLR2 = chctlr;
  TIM2->CCER = ccer;

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
  TIM2->CTLR1 |= TIM_ARPE;
}

void TIM2_Start() {
  // アップデートイベント（UG）を発生させ、プリロードを行う
  TIM2->SWEVGR |= TIM_UG;

  // TIM2 をスタートする
  TIM2->CTLR1 |= TIM_CEN;
}

int main() {
  SystemInit();
  funGpioInitAll();

  // Partial remap: CH1/ETR/PC5, CH2/PC2, CH3/PD2, CH4/PC1
  AFIO->PCFR1 = AFIO_PCFR1_TIM2_REMAP_PARTIALREMAP1;

  funPinMode(PC5, GPIO_CFGLR_OUT_50Mhz_AF_PP); // T2CH1ETR
  funPinMode(PC2, GPIO_CFGLR_OUT_10Mhz_AF_PP); // T2CH2
  funPinMode(PD2, GPIO_CFGLR_OUT_2Mhz_AF_PP);  // T2CH3
  funPinMode(PC1, GPIO_CFGLR_OUT_50Mhz_AF_PP); // T2CH4

  TIM2_InitForPWM(0, 65535, 15, 0);
  TIM2->CH1CVR = 1;
  TIM2->CH2CVR = 1;
  TIM2->CH3CVR = 1;
  TIM2->CH4CVR = 2;
  TIM2_Start();

  while (1) {
    Delay_Ms(500);
  }
}
