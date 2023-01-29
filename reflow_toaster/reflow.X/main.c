#include <math.h>
#include <stdio.h>

#include "mcc_generated_files/mcc.h"

// DAC の出力電圧（mv）
#define DAC_MV 704

volatile uint8_t phase_tick; // ゼロクロス点からの位相（ms）
volatile uint8_t phase_load0, phase_load1; // 負荷を ON にする位相（ms）

void PhaseISR() {
  // 最速で TMR2 を開始したい
  TMR2_WriteTimer(0);
  TMR2_StartTimer();

  IO_LOAD0_LAT = phase_load0 == 0;
  IO_LOAD1_LAT = phase_load1 == 0;
  TMR4_Start();

  phase_tick = 0;
}

void Tmr2ISR() {
  // TMR2 周期 1ms、 AC 電源周期 20ms
  phase_tick++;

  IO_LOAD0_LAT = phase_load0 == phase_tick;
  IO_LOAD1_LAT = phase_load1 == phase_tick;
  TMR4_Start();
}

void Tmr4ISR() {
  // ワンショットタイマ、トライアックゲート制御用  
  if (phase_tick >= 9) {
    TMR2_Stop();
  }

  IO_LOAD0_LAT = 0;
  IO_LOAD1_LAT = 0;
}

volatile unsigned long tick_ms;
void Tmr6ISR() {
  tick_ms++;
}

volatile float mcp_filtered, tc1_filtered;
#define TC_FILTER_FACTOR 0.1

float FilterThermoValue(float v_filtered, adcc_channel_t ch) {
  return (1 - TC_FILTER_FACTOR) * v_filtered +
         TC_FILTER_FACTOR * ADCC_GetSingleConversion(ch);
}

void UpdateThermoValues() {
  mcp_filtered = FilterThermoValue(mcp_filtered, channel_MCP);
  tc1_filtered = FilterThermoValue(tc1_filtered, channel_TC1);
}

void FlushStdin() {
  int c;
  while ((c = getchar()) != '\n' && c != EOF);
}

/** 気温を読み取る
 * 
 * @return 気温
 */
float ReadAirTemp() {
  // MCP9700A: 0℃=500mV, 10mV/℃
  return (mcp_filtered - 500) / 10;
}

static const int KTC_UV[] = { // K型熱電対の起電力 [uv]
//          10     20     30     40     50     60     70     80     90    100
/*   0 */0,397,   798,  1203,  1612,  2023,  2436,  2851,  3267,  3682,  4096,
/* 100 */ 4509,  4920,  5328,  5735,  6138,  6540,  6941,  7340,  7739,  8138,
/* 200 */ 8539,  8940,  9343,  9747, 10153, 10561, 10971, 11382, 11795, 12209,
};

/** K型熱電対（K ThermoCouple）の起電力から温度差を計算する。
 * 
 * @param amp25_mv  熱電対の起電力を 25 倍した値
 * @return 温度差（℃）
 */
int KTC_CalcTempDiff(int amp25_mv) {
  int ktc_uv = amp25_mv * 40; // 熱電対の起電力を μV 単位に変換
  int i = 0;
  while (KTC_UV[i + 1] <= ktc_uv) {
    i++;
  }
  // ここで KTC_UV[i] ≦ ktc_uv ＜ KTC_UV[i + 1]
  // KTC_UV[i] ～ KTC_UV[i + 1] で線形補間
  return 10 * i + 10 * (ktc_uv - KTC_UV[i]) / (KTC_UV[i + 1] - KTC_UV[i]);
}

int ReadTC1Temp() {
  return KTC_CalcTempDiff((int)tc1_filtered - DAC_MV) + (int)round(ReadAirTemp());
}

/* ヒーターの出力とトライアックを ON するまでの待ち時間の表
 * ヒーターの最大出力を 1、オフを 0 とする。
 * 位相は正弦波のゼロクロス点を 0 とする。
 * 
 * 出力  位相  待ち時間（ms）
 * 1.00     0  0.0
 * 0.95    25  1.4
 * 0.90    36  2.0
 * 0.85    45  2.5
 * 0.80    53  3.0
 * 0.75    60  3.3
 * 0.70    66  3.7
 * 0.65    72  4.0
 * 0.60    78  4.4
 * 0.55    84  4.7
 * 0.50    90  5.0
 * 0.45    95  5.3
 * 0.40   101  5.6
 * 0.35   107  6.0
 * 0.30   113  6.3
 * 0.25   120  6.7
 * 0.20   126  7.0
 * 0.15   134  7.5
 * 0.10   143  8.0
 * 0.05   154  8.6
 * 0.00   180  10.0
 */

volatile int target_temp;

void ControlHeaters() {
  int tc1_temp = ReadTC1Temp();
  int temp_diff = tc1_temp - target_temp;
  
  // temp_diff: -300 ～ 300 程度
  if (temp_diff < -20) {
    phase_load0 = phase_load1 = 0; // 最大出力
  } else if (temp_diff < -10) {
    phase_load0 = phase_load1 = 2; // 出力 90%
  } else if (temp_diff < -5) {
    phase_load0 = phase_load1 = 3; // 出力 80%
  } else if (temp_diff < 0) {
    phase_load0 = phase_load1 = 5; // 出力 50%
  } else if (temp_diff < 5) {
    phase_load0 = phase_load1 = 6; // 出力 35%
  } else {
    phase_load0 = phase_load1 = 10; // ヒーター停止
  }
}

void main(void) {
  SYSTEM_Initialize();
  IOCCF5_SetInterruptHandler(PhaseISR);
  TMR2_SetInterruptHandler(Tmr2ISR);
  TMR4_SetInterruptHandler(Tmr4ISR);
  TMR6_SetInterruptHandler(Tmr6ISR);
  INTERRUPT_GlobalInterruptEnable();
  INTERRUPT_PeripheralInterruptEnable();
  
  printf("hello, reflow toaster!\n");

  mcp_filtered = ADCC_GetSingleConversion(channel_MCP);
  tc1_filtered = ADCC_GetSingleConversion(channel_TC1);
  
  target_temp = 30;
  unsigned long current_tick_ms = tick_ms;

  for (;;) {
    int tc1 = ReadTC1Temp();
    printf("Tair=%d Ttc1=%d Ttgt=%d\n", (int)round(ReadAirTemp()), tc1, target_temp);
    printf("phase_load0=%d ms\n", phase_load0);
    
    if (target_temp == 30 && tc1 > target_temp) {
      target_temp = 20;
    } else if (target_temp == 20 && tc1 < 25) {
      target_temp = 30;
    }

    ControlHeaters();
    
    while (tick_ms < current_tick_ms + 500) {
      unsigned long next_ms = current_tick_ms;
      if (tick_ms >= next_ms) {
        UpdateThermoValues();
        next_ms += 50;
      }
    }
    current_tick_ms += 500;
  }
}