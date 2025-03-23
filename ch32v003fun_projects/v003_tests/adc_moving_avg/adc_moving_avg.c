/*
 * Example for calculating moving average of ADC values
 * by Kota UCHIDA
 *
 * This is based on ch32v003fun/examples/adc_fixed_fs by eeucalyptus.
 */

#include "ch32fun.h"
#include <stdio.h>
#include <stdlib.h>

#define WINDOW_LEN 128
volatile uint16_t adc_values[WINDOW_LEN] = {0};
volatile uint8_t adc_next_pos = 0;
volatile uint32_t adc_sum = 0;

/*
 *              <-- WINDOW_LEN --->
 * adc_values: [ v0 v1 v2 v3 .. vn ]
 *                  ^  ^
 *                  |  |
 *                  |  adc_next_pos = 最古の変換値
 *                  |
 *                  最新の変換値
 *
 * adc_next_pos は常に「最新の変換値」の 1 つ先を指す。
 * adc_sum は adc_values の総和を表す。
 */

/*
 * ADC の変換タイミング用に TIM1 を設定
 *
 * @param psc  プリスケーラの設定値（0 => 1:1）
 * @param period  タイマ周期
 */
void TIM1_InitForADC(uint16_t psc, uint16_t period) {
  // TIMER
  RCC->APB2PCENR |= RCC_APB2Periph_TIM1;
  TIM1->CTLR1 |= TIM_CounterMode_Up | TIM_CKD_DIV1;

  // ADC の変換トリガに使うために TRIGO に Update イベントを出力。
  TIM1->CTLR2 = TIM_TRGOSource_Update;

  // 48MHz / 1000 = 48KHz
  TIM1->PSC = psc;
  TIM1->ATRLR = period;

  // アップデートイベント（UG）を発生させ、プリロードを行う
  TIM1->SWEVGR = TIM_UG;

  // TIM1 をスタートする
  TIM1->CTLR1 |= TIM_CEN;
}

/*
 * TIM1 の Update イベントで変換を開始するように ADC を設定
 *
 * @param adc_ch  変換対象のチャンネル
 */
void ADC_Init(uint8_t adc_ch) {

  RCC->APB2PCENR |= RCC_APB2Periph_ADC1;

  // ADC のレジスタを初期化
  RCC->APB2PRSTR |= RCC_APB2Periph_ADC1;
  RCC->APB2PRSTR &= ~RCC_APB2Periph_ADC1;

  // ADCCLK を 24 MHz に設定
  RCC->CFGR0 &= ~RCC_ADCPRE;
  RCC->CFGR0 |= RCC_ADCPRE_DIV2;

  // TIM1 の TRGO をトリガとして変換を行う
  ADC1->CTLR2 = ADC_ADON | ADC_EXTTRIG | ADC_ExternalTrigConv_T1_TRGO;

  // 信号サンプリング時間を設定
  ADC1->SAMPTR2 = ADC_SampleTime_15Cycles << (3 * adc_ch);

  // シーケンス 1 に変換対象のチャンネルを設定。
  // SCAN=0 のシングル変換モードなので、シーケンス 1 のみが有効。
  ADC1->RSQR3 = adc_ch;

  // キャリブレーション
  ADC1->CTLR2 |= ADC_RSTCAL;
  while(ADC1->CTLR2 & ADC_RSTCAL);
  ADC1->CTLR2 |= ADC_CAL;
  while(ADC1->CTLR2 & ADC_CAL);

  // 変換完了割り込みを有効化
  ADC1->CTLR1 |= ADC_EOCIE;
  NVIC_EnableIRQ(ADC_IRQn);
}

void ADC1_IRQHandler(void) __attribute__((interrupt));
void ADC1_IRQHandler() {
  if(ADC1->STATR & ADC_FLAG_EOC) {
    // 最古の変換値を adc_sum から引く
    adc_sum -= adc_values[adc_next_pos];

    // RDATAR の読み込みで EOC フラグが自動で 0 になる
    uint16_t adc_value = ADC1->RDATAR;

    adc_values[adc_next_pos] = adc_value;
    adc_next_pos = (adc_next_pos + 1) & (WINDOW_LEN - 1);

    // 最新の変換値を adc_sum へ足す
    adc_sum += adc_value;
  }
}

int main() {
  SystemInit();

  printf("initializing\n");

  funGpioInitAll();
  funPinMode(PA1, GPIO_CFGLR_IN_ANALOG);
  ADC_Init(ADC_Channel_1);
  TIM1_InitForADC(0, 1000);

  printf("looping\n");

  while(1) {
    Delay_Ms(500);
    uint16_t adc_avg = (adc_sum + WINDOW_LEN/2) / WINDOW_LEN;
    uint16_t adc_latest_value = adc_values[(adc_next_pos - 1) & (WINDOW_LEN - 1)];
    printf("latest=%5u avg=%5u sum=%6lu", adc_latest_value, adc_avg, adc_sum);
    printf("\n");
  }
}
