#include "ch32fun.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "msmpdbg.h"

/********************
 重要なグローバル変数
 ********************/
// 96KHz でタイマ割り込みでカウントアップする変数
volatile tick_t tick;
volatile bool transmit_on_receive_mode = true;
volatile struct Message msg[MSG_BUF_LEN];
volatile size_t msg_wpos; // msg の書き込み位置
volatile size_t raw_msg_wpos; // msg.raw_msg の書き込み位置
uint8_t msmp_my_addr = 0x0E;
struct Message transmit_msg = {
  .start_tick = 0, // 送信時は raw_msg の添え字として使用
  .addr = 0xFE,
  .len = 8,
  .body = "MSMP-DBG",
};
uint16_t transmit_period_ms = 20;

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
 * TIM3 の周期を現在の transmit_period_ms に更新
 */
void TIM3_UpdatePeriod(void) {
  // TIM3 の周期
#if (FUNCONF_SYSTEM_CORE_CLOCK / 1000) < UINT16_MAX
  TIM3->PSC = FUNCONF_SYSTEM_CORE_CLOCK / 1000; // 1ms
  TIM3->ATRLR = transmit_period_ms - 1;
#else
  TIM3->PSC = FUNCONF_SYSTEM_CORE_CLOCK / 5000; // 0.2ms
  TIM3->ATRLR = transmit_period_ms * 5 - 1;
#endif

  // アップデートイベント（UG）を発生させ、プリロードを行う
  TIM3->SWEVGR = TIM_PSCReloadMode_Immediate;
}

/*
 * TIM3 を MSMP 送信間隔タイマとして設定
 */
void TIM3_InitForMSMPTimer() {
  // TIM3 を有効化
  RCC->APB1PCENR |= RCC_APB1Periph_TIM3;

  // TIM3 をリセット
  RCC->APB1PRSTR |= RCC_APB1Periph_TIM3;
  RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM3;

  // 周期を設定
  TIM3_UpdatePeriod();

  // 割り込み有効化
  NVIC_EnableIRQ(TIM3_IRQn);
  TIM3->DMAINTENR |= TIM_IT_Update;
}

void TIM3_Start(void) {
  // カウンタをリセット
  TIM3->CNT = 0;

  // 周期を更新
  TIM3_UpdatePeriod();

  // カウンタを有効化
  TIM3->CTLR1 |= TIM_CEN;
}

void TIM3_Stop(void) {
  // カウンタを無効化
  TIM3->CTLR1 &= ~TIM_CEN;
}

/*
 * TIM3 割り込みハンドラ
 */
void TIM3_IRQHandler(void) __attribute__((interrupt));
void TIM3_IRQHandler(void) {
  // 割り込みフラグをクリア
  TIM3->INTFR &= ~TIM_FLAG_Update;
  if (transmit_on_receive_mode && (MSMP_USART->STATR & USART_FLAG_TXE)) {
    /*
    TIM3 割り込みの初回が、なぜか TIM3 スタートの直後に発生し、
    先頭バイトの送信が次のバイトで上書きされてしまう。
    USART TXE フラグを確認することで、先頭バイトの上書きを阻止する。
    */
    MSMP_USART->DATAR = transmit_msg.raw_msg[transmit_msg.start_tick++];
    if (transmit_msg.start_tick == transmit_msg.len + 2) {
      transmit_msg.start_tick = 0;
      TIM3_Stop();
    }
  }
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
  if (MSMP_USART->STATR & USART_FLAG_RXNE) {
    uint8_t recv_data = MSMP_USART->DATAR;
    MSMP_USART->STATR &= ~USART_FLAG_RXNE; // 受信割り込みフラグをクリア
    ProcByte(recv_data);
  }
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

void MsgStarted(void) {
  if (transmit_on_receive_mode) {
    transmit_msg.start_tick = 0;
    MSMP_USART->DATAR = transmit_msg.raw_msg[transmit_msg.start_tick++];
    TIM3_Start();
  }
}

void ProcCommand(char *cmd) {
  if (strcmp(cmd, "start rec") == 0) {
    sig_wpos = 0;
    sig_record_mode = true;
  } else if (strcmp(cmd, "dump rec") == 0) {
    PlotSignal(10);
  } else if (strcmp(cmd, "dump msg") == 0) {
    DumpMessages(3);
  } else if (strncmp(cmd, "set addr ", 9) == 0) {
    msmp_my_addr = strtol(cmd + 9, NULL, 0);
    transmit_msg.addr = (transmit_msg.addr & 0xF0) | msmp_my_addr;
    printf("New address: %d\r\n", msmp_my_addr);
  } else if (strcmp(cmd, "enable txonrx") == 0) {
    transmit_on_receive_mode = true;
  } else if (strcmp(cmd, "disable txonrx") == 0) {
    transmit_on_receive_mode = false;
  } else if (strncmp(cmd, "set txaddr ", 11) == 0) {
    uint8_t txaddr = strtol(cmd + 11, NULL, 0) & 15;
    transmit_msg.addr = (txaddr << 4) | msmp_my_addr;
  } else if (strncmp(cmd, "set txbody ", 11) == 0) {
    char *body = cmd + 11;
    transmit_msg.len = strlen(body);
    memcpy(transmit_msg.body, body, transmit_msg.len);
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
  TIM3_InitForMSMPTimer();

  char cmd[64];
  size_t cmd_i = 0;

  while (1) {
    if (CMD_USART->STATR & USART_FLAG_RXNE) {
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
    } else if (msmp_flags & MFLAG_MSG_TO_ME) {
      msmp_flags &= ~MFLAG_MSG_TO_ME;
      printf("A msg to me is being received.\r\n");
    } else if (msmp_flags & MFLAG_MSG_TO_FORWARD) {
      msmp_flags &= ~MFLAG_MSG_TO_FORWARD;
      printf("A msg to forward is being received.\r\n");
    } else {
      __WFI();
    }
  }
}
