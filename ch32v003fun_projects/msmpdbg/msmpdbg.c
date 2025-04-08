#include "ch32fun.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "msmpdbg.h"

/********************
 重要なグローバル変数
 ********************/
// 96KHz でタイマ割り込みでカウントアップする変数
volatile tick_t tick;
volatile struct Message msg[MSG_BUF_LEN];
volatile size_t msg_wpos; // msg の書き込み位置
volatile size_t raw_msg_wpos; // msg.raw_msg の書き込み位置

// 送受信同時デバッグモード
bool transmit_when_receive_mode;

uint8_t msmp_transmit_queue[TX_BUF_LEN];
size_t msmp_transmit_queue_len;
volatile size_t msmp_transmit_rpos;

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
  SenseSignal(tick, funDigitalRead(MSMP_RX_PIN));
}

/*
 * USART を MSMP 用に初期化
 */
void MSMP_USART_Init() {
  // 入出力ピンの設定（GPIO の RCC の設定は関数呼び出し側で行う）
  funPinMode(MSMP_TX_PIN, GPIO_CFGLR_OUT_10Mhz_AF_OD);
  funPinMode(MSMP_RX_PIN, GPIO_CFGLR_IN_FLOAT);

	// USART を有効化 & リセット
#if MSMP_USART_NUM == 1
	RCC->APB2PCENR |= RCC_APB2Periph_USART1;
  RCC->APB2PRSTR |= RCC_APB2Periph_USART1;
  RCC->APB2PRSTR &= ~RCC_APB2Periph_USART1;
  #define MSMP_PCLOCK FUNCONF_SYSTEM_CORE_CLOCK
#else
	RCC->APB1PCENR |= RCC_APB1Periph_USART2;
  RCC->APB1PRSTR |= RCC_APB1Periph_USART2;
  RCC->APB1PRSTR &= ~RCC_APB1Periph_USART2;
  #define MSMP_PCLOCK (FUNCONF_SYSTEM_CORE_CLOCK/2)
#endif

  // 割り込み許可
  NVIC_EnableIRQ(MSMP_USART_IRQn);

	// USART1 を 8n1 に設定、割り込みを有効化
	MSMP_USART->CTLR1 = USART_WordLength_8b | USART_Parity_No | USART_Mode_Tx
                    | USART_Mode_Rx | USART_CTLR1_RXNEIE;
	MSMP_USART->CTLR2 = USART_StopBits_1;

	// ボーレートを設定
	MSMP_USART->BRR = (MSMP_PCLOCK + MSMP_BAUDRATE/2) / MSMP_BAUDRATE;

  // USART1 を有効化
	MSMP_USART->CTLR1 |= CTLR1_UE_Set;
}

/*
 * MSMP USART 割り込みハンドラ
 */
void MSMP_USART_IRQHandler(void) __attribute__((interrupt));
void MSMP_USART_IRQHandler(void) {
  uint8_t recv_data = MSMP_USART->DATAR;
  MSMP_USART->STATR &= ~USART_FLAG_RXNE; // 受信割り込みフラグをクリア

  ProcByte(recv_data);
}

/*
 * USART を 操作用に初期化
 */
void CMD_USART_Init() {
  // 入出力ピンの設定（GPIO の RCC の設定は関数呼び出し側で行う）
  funPinMode(CMD_TX_PIN, GPIO_CFGLR_OUT_10Mhz_AF_PP);
  funPinMode(CMD_RX_PIN, GPIO_CFGLR_IN_FLOAT);

	// USART を有効化 & リセット
#if CMD_USART_NUM == 1
	RCC->APB2PCENR |= RCC_APB2Periph_USART1;
  RCC->APB2PRSTR |= RCC_APB2Periph_USART1;
  RCC->APB2PRSTR &= ~RCC_APB2Periph_USART1;
  #define CMD_PCLOCK FUNCONF_SYSTEM_CORE_CLOCK
#else
	RCC->APB1PCENR |= RCC_APB1Periph_USART2;
  RCC->APB1PRSTR |= RCC_APB1Periph_USART2;
  RCC->APB1PRSTR &= ~RCC_APB1Periph_USART2;
  #define CMD_PCLOCK (FUNCONF_SYSTEM_CORE_CLOCK/2)
#endif

  // 割り込み許可
  //NVIC_EnableIRQ(CMD_USART_IRQn);

	// USART1 を 8n1 に設定、割り込みを有効化
	CMD_USART->CTLR1 = USART_WordLength_8b | USART_Parity_No | USART_Mode_Tx
                   | USART_Mode_Rx;
	CMD_USART->CTLR2 = USART_StopBits_1;

	// ボーレートを設定
	CMD_USART->BRR = (CMD_PCLOCK + 115200/2) / 115200;

  // USART1 を有効化
	CMD_USART->CTLR1 |= CTLR1_UE_Set;
}

void ProcCommand(char *cmd) {
  if (strcmp(cmd, "start rec") == 0) {
    sig_wpos = 0;
    sig_record_mode = true;
  } else if (strcmp(cmd, "dump rec") == 0) {
    PlotSignal(10);
  } else if (strcmp(cmd, "dump msg") == 0) {
    DumpMessages(3);
  } else {
    printf("Unknown command: '%s'\r\n", cmd);
  }
}

// For debug writing to the UART.
int _write(int fd, const char *buf, int size) {
	for(int i = 0; i < size; i++){
	    while( !(CMD_USART->STATR & USART_FLAG_TC));
	    CMD_USART->DATAR = *buf++;
	}
	return size;
}

// single char to UART
int putchar(int c) {
	while( !(CMD_USART->STATR & USART_FLAG_TC));
	CMD_USART->DATAR = c;
	return 1;
}

/*
 * LEDs D2-5: PA4-7
 */
int main() {
  SystemInit();
  funGpioInitAll();
  funPinMode(PA4, GPIO_CFGLR_OUT_10Mhz_PP);
  funPinMode(PA5, GPIO_CFGLR_OUT_10Mhz_PP);
  funPinMode(PA6, GPIO_CFGLR_OUT_10Mhz_PP);
  funPinMode(PA7, GPIO_CFGLR_OUT_10Mhz_PP);

  CMD_USART_Init();

  uint32_t rcc_cfgr0 = RCC->CFGR0;
  printf("RCC_CFGR0 = %08lX (PPRE1=%lX PPRE2=%lX)\r\n",
         rcc_cfgr0,
         (rcc_cfgr0 & RCC_PPRE1) >> 8,
         (rcc_cfgr0 & RCC_PPRE2) >> 11);

  MSMP_USART_Init();

  TIM2_InitForPeriodicTimer(0, SIG_RECORD_TIM_PERIOD - 1); // 48MHz / 96KHz = 500
  
  char cmd[64];
  size_t cmd_i = 0;

  while (1) {
    GPIOA->OUTDR = cmd_i << 4;
    while ((CMD_USART->STATR & USART_FLAG_RXNE) == 0) {
      __WFI();
    }
    uint8_t c = CMD_USART->DATAR;

    if (c == '\r' || c == '\n') {
      putchar('\r');
      putchar('\n');
      if (cmd_i > 0) {
        cmd[cmd_i] = '\0';
        cmd_i = 0;
        ProcCommand(cmd);
      }
    } else if (c == '\b' || c == 0x7f) {
      if (cmd_i > 0) {
        --cmd_i;
        putchar('\b');
      }
    } else {
      cmd[cmd_i++] = c;
      putchar(c);
    }
  }
}
