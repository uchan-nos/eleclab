/*
 * Copyright (c) 2025 Kota UCHIDA
 *
 * LED テスター 2 のファームウェア（LCD コントローラ用）
 */
#include "ch32fun.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../common.h"

#define QUEUE_CAP 8
#define MK_CMD 1

struct Message {
  uint8_t kind;
  union {
    struct {
      uint8_t cmd;
      uint8_t argc;
      uint8_t argv[32];
    } cmd;
  };
};

static struct Message queue[QUEUE_CAP];
static volatile uint8_t rp = 0, wp = 0, free = QUEUE_CAP;

#define INC_PTR(p) \
  do {\
    if (++p >= QUEUE_CAP) {\
      p = 0;\
    }\
  } while (0)

uint8_t Queue_IsEmpty(void) {
  return free == QUEUE_CAP;
}

struct Message *Queue_Front(void) {
  if (free < QUEUE_CAP) {
    return queue + rp;
  }
  return NULL;
}

void Queue_Pop(void) {
  if (free < QUEUE_CAP) {
    INC_PTR(rp);
    ++free;
  }
}

struct Message *Queue_Bottom(void) {
  if (free > 0) {
    return queue + wp;
  }
  return NULL;
}

void Queue_Push(void) {
  if (free > 0) {
    INC_PTR(wp);
    --free;
  }
}

#define SCL_PIN PC2
#define SDA_PIN PC1

#define LCD_E   PD0
#define LCD_RS  PC0
#define LCD_RW  PC3
#define LCD_DB4 PC4
#define LCD_DB5 PC5
#define LCD_DB6 PC6
#define LCD_DB7 PC7

#define BL_PIN   PD3
#define NEXT_PIN PD6

void OPA1_Init(int enable, int neg_pin, int pos_pin) {
  assert(neg_pin == PA1 || neg_pin == PD0);
  assert(pos_pin == PA2 || pos_pin == PD7);

  uint32_t ctr = EXTEN->EXTEN_CTR;
  ctr |= enable ? EXTEN_OPA_EN : 0;
  ctr |= neg_pin == PD0 ? EXTEN_OPA_NSEL : 0;
  ctr |= pos_pin == PD7 ? EXTEN_OPA_PSEL : 0;
  EXTEN->EXTEN_CTR = ctr;
}

void I2C1_InitAsSlave(uint8_t addr7) {
  uint16_t tempreg;

  // I2C を有効化
  RCC->APB1PCENR |= RCC_APB1Periph_I2C1;

  // I2C1 をリセット
  RCC->APB1PRSTR |= RCC_APB1Periph_I2C1;
  RCC->APB1PRSTR &= ~RCC_APB1Periph_I2C1;

  I2C1->CTLR1 |= I2C_CTLR1_SWRST;
  I2C1->CTLR1 &= ~I2C_CTLR1_SWRST;

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

  // アドレスを設定
  I2C1->OADDR1 = addr7 << 1;

  // I2C1 モジュールを有効化
  I2C1->CTLR1 |= I2C_CTLR1_PE;

  // ACK 応答を有効化（PE=1 にした後に設定しなければならない）
  I2C1->CTLR1 |= I2C_CTLR1_ACK;
}

uint32_t I2C1_ReadStatus() {
  // STAR1 を STAR2 より先に読み出す必要がある
  uint32_t status = I2C1->STAR1;
  status |= I2C1->STAR2 << 16;
  return status;
}

enum I2CState {
  IS_IDLE, // 受信開始待ち
  IS_CMD,  // コマンドバイトの受信待ち
  IS_ARG,  // 引数の受信待ち
};

static enum I2CState i2c_state;
static uint8_t i2c_cmd, i2c_argc, i2c_argi;
static uint8_t i2c_argv[32];

