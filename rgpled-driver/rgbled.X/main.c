/*
 * File:   main.c
 * Author: uchan
 *
 * Created on 2023/06/13, 9:45
 */

#include "mcc_generated_files/mcc.h"
#include "common.h"

extern void tmr2_handler_asm();

uint8_t led_index, led_bit_index, led_curdata;

// LED 信号の送信状態を示すフラグ集（各ビットの意味は common.h を参照）
uint8_t led_status;

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
#ifdef SHOW_HANDLER_TIMING
  IO_RA0_PORT = 1;
#endif
  if (led_index < 3 * NUM_LED) {
    PWMDCH = (led_curdata & 0x80u) ? T1H_DCH : T0H_DCH;
    led_curdata <<= 1;
    led_bit_index = (led_bit_index + 1) & 7;
    if (led_bit_index == 0) {
      led_index++;
      led_curdata = led_data[led_index];
    }
  } else {
    PWMDCL = 0;
    PWMDCH = 0;
    TMR2_StopTimer();
  }
#ifdef SHOW_HANDLER_TIMING
  IO_RA0_PORT = 0;
#endif
}

void LEDSendData() {
  led_index = 0;
  led_bit_index = 0x80 >> 1;
  led_curdata = led_data[0];
  PWMDCL = 2 << 6;
  PWMDCH = (led_curdata & 0x80) ? T1H_DCH : T0H_DCH;
  led_curdata <<= 1;
  led_status = 1;
  TMR2_StartTimer();
}

#ifndef USE_MCC_GENERATED_ISR
void __interrupt() MyISR() {
  if (led_status & (1 << LED_STATUS_SENDING_POSN)) {
    // LED 信号を送信中は TMR2 割り込みだけを受け付ける
    if (TMR2IF) {
      TMR2IF = 0;
      //Tmr2TimeoutHandler();
      tmr2_handler_asm();
    }
  } else if (PIE1bits.RCIE == 1 && PIR1bits.RCIF == 1) {
    EUSART_RxDefaultInterruptHandler();
  } else if (PIE1bits.TXIE == 1 && PIR1bits.TXIF == 1) {
    EUSART_TxDefaultInterruptHandler();
  } else if (PIE1bits.BCL1IE == 1 && PIR1bits.BCL1IF == 1) {
    MSSP1_InterruptHandler();
  } else if (PIE1bits.SSP1IE == 1 && PIR1bits.SSP1IF == 1) {
    MSSP1_InterruptHandler();
  } else {
    //Unhandled Interrupt
  }
}
#endif

void main(void) {
  SYSTEM_Initialize();
  TMR2_SetInterruptHandler(Tmr2TimeoutHandler);

  INTERRUPT_PeripheralInterruptEnable();
  INTERRUPT_GlobalInterruptEnable();
  
  LEDSendData();
  
  while (1) {
    __delay_ms(300);
    uint8_t tmp[3];
    tmp[0] = led_data[3 * NUM_LED - 3];
    tmp[1] = led_data[3 * NUM_LED - 2];
    tmp[2] = led_data[3 * NUM_LED - 1];
    for (int i = NUM_LED - 1; i > 0; i--) {
      led_data[3 * i + 0] = led_data[3 * (i - 1) + 0];
      led_data[3 * i + 1] = led_data[3 * (i - 1) + 1];
      led_data[3 * i + 2] = led_data[3 * (i - 1) + 2];
    }
    led_data[0] = tmp[0];
    led_data[1] = tmp[1];
    led_data[2] = tmp[2];

    LEDSendData();
  }
}
