/*
 * main.c
 *
 * 2023/07/06 Created by uchan-nos
 *
 * 割り込みネスティングの実験プロジェクト
 *
 * ■CH32V003J4M6 ピン配置
 * - Pin 1 (PA1): LED1 は TIM1 割り込みが使用する
 * - Pin 2 (VSS)
 * - Pin 3 (PA2): LED2 は EXTI 割り込みが使用する
 * - Pin 4 (VDD)
 * - Pin 5 (PC1): 外部割り込みピン（アクティブ Low）
 * - Pin 6 NC
 * - Pin 7 (PC4): LED3 は main 関数が使用する
 * - Pin 8 (PD1): SWIO serial programming
 *
 * ■スイッチの接続
 * PC1 にはプッシュスイッチを接続し、スイッチ ON で VSS とショートするようにします（アクティブ Low）。
 * PC1 はプルアップ入力に設定しているため、外付けのプルアップ抵抗は不要です。
 *
 * ■プログラムの動作
 * main 関数は 1 秒に 5 回程度、LED3 の点滅を繰り返します。
 *
 * TIM1 は 1KHz でカウントアップし続けます。5 秒周期（TIM1.CNT == 5000）で一周し、割り込みが発生します。
 * 割り込み処理の先頭で LED1 を点灯し、TIM1.CNT が 2000 に達すると LED1 を消灯し割り込み処理を終わります。
 *
 * スイッチを押す（PC1 に立ち下がりエッジが入力される）と EXTI 割り込みが発生します。
 * 割り込み処理の先頭で LED2 を点灯します。1 秒待ち、LED2 を消灯し割り込み処理を終わります。
 * EXTI の割り込み処理中に再度 EXTI 割り込みが発生すると、連続して EXTI 割り込み処理が実行されます。
 * つまり、LED2 点灯中にスイッチを連打すると、途切れることなく LED2 を点灯させ続けられます。
 * （割り込み処理終了の直前から次の割り込み処理が始まるまで、一瞬だけ消えるはずですが、目には見えません）
 *
 * ■割り込みネスティング
 * EXTI が TIM1 割り込みより優先するように設定しています。そのため、TIM1 割り込みの処理中（LED1 が点灯中）
 * にスイッチを押すと即座に EXTI 割り込み処理に移り、LED2 が点灯します。（LED1 と LED2 が両方点灯）
 *
 * 逆に、EXTI 割り込み処理中に TIM1 割り込みが発生しても、TIM1 の割り込み処理は EXTI の割り込み処理が終わる
 * まで待たされます。LED1 が消えているときにスイッチを連打すると、ずっと EXTI 割り込みが起き続けるため、
 * LED2 だけ点灯し、LED1 はまったく点灯しません。
 */

#include "debug.h"

#define LED1_PORT GPIOA
#define LED1_PIN  GPIO_Pin_1
#define LED2_PORT GPIOA
#define LED2_PIN  GPIO_Pin_2
#define LED3_PORT GPIOC
#define LED3_PIN  GPIO_Pin_4
#define SW_PORT GPIOC
#define SW_PIN  GPIO_Pin_1

void InitGPIO() {
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);

  // Set pins with LEDs as push-pull output.
  GPIO_InitTypeDef gpio_init = {
    .GPIO_Pin = LED1_PIN,
    .GPIO_Mode = GPIO_Mode_Out_PP,
    .GPIO_Speed = GPIO_Speed_50MHz,
  };
  GPIO_Init(LED1_PORT, &gpio_init);
  gpio_init.GPIO_Pin = LED2_PIN;
  GPIO_Init(LED2_PORT, &gpio_init);
  gpio_init.GPIO_Pin = LED3_PIN;
  GPIO_Init(LED3_PORT, &gpio_init);

  // Set PC1 as pull-up input
  gpio_init.GPIO_Pin = GPIO_Pin_1;
  gpio_init.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOC, &gpio_init);
}

void InitEXTI() {
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

  // Enable external interrupt on PC1.
  EXTI_InitTypeDef exti_init = {
    .EXTI_Line = SW_PIN,
    .EXTI_Mode = EXTI_Mode_Interrupt,
    .EXTI_Trigger = EXTI_Trigger_Falling,
    .EXTI_LineCmd = ENABLE,
  };
  EXTI_Init(&exti_init);

  GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource1);

  NVIC_InitTypeDef nvic_init = {
    .NVIC_IRQChannel = EXTI7_0_IRQn,
    .NVIC_IRQChannelPreemptionPriority = 0,
    .NVIC_IRQChannelSubPriority = 0,
    .NVIC_IRQChannelCmd = ENABLE,
  };
  NVIC_Init(&nvic_init);
}

void InitTIM1() {
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

  // システムクロックはデフォルトの内蔵 24MHz RC クロック
  TIM_TimeBaseInitTypeDef tim_tbinit = {
    .TIM_Prescaler = 24000 - 1,
    .TIM_CounterMode = TIM_CounterMode_Up,
    .TIM_Period = 5000,
    .TIM_ClockDivision = TIM_CKD_DIV1,
    .TIM_RepetitionCounter = 0,
  };
  TIM_TimeBaseInit(TIM1, &tim_tbinit);

  TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
  TIM_SetCounter(TIM1, 3000);

  NVIC_InitTypeDef nvic_init = {
    .NVIC_IRQChannel = TIM1_UP_IRQn,
    .NVIC_IRQChannelPreemptionPriority = 1,
    .NVIC_IRQChannelSubPriority = 0,
    .NVIC_IRQChannelCmd = ENABLE,
  };
  NVIC_Init(&nvic_init);

  TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
  TIM_Cmd(TIM1, ENABLE);
}

void main() {
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

  Delay_Init();
  InitGPIO();
  InitEXTI();
  InitTIM1();

  while (1) {
    GPIO_WriteBit(GPIOC, GPIO_Pin_4, 1);
    Delay_Ms(100);
    GPIO_WriteBit(GPIOC, GPIO_Pin_4, 0);
    Delay_Ms(100);
  }
}

void EXTI7_0_IRQHandler() __attribute__((interrupt("WCH-Interrupt-fast")));

void EXTI7_0_IRQHandler() {
  EXTI_ClearITPendingBit(SW_PIN);
  GPIO_WriteBit(LED2_PORT, LED2_PIN, 1);
  Delay_Ms(1000);
  GPIO_WriteBit(LED2_PORT, LED2_PIN, 0);
}

void TIM1_UP_IRQHandler() __attribute__((interrupt("WCH-Interrupt-fast")));

void TIM1_UP_IRQHandler() {
  TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
  GPIO_WriteBit(LED1_PORT, LED1_PIN, 1);
  while (TIM_GetCounter(TIM1) < 2000);
  GPIO_WriteBit(LED1_PORT, LED1_PIN, 0);
}