void I2C1_EV_IRQHandler(void) __attribute__((interrupt));
void I2C1_EV_IRQHandler(void) {
  uint32_t status = I2C1_ReadStatus();

  if (status & I2C_IT_ADDR) { // アドレスバイトを受信した
    i2c_state = IS_CMD;
  }

  if (status & I2C_IT_RXNE) { // データバイトを受信
    if (i2c_state == IS_IDLE || i2c_state == IS_CMD) {
      i2c_cmd = I2C1->DATAR;
      i2c_argi = 0;

      if ((i2c_cmd & 0xF0) == 0x00) {
        i2c_argc = 0;
      } else if ((i2c_cmd & 0xF0) == 0x10) {
        i2c_argc = 1;
      } else if ((i2c_cmd & 0xE0) != 0) {
        i2c_argc = i2c_cmd & 0x1F;
      }

      if (i2c_argc == 0) {
        i2c_state = IS_IDLE;
      } else {
        i2c_state = IS_ARG;
      }
    } else if (i2c_state == IS_ARG) {
      i2c_argv[i2c_argi++] = I2C1->DATAR;
      if (i2c_argi == i2c_argc) {
        i2c_state = IS_IDLE;
      }
    } else {
      printf("I2C_IT_RXNE: unexpected i2c_state=%d\n", i2c_state);
    }

    if (i2c_state == IS_IDLE) {
      struct Message *msg = Queue_Bottom();
      if (msg) {
        msg->kind = MK_CMD;
        msg->cmd.cmd = i2c_cmd;
        msg->cmd.argc = i2c_argc;
        memcpy(msg->cmd.argv, i2c_argv, sizeof(msg->cmd.argv));
        Queue_Push();
      }
    }
  }

  if (status & I2C_IT_STOPF) { // ストップビットを受信
    I2C1->CTLR1 &= ~(I2C_CTLR1_STOP); // ストップビットをクリア
  }
}

void I2C1_ER_IRQHandler(void) __attribute__((interrupt));
void I2C1_ER_IRQHandler(void) {
  // とりあえずすべてのエラーを無視
  I2C1->STAR1 = ~I2C1->STAR1;
}

static void lcd_out4(uint8_t rs, uint8_t val) {
  funDigitalWrite(LCD_RS, rs);
  funDigitalWrite(LCD_E, 1);

  funDigitalWrite(LCD_DB4, val & 0b0001u ? 1 : 0);
  funDigitalWrite(LCD_DB5, val & 0b0010u ? 1 : 0);
  funDigitalWrite(LCD_DB6, val & 0b0100u ? 1 : 0);
  funDigitalWrite(LCD_DB7, val & 0b1000u ? 1 : 0);

  Delay_Us(1);
  funDigitalWrite(LCD_E, 0);
  Delay_Us(1);
}

void lcd_out8(uint8_t rs, uint8_t val) {
  lcd_out4(rs, val >> 4u);
  lcd_out4(rs, val);
}

void lcd_exec(uint8_t cmd) {
  lcd_out8(0, cmd);
  if (cmd == 0x01u || cmd == 0x02u) {
    Delay_Us(1640);
  } else {
    Delay_Us(40);
  }
}

void lcd_putc(char c) {
  lcd_out8(1, c);
  Delay_Us(40);
}

void lcd_puts(const char* s) {
  while (*s) {
    lcd_putc(*s);
    ++s;
  }
}

void LCD_Init() {
  Delay_Ms(40);
  lcd_out4(0, 0x3u);
  Delay_Us(4100);
  lcd_out4(0, 0x3u);
  Delay_Us(40);
  lcd_out4(0, 0x3u);
  Delay_Us(40);
  lcd_out4(0, 0x2u);  // 4bit mode after this line
  Delay_Us(40);

  lcd_exec(0x28u); // function set: 4bit, 2lines, 5x7dots
  lcd_exec(0x08u); // display on/off: display off, cursor off, no blink
  lcd_exec(0x01u); // clear display
  lcd_exec(0x06u); // entry mode set: increment, with display shift
  lcd_exec(0x0cu); // display on/off: display on, cursor off, no blink
}

void TIM2_InitForPWM(uint16_t psc, uint16_t period, uint8_t channels, int default_high) {
  // TIM2 を有効化
  RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

  // TIM2 をリセット
  RCC->APB1PRSTR |= RCC_APB1Periph_TIM2;
  RCC->APB1PRSTR &= ~RCC_APB1Periph_TIM2;

  // TIM2 の周期
  TIM2->PSC = psc;
  TIM2->ATRLR = period;

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
  TIM2->CHCTLR1 = chctlr;

  chctlr = 0;
  if (channels & 4) {
    chctlr |= ocmode;
    ccer |= ccconf << 8;
  }
  if (channels & 8) {
    chctlr |= ocmode << 8;
    ccer |= ccconf << 12;
  }
  TIM2->CHCTLR2 = chctlr;
  TIM2->CCER = ccer;

  // 出力を有効化
  TIM2->BDTR |= TIM_MOE;

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
  TIM2->CTLR1 |= TIM_ARPE;
}

void TIM2_Start() {
  // アップデートイベント（UG）を発生させ、プリロードを行う
  TIM2->SWEVGR |= TIM_UG;

  // TIM2 をスタートする
  TIM2->CTLR1 |= TIM_CEN;
}

