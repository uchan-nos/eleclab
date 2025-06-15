#pragma once

#include <stdint.h>
#include "../common.h"

/************
 ** main.c **
 ************/

#define LED_NUM 4

// LED 1 つあたりの制御周期
// 特定の LED については LED_NUM 倍の時間間隔で制御される
#define CTRL_PERIOD_MS 1

#define TICK_MS 10

#define INC_MOD(var, modulo) \
  do {\
    if (++var >= modulo) {\
      var = 0;\
    }\
  } while (0)

#define DEC_MOD(var, modulo) \
  do {\
    if (--var >= modulo) {\
      var = modulo - 1;\
    }\
  } while (0)

// キューのメッセージ
typedef uint8_t MessageType;
#define MSG_TICK      0x00
#define MSG_CW        0x10
#define MSG_CCW       0x11
#define MSG_MODE_PRS  0x12
#define MSG_MODE_REL  0x13
#define MSG_LED_PRS   0x14
#define MSG_LED_REL   0x15

/************
 * periph.c *
 ************/

/*
 * @param psc  プリスケーラの設定値（0 => 1:1）
 * @param period  タイマ周期
 * @param channels  有効化するチャンネル（ビットマップ）
 * @param default_high  1 なら無信号時の出力を 1 にする
 */
void TIM1_InitForPWM(uint16_t psc, uint16_t period, uint8_t channels, int default_high);

void TIM1_Start();

/*
 * TIM1 のパルス幅を設定する
 *
 * @param channel  設定対象のチャンネル（0～3）
 * @param width  パルス幅（0～65535）
 */
void TIM1_SetPulseWidth(uint8_t channel, uint16_t width);

/*
 * @param psc  プリスケーラの設定値（0 => 1:1）
 * @param period  タイマ周期
 */
void TIM2_InitForSimpleTimer(uint16_t psc, uint16_t period);

void TIM2_Start();

void TIM2_Updated(void);

void OPA1_Init(int enable, int neg_pin, int pos_pin);

/*
 * DMA1 を ADC1 のために設定する。
 * ADC1 は Channel1 に対応する。
 *
 * @param buf  DMA 転送先のバッファ
 * @param count  DMA 転送するデータ数
 * @param it_en  有効化する割り込み要因
 */
void DMA1_InitForADC1(uint16_t* buf, uint16_t count, uint32_t it_en);

void DMA1_Channel1_Transferred(void);


#define ADC_BITS 10

/*
 * ADC1 を DMA で動作させるための初期化を行う。
 *
 * @param trig  トリガの設定（ADC_ExternalTrigConv_x）
 */
void ADC1_InitForDMA(uint32_t trig);

void ADC1_StopContinuousConv();
void ADC1_StartContinuousConv();

void LCD_ShowCursor();
void LCD_HideCursor();
void LCD_MoveCursor(int x, int y);
void LCD_PutString(const char *s, int n);
void LCD_PutSpaces(int n);
int LCD_SetBLBrightness(uint8_t brightness);

uint16_t GetVF(uint8_t led);

void I2C1_Init(void);
int I2C1_Send(uint8_t addr, uint8_t *data, uint8_t sz);
uint8_t I2C1_Recv(uint8_t addr, uint8_t *data, uint8_t sz);

/************
 * pwctrl.c *
 ************/

uint16_t GetGoalCurrent(uint8_t led);
void SetGoalCurrent(uint8_t led, uint16_t goal_ua);
void IncGoalCurrent(uint8_t led, int amount_ua);

/*
 * 次の PWM パルス幅を計算する
 *
 * @param led  計算対象の LED
 * @param if_ua  現在の LED 電流（μA）
 * @return 次の PWM パルス幅（0x0000 - 0xffff）
 */
uint16_t NextPW(uint8_t led, uint16_t if_ua);

/************
 *** ui.c ***
 ************/

enum DispMode {
  DM_MODESELECT,
  DM_MULTI_CC,
  DM_MULTI_CV,
  DM_SINGLE_CC,
  DM_SINGLE_CV,
  DM_GLOBAL_CC,
  DM_GLOBAL_CV,
  DM_CONFIG,
  DM_CONFIG_BL,
};

void InitUI();
void HandleUIEvent(uint32_t tick, MessageType msg);

/************
 * queue.c **
 ************/
// 割り込みハンドラからメインスレッドへメッセージを伝えるキュー
#define QUEUE_CAP 4
typedef MessageType QueueElemType;

void Queue_Init(void);
uint8_t Queue_IsEmpty(void);
QueueElemType Queue_Pop(void);
void Queue_Push(QueueElemType v);
