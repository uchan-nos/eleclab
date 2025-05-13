/*
 * Copyright (c) 2025 Kota UCHIDA
 *
 * LED テスター 2 のファームウェア
 */
#include "ch32fun.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "periph.h"

#define DMA_CNT 32
uint16_t adc_buf[DMA_CNT];

#define SEL1_PIN PC0
#define SEL2_PIN PC7
#define AMPx49_PIN PD4
#define AMPx49_AN ANALOG_7
#define AMPx5_PIN PD6
#define AMPx5_AN ANALOG_6
#define VR_AN ANALOG_0
#define VF_AN ANALOG_5
#define ADC_BITS 10

static uint32_t adc_conv_start_tick, adc_conv_end_tick;

void TIM2_Updated(void) {
  printf("TIM2: update\n");
  DMA1_Channel1->CNTR = DMA_CNT;
  ADC1_StartContinuousConv();
  adc_conv_start_tick = SysTick->CNT;
}

void DMA1_Channel1_Transferred(void) {
  adc_conv_end_tick = SysTick->CNT;
  ADC1_StopContinuousConv();
  uint16_t adc_sum = 0;
  for (int i = 0; i < DMA_CNT; ++i) {
    adc_sum += adc_buf[i];
  }
  uint16_t adc_avg = adc_sum / DMA_CNT;
  uint32_t adc_avg_end_tick = SysTick->CNT;
  printf("DMA1_TC1: sum=%u avg=%u (%u ticks, %u ticks)\n",
          adc_sum,
          adc_avg,
          adc_conv_end_tick - adc_conv_start_tick,
          adc_avg_end_tick - adc_conv_end_tick);
}

void SelectCh(uint8_t ch) {
  funDigitalWrite(SEL1_PIN, ch & 1);
  funDigitalWrite(SEL2_PIN, (ch >> 1) & 1);
}

uint16_t LEDIfToPWM(uint32_t if_ua) {
  return (69905 * if_ua) / 100000 + 7864;
}

#define MAKE_ADC_SAMPTR(ch, sample_time) \
  ((sample_time) << (3 * (ch)))

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
  TIM1->CH1CVR = 0;
  TIM1_Start();

  //TIM2_InitForEncoder();
  //TIM2->CNT = 20;

  SelectCh(0);

  // 出力 0 のときの ADC 値を下限とする
  uint16_t adc_min = 0;
  TIM1->CH1CVR = 0;
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
  TIM1->CH1CVR = LEDIfToPWM(goal_ua);

  int settled = 0; // 定常状態になったら 1

  DMA1_InitForADC1(adc_buf, DMA_CNT, DMA_IT_TC);
  NVIC_EnableIRQ(DMA1_Channel1_IRQn);

  ADC1_InitForDMA(ADC_ExternalTrigConv_None);
  ADC1->SAMPTR2 =
    MAKE_ADC_SAMPTR(AMPx5_AN, ADC_SampleTime_15Cycles) |
    MAKE_ADC_SAMPTR(AMPx49_AN, ADC_SampleTime_15Cycles);
  ADC1->RSQR1 = 0; // 1 チャンネル
  ADC1->RSQR3 = AMPx49_AN;

  TIM2_InitForSimpleTimer(48000, 1000); // 1s
  TIM2->DMAINTENR |= TIM_IT_Update;
  NVIC_EnableIRQ(TIM2_IRQn);

  printf("Starting TIM2\n");
  TIM2_Start();

  while (1);

  /*
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

    uint16_t cvr = TIM1->CH1CVR;
    if (diff < 0 && cvr < -diff) {
      cvr = 0;
    } else if (diff > 0 && cvr > 20000) {
      cvr = 20000;
    } else {
      cvr += diff;
    }
    TIM1->CH1CVR = cvr;

    if (abs(diff) == 0 && !settled) {
      printf("settled. adc=%d diff=%d cvr=%u\n", adc, diff, cvr);
      settled = 1;
    }
    //printf("adc=%d\n", adc);
  }
  */
}
