#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>

// ADC デバイスのアドレス
#define I2C_ADDR 0x18

// I2C 通信速度（I2C モジュールのクロック周波数より低くなければならない）
#define I2C_CLKRATE 100000

// I2C モジュールのクロック周波数（2～36MHz で、通信速度より高くなければならない）
#define I2C_PRERATE 2000000

// 高速クロックの際に 36% デューティにする場合にコメントを外す（デフォルトは 33%）
//#define I2C_DUTY_16_9

// I2C タイムアウト回数
#define TIMEOUT_MAX 1000000

// UART 通信速度
#define UART_BAUD_RATE 115200

// UART の 1 ビットあたりのクロック数
#define UART_BIT_TICKS (FUNCONF_SYSTEM_CORE_CLOCK / UART_BAUD_RATE)

void I2cSetup(void) {
  uint16_t tempreg;

  // GPIOC と I2C を有効化
  RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
  RCC->APB1PCENR |= RCC_APB1Periph_I2C1;

  // SDA=PC1, 10MHz Output, alt func, open-drain
  GPIOC->CFGLR &= ~(0xf<<(4*1));
  GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_OD_AF)<<(4*1);

  // SCL=PC2, 10MHz Output, alt func, open-drain
  GPIOC->CFGLR &= ~(0xf<<(4*2));
  GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_OD_AF)<<(4*2);

  // I2C1 をリセット
  RCC->APB1PRSTR |= RCC_APB1Periph_I2C1;
  RCC->APB1PRSTR &= ~RCC_APB1Periph_I2C1;

  // I2C モジュールのクロックを設定
  I2C1->CTLR2 = (FUNCONF_SYSTEM_CORE_CLOCK/I2C_PRERATE) & I2C_CTLR2_FREQ;

  // I2C 通信速度を設定
#if (I2C_CLKRATE <= 100000)
  // 標準モード（<=100kHz）
  tempreg = (FUNCONF_SYSTEM_CORE_CLOCK/(2*I2C_CLKRATE)) & I2C_CKCFGR_CCR;
#else
  // 高速モード（>100kHz）
#  ifdef I2C_DUTY_16_9
  tempreg = (FUNCONF_SYSTEM_CORE_CLOCK/(25*I2C_CLKRATE)) & I2C_CKCFGR_CCR;
  tempreg |= I2C_CKCFGR_DUTY;
#  else
  // 33.3% duty cycle
  tempreg = (FUNCONF_SYSTEM_CORE_CLOCK/(3*I2C_CLKRATE)) & I2C_CKCFGR_CCR;
#  endif
  tempreg |= I2C_CKCFGR_FS;
#endif
  I2C1->CKCFGR = tempreg;

  // I2C1 モジュールを有効化
  I2C1->CTLR1 |= I2C_CTLR1_PE;

  // ACK 応答を有効化（PE=1 にした後に設定しなければならない）
  I2C1->CTLR1 |= I2C_CTLR1_ACK;
}

// I2C エラー定義
enum I2cErrors {
  I2CERR_NOT_BUSY,
  I2CERR_MASTER_MODE,
  I2CERR_TRANSMIT_MODE,
  I2CERR_TX_EMPTY,
  I2CERR_BYTE_TRANSMITTED,
  I2CERR_BYTE_RECEIVED,
  I2CERR_RECEIVE_MODE,
};
char *i2c_error_strings[] = {
  "not busy",
  "master mode",
  "transmit mode",
  "tx empty",
  "byte transmitted",
  "byte received",
  "receive mode",
};

// I2C エラー表示
int I2cError(enum I2cErrors err) {
  printf("I2cError: timeout waiting for %s\n\r", i2c_error_strings[err]);
  I2cSetup(); // I2C をリセット
  return 1;
}

uint32_t I2cReadStatus() {
  // STAR1 を STAR2 より先に読み出す必要がある
  uint32_t status = I2C1->STAR1;
  status |= I2C1->STAR2 << 16;
  return status;
}

