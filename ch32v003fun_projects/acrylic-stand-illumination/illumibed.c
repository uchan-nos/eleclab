#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>

// クロック定義
#define TIM_CLOCK_MH (FUNCONF_SYSTEM_CORE_CLOCK/1000000)

// ナノ秒でのパルス幅
#define T0H_WIDTH_NS 333
#define T1H_WIDTH_NS 625
#define TIM_PERIOD_NS 1300

// タイマ値に変換
#define NS_TO_TIM(ns) (((ns) * TIM_CLOCK_MH + 1000/2) / 1000)
#define T0H_WIDTH  NS_TO_TIM(T0H_WIDTH_NS)
#define T1H_WIDTH  NS_TO_TIM(T1H_WIDTH_NS)
#define TIM_PERIOD NS_TO_TIM(TIM_PERIOD_NS)

#define DMACH_SIGGEN DMA1_Channel5
#define DMACH_SIGGEN_IRQ DMA1_Channel5_IRQn

// NeoPixel 信号生成部（タイマと GPIO）を初期化
void InitSigGen() {
  // Enable GPIOC and TIM1
  RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_TIM1;

  // PC4 (Pin 7) を  TIM1CH4 の出力として使うため、PP_AF モードに設定
  // PP_AF: Push Pull, Alternate Function
  GPIOC->CFGLR &= ~(0xf<<(4*4));
  GPIOC->CFGLR |= (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP_AF)<<(4*4);

  // TIM1 をリセットしてレジスタを初期化する
  RCC->APB2PRSTR |= RCC_APB2Periph_TIM1;
  RCC->APB2PRSTR &= ~RCC_APB2Periph_TIM1;

  // プリスケーラ
  TIM1->PSC = 0x0000;

  // T0H=320ns, T1H=640ns, 周期=1300ns にしたい。
  // Period=31 のとき、周期=31*1000/24=1292ns
  TIM1->ATRLR = TIM_PERIOD;

  // CH4 を有効化
  TIM1->CCER |= TIM_CC4E;

  // CH4 は PWM mode 1 (CC4S = 00 OC4M = 110)
  TIM1->CHCTLR2 |= TIM_OC4M_2 | TIM_OC4M_1;

  // CH4 Compare Capture Register
  TIM1->CH4CVR = 10;

  // TIM1 出力を有効化
  TIM1->BDTR |= TIM_MOE;

  // タイマー更新で DMA を発動
  TIM1->DMAINTENR = TIM_DMA_Update;
}

// DMA のバッファは 2 バイト = 16 ビット分
uint8_t dma_sig_buf[16];

// NeoPixel 信号生成部にデータを供給する DMA を初期化
void InitSigDMA() {
  RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;

  // DMA を初期化
  DMACH_SIGGEN->CFGR = 0;
  DMACH_SIGGEN->CNTR = 0;
  DMACH_SIGGEN->PADDR = 0;
  DMACH_SIGGEN->MADDR = 0;
  DMA1->INTFCR |= DMA1_Channel5_IT_Mask;

  // dma_sig_buf -> TIM1->CH4CVR への DMA 転送を設定
  DMACH_SIGGEN->PADDR = (uint32_t)&TIM1->CH4CVR;
  DMACH_SIGGEN->MADDR = (uint32_t)dma_sig_buf;
  DMACH_SIGGEN->CNTR = 16;
  DMACH_SIGGEN->CFGR  =
    DMA_DIR_PeripheralDST |
    DMA_M2M_Disable |
    DMA_Priority_VeryHigh |
    DMA_MemoryDataSize_Byte |
    DMA_PeripheralDataSize_HalfWord |
    DMA_MemoryInc_Enable |
    DMA_Mode_Circular |
    DMA_IT_TC | // 転送完了割り込み
    DMA_IT_HT;  // 半転送完了割り込み

  // DMA 割り込みを有効化
  NVIC_EnableIRQ(DMACH_SIGGEN_IRQ);

  // DMA を有効化
  DMACH_SIGGEN->CFGR |= DMA_CFGR1_EN;
}

void InitPA() {
  RCC->APB2PCENR |= RCC_APB2Periph_GPIOA;

  // PA1 (Pin 1) と PA2 (Pin 3) を出力に設定
  GPIOA->CFGLR =
    (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*1) |
    (GPIO_Speed_50MHz | GPIO_CNF_OUT_PP)<<(4*2);
}

void InitSwitch() {
  RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;

  // PC1 (Pin 5), PC2 (Pin 6) をスイッチ入力として使うため、プルアップ入力に設定
  GPIOC->CFGLR &= ~(0xff<<(4*1));
  GPIOC->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_PUPD)<<(4*1);
  GPIOC->CFGLR |= (GPIO_Speed_In | GPIO_CNF_IN_PUPD)<<(4*2);
  GPIOC->OUTDR |= 0x06; // pull-up
}

#define NUM_LED_MAX 8
uint8_t send_data[3 * NUM_LED_MAX + 1] = {
    // Green, Red, Blue
    5, 0x16, 0x17,
};
size_t send_data_len = 1 * 3;
size_t send_index = 0;

uint8_t CalcNextSendData() {
  return ((uint16_t)send_data[send_index] << 2) |
         ((uint16_t)send_data[send_index + 1] >> 6);
}

