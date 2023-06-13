/*
 * File:   main.c
 * Author: uchan
 *
 * Created on 2023/06/13, 9:45
 */

#include "mcc_generated_files/mcc.h"

#define NUM_LED 7
#define PWMDCH PWM5DCH
#define PWMDCL PWM5DCL
#define T0H_DCH 2
#define T1H_DCH 5
#define PWMTRIS TRISA2

uint8_t led_index = 0;
uint8_t led_index_off = 0;
uint8_t led_bit_index = 1;
uint8_t led_data[3 * NUM_LED] = {
  0x3f, 0x00, 0x00, // RGB WS2851B
  0x00, 0x3f, 0x00, // RGB WS2851B
  0x00, 0x00, 0x3f, // RGB WS2851B
  0x00, 0x20, 0x20, // GRB SK6812MINI-E
  0x20, 0x00, 0x20, // GRB SK6812MINI-E
  0x20, 0x20, 0x00, // GRB OSTW3535C1A
  0x15, 0x15, 0x15, // GRB OSTW3535C1A
};

void Tmr2TimeoutHandler() {
  if (led_index < 3 * NUM_LED) {
    uint8_t i = led_index + led_index_off;
    if (i >= 3 * NUM_LED) {
      i -= 3 * NUM_LED;
    }
    uint8_t led_bit = (led_data[i] << led_bit_index) & 0x80u;
    led_bit_index = (led_bit_index + 1) & 7;
    if (led_bit_index == 0) {
      led_index++;
    }

    PWMDCH = led_bit ? T1H_DCH : T0H_DCH;
  } else {
    PWMDCL = 0;
    PWMDCH = 0;
  }
}

void main(void) {
  SYSTEM_Initialize();
  TMR2_StopTimer();
  TMR2_SetInterruptHandler(Tmr2TimeoutHandler);

  INTERRUPT_PeripheralInterruptEnable();
  INTERRUPT_GlobalInterruptEnable();
  
  PWMDCL = 2 << 6;
  PWMDCH = (led_data[0] & 0x80) ? T1H_DCH : T0H_DCH;
  
  __delay_ms(2);
  TMR2_StartTimer();
  
  while (1) {
    __delay_ms(300);
    if (led_index_off < 3 * (NUM_LED - 1)) {
      led_index_off += 3;
    } else {
      led_index_off = 0;
    }
    led_index = 0;
  }
}
