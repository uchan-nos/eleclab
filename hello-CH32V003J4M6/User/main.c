/*
 * main.c
 *
 * 2023/06/23 Created by uchan-nos
 *
 * PC4: TIM1_CH4  NeoPixel signal output
 */

#include <stdbool.h>
#include <string.h>
#include "debug.h"

// デバッグ用にタイマをゆっくり動かすには、以下をコメントアウト
#define TIM_HIGH_SPEED

// クロック定義
#define TIM_CLOCK UINT64_C(48000000)
#define SEC_IN_NS UINT64_C(1000000000)


#ifdef TIM_HIGH_SPEED

// ナノ秒でのパルス幅
#define T0H_WIDTH_NS 333
//#define T1H_WIDTH_NS 625
#define T1H_WIDTH_NS 900
#define TIM_PERIOD_NS 1300

// タイマ値に変換
#define NS_TO_TIM(ns) (((ns) * TIM_CLOCK + SEC_IN_NS/2) / SEC_IN_NS)
#define T0H_WIDTH  NS_TO_TIM(T0H_WIDTH_NS)
#define T1H_WIDTH  NS_TO_TIM(T1H_WIDTH_NS)
#define TIM_PERIOD NS_TO_TIM(TIM_PERIOD_NS)

#else
#define T0H_WIDTH 20
#define T1H_WIDTH 250
#define TIM_PERIOD 400
#endif

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
    .TIM_Prescaler = 0xffff,
#endif
    .TIM_CounterMode = TIM_CounterMode_Up,
    // T0H=320ns, T1H=640ns, 周期=1300ns にしたい。
    // Period=31 のとき、周期=31*1000/24≒1292ns
    .TIM_Period = TIM_PERIOD,
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
    .TIM_Pulse = T1H_WIDTH,
    // 主力の論理をアクティブ High とする
    .TIM_OCPolarity = TIM_OCPolarity_High,
    .TIM_OCNPolarity = TIM_OCPolarity_High,
    // アイドル時は 0(Reset) 出力
    .TIM_OCIdleState = TIM_OCIdleState_Reset,
    .TIM_OCNIdleState = TIM_OCIdleState_Reset,
  };
  TIM_OC4Init(TIM1, &tim_ocinit); // Ch4 を設定
  TIM_CtrlPWMOutputs(TIM1, ENABLE);
}

// DMA のバッファは 2 バイト = 16 ビット分
uint8_t dma_sig_buf[16];

