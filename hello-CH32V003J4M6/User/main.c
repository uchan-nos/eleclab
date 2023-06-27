/*
 * main.c
 *
 * 2023/06/23 Created by uchan-nos
 *
 * PC4: TIM1_CH4  NeoPixel signal output
 */

#include "debug.h"

//#define TIM_HIGH_SPEED

void InitPeripheral() {
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_TIM1, ENABLE);

  // PC4 (Pin 7) を  TIM1CH4 の出力として使うため、AF_PP モードに設定
  // AF_PP: Alternate Function, Push Pull
  GPIO_InitTypeDef gpio_init = {
    .GPIO_Pin = GPIO_Pin_4,
    .GPIO_Mode = GPIO_Mode_AF_PP,
    .GPIO_Speed = GPIO_Speed_50MHz,
  };
  GPIO_Init(GPIOC, &gpio_init);

  // システムクロックはデフォルトの内蔵 24MHz RC クロック
  TIM_TimeBaseInitTypeDef tim_tbinit = {
#ifdef TIM_HIGH_SPEED
    .TIM_Prescaler = 0,
#else
    .TIM_Prescaler = 24000 - 1,
#endif
    .TIM_CounterMode = TIM_CounterMode_Up,
#ifdef TIM_HIGH_SPEED
    // T0H=320ns, T1H=640ns, 周期=1300ns にしたい。
    // Period=31 のとき、周期=31*1000/24≒1292ns
    .TIM_Period = 31,
#else
    .TIM_Period = 500,
#endif
    .TIM_ClockDivision = TIM_CKD_DIV1,
    .TIM_RepetitionCounter = 0,
  };
  TIM_TimeBaseInit(TIM1, &tim_tbinit);

  // OC(Output Compare) の設定
  TIM_OCInitTypeDef tim_ocinit = {
    // OCMode_PWM1: カウンタ < CVR のときにアクティブ、それ以降非アクティブ
    .TIM_OCMode = TIM_OCMode_PWM1,
    // 正出力を有効、相補出力は無効
    .TIM_OutputState = TIM_OutputState_Enable,
    .TIM_OutputNState = TIM_OutputState_Disable,
#ifdef TIM_HIGH_SPEED
    // CVR=8:  333.3ns
    // CVR=15: 625.0ns
    .TIM_Pulse = 8,
#else
    .TIM_Pulse = 100,
#endif
    // 主力の論理をアクティブ High とする
    .TIM_OCPolarity = TIM_OCPolarity_High,
    .TIM_OCNPolarity = TIM_OCPolarity_High,
    // アイドル時は 0(Reset) 出力
    .TIM_OCIdleState = TIM_OCIdleState_Reset,
    .TIM_OCNIdleState = TIM_OCIdleState_Reset,
  };
  TIM_OC4Init(TIM1, &tim_ocinit); // Ch4 を設定
}

void main() {
  InitPeripheral();

  TIM_Cmd(TIM1, ENABLE);
  TIM_CtrlPWMOutputs(TIM1, ENABLE);

  while (1) {
  }
}
