#include "mcc_generated_files/mcc.h"

#define TARGET_CURRENT_MA 200
#define TARGET_VOLTAGE_MV 4350
#define NO_BATTERY_MV     4074

#define MV_TO_ADC(mv)       ((adc_result_t)(mv / 2 / 4))
#define TARGET_VOLTAGE_ADC  MV_TO_ADC(TARGET_VOLTAGE_MV)
#define NO_BATTERY_ADC      MV_TO_ADC(NO_BATTERY_MV)
#define DACFVR_MV 1024

enum RunState {
  NO_BATTERY, // 充放電対象の電池が接続されていない
  CHARGE_CC,
  CHARGE_CV,
  CHARGED,
  HIGHVOLT,   // MOSFET に規定以上の電圧が加わっている
  DISCHARGE_CC,
};
volatile enum RunState run_state = NO_BATTERY;

/*
uint16_t adc_to_mv(adc_result_t adc) {
  return adc * 4; // 1024 = 4.096V (FVRx4)
}

void set_current(uint8_t dac_val) {
  DACCON0bits.DAC1NSS = 1;
  DAC1_SetOutput(dac_val);
}


void constant_current() {
  adc_result_t bat_adc = ADC_GetConversion(channel_BAT);
  if (bat_adc >= TARGET_VOLTAGE_ADC) {
    charge_state = CHARGE_CV;
    // 何回か継続して上回ったら、とした方がよさそう。ノイズ対策。
    return;
  }

  //uint16_t vf_mv = adc_to_mv(ADC_GetConversion(channel_VF));
  //uint16_t dac_fullrange_mv = DACFVR_MV - vf_mv;
  //uint16_t new_dac_val = 32 * TARGET_CURRENT_MA / dac_fullrange_mv;
  // DAC(mV):Current(mA) = 4:1
  // DAC=0mV@0～1024mV@32 ==> Current=0mA@0～256mA@32
  uint16_t new_dac_val = 32 * TARGET_CURRENT_MA / 1024;
  set_current(new_dac_val);
}

void constant_voltage() {
  adc_result_t bat_adc = ADC_GetConversion(channel_BAT);
  if (bat_adc < TARGET_VOLTAGE_ADC) {
    return;
  }

  uint8_t dac_val = DAC1_GetOutput();
  if (dac_val > 1) {
    set_current(dac_val - 1);
  } else {
    stop_current();
    charge_state = CHARGED;
  }
}

void control_dac() {
  switch (charge_state) {
  case NO_BATTERY:
    //if (battery_exist()) {
    //  charge_state = CHARGE_CC;
    //}
    break;
  case CHARGE_CC:
    constant_current();
    break;
  case CHARGE_CV:
    constant_voltage();
    break;
  case CHARGED:
    //if (!battery_exist()) {
    //  charge_state = NO_BATTERY;
    //}
    break;
  }
}
*/

void stop_current() {
  DAC1_SetOutput(0);
}

_Bool BatteryExist() {
  //set_current(1);
  //adc_result_t vf_adc = ADC_GetConversion(channel_VF);
  stop_current();
  //return vf_adc > NO_BATTERY_VF_ADC;
  return 0;
}

// 電池電圧/2 [mV] を表す変数
// 4 ビット左シフトした固定小数点表現（Q4 フォーマット）
volatile int32_t bat_mv2_q4;
volatile adc_result_t bat_mv2_latest;

int32_t FilterADCValueQ4(int32_t v_filtered, adc_result_t v_adc) {
  return v_filtered - (v_filtered >> 4) + (int16_t)v_adc;
}

#define BAT_MV ((bat_mv2_q4) >> 3)

enum RunState NextRunState() {
  if (CMP1_GetOutputStatus()) {
    return HIGHVOLT;
  }
  
  if (BAT_MV < (NO_BATTERY_MV - 200) || BAT_MV > (NO_BATTERY_MV + 200)) {
    return IO_MODE_PORT ? CHARGE_CC : DISCHARGE_CC;
  } else {
    // 電流を一瞬流し、電圧変動があるかをみる
    if (BatteryExist()) {
      return IO_MODE_PORT ? CHARGE_CC : DISCHARGE_CC;
    }
    return NO_BATTERY;
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
  //control_dac();
  
  if ((tick % 10) == 0) {
    ADC_StartConversion(channel_TEMP);
  }
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
  INTERRUPT_PeripheralInterruptEnable();
  INTERRUPT_GlobalInterruptEnable();
  TMR2_StartTimer();
  
  /*
  int nobat = 0;
   */
  stop_current();

  //IO_RB1_LAT = 0;

  //while (ADC_GetConversion(channel_BAT) > 1000);

  TRISBbits.TRISB1 = 0;
  LATBbits.LATB1 = 0;
  OPA2CONbits.OPA2EN = 0;
  IO_OPA1OUT_LAT = 0;
  IO_OPA1OUT_TRIS = 1;
  
  while (1) {
    IO_LED_LAT = CMP1_GetOutputStatus();
    printf("fvr=%d temp=%d an3=%d\n", ADC_GetConversion(channel_FVR), ADC_GetConversion(channel_TEMP), ADC_GetConversion(channel_AN3));
    __delay_ms(300);
  }
  
  bat_mv2_q4 = (int16_t)ADC_GetConversion(channel_TEMP) << 4;

  long temp, batn;
  while (1) {
    //printf("bat3=%ld bat=%ld adc=%04x\n", bat_mv2_q4 >> 4, (bat_mv2_q4 >> 4) * 3, bat_mv2_latest);
    temp = ADC_GetConversion(channel_TEMP);
    batn = ADC_GetConversion(channel_BATN);
    printf("TEMP=%ld BATN=%ld BAT=%ld\n", temp, batn, (temp - batn) * 3);
    __delay_ms(300);
  }
  while (1) {
    run_state = NextRunState();

    switch (run_state) {
    case NO_BATTERY:
      IO_LED_LAT = 1;
      sleep_tick(5);
      IO_LED_LAT = 0;
      sleep_tick(95);
      stop_current();
      break;
    case CHARGE_CC:
      IO_LED_LAT = 1;
      sleep_tick(50);
      IO_LED_LAT = 0;
      sleep_tick(200);
      break;
    case CHARGE_CV:
      IO_LED_LAT = 1;
      sleep_tick(50);
      IO_LED_LAT = 0;
      sleep_tick(50);
      break;
    case CHARGED:
      IO_LED_LAT = 1;
      sleep_tick(20);
      IO_LED_LAT = 0;
      sleep_tick(20);
      break;
    case HIGHVOLT:
      IO_LED_LAT = 1;
      sleep_tick(45);
      IO_LED_LAT = 0;
      sleep_tick(5);
      break;
    case DISCHARGE_CC:
      IO_LED_LAT = 1;
      sleep_tick(200);
      IO_LED_LAT = 0;
      sleep_tick(200);
      break;
    }
  }
}