// 最後に発生したイベントが引数で指定されたものであれば真を返す。
int I2cCheckEvent(uint32_t event) {
  return (I2cReadStatus() & event) == event;
}

// ビジー状態が終わるのを待つ。
int I2cWaitBusy() {
  int timeout = TIMEOUT_MAX;
  while ((I2C1->STAR2 & I2C_STAR2_BUSY) && timeout--);
  return timeout;
}

// イベント成立を待つ。成立時のタイムアウト値を返す。
int I2cWaitEvent(uint32_t event) {
  int timeout = TIMEOUT_MAX;
  while((!I2cCheckEvent(event)) && timeout--);
  return timeout;
}

// I2C バスへパケットを送る（割り込みを使わずブロッキングモード）
int I2cSend(uint8_t addr, uint8_t *data, uint8_t sz) {
  if (I2cWaitBusy() == -1) {
    return I2cError(I2CERR_NOT_BUSY);
  }

  // スタートビットを送る
  I2C1->CTLR1 |= I2C_CTLR1_START;
  if (I2cWaitEvent(I2C_EVENT_MASTER_MODE_SELECT) == -1) {
    return I2cError(I2CERR_MASTER_MODE);
  }

  // 7 ビットアドレスと write フラグを送る
  I2C1->DATAR = addr<<1;
  if (I2cWaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) == -1) {
    return I2cError(I2CERR_TRANSMIT_MODE);
  }

  // 1 バイトずつ送信
  while (sz--) {
    I2C1->DATAR = *data++;
    if (I2cWaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) == -1) {
      return I2cError(I2CERR_BYTE_TRANSMITTED);
    }
  }

  // ストップビットを送る
  I2C1->CTLR1 |= I2C_CTLR1_STOP;
  return 0;
}

// I2C バスからパケットを受け取る（割り込みを使わずブロッキングモード）
uint8_t I2cRecv(uint8_t addr, uint8_t *data, uint8_t sz) {
  if (I2cWaitBusy() == -1) {
    return I2cError(I2CERR_NOT_BUSY);
  }

  // スタートビットを送る
  I2C1->CTLR1 |= I2C_CTLR1_START;
  if (I2cWaitEvent(I2C_EVENT_MASTER_MODE_SELECT) == -1) {
    return I2cError(I2CERR_MASTER_MODE);
  }

  // 7 ビットアドレスと read フラグを送る
  I2C1->DATAR = addr<<1 | 1;
  if (I2cWaitEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) == -1) {
    return I2cError(I2CERR_RECEIVE_MODE);
  }

  // 1 バイトずつ受信
  while (sz--) {
    if (I2cWaitEvent(I2C_EVENT_MASTER_BYTE_RECEIVED) == -1) {
      return I2cError(I2CERR_BYTE_RECEIVED);
    }
    *data++ = I2C1->DATAR;
  }

  // ストップビットを送る
  I2C1->CTLR1 |= I2C_CTLR1_STOP;
  return 0;
}

#define UART_WRITE_TX(x) do { GPIOC->BSHR = 0x0010 << ((x) ? 0 : 16); } while (0)
#define UART_PHASE_START 10

volatile uint8_t tim1_buf;
volatile uint8_t tim1_phase;

void TIM1_UP_IRQHandler( void ) __attribute__((interrupt)) __attribute__((section(".srodata")));

void TIM1_UP_IRQHandler(void) {
  // 割り込みフラグをクリア
  TIM1->INTFR = ~TIM_EventSource_Update;

  if (tim1_phase == UART_PHASE_START) { // スタートビット
    UART_WRITE_TX(0);
    tim1_phase--;
  } else if (tim1_phase == 1) { // ストップビット
    UART_WRITE_TX(1);
    tim1_phase--;
  } else if (tim1_phase == 0) { // 待機状態では 1 を出す
    UART_WRITE_TX(1);
  } else { // データビット
    UART_WRITE_TX(tim1_buf & 1);
    tim1_buf >>= 1;
    tim1_phase--;
  }
}

