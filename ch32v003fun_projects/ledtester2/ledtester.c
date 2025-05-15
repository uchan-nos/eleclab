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
#define LED_NUM 4
#define CSR 50 // 電流検出抵抗（Current Sensing Resistor）
static uint16_t adc_buf[DMA_CNT];
static uint32_t adc_conv_start_tick, adc_conv_end_tick;
static uint8_t adc_current_ch, led_current_ch;
static uint16_t goals_ua[LED_NUM]; // 制御目標の電流値（μA）
static int16_t errors_ua[LED_NUM]; // 目標と現在の電流値の差
static uint16_t led_pulse_width[LED_NUM]; // PWM パルス幅

// VCC の電圧（0.1mV 単位。5V=50000）
// ADC1_MeasureVCC() が設定する
uint16_t vcc_01mv;

#define SEL1_PIN PC0
#define SEL2_PIN PC7
#define AMPx49_PIN PD4
#define AMPx49_AN ANALOG_7
#define AMPx5_PIN PD6
#define AMPx5_AN ANALOG_6
#define VR_AN ANALOG_0
#define VF_AN ANALOG_5

// ADC の読み取り値を 0.1mV 単位に変換する
#define ADC_TO_01MV(adc) (((uint32_t)vcc_01mv * (adc)) >> ADC_BITS)

void __assert_func(const char *file, int line, const char *func, const char *expr) {
  printf("assertion failed inside %s (%s:%d): %s\n", func, file, line, expr);
  while (1) {
    __WFE();
  }
}

// Vref を用いて VCC を測定する
void MeasureVCC(void) {
  uint16_t adc_vref = 0;
  for (int i = 0; i < 16; ++i) {
    adc_vref += funAnalogRead(ANALOG_8);
  }
  adc_vref >>= 4;

  // Vref = 1.2V (typical)
  // adc_vref : ADC@Vcc = 1.2 : Vcc
  // Vcc = 1.2 * ADC@Vcc / adc_vref
  vcc_01mv = (UINT32_C(12000) << ADC_BITS) / adc_vref;
}

// ADC の読み取り値から電流値（μA 単位）を計算する
uint16_t CalcIF(uint16_t adc_vr, uint8_t adc_ch) {
  uint32_t vr_01mv = ADC_TO_01MV(adc_vr);
  uint32_t if_ua = (vr_01mv * 100) / CSR;
  switch (adc_ch) {
  case AMPx49_AN:
    if_ua /= 49;
    break;
  case AMPx5_AN:
    if_ua = 49 * if_ua / 10;
    break;
  case VR_AN:
    break;
  default:
    assert(0);
  }
  return if_ua;
}

void UpdateLEDCurrent(uint16_t if_ua) {
  uint16_t goal_ua = goals_ua[led_current_ch];
  int16_t err_ua = goal_ua - if_ua;
  int16_t d_err_ua = err_ua - errors_ua[led_current_ch];
  errors_ua[led_current_ch] = err_ua;

  uint16_t pw = led_pulse_width[led_current_ch];
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
  pw += (350 * (int32_t)err_ua >> 10) + (450 * (int32_t)d_err_ua >> 10);

  if (pw > 30000) {
    pw = 0;
  }

  //if (led_current_ch == 0) {
  //  printf("%s: led=%d goal_ua=%u if_ua=%u err_ua=%d d_err_ua=%d pw=%u\n",
  //      __func__, led_current_ch, goal_ua, if_ua, err_ua, d_err_ua, pw);
  //}
  led_pulse_width[led_current_ch] = pw;
  TIM1_SetPulseWidth(led_current_ch, pw);
}

// 使用するアンプ倍率を決定し、対応する ADC チャンネルを選択
void DetermineADCChForVR() {
  ADC1->CTLR2 &= ~ADC_DMA;
  uint16_t adc_vr = funAnalogRead(VR_AN);
  ADC1->CTLR2 |= ADC_DMA;
  //uint16_t adc_vr = 0;
  if (adc_vr < 20) {
    adc_current_ch = AMPx49_AN;
  } else if (adc_vr < 200) {
    adc_current_ch = AMPx5_AN;
  } else {
    adc_current_ch = VR_AN;
  }
  ADC1->RSQR3 = adc_current_ch;
}

int tim2_updated = 0;
void TIM2_Updated(void) {
  ++tim2_updated;

  ++led_current_ch;
  if (led_current_ch == LED_NUM) {
    led_current_ch = 0;
  }
  SelectLEDCh(led_current_ch);

  DetermineADCChForVR();

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
  uint16_t if_ua;

  switch (adc_current_ch) {
  case AMPx49_AN:
    // fall through
  case AMPx5_AN:
    // fall through
  case VR_AN:
    {
      uint16_t if_ua = CalcIF(adc_avg, adc_current_ch);
      UpdateLEDCurrent(if_ua);
    }
    break;
  case VF_AN:
    // something
    break;
  default:
    assert(0);
  }

  uint32_t adc_avg_end_tick = SysTick->CNT;
  //printf("DMA1_TC1: sum=%u avg=%u (%u ticks, %u ticks)\n",
  //        adc_sum,
  //        adc_avg,
  //        adc_conv_end_tick - adc_conv_start_tick,
  //        adc_avg_end_tick - adc_conv_end_tick);
}

void SelectLEDCh(uint8_t ch) {
  led_current_ch = ch;
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

  SelectLEDCh(0);

  // 出力 0 のときの ADC 値を下限とする
  uint16_t adc_min = 0;
  TIM1->CH1CVR = 0;
  Delay_Ms(100);
  for (int i = 0; i < 16; ++i) {
    Delay_Ms(5);
    adc_min += funAnalogRead(AMPx49_AN);
  }
  adc_min >>= 4;

  MeasureVCC();

  printf("adc_min=%u\n", adc_min);
  printf("Vcc=%d.%d(mV)\n", vcc_01mv / 10, vcc_01mv % 10);

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

  TIM2_InitForSimpleTimer(48000, 10); // 1s
  TIM2->DMAINTENR |= TIM_IT_Update;
  NVIC_EnableIRQ(TIM2_IRQn);

  goals_ua[0] = 100;

  printf("Starting TIM2\n");
  TIM2_Start();

  while (1) {
    printf("goal=100\n");
    goals_ua[0] = 100;
    Delay_Ms(5000);
    printf("goal=300\n");
    goals_ua[0] = 300;
    Delay_Ms(5000);
    printf("goal=1000\n");
    goals_ua[0] = 1000;
    Delay_Ms(5000);
    printf("goal=2000\n");
    goals_ua[0] = 1000;
    Delay_Ms(5000);
    printf("goal=0\n");
    goals_ua[0] = 0;
    Delay_Ms(5000);
  }

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
