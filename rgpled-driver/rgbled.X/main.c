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

#define LED_DATA_MAX (3 * 10)
// LED 送信データのバイト数
uint8_t led_data_bytes = 3 * 7;

uint8_t led_data[LED_DATA_MAX] = {
  0x3f, 0x00, 0x00, // RGB WS2851B
  0x00, 0x3f, 0x00, // RGB WS2851B
  0x00, 0x00, 0x3f, // RGB WS2851B
  0x00, 0x20, 0x20, // GRB SK6812MINI-E
  0x20, 0x00, 0x20, // GRB SK6812MINI-E
  0x20, 0x20, 0x00, // GRB OSTW3535C1A
  0x15, 0x15, 0x15, // GRB OSTW3535C1A
};

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

  INTERRUPT_PeripheralInterruptEnable();
  INTERRUPT_GlobalInterruptEnable();
  
  LEDSendData();
  
  while (1) {
    if (EUSART_is_rx_ready()) {
      uint8_t len = EUSART_Read();
      if (len >= 0xf0) {
        // コマンド
        switch (len) {
        case 0xf0:
          LEDSendData();
          break;
        default:
          EUSART_Write(0xff); // Unknown Command
        }
      } else if (len > LED_DATA_MAX) {
        EUSART_Write(0xfe); // Too large length
      } else {
        led_data_bytes = len;
        for (uint8_t i = 0; i < len; i++) {
          led_data[i] = EUSART_Read();
        }
        EUSART_Write(len); // Successfully Received
      }
    }
  }
}
