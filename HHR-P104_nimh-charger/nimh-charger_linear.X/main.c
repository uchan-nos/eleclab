#include "mcc_generated_files/mcc.h"

#define TARGET_CURRENT_MA 200
#define TARGET_VOLTAGE_MV 4350
#define NO_BATTERY_MV     4700
#define NO_BATTERY_VF_MV  100

#define MV_TO_ADC(mv)       ((adc_result_t)(mv / 2 / 4))
#define TARGET_VOLTAGE_ADC  MV_TO_ADC(TARGET_VOLTAGE_MV)
#define NO_BATTERY_ADC      MV_TO_ADC(NO_BATTERY_MV)
#define NO_BATTERY_VF_ADC   MV_TO_ADC(NO_BATTERY_VF_MV)
#define DACFVR_MV 1024

enum RunState {
  NO_BATTERY, // 充放電対象の電池が接続されていない
  CHARGE_CC,
  CHARGE_CV,
  CHARGED,
  HIGHVOLT,   // MOSFET に規定以上の電圧が加わっている
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

_Bool battery_exist() {
  set_current(1);
  adc_result_t vf_adc = ADC_GetConversion(channel_VF);
  stop_current();
  return vf_adc > NO_BATTERY_VF_ADC;
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

volatile uint8_t tick = 0;
void tmr_isr() {
  // interrupt period = 10ms
  tick++;
  //control_dac();
  
  if (CMP1_GetOutputStatus()) {
    run_state = HIGHVOLT;
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
  INTERRUPT_PeripheralInterruptEnable();
  INTERRUPT_GlobalInterruptEnable();
  TMR2_StartTimer();
  
  /*
  int nobat = 0;
   */
  stop_current();

  OPA2CONbits.OPA2EN = 0;
  TRISBbits.TRISB1 = 0;
  LATBbits.LATB1 = 0;
  //IO_RB1_LAT = 0;

  for (int i = 0; i < 2; i++) {
    IO_LED_LAT = 1;
    __delay_ms(500);
    IO_LED_LAT = 0;
    __delay_ms(500);
  }

  while (ADC_GetConversion(channel_BAT) > 1000);

  /*
  while (1) {
    IO_LED_LAT = 1;
  }
   */
  
  while (1) {
    switch (run_state) {
    case NO_BATTERY:
      IO_LED_LAT = 1;
      sleep_tick(5);
      IO_LED_LAT = 0;
      sleep_tick(95);
      stop_current();
      
      //nobat++;
      //if (nobat >= 3) {
      //  charge_state = CHARGE_CC;
      //}
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
    }
  }
}