// UART 送信用タイマを設定する
void UartSetupTim() {
  // TIM1 を有効化
  RCC->APB2PCENR |= RCC_APB2Periph_TIM1;

  // TIM1 をリセットしてレジスタを初期化
  RCC->APB2PRSTR |= RCC_APB2Periph_TIM1;
  RCC->APB2PRSTR &= ~RCC_APB2Periph_TIM1;

  // プリスケーラ
  TIM1->PSC = 0x0000;

  // 周期
  TIM1->ATRLR = UART_BIT_TICKS;

  // タイマー更新で割り込み
  TIM1->DMAINTENR = TIM_EventSource_Update;
  NVIC_EnableIRQ(TIM1_UP_IRQn);

  // タイマを始動
  TIM1->CTLR1 |= TIM_CEN;
}

void UartSetup() {
  // UART と、UART が必要とする IO ポートを有効化
  RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1;

  // 送信端子として PD5(UTX) の代わりに PC4 を使う
  // Push-Pull, 10MHz Output, PC4
  GPIOC->CFGLR &= ~(0xf<<(4*4));
  GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*4);

  // データ 8 ビット、ストップ 1 ビット、パリティ無し。
  // RX モードのみ有効化（TX は使わず、ソフトで実装）。
  USART1->CTLR1 = USART_WordLength_8b | USART_Parity_No | USART_Mode_Rx;
  USART1->CTLR2 = USART_StopBits_1;
  USART1->CTLR3 = USART_HardwareFlowControl_None;

  USART1->BRR = (FUNCONF_SYSTEM_CORE_CLOCK + UART_BAUD_RATE/2) / UART_BAUD_RATE;

  // UART 機能を開始
  USART1->CTLR1 |= CTLR1_UE_Set;

  UartSetupTim();
}

// UART へ 1 バイト送信
void UartPutChar(char c) {
  while (tim1_phase != 0);
  tim1_buf = c;
  tim1_phase = UART_PHASE_START;
}

// UART へバイト列を送信
int UartWrite(const char *buf, size_t size) {
  for(int i = 0; i < size; i++) {
    UartPutChar(buf[i]);
  }
  return size;
}

// I2C バスを介して ADC から 12 ビット値を受信
int16_t AdcRead() {
  uint8_t data[2];
  I2cRecv(I2C_ADDR, data, 2);
  uint16_t adc_raw = (data[0] << 4) | ((data[1] >> 4) & 0xf);
  return adc_raw & 0x800 ? adc_raw | 0xf000 : adc_raw;
}

void CmdAdcMulti(int n) {
  char s[64];
  for (int i = 0; i < n; i++) {
    Delay_Ms(2);
    int len = snprintf(s, sizeof(s), "%d\t%d\n", i, AdcRead());	
    UartWrite(s, len);
  }
}

void CmdAdcOnce() {
  char s[64];
  int len = snprintf(s, sizeof(s), "%d\n", AdcRead());	
  UartWrite(s, len);
}

int main() {
  char cmd[64], s[64];

  SystemInit();

  printf("UART_BIT_TICKS=%d\n", UART_BIT_TICKS);

  I2cSetup();
  UartSetup();

  while (1) {
    for (int i = 0; i < sizeof(cmd); i++) {
      while ((USART1->STATR & USART_STATR_RXNE) == 0);
      cmd[i] = USART1->DATAR;
      if (cmd[i] == '\n') {
        cmd[i] = '\0';
        break;
      }
    }

    if (strcmp(cmd, "r") == 0) {
      CmdAdcOnce();
    } else if (strcmp(cmd, "r1000") == 0) {
      CmdAdcMulti(1000);
    } else if (strcmp(cmd, "rc") == 0) {
      while (1) {
        CmdAdcOnce();
        Delay_Ms(1000);
        if ((USART1->STATR & USART_STATR_RXNE) != 0) {
          break;
        }
      }
    } else {
      int len = snprintf(s, sizeof(s), "unknown command '%s'\n", cmd);
      UartWrite(s, len);
    }
  }
}
