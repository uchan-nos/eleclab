/*
 * main.c
 *
 * 2023/06/23 Created by uchan-nos
 *
 * PC4: TIM1_CH4  NeoPixel signal output
 */

#include "debug.h"

//#define TIM_HIGH_SPEED

// 8  :  333.3ns
// 15 :  625.0ns
// 31 : 1292.7ns
#define T0H_WIDTH 8
#define T1H_WIDTH 15
#define TIM_PERIOD 31

#define DMACH_SIGGEN DMA1_Channel5

// NeoPixel 信号生成部（タイマと GPIO）を初期化
void InitSigGen() {
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_TIM1, ENABLE);

  // PC4 (Pin 7) を  TIM1CH4 の出力として使うため、AF_PP モードに設定
  // AF_PP: Alternate Function, Push Pull
  GPIO_InitTypeDef gpio_init = {
    .GPIO_Pin = GPIO_Pin_4,
    .GPIO_Mode = GPIO_Mode_AF_PP,
    .GPIO_Speed = GPIO_Speed_50MHz,
  };
  GPIO_Init(GPIOC, &gpio_init);

  // システムクロックはデフォルトの内蔵 24MHz RC クロック
  TIM_TimeBaseInitTypeDef tim_tbinit = {
#ifdef TIM_HIGH_SPEED
    .TIM_Prescaler = 0,
#else
    .TIM_Prescaler = 24000 - 1,
#endif
    .TIM_CounterMode = TIM_CounterMode_Up,
#ifdef TIM_HIGH_SPEED
    // T0H=320ns, T1H=640ns, 周期=1300ns にしたい。
    // Period=31 のとき、周期=31*1000/24≒1292ns
    .TIM_Period = TIM_PERIOD,
#else
    .TIM_Period = 1000,
#endif
    .TIM_ClockDivision = TIM_CKD_DIV1,
    .TIM_RepetitionCounter = 0,
  };
  TIM_TimeBaseInit(TIM1, &tim_tbinit);

  // OC(Output Compare) の設定
  TIM_OCInitTypeDef tim_ocinit = {
    // OCMode_PWM1: カウンタ < CVR のときにアクティブ、それ以降非アクティブ
    .TIM_OCMode = TIM_OCMode_PWM1,
    // 正出力を有効、相補出力は無効
    .TIM_OutputState = TIM_OutputState_Enable,
    .TIM_OutputNState = TIM_OutputState_Disable,
#ifdef TIM_HIGH_SPEED
    .TIM_Pulse = T1H_WIDTH,
#else
    .TIM_Pulse = 490,
#endif
    // 主力の論理をアクティブ High とする
    .TIM_OCPolarity = TIM_OCPolarity_High,
    .TIM_OCNPolarity = TIM_OCPolarity_High,
    // アイドル時は 0(Reset) 出力
    .TIM_OCIdleState = TIM_OCIdleState_Reset,
    .TIM_OCNIdleState = TIM_OCIdleState_Reset,
  };
  TIM_OC4Init(TIM1, &tim_ocinit); // Ch4 を設定

  // OC4PE をセットする
  // 電源投入直後の LED 点灯パターンが 490ms, 250ms, 30ms, 100ms, ... となれば期待通り
  TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);
}

#define DMA_SIG_LEN 16
uint8_t dma_sig_buf[DMA_SIG_LEN] = {
  T1H_WIDTH, T0H_WIDTH, T1H_WIDTH, T0H_WIDTH, T1H_WIDTH, T0H_WIDTH, T1H_WIDTH, T0H_WIDTH,
  T1H_WIDTH, T0H_WIDTH, T1H_WIDTH, T0H_WIDTH, T1H_WIDTH, T0H_WIDTH, T1H_WIDTH, T0H_WIDTH,
};

// NeoPixel 信号生成部にデータを供給する DMA を初期化
void InitSigDMA() {
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  DMA_DeInit(DMACH_SIGGEN);

  DMA_InitTypeDef dma_init = {
    .DMA_BufferSize = DMA_SIG_LEN,
    .DMA_DIR = DMA_DIR_PeripheralDST,
    .DMA_M2M = DMA_M2M_Disable,
    .DMA_MemoryBaseAddr = (uint32_t)&dma_sig_buf,
    .DMA_MemoryDataSize = DMA_MemoryDataSize_Byte,
    .DMA_MemoryInc = DMA_MemoryInc_Enable,
    .DMA_Mode = DMA_Mode_Circular,
    //.DMA_Mode = DMA_Mode_Normal,
    .DMA_PeripheralBaseAddr = (uint32_t)&TIM1->CH4CVR,
    .DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord,
    .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
    .DMA_Priority = DMA_Priority_High,
  };
  DMA_Init(DMACH_SIGGEN, &dma_init);
  DMA_Cmd(DMACH_SIGGEN, ENABLE);
}

volatile uint32_t it_cnt = 0;

void main() {
  InitSigGen();
  InitSigDMA();

  GPIO_InitTypeDef gpio_init = {
    .GPIO_Pin = GPIO_Pin_1,
    .GPIO_Mode = GPIO_Mode_Out_PP,
    .GPIO_Speed = GPIO_Speed_50MHz,
  };
  GPIO_Init(GPIOC, &gpio_init);


  NVIC_InitTypeDef nvic_init = {
    .NVIC_IRQChannel = TIM1_UP_IRQn,
    .NVIC_IRQChannelPreemptionPriority = 0,
    .NVIC_IRQChannelSubPriority = 0,
    .NVIC_IRQChannelCmd = ENABLE,
  };
  NVIC_Init(&nvic_init);

  nvic_init.NVIC_IRQChannel = DMA1_Channel5_IRQn;
  NVIC_Init(&nvic_init);

  DMA_ITConfig(DMACH_SIGGEN, DMA_IT_TC, ENABLE);
  //TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);

  TIM_DMACmd(TIM1,  TIM_DMA_Update, ENABLE);

  TIM_CtrlPWMOutputs(TIM1, ENABLE);
  TIM_Cmd(TIM1, ENABLE);
  TIM1->CH4CVR = T0H_WIDTH;

  while (1) {
  }
}

void DMA1_Channel5_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void DMA1_Channel5_IRQHandler(void) {
  DMA_ClearITPendingBit(DMA1_IT_TC5);
}

void TIM1_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void TIM1_UP_IRQHandler(void) {
  static uint8_t pin_stat = 1;
  GPIO_WriteBit(GPIOC, GPIO_Pin_1, pin_stat);
  pin_stat = 1 - pin_stat;
  it_cnt++;
  TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
}
