#include <math.h>
#include <stdio.h>
#include <string.h>

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

  phase_tick = 0;
}

void Tmr2ISR() {
  // TMR2 周期 1ms、 AC 電源周期 20ms
  IO_LOAD0_LAT = phase_load0 == phase_tick;
  IO_LOAD1_LAT = phase_load1 == phase_tick;  

  // 位相検出タイミングよりヒーター電源の位相が若干遅れるのでパルスを 1ms 遅らせる
  phase_tick++;
  
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

/** 現在の庫内温度に応じてヒーターの出力を調整する
 * 
 * @return 現在の庫内温度
 */
int ControlHeaters() {
  int tc1_temp = ReadTC1Temp();
  int temp_diff = tc1_temp - target_temp;
  
  // temp_diff: -300 ～ 300 程度
  if (temp_diff < -10) {
    phase_load0 = phase_load1 = 0; // 最大出力
  } else if (temp_diff < -5) {
    phase_load0 = phase_load1 = 2; // 出力 90%
  } else if (temp_diff < -2) {
    phase_load0 = phase_load1 = 3; // 出力 80%
  } else if (temp_diff < 0) {
    phase_load0 = phase_load1 = 5; // 出力 50%
  } else if (temp_diff < 5) {
    phase_load0 = phase_load1 = 6; // 出力 35%
  } else {
    phase_load0 = phase_load1 = 10; // ヒーター停止
  }
  
  return tc1_temp;
}

void PrintStatus() {
  printf("Tair=%d Ttc1=%d Ttgt=%d\n",
         (int)round(ReadAirTemp()), ReadTC1Temp(), target_temp);
  printf("phase_load0=%d ms\n", phase_load0);
}

_Bool repeat_status;

int ExecCmd(const char *cmd) {
  if (strncmp(cmd, "sett ", 5) == 0) { // set temperature
    target_temp = atoi(cmd + 5);
  } else if (strcmp(cmd, "stat") == 0) { // status
    PrintStatus();
  } else if (strcmp(cmd, "reps") == 0) {
    repeat_status = 1;
  } else if (strcmp(cmd, "quiet") == 0) {
    repeat_status = 0;
  } else {
    printf("unknown command: '%s'\n", cmd);
    return 1;
  }
  return 0;
}

struct TargetTemp {
  uint8_t temp;     // 目標温度
  uint16_t duration; // 目標温度の継続時間（s）
};

const struct TargetTemp target_temp_list[32] = {
  { 150, 60 },
  { 250, 10 },
  { 0, 0 },
};

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
  
  uint8_t target_temp_index = 0; // 現在の目標温度の番号
  uint16_t target_temp_tick_s = 0; // 目標温度になったときの時刻（s）
  _Bool heating = 1; // 加熱中=1、冷却中=0
  _Bool maintaining = 0; // 目標温度を維持=1、目標温度に遷移中=0
  
  target_temp = target_temp_list[target_temp_index].temp;
  unsigned long current_tick_ms = tick_ms;

  char cmd[32];
  size_t cmd_i = 0;
  
  for (;;) {
    int oven_temp = ControlHeaters();
    
    if (repeat_status) {
      PrintStatus();
    }
    
    if (target_temp_list[target_temp_index].temp == 0 &&
        target_temp_list[target_temp_index].duration == 0) {
      // すべての工程が完了
    } else {
      if (maintaining) {
        // 加熱・冷却が完了して目標温度になった時点から時間待ちを開始
        unsigned long duration_s = current_tick_ms / 1000 - target_temp_tick_s;
        if (duration_s >= target_temp_list[target_temp_index].duration) {
          target_temp_index++;
          maintaining = 0;
          
          uint8_t new_temp = target_temp_list[target_temp_index].temp;
          heating = new_temp > target_temp;
          target_temp = new_temp;
        }
      } else if ((heating && oven_temp < target_temp) ||
                 (!heating && oven_temp > target_temp)) {
        // 加熱・冷却中は target_temp_tick_s を現在時刻で更新しつづける
        target_temp_tick_s = current_tick_ms / 1000;
      } else {
        // 炉内の温度が目標温度に到達した
        maintaining = 1;
      }
    }
    
    unsigned long sensor_tick = current_tick_ms;
    while (tick_ms < current_tick_ms + 1000) {
      uint8_t c;
      while (EUSART_DataReady) {
        c = EUSART_Read();
        if (c == '\n') {
          if (cmd_i > 0) {
            cmd[cmd_i] = '\0';
            cmd_i = 0;
            if (ExecCmd(cmd) == 0) {
              printf("cmd succeeded\n");
            }
          }
        } else {
          cmd[cmd_i++] = c;
        }
      }
      if (tick_ms >= sensor_tick) {
        UpdateThermoValues();
        sensor_tick += 50;
      }
    }
    current_tick_ms += 1000;
  }
}