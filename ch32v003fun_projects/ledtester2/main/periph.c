#include "main.h"

#include "ch32fun.h"
#include <assert.h>
#include <stdio.h>

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

void LCD_ShowCursor() {
  uint8_t cmd = LCD_SHOW_CURSOR;
  I2C1_Send(LCD_I2C_ADDR, &cmd, 1);
}

void LCD_HideCursor() {
  uint8_t cmd = LCD_HIDE_CURSOR;
  Delay_Ms(2);
  I2C1_Send(LCD_I2C_ADDR, &cmd, 1);
}

void LCD_MoveCursor(int x, int y) {
  uint8_t cmd[2] = {LCD_MOVE_CURSOR, (y << 6) | (x & 0x3f)};
  I2C1_Send(LCD_I2C_ADDR, cmd, 2);
}

void LCD_PutString(const char *s, int n) {
  uint8_t cmd[33];
  n &= 0x1f;
  cmd[0] = LCD_PUT_STRING | n;
  strncpy(cmd + 1, s, n);
  I2C1_Send(LCD_I2C_ADDR, cmd, n + 1);
}

void LCD_PutSpaces(int n) {
  uint8_t cmd[2] = {LCD_PUT_SPACES, n};
  I2C1_Send(LCD_I2C_ADDR, cmd, 2);
}

int LCD_SetBLBrightness(uint8_t brightness) {
  uint8_t cmd[2] = {LCD_SET_BL, brightness};
  return I2C1_Send(LCD_I2C_ADDR, cmd, 2);
}

static volatile vfs_mv[LED_NUM];

uint16_t GetVF(uint8_t led) {
  return vfs_mv[led];
}

void I2C1_Init(void) {
  uint16_t tempreg;

  // I2C を有効化
  RCC->APB1PCENR |= RCC_APB1Periph_I2C1;

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
enum I2C_Errors {
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
int I2C_Error(enum I2C_Errors err) {
  printf("I2C_Error: timeout waiting for %s\n\r", i2c_error_strings[err]);
  I2C1_Init(); // I2C をリセット
  return 1;
}

uint32_t I2C1_ReadStatus() {
  // STAR1 を STAR2 より先に読み出す必要がある
  uint32_t status = I2C1->STAR1;
  status |= I2C1->STAR2 << 16;
  return status;
}

// 最後に発生したイベントが引数で指定されたものであれば真を返す。
int I2C1_CheckEvent(uint32_t event) {
  return (I2C1_ReadStatus() & event) == event;
}

// ビジー状態が終わるのを待つ。
int I2C1_WaitBusy() {
  int timeout = I2C_TIMEOUT_MAX;
  while ((I2C1->STAR2 & I2C_STAR2_BUSY) && timeout--);
  return timeout;
}

// イベント成立を待つ。成立時のタイムアウト値を返す。
int I2C1_WaitEvent(uint32_t event) {
  int timeout = I2C_TIMEOUT_MAX;
  while ((!I2C1_CheckEvent(event)) && timeout--);
  return timeout;
}

// I2C バスへパケットを送る（割り込みを使わずブロッキングモード）
int I2C1_Send(uint8_t addr, uint8_t *data, uint8_t sz) {
  if (I2C1_WaitBusy() == -1) {
    return I2C_Error(I2CERR_NOT_BUSY);
  }

  // スタートビットを送る
  I2C1->CTLR1 |= I2C_CTLR1_START;
  if (I2C1_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) == -1) {
    return I2C_Error(I2CERR_MASTER_MODE);
  }

  // 7 ビットアドレスと write フラグを送る
  I2C1->DATAR = addr<<1;
  if (I2C1_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) == -1) {
    return I2C_Error(I2CERR_TRANSMIT_MODE);
  }

  // 1 バイトずつ送信
  while (sz--) {
    I2C1->DATAR = *data++;
    if (I2C1_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) == -1) {
      return I2C_Error(I2CERR_BYTE_TRANSMITTED);
    }
  }

  // ストップビットを送る
  I2C1->CTLR1 |= I2C_CTLR1_STOP;
  return 0;
}

// I2C バスからパケットを受け取る（割り込みを使わずブロッキングモード）
uint8_t I2C1_Recv(uint8_t addr, uint8_t *data, uint8_t sz) {
  if (I2C1_WaitBusy() == -1) {
    return I2C_Error(I2CERR_NOT_BUSY);
  }

  // スタートビットを送る
  I2C1->CTLR1 |= I2C_CTLR1_START;
  if (I2C1_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) == -1) {
    return I2C_Error(I2CERR_MASTER_MODE);
  }

  // 7 ビットアドレスと read フラグを送る
  I2C1->DATAR = addr<<1 | 1;
  if (I2C1_WaitEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) == -1) {
    return I2C_Error(I2CERR_RECEIVE_MODE);
  }

  // 1 バイトずつ受信
  while (sz--) {
    if (I2C1_WaitEvent(I2C_EVENT_MASTER_BYTE_RECEIVED) == -1) {
      return I2C_Error(I2CERR_BYTE_RECEIVED);
    }
    *data++ = I2C1->DATAR;
  }

  // ストップビットを送る
  I2C1->CTLR1 |= I2C_CTLR1_STOP;
  return 0;
}
