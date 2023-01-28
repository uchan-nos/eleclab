#include <math.h>
#include <stdio.h>

#include "mcc_generated_files/mcc.h"

volatile unsigned long phase_cnt;
volatile uint16_t tmr0_reload_value;

void PhaseISR() {
  TMR0_WriteTimer(tmr0_reload_value);
  TMR0_StartTimer();
  phase_cnt++;
}

volatile unsigned long tmr0_cnt;

void Tmr0ISR() {
  tmr0_cnt++;

  IO_LOAD0_LAT = 1;
  __delay_us(200);
  IO_LOAD0_LAT = 0;
}

volatile float mcp_filtered, tc1_filtered;
#define TC_FILTER_FACTOR 0.02

float FilterThermoValue(float v_filtered, adcc_channel_t ch) {
  return (1 - TC_FILTER_FACTOR) * v_filtered +
         TC_FILTER_FACTOR * ADCC_GetSingleConversion(ch);
}

void UpdateThermoValues() {
  mcp_filtered = FilterThermoValue(mcp_filtered, channel_MCP);
  tc1_filtered = FilterThermoValue(tc1_filtered, channel_TC1);
}

void Tmr6ISR() {
  UpdateThermoValues();
}

void FlushStdin() {
  int c;
  while ((c = getchar()) != '\n' && c != EOF);
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
 * @return 温度差
 */
int KTC_CalcTemp(int amp25_mv) {
  int ktc_uv = amp25_mv * 40; // 熱電対の起電力を μV 単位に変換
  int i = 0;
  while (KTC_UV[i + 1] <= ktc_uv) {
    i++;
  }
  // ここで KTC_UV[i] ≦ ktc_uv ＜ KTC_UV[i + 1]
  // KTC_UV[i] ～ KTC_UV[i + 1] で線形補間
  return 10 * i + 10 * (ktc_uv - KTC_UV[i]) / (KTC_UV[i + 1] - KTC_UV[i]);
}

void main(void) {
  SYSTEM_Initialize();
  IOCBF0_SetInterruptHandler(PhaseISR);
  TMR0_SetInterruptHandler(Tmr0ISR);
  TMR6_SetInterruptHandler(Tmr6ISR);
  INTERRUPT_GlobalInterruptEnable();
  INTERRUPT_PeripheralInterruptEnable();
  
  printf("hello, reflow toaster!\n");
  
  tc1_filtered = ADCC_GetSingleConversion(channel_TC1);
  
  TMR0_StartTimer();

  char s[32];
  unsigned long v = 6000;
  for (;;) {
    
    int mcp_value = mcp_filtered;
    // MCP9700A: 0℃=500mV, 10mV/℃
    int deg10 = mcp_value - 500;
    printf("mcp =%u (%d.%d deg)\n", mcp_value, deg10 / 10, deg10 % 10);
    int tc1_value = ADCC_GetSingleConversion(channel_TC1);
    printf("tc1 =%u (%d deg)\n", tc1_value, deg10 / 10 + KTC_CalcTemp(tc1_value - 704));
    printf("tc1f=%u\n", (unsigned int)round(tc1_filtered));
    
    __delay_ms(500);
    
    /*
    printf("DAC(%d)>", DAC_GetOutput());
    scanf("%d", &dac);
    DAC_SetOutput(dac);
    FlushStdin();
    */
  }
}