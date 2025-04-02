#include "ch32fun.h"

#include <stddef.h>
#include <stdint.h>
#include "msmpdbg.h"

/********
 各種設定
 ********/
// 信号記録のサンプリングレート
#define SIG_RECORD_RATE 96000 // 96KHz
// メッセージ受信バッファサイズ（バイト）
#define MSG_BUF_SIZE 1024 // 1KB
// 信号記録バッファサイズ（バイト）
#define SIG_BUF_SIZE (17*1024)

/************************
 設定値から導出される定数
 ************************/
// 信号記録の周期
#define SIG_RECORD_TIM_PERIOD (FUNCONF_SYSTEM_CORE_CLOCK / SIG_RECORD_RATE)
// メッセージ受信バッファの要素数
#define MSG_BUF_LEN (MSG_BUF_SIZE / sizeof(struct Message))
// 信号記録バッファの要素数
#define SIG_BUF_LEN (SIG_BUF_SIZE / sizeof(tick_t))

/********************
 重要なグローバル変数
 ********************/
// 96KHz でタイマ割り込みでカウントアップする変数
volatile tick_t tick;
volatile struct Message msg[MSG_BUF_LEN];
volatile size_t msg_wpos;
volatile tick_t sig[SIG_BUF_LEN];
volatile size_t sig_wpos;

/*
 * TIM1: 16 bit ADTM
 * TIM2/3/4: 16 bit GDTM
 */

/*
 * TIM2 を周期タイマとして設定
 *
 * @param psc  プリスケーラの設定値（0 => 1:1）
 * @param period  タイマ周期
 */
void TIM2_InitForPeriodicTimer(uint16_t psc, uint16_t period) {
  // TIM2 を有効化
  RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

  // TIM2 をリセット
  RCC->APB1PRSTR |= RCC_APB1Periph_TIM2;
  RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM2;

  // TIM2 の周期
  TIM2->PSC = psc;
  TIM2->ATRLR = period;

  // アップデートイベント（UG）を発生させ、プリロードを行う
  TIM2->SWEVGR = TIM_PSCReloadMode_Immediate;

  // 割り込み有効化
  NVIC_EnableIRQ(TIM2_IRQn);
  TIM2->DMAINTENR |= TIM_IT_Update;

  // カウンタを有効化
  TIM2->CTLR1 |= TIM_CEN;
}

/*
 * TIM2 割り込みハンドラ
 */
void TIM2_IRQHandler(void) __attribute__((interrupt));
void TIM2_IRQHandler(void) {
  // 割り込みフラグをクリア
  TIM2->INTFR &= ~TIM_FLAG_Update;

  ++tick;
}

/*
 * LEDs D2-5: PA4-7
 */
int main() {
  SystemInit();
  funGpioInitAll();
  funPinMode( PA4, GPIO_CFGLR_OUT_10Mhz_PP );
  funPinMode( PA5, GPIO_CFGLR_OUT_10Mhz_PP );
  funPinMode( PA6, GPIO_CFGLR_OUT_10Mhz_PP );
  funPinMode( PA7, GPIO_CFGLR_OUT_10Mhz_PP );

  TIM2_InitForPeriodicTimer(0, SIG_RECORD_TIM_PERIOD - 1); // 48MHz / 96KHz = 500
  uint32_t last_tick = 0;
  uint8_t cnt = 0;

  while (1) {
    ++cnt;
    GPIOA->OUTDR = cnt << 4;

    while (tick < last_tick + SIG_RECORD_RATE/2) {
      __WFI();
    }
    last_tick += SIG_RECORD_RATE/2;
  }
}
