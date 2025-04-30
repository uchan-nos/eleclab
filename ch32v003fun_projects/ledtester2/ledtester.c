/*
 * Copyright (c) 2025 Kota UCHIDA
 *
 * LED テスター 2 のファームウェア
 */
#include "ch32fun.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

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

#define SEL1_PIN PC0
#define SEL2_PIN PC7
#define AMPx49_PIN PD4
#define AMPx49_AN ANALOG_7
#define AMPx5_PIN PD6
#define AMPx5_AN ANALOG_6
#define VR_AN ANALOG_0
#define VF_AN ANALOG_5
#define ADC_BITS 10

void SelectCh(uint8_t ch) {
  funDigitalWrite(SEL1_PIN, ch & 1);
  funDigitalWrite(SEL2_PIN, (ch >> 1) & 1);
}

void OPA1_Init(int enable, int neg_pin, int pos_pin) {
  assert(neg_pin == PA1 || neg_pin == PD0);
  assert(pos_pin == PA2 || pos_pin == PD7);

  uint32_t ctr = EXTEN->EXTEN_CTR;
  ctr |= enable ? EXTEN_OPA_EN : 0;
  ctr |= neg_pin == PD0 ? EXTEN_OPA_NSEL : 0;
  ctr |= pos_pin == PD7 ? EXTEN_OPA_PSEL : 0;
  EXTEN->EXTEN_CTR = ctr;
}

uint16_t LEDIfToPWM(uint32_t if_ua) {
  return (69905 * if_ua) / 100000 + 7864;
}

int main() {
  SystemInit();

  funAnalogInit();
  funGpioInitAll();

  funPinMode(AMPx49_PIN, GPIO_CFGLR_IN_ANALOG);
  funPinMode(AMPx5_PIN, GPIO_CFGLR_IN_ANALOG);
  funPinMode(SEL1_PIN, GPIO_CFGLR_OUT_50Mhz_PP);
  funPinMode(SEL2_PIN, GPIO_CFGLR_OUT_50Mhz_PP);
  funPinMode(PD2, GPIO_CFGLR_OUT_50Mhz_AF_PP); // T1CH1
  funPinMode(PA1, GPIO_CFGLR_OUT_50Mhz_AF_PP); // T1CH2
  funPinMode(PC3, GPIO_CFGLR_OUT_50Mhz_AF_PP); // T1CH3
  funPinMode(PC4, GPIO_CFGLR_OUT_50Mhz_AF_PP); // T1CH4

  AFIO->PCFR1 = AFIO_PCFR1_TIM2_REMAP_PARTIALREMAP2;
  funPinMode(PC1, GPIO_CFGLR_IN_PUPD); // T2CH1_2
  funPinMode(PD3, GPIO_CFGLR_IN_PUPD); // T2CH2_2
  funDigitalWrite(PC1, 1); // pull up
  funDigitalWrite(PD3, 1); // pull up

  OPA1_Init(1, PD0, PA2);

  TIM1_InitForPWM(0, 65535, 15, 0);
  TIM1->CH1CVR = 6000;
  TIM1->CH2CVR = 9000;
  TIM1->CH3CVR = 10000;
  TIM1->CH4CVR = 0;
  TIM1_Start();

  TIM2_InitForEncoder();
  TIM2->CNT = 20;

  SelectCh(3);

  // 出力 0 のときの ADC 値を下限とする
  uint16_t adc_min = 0;
  TIM1->CH4CVR = 0;
  Delay_Ms(100);
  for (int i = 0; i < 16; ++i) {
    Delay_Ms(5);
    adc_min += funAnalogRead(AMPx49_AN);
  }
  adc_min >>= 4;

  printf("adc_min=%u\n", adc_min);

  uint16_t adc_vref = 0;
  for (int i = 0; i < 16; ++i) {
    adc_vref += funAnalogRead(ANALOG_8);
  }
  adc_vref >>= 4;
  // Vref = 1.2V (typical)
  // adc_vref/16 : ADC@Vcc = 1.2 : Vcc
  // Vcc = 1.2 * ADC@Vcc / (adc_vref/16)
  const uint16_t vcc_mv = (UINT32_C(1200) * (1 << ADC_BITS)) / adc_vref;
  printf("adc_vref=%u Vcc=%d(mV)\n", adc_vref, vcc_mv);

  uint16_t prev_cnt = TIM2->CNT;
  uint32_t goal_ua = prev_cnt * 10;
  TIM1->CH4CVR = LEDIfToPWM(goal_ua);

  int settled = 0; // 定常状態になったら 1


  while (1) {
    Delay_Ms(20);
    uint16_t cnt = TIM2->CNT;
    if (prev_cnt != cnt) {
      prev_cnt = cnt;
      goal_ua = cnt * 10;
      settled = 0;
    } else if (settled) {
      continue;
    }

    int adc = funAnalogRead(AMPx49_AN);
    // adc/ADC@Vcc * Vcc = 49 * Vr
    // Vr = If * 50
    // If = Vr/50
    //    = adc/ADC@Vcc * Vcc / (49 * 50)
    //    = (adc * Vcc) / (ADC@Vcc * 49 * 50)
    uint32_t if_ua = (adc * 5) * 100000 / ((1 << ADC_BITS) * 49 * 5);
    int diff = goal_ua - if_ua;

    if (diff < 0 && adc <= adc_min) {
      printf("adc=%u adc_min=%d\n", adc, adc_min);
      continue; // これ以上下がらない
    }

    uint16_t cvr = TIM1->CH4CVR;
    if (diff < 0 && cvr < -diff) {
      cvr = 0;
    } else if (diff > 0 && cvr > 20000) {
      cvr = 20000;
    } else {
      cvr += diff;
    }
    TIM1->CH4CVR = cvr;

    if (abs(diff) == 0 && !settled) {
      printf("settled. adc=%d diff=%d cvr=%u\n", adc, diff, cvr);
      settled = 1;
    }
    //printf("adc=%d\n", adc);
  }
}