void ByteToPulsePeriodArray(uint8_t *pulse_array, uint8_t data) {
  for (size_t i = 0; i < 8; i++) {
    pulse_array[i] = data & 0x80 ? T1H_WIDTH : T0H_WIDTH;
    data <<= 1;
  }
}

volatile int it_cnt = 0;
int stop_signal = 0;
int stop_tim = 0;

int SendLEDData() {
  if (TIM1->CTLR1 & TIM_CEN) {
    // まだタイマが動作中なので、何もしない
    printf("SendLEDData has been canceled.\n");
    return -1;
  }

  // 先頭のビットから送出するために、毎回 DMA とタイマを初期化
  InitSigDMA();
  InitSigGen();

  // 状態を初期化
  stop_signal = 0;
  stop_tim = 0;

  for (send_index = 0; send_index < 2; send_index++) {
    uint8_t *buf = dma_sig_buf + (send_index << 3);
    ByteToPulsePeriodArray(buf, CalcNextSendData());
  }

  // タイマの初期値を設定するために、一旦プリロードを無効にする
  TIM1->CHCTLR2 &= ~(TIM_OCPreload_Enable << 8);
  TIM1->CH4CVR = send_data[0] & 0x80 ? T1H_WIDTH : T0H_WIDTH;

  // 波形安定化のためにプリロードを有効にする
  TIM1->CHCTLR2 |= (TIM_OCPreload_Enable << 8);
  TIM1->CH4CVR = send_data[0] & 0x40 ? T1H_WIDTH : T0H_WIDTH;

  // ビット列の送出を開始する
  TIM1->CTLR1 |= TIM_CEN;

  return 0;
}

#define SHOW_PROCESS_TIME

int main() {
  SystemInit();
  InitSwitch();

#ifdef SHOW_PROCESS_TIME
  InitPA();
#endif

  printf("Init finished.\n");
  printf(" FUNCONF_USE_PLL = %d\n", FUNCONF_USE_PLL);
  printf(" FUNCONF_SYSTEM_CORE_CLOCK = %d\n", FUNCONF_SYSTEM_CORE_CLOCK);
  printf(" T0H_WIDTH = %d = %d (ns)\n", T0H_WIDTH, T0H_WIDTH_NS);
  printf(" T1H_WIDTH = %d = %d (ns)\n", T1H_WIDTH, T1H_WIDTH_NS);
  printf(" TIM_PERIOD = %d = %d (ns)\n", TIM_PERIOD, TIM_PERIOD_NS);

  GPIOA->BSHR = GPIO_Pin_1;

  SendLEDData();

  Delay_Ms(500);

  GPIOA->BSHR = GPIO_Pin_1 << 16;

  // TIM1 を有効化
  TIM1->CTLR1 |= TIM_CEN;

  printf("timer is enabled.\n");

  int prev_sw1 = 0, prev_sw2 = 0;

  int loop_cnt = 0;
  while (1) {
    loop_cnt++;
    Delay_Ms(10);

    GPIOA->BSHR = GPIO_Pin_1;

    int sw1 = (GPIOC->INDR & GPIO_Pin_1) == 0;
    int sw2 = (GPIOC->INDR & GPIO_Pin_2) == 0;

    if (!prev_sw1 && sw1) {
      send_data[0] += 16;
    } else if (!prev_sw2 && sw2) {
      send_data[0] -= 16;
    }

    prev_sw1 = sw1;
    prev_sw2 = sw2;

    SendLEDData();

    GPIOA->BSHR = GPIO_Pin_1 << 16;
  }
}

void DMA1_Channel5_IRQHandler( void ) __attribute__((interrupt)) __attribute__((section(".srodata")));

void DMA1_Channel5_IRQHandler(void) {
#ifdef SHOW_PROCESS_TIME
  GPIOA->BSHR = GPIO_Pin_2;
#endif
  it_cnt++;

  uint8_t data = 0;
  uint32_t intfr = DMA1->INTFR;

  if (stop_signal == 0) {
    if (send_index < send_data_len) {
      data = CalcNextSendData();
      send_index++;
    } else {
      stop_signal = 1;
    }
  }

  if (intfr & stop_tim) {
    // タイマを止める
    TIM1->CTLR1 &= ~TIM_CEN;
  }

  DMA1->INTFCR = DMA1_IT_GL5;

  if (intfr & DMA1_IT_HT5) {
    // dma_sig_buf の前半部分が転送完了
    if (stop_signal) {
      memset(dma_sig_buf, 0, 8);
      if (stop_tim == 0) {
        stop_tim = DMA1_IT_HT5;
      }
    } else {
      ByteToPulsePeriodArray(dma_sig_buf, data);
      if (send_index == send_data_len) {
        dma_sig_buf[6] = dma_sig_buf[7] = 0;
      }
    }
  } else if (intfr & DMA1_IT_TC5) {
    // dma_sig_buf の後半部分が転送完了
    if (stop_signal) {
      memset(dma_sig_buf + 8, 0, 8);
      if (stop_tim == 0) {
        stop_tim = DMA1_IT_TC5;
      }
    } else {
      ByteToPulsePeriodArray(dma_sig_buf + 8, data);
      if (send_index == send_data_len) {
        dma_sig_buf[8 + 6] = dma_sig_buf[8 + 7] = 0;
      }
    }
  }

#ifdef SHOW_PROCESS_TIME
  GPIOA->BSHR = GPIO_Pin_2 << 16;
#endif
}
