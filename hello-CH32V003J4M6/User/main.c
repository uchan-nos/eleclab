/*
 * main.c
 *
 * 2023/06/23 Created by uchan-nos
 *
 * PC4: TIM1_CH4  NeoPixel signal output
 */

#include "debug.h"

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
  // TIM1 を 1 秒の周期に設定
  TIM_TimeBaseInitTypeDef tim_tbinit = {
    .TIM_Prescaler = 1000,
    .TIM_CounterMode = TIM_CounterMode_Up,
    .TIM_Period = 24000,
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
    // CVR に周期の 1/3 を設定（デューティー比 33%）
    .TIM_Pulse = 24000 / 3,
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
