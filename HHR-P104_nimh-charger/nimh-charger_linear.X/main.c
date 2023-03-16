#include "mcc_generated_files/mcc.h"

#include <string.h>
#include "sc1602.h"

#define CHARGE_CURRENT_MA 200
#define TARGET_VOLTAGE_MV 4350
#define DISCHARGE_CURRENT_MA 1000
#define DISCHARGE_HARD_LIMIT_MV  (900 * 3)
#define NO_BATTERY_MV     4074
#define BAT_TOO_LOW_MV    (300 * 3)

#define MV_TO_ADC(mv)       ((adc_result_t)(mv / 2 / 4))
#define TARGET_VOLTAGE_ADC  MV_TO_ADC(TARGET_VOLTAGE_MV)
#define NO_BATTERY_ADC      MV_TO_ADC(NO_BATTERY_MV)
#define DACFVR_MV 1024

// DAC VREF+ = FVR (1.024V)、電流検出抵抗 1Ω なので電流範囲は 0mA ～ 1024mA
// DAC の精度を N ビットとすると、電流分解能は 2^10/2^N = 2^(10-N) [mA/div]
#define DAC_PRECISION_BITS  8
#define DAC_TO_MA(dac_val)  ((uint16_t)(dac_val) << (10 - DAC_PRECISION_BITS))
#define MA_TO_DAC(cur_ma)   ((uint8_t)((cur_ma) >> (10 - DAC_PRECISION_BITS)))

enum RunState {
  NO_BATTERY, // 充放電対象の電池が接続されていない
  CHARGE_CC,
  CHARGE_CV,
  CHARGED,
  DISCHARGE_CC,
  DISCHARGED,
  HIGHVOLT,   // MOSFET に規定以上の電圧が加わっている
  BAT_TOO_LOW, // 電池が過放電の状態
};
volatile enum RunState run_state = NO_BATTERY;


#define SIN_STRIDE_DEG 5 /* 1 ステップの角度 */
#define SIN_NUM_STEPS  (360 / SIN_STRIDE_DEG)

// 5 - 80 度の 5 度刻みの sin テーブル（0, 90 度は自明なので省略）
const uint8_t sin_table[SIN_NUM_STEPS] = {
    0,   0,   2,   4,   8,  12,  17,  23,  30,  37,  46,  54,  64,  74,  84,  95, 105, 116,
  127, 139, 150, 160, 171, 181, 191, 201, 209, 218, 225, 232, 238, 243, 247, 251, 253, 255,
  255, 255, 253, 251, 247, 243, 238, 232, 225, 218, 209, 201, 191, 181, 171, 160, 150, 139,
  128, 116, 105,  95,  84,  74,  64,  54,  46,  37,  30,  23,  17,  12,   8,   4,   2,   0
};
volatile uint8_t sin_step_index = 0;
volatile uint8_t pwmtmr_count = 0;

// LED の点滅モード設定
// 0～3: 0.72, 1.44, 2.88, 5.76 秒周期のサイン波点滅 （T=0.72×2^n）
// 4: 予約
// 5 - 250: 10×n ミリ秒周期の矩形波点滅（ON 時間は led_on_width で指定）
// 251 - 255: 予約
volatile uint8_t led_mode = 5, led_on_width = 0;

// 緩やかに LED を点滅させる
void pwmtmr_isr() {
  if (led_mode > 3) {
    return;
  }
  IO_LED_LAT = pwmtmr_count <= sin_table[sin_step_index];
  pwmtmr_count += 2;
}

// bat_mv2_q4: 電池電圧/2 [mV] を表す変数
// 4 ビット左シフトした固定小数点表現（Q4 フォーマット）
// ADC は 12 ビット、2 つ使って差動モードなので 13 ビットレンジ。
// そのため、変数の下位 17 ビットが有効値となる。
volatile int32_t bat_mv2_q4;
volatile adc_result_t bat_mv2_latest;

#define BAT_MV ((int16_t)((bat_mv2_q4) >> 3))

uint16_t adc_to_mv(adc_result_t adc) {
  return adc * 4; // 1024 = 4.096V (FVRx4)
}

void SetCurrentMA(uint16_t cur_ma) {
  uint8_t dac_val = MA_TO_DAC(cur_ma);
  DAC1_SetOutput(dac_val);
  OPA2CONbits.OPA2EN = 1;
}

void StopCurrent() {
  OPA2CONbits.OPA2EN = 0;
  DAC1_SetOutput(0);
}

void ChargeCV() {
  if (BAT_MV < TARGET_VOLTAGE_MV) {
    return;
  }

  uint8_t dac_val = DAC1_GetOutput();
  if (dac_val > 1) {
    DAC1_SetOutput(dac_val - 1);
  } else {
    StopCurrent();
  }
}

int16_t discharge_stop_mv = 3000;

void ControlDAC() {
  switch (run_state) {
  case NO_BATTERY:
  case HIGHVOLT:
  case BAT_TOO_LOW:
  case CHARGED:
  case DISCHARGED:
    StopCurrent();
    break;
  case CHARGE_CC:
    SetCurrentMA(CHARGE_CURRENT_MA);
    break;
  case CHARGE_CV:
    ChargeCV();
    break;
  case DISCHARGE_CC:
    SetCurrentMA(DISCHARGE_CURRENT_MA);
    break;
  }
}

_Bool BatteryExist() {
  //SetCurrentMA(1);
  //adc_result_t vf_adc = ADC_GetConversion(channel_VF);
  StopCurrent();
  //return vf_adc > NO_BATTERY_VF_ADC;
  return 0;
}