// NeoPixel 信号生成部にデータを供給する DMA を初期化
void InitSigDMA() {
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  DMA_DeInit(DMACH_SIGGEN);

  DMA_InitTypeDef dma_init = {
    .DMA_BufferSize = 16,
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
  // 転送完了（TC）と半転送完了（HT）で割り込みをかける
  DMA_ITConfig(DMACH_SIGGEN, DMA_IT_TC | DMA_IT_HT, ENABLE);
  DMA_Cmd(DMACH_SIGGEN, ENABLE);

  TIM_DMACmd(TIM1, TIM_DMA_Update, ENABLE);

  NVIC_InitTypeDef nvic_init = {
    .NVIC_IRQChannel = DMA1_Channel5_IRQn,
    .NVIC_IRQChannelPreemptionPriority = 0,
    .NVIC_IRQChannelSubPriority = 0,
    .NVIC_IRQChannelCmd = ENABLE,
  };
  NVIC_Init(&nvic_init);
}

void InitPA2() {
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

  // PA2 (Pin 3) を出力に設定
  GPIO_InitTypeDef gpio_init = {
    .GPIO_Pin = GPIO_Pin_2,
    .GPIO_Mode = GPIO_Mode_Out_PP,
    .GPIO_Speed = GPIO_Speed_50MHz,
  };
  GPIO_Init(GPIOA, &gpio_init);

  /*
  GPIO_Init(GPIOC, &gpio_init);

  gpio_init.GPIO_Pin = GPIO_Pin_6;
  GPIO_Init(GPIOD, &gpio_init);
  */
}

#define NUM_LED_MAX 512

/* LED 色データを 6 ビット右シフトした値を格納した配列
 *
 * RGB の順で格納する例：
 *            7    2 1  0   7    2 1  0   7    2 1  0
 * send_data [ 無効 |R7:6] [ R5:0 |G7:6] [ G5:0 |B7:6] ...
 *     index       0             1             2       ...
 */

uint8_t send_data[3 * NUM_LED_MAX + 1];
size_t send_data_len;
size_t send_index = 0;

uint8_t NextSendData() {
  return send_data[send_index++];
}

void ByteToPulsePeriodArray(uint8_t *pulse_array, uint8_t data) {
  for (size_t i = 0; i < 8; i++) {
    pulse_array[i] = data & 0x80 ? T1H_WIDTH : T0H_WIDTH;
    data <<= 1;
  }
}

#define SHOW_PROCESS_TIME
volatile bool stop_tim1_next = false;

void main() {
  Delay_Init();
  InitSigGen();
  InitSigDMA();

#ifdef SHOW_PROCESS_TIME
  InitPA2();
#endif

  memset(send_data, 0, sizeof(send_data));
  send_data_len = 6;
  uint8_t data = 0;
  for (size_t i = 0; i < send_data_len; i++) {
    data = 16 * (i + 1) + i + 1;
    send_data[i] |= (data >> 6) & 0x03;
    send_data[i + 1] = (data << 2) & 0xfc;
  }

  uint8_t first_2bits = NextSendData();
  ByteToPulsePeriodArray(dma_sig_buf + 0, NextSendData());
  ByteToPulsePeriodArray(dma_sig_buf + 8, NextSendData());

  // タイマの初期値を設定するために、一旦プリロードを無効にする
  TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Disable);
  TIM1->CH4CVR = first_2bits & 2 ? T1H_WIDTH : T0H_WIDTH;

  // 波形安定化のためにプリロードを有効にする
  TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);
  TIM1->CH4CVR = first_2bits & 1 ? T1H_WIDTH : T0H_WIDTH;

  TIM_Cmd(TIM1, ENABLE);

  while (TIM1->CTLR1 & TIM_CEN);
  Delay_Us(400);

  TIM1->CNT = 0;
  InitSigDMA();

  send_index = 0;
  stop_tim1_next = false;

  first_2bits = NextSendData();
  ByteToPulsePeriodArray(dma_sig_buf + 0, NextSendData());
  ByteToPulsePeriodArray(dma_sig_buf + 8, NextSendData());

  // タイマの初期値を設定するために、一旦プリロードを無効にする
  TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Disable);
  TIM1->CH4CVR = first_2bits & 2 ? T1H_WIDTH : T0H_WIDTH;

  // 波形安定化のためにプリロードを有効にする
  TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);
  TIM1->CH4CVR = first_2bits & 1 ? T1H_WIDTH : T0H_WIDTH;

  TIM_Cmd(TIM1, ENABLE);

  while (1) {
  }
}

void DMA1_Channel5_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

void DMA1_Channel5_IRQHandler(void) {
#ifdef SHOW_PROCESS_TIME
  GPIO_WriteBit(GPIOA, GPIO_Pin_2, Bit_SET);
#endif

  uint8_t *dma_buf = dma_sig_buf;
  if (DMA_GetITStatus(DMA1_IT_HT5) == SET) {
    // dma_sig_buf の前半部分が転送完了
    DMA_ClearITPendingBit(DMA1_IT_HT5);
  } else {
    // dma_sig_buf の後半部分が転送完了
    DMA_ClearITPendingBit(DMA1_IT_TC5);
    dma_buf += 8;
  }

  if (send_index <= send_data_len) {
    uint8_t data = NextSendData();
    ByteToPulsePeriodArray(dma_buf, data);
    if (send_index > send_data_len) {
      dma_buf[6] = dma_buf[7] = 0;
    }
  } else if (!stop_tim1_next) {
    memset(dma_buf, 0, 8);
    stop_tim1_next = true;
  } else {
    TIM_Cmd(TIM1, DISABLE);
  }

#ifdef SHOW_PROCESS_TIME
  GPIO_WriteBit(GPIOA, GPIO_Pin_2, Bit_RESET);
#endif
}

/*
 * DMA 割り込みの処理時間
 * 1: 6.7375us
 * 2: 6.49us
 * 3: 6.655us
 *
 * 1: 6.5275us
 * 2: 6.235us
 * 3: 6.4875us
 * 4: 6.2375us
 */
