#include "periph.h"

#include "ch32fun.h"
#include <assert.h>

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

void TIM1_SetPulseWidth(uint8_t channel, uint16_t width) {
  switch (channel) {
  case 0: TIM1->CH1CVR = width; break;
  case 1: TIM1->CH2CVR = width; break;
  case 2: TIM1->CH3CVR = width; break;
  case 3: TIM1->CH4CVR = width; break;
  default: assert(0);
  }
}

void TIM2_InitForSimpleTimer(uint16_t psc, uint16_t period) {
  // TIM2 を有効化
  RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

  // TIM2 をリセット
  RCC->APB1PRSTR |= RCC_APB1Periph_TIM2;
  RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM2;

  // TIM2 の周期
  TIM2->PSC = psc;
  TIM2->ATRLR = period;

  TIM2->CTLR1 =
    TIM_OPMode_Repetitive |
    TIM_CKD_DIV1 |
    TIM_CounterMode_Up |
    TIM_ARPE;
}

void TIM2_Start() {
  // アップデートイベント（UG）を発生させ、プリロードを行う
  TIM2->SWEVGR |= TIM_UG;

  // TIM2 をスタートする
  TIM2->CTLR1 |= TIM_CEN;
}

void TIM2_IRQHandler(void) __attribute__((interrupt));
void TIM2_IRQHandler(void) {
  if (TIM2->INTFR & TIM_FLAG_Update) {
    // タイマの更新割り込み
    TIM2->INTFR = ~TIM_FLAG_Update;
    TIM2_Updated();
  }
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

void DMA1_InitForADC1(uint16_t* buf, uint16_t count, uint32_t it_en) {
  RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;

  // DMA を ADC1 向けに設定
  DMA1_Channel1->PADDR = (uint32_t)&ADC1->RDATAR;
  DMA1_Channel1->MADDR = (uint32_t)buf;
  DMA1_Channel1->CNTR = count;
  DMA1_Channel1->CFGR =
		DMA_M2M_Disable |
		DMA_Priority_VeryHigh |
		DMA_MemoryDataSize_HalfWord |
		DMA_PeripheralDataSize_HalfWord |
		DMA_MemoryInc_Enable |
    DMA_PeripheralInc_Disable |
		DMA_Mode_Normal |
		DMA_DIR_PeripheralSRC |
    it_en;

  // 全ての設定が終わってから DMA を有効化
  DMA1_Channel1->CFGR |= DMA_CFGR1_EN;
}

void DMA1_Channel1_IRQHandler(void) __attribute__((interrupt));
void DMA1_Channel1_IRQHandler(void) {
  if (DMA1->INTFR & DMA1_IT_TC1) {
    // DMA 転送完了
    DMA1->INTFCR = DMA1_IT_TC1;
    DMA1_Channel1_Transferred();
  }
}

void ADC1_InitForDMA(uint32_t trig) {
  RCC->APB2PCENR |= RCC_APB2Periph_ADC1;

  // ADC のレジスタを初期化
  RCC->APB2PRSTR |= RCC_APB2Periph_ADC1;
  RCC->APB2PRSTR &= ~RCC_APB2Periph_ADC1;

  // ADCCLK を 24 MHz に設定
  RCC->CFGR0 &= ~RCC_ADCPRE;
  RCC->CFGR0 |= RCC_ADCPRE_DIV2;

  // ADC の設定
  ADC1->CTLR2 =
    (trig == ADC_ExternalTrigConv_None ? 0 : ADC_EXTTRIG) |
    trig |
    ADC_DMA |
    ADC_ADON;

  // キャリブレーションをリセット
  ADC1->CTLR2 |= ADC_RSTCAL;
  while (ADC1->CTLR2 & ADC_RSTCAL);

  // キャリブレーション
  ADC1->CTLR2 |= ADC_CAL;
  while (ADC1->CTLR2 & ADC_CAL);
}

void ADC1_StopContinuousConv() {
  ADC1->CTLR2 &= ~ADC_CONT;
}

void ADC1_StartContinuousConv() {
  ADC1->CTLR2 |= ADC_CONT;
  ADC1->CTLR2 |= ADC_SWSTART;
}
