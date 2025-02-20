/*
 * Copyright (c) 2025 Kota UCHIDA
 *
 * はんだこて切り忘れ防止装置のファームウェア
 */
#include "ch32fun.h"

#define PIN_LED_SWITCH PC1
#define PIN_LED_SENSOR PC2
#define PIN_LED_NOTIFY PC4
#define PIN_SPEAKER    PA2
#define PIN_SENSE      PA1

// 1 tick の時間
#define TICK_MS 100

// センサの反応がない場合に通知ランプを光らせるまでの時間（tick 単位）
#define NOTIFY_TIME 1000
// センサの反応がない場合に電源を落とすまでの時間（tick 単位）
#define OFF_TIME 1200

// センサが反応したら割り込みハンドラの中で 1 が書かれる
volatile int sensor_detect;

// センサが反応したら割り込まれる
void EXTI7_0_IRQHandler(void) __attribute__((interrupt));
void EXTI7_0_IRQHandler(void) {
  // 割り込み処理を受け付けたことを報告
  EXTI->INTFR = EXTI_Line1;
  sensor_detect = 1;

  // ここで LED を点灯し、タイマで消灯させる
  funDigitalWrite(PIN_LED_SENSOR, FUN_HIGH);
  TIM1->CTLR1 |= TIM_CEN;
}

void EnableSensorInterrupt() {
  EXTI->INTENR = EXTI_Line1;
}

void DisableSensorInterrupt() {
  EXTI->INTENR = 0;
}

void InitGpio() {
  funGpioInitAll();
  funPinMode(PIN_LED_SWITCH, GPIO_CFGLR_OUT_10Mhz_PP);
  funPinMode(PIN_LED_SENSOR, GPIO_CFGLR_OUT_10Mhz_PP);
  funPinMode(PIN_LED_NOTIFY, GPIO_CFGLR_OUT_10Mhz_PP);
  funPinMode(PIN_SPEAKER,    GPIO_CFGLR_OUT_10Mhz_PP);
  funPinMode(PIN_SENSE,      GPIO_CFGLR_IN_FLOAT);

  NVIC_EnableIRQ(EXTI7_0_IRQn);
  EnableSensorInterrupt();
  EXTI->RTENR = EXTI_Line1;
}

void TIM1_UP_IRQHandler(void) __attribute__((interrupt));
void TIM1_UP_IRQHandler(void) {
  // 割り込みフラグをクリア
  TIM1->INTFR = ~TIM_EventSource_Update;

  funDigitalWrite(PIN_LED_SENSOR, FUN_LOW);
}

// LED_SENSOR の点灯時間を決めるタイマ
void InitTim1() {
  // TIM1 を有効化
  RCC->APB2PCENR |= RCC_APB2Periph_TIM1;

  // TIM1 をリセットしてレジスタを初期化
  RCC->APB2PRSTR |= RCC_APB2Periph_TIM1;
  RCC->APB2PRSTR &= ~RCC_APB2Periph_TIM1;

  // プリスケーラ 1ms 単位
  TIM1->PSC = 48000 - 1;

  // 周期
  TIM1->ATRLR = 20;

  // 単パルスモード
  TIM1->CTLR1 = TIM_OPM;

  // タイムアップ割り込みを有効化
  TIM1->DMAINTENR = TIM_EventSource_Update;
  NVIC_EnableIRQ(TIM1_UP_IRQn);
}

int main() {
  SystemInit();
  InitTim1();
  InitGpio();

  uint32_t tick = 0;

  while (1) {
    if (sensor_detect) {
      sensor_detect = 0;
      tick = 0;
    }

    // 通電ランプと点灯させるとリレーも一緒に動作する回路になっている

    if (tick < NOTIFY_TIME) {
      // 通電ランプを点灯
      funDigitalWrite(PIN_LED_SWITCH, FUN_HIGH);
      // 通知ランプを消灯
      funDigitalWrite(PIN_LED_NOTIFY, FUN_LOW);
    } else if (tick < ((NOTIFY_TIME + OFF_TIME) / 2)) {
      // 通知ランプを点灯
      funDigitalWrite(PIN_LED_NOTIFY, FUN_HIGH);
    } else if (tick < OFF_TIME) {
      // 通知ランプを点滅
      funDigitalWrite(PIN_LED_NOTIFY, (tick >> 1) & 1);
    } else if (tick == OFF_TIME) {
      // 通知音やリレー作動の振動で誤動作しないように割り込みを無効化
      // EXTI_INTENR を 0 にすれば EXTI_INTFR のフラグは立たなくなる
      DisableSensorInterrupt();
      sensor_detect = 0;

      // 遮断通知音
      for (int i = 0; i < 200; i++) {
        funDigitalWrite(PIN_SPEAKER, FUN_HIGH);
        Delay_Us(500);
        funDigitalWrite(PIN_SPEAKER, FUN_LOW);
        Delay_Us(500);
      }

      // 通電ランプと通知ランプを消灯
      funDigitalWrite(PIN_LED_SWITCH, FUN_LOW);
      funDigitalWrite(PIN_LED_NOTIFY, FUN_LOW);
    } else if (tick == OFF_TIME + 1) {
      EnableSensorInterrupt();
    }

    Delay_Ms(100);
    if (tick < UINT32_MAX) {
      ++tick;
    }
  }
}