void TIM2_SetPulseWidth(uint8_t channel, uint16_t width) {
  switch (channel) {
  case 0: TIM2->CH1CVR = width; break;
  case 1: TIM2->CH2CVR = width; break;
  case 2: TIM2->CH3CVR = width; break;
  case 3: TIM2->CH4CVR = width; break;
  default: assert(0);
  }
}

int main() {
  SystemInit();

  funAnalogInit();
  funGpioInitAll();

  funPinMode(PA1, GPIO_CFGLR_IN_ANALOG); // Opamp Input
  funPinMode(PA2, GPIO_CFGLR_IN_ANALOG); // Opamp Input
  //funPinMode(PD4, GPIO_CFGLR_IN_ANALOG); // Opamp Output

  funDigitalWrite(SCL_PIN, 1);
  funDigitalWrite(SDA_PIN, 1);
  funPinMode(SCL_PIN, GPIO_CFGLR_OUT_50Mhz_AF_OD);
  funPinMode(SDA_PIN, GPIO_CFGLR_OUT_50Mhz_AF_OD);

  funPinMode(LCD_E,   GPIO_CFGLR_OUT_10Mhz_PP);
  funPinMode(LCD_RS,  GPIO_CFGLR_OUT_10Mhz_PP);
  funPinMode(LCD_RW,  GPIO_CFGLR_OUT_10Mhz_PP);
  funPinMode(LCD_DB4, GPIO_CFGLR_OUT_10Mhz_PP);
  funPinMode(LCD_DB5, GPIO_CFGLR_OUT_10Mhz_PP);
  funPinMode(LCD_DB6, GPIO_CFGLR_OUT_10Mhz_PP);
  funPinMode(LCD_DB7, GPIO_CFGLR_OUT_10Mhz_PP);

  OPA1_Init(1, PA1, PA2);

  LCD_Init();

  I2C1_InitAsSlave(LCD_I2C_ADDR);
  I2C1->CTLR2 |= I2C_IT_BUF | I2C_IT_EVT | I2C_IT_ERR;
  NVIC_EnableIRQ(I2C1_EV_IRQn);
  NVIC_EnableIRQ(I2C1_ER_IRQn);

  funPinMode(BL_PIN, GPIO_CFGLR_OUT_10Mhz_AF_PP);
  TIM2_InitForPWM(0, 65535, 1 << 1, 0);
  TIM2_SetPulseWidth(1, 32);
  TIM2_Start();

  while (1) {
    __disable_irq();
    if (Queue_IsEmpty()) {
      __enable_irq();
      continue;
    }
    struct Message *msg = Queue_Front();
    __enable_irq();

    if (msg->kind == MK_CMD) {
      printf("cmd=%02X argc=%d\n", msg->cmd.cmd, msg->cmd.argc);
      if (msg->cmd.cmd == LCD_HIDE_CURSOR) {
        lcd_exec(0x0C); // display=on, cursor=off, curpos=off
      } else if (msg->cmd.cmd == LCD_SHOW_CURSOR) {
        lcd_exec(0x0D); // display=on, cursor=off, curpos=on
      } else if (msg->cmd.cmd == LCD_MOVE_CURSOR) {
        uint8_t xy = msg->cmd.argv[0];
        uint8_t x = xy & 0x3F;
        uint8_t y = (xy >> 6) & 3;
        uint8_t addr = 0x80; // set DDRAM addr
        addr |= (y & 1) << 6;
        addr |= (y & 2) << 3;
        addr |= x;
        lcd_exec(addr);
      } else if (msg->cmd.cmd == LCD_PUT_SPACES) {
        uint8_t n = msg->cmd.argv[0];
        for (uint8_t i = 0; i < n; ++i) {
          lcd_putc(' ');
        }
      } else if (msg->cmd.cmd == LCD_SET_BL) {
        uint8_t brightness = msg->cmd.argv[0];
        uint16_t pw;
        if (brightness == 0) {
          pw = 0;
        } else if (brightness <= 14) {
          pw = 1 << (brightness + 1);
        } else {
          pw = 65535;
        }
        TIM2_SetPulseWidth(1, pw);
      } else if ((msg->cmd.cmd & 0xE0) == LCD_PUT_STRING) {
        uint8_t n = msg->cmd.cmd & 0x1F;
        for (uint8_t i = 0; i < n; ++i) {
          lcd_putc(msg->cmd.argv[i]);
        }
      }
    } else {
      printf("unknown msg\n");
    }

    __disable_irq();
    Queue_Pop();
    __enable_irq();
  }
}