int32_t FilterADCValueQ4(int32_t v_filtered, adc_result_t v_adc) {
  return v_filtered - (v_filtered >> 4) + (int16_t)v_adc;
}

/** 異常発生を検知する。
 *
 * @return 異常の種類。異常が無ければ現在の run_state をそのまま返す。
 */
enum RunState CheckAnomary() {
  // 高電圧が発生していないか
  if (CMP1_GetOutputStatus()) {
    return HIGHVOLT;
  }

  int16_t bat_mv = BAT_MV;
  int16_t bat_mv_abs = abs(bat_mv);
  if ((IO_MODE_PORT && bat_mv > 4900) ||
      (!IO_MODE_PORT && bat_mv_abs < 100)) {
    return NO_BATTERY;
  } else if (bat_mv_abs < BAT_TOO_LOW_MV) {
    return BAT_TOO_LOW;
  }

  return run_state;
}

enum RunState NextRunState() {
  // 異常を最初にチェック
  enum RunState anomary = CheckAnomary();

  if (IO_MODE_PORT) { // 充電モード
    switch (anomary) {
    case NO_BATTERY:
    case HIGHVOLT:
    case BAT_TOO_LOW:
      return anomary;
    case CHARGE_CC:
      return BAT_MV < TARGET_VOLTAGE_MV ? CHARGE_CC : CHARGE_CV;
    case CHARGE_CV:
      return DAC1_GetOutput() >= 1 ? CHARGE_CV : CHARGED;
    case CHARGED:
      return CHARGED;
    default:
      return BAT_MV < TARGET_VOLTAGE_MV ? CHARGE_CC : CHARGED;
    }
  } else { // 放電モード
    switch (run_state) {
    case NO_BATTERY:
    case HIGHVOLT:
    case BAT_TOO_LOW:
      return anomary;
    case DISCHARGE_CC:
      return BAT_MV > discharge_stop_mv ? DISCHARGE_CC : DISCHARGED;
    case DISCHARGED:
      return DISCHARGED;
    default:
      return DISCHARGE_CC;
    }
  }
}

void adc_isr() {
  // ADC は 12bit モード かつ VREF+ = FVR 4.096V なので分解能は 1mV
  // channel_BAT には 電池電圧/2 が印加される
  //adc_result_t bat_adc = ADC_GetConversionResult();
  bat_mv2_latest = ADC_GetConversionResult();

  // bat_mv = (15.0/16) * bat_mv + (1.0/16) * bat_adc; を固定小数点数で計算
  bat_mv2_q4 = FilterADCValueQ4(bat_mv2_q4, bat_mv2_latest);
}

volatile uint8_t tick = 0;
void tmr_isr() {
  // interrupt period = 10ms
  tick++;

  ADC_StartConversion(channel_BAT);
  
  if (led_mode <= 3 && (tick & ((1u << led_mode) - 1)) == 0) {
    if (sin_step_index < SIN_NUM_STEPS - 1) {
      sin_step_index++;
    } else {
      sin_step_index = 0;
    }
  } else if (5 <= led_mode && led_mode <= 250) {
    if (sin_step_index < led_mode) {
      sin_step_index++;
    } else {
      sin_step_index = 0;
    }
    IO_LED_LAT = sin_step_index <= led_on_width;
  }
}

void cmp_isr() {
  if (!CMP1_GetOutputStatus()) {
    return;
  }

  run_state = HIGHVOLT;
  StopCurrent();
}

void sleep_tick(uint8_t duration_tick) {
  uint8_t current_tick = tick;
  uint8_t end_tick = current_tick + duration_tick;
  if (end_tick < current_tick) { // overflow
    while (tick > 0);
  }
  while (tick < end_tick);
}

void main(void) {
  SYSTEM_Initialize();
  
  uint8_t current = 0;
  TMR2_SetInterruptHandler(tmr_isr);
  ADC_SetInterruptHandler(adc_isr);
  TMR0_SetInterruptHandler(pwmtmr_isr);
  INTERRUPT_PeripheralInterruptEnable();
  INTERRUPT_GlobalInterruptEnable();
  TMR2_StartTimer();
  
  IO_LCD_RW_LAT = 0;
  lcd_init();
  char s[17];

  /*
  int nobat = 0;
   */
  StopCurrent();

  //IO_RB1_LAT = 0;

  //while (ADC_GetConversion(channel_BAT) > 1000);

  TRISBbits.TRISB1 = 0;
  LATBbits.LATB1 = 0;
  OPA2CONbits.OPA2EN = 0;

  bat_mv2_q4 = (int16_t)ADC_GetConversion(channel_BAT) << 4;

  while (1) {
    run_state = NextRunState();
    ControlDAC();
    
    strcpy(s, "BAT= 0.000V");
    int16_t bat_mv = BAT_MV;
    if (bat_mv < 0) {
      s[4] = '-';
      bat_mv = -bat_mv;
    }
    format_dec(s + 5, bat_mv, 5, 1);
    lcd_cursor_at(0, 0);
    lcd_puts(s);

    switch (run_state) {
    case NO_BATTERY:
      led_mode = 100;
      led_on_width = 5;
      break;
    case CHARGE_CC:
      led_mode = 250;
      led_on_width = 50;
      break;
    case CHARGE_CV:
      led_mode = 100;
      led_on_width = 50;
      break;
    case CHARGED:
      led_mode = 40;
      led_on_width = 20;
      break;
    case HIGHVOLT:
      led_mode = 50;
      led_on_width = 45;
      break;
    case DISCHARGE_CC:
      led_mode = 3;
      break;
    case DISCHARGED:
      led_mode = 1;
      break;
    case BAT_TOO_LOW:
      led_mode = 100;
      led_on_width = 95;
      break;
    }
  }
}
