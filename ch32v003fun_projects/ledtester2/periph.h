#pragma once

#include <stdint.h>

/*
 * @param psc  プリスケーラの設定値（0 => 1:1）
 * @param period  タイマ周期
 * @param channels  有効化するチャンネル（ビットマップ）
 * @param default_high  1 なら無信号時の出力を 1 にする
 */
void TIM1_InitForPWM(uint16_t psc, uint16_t period, uint8_t channels, int default_high);

void TIM1_Start();

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