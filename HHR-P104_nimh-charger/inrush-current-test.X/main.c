#include "mcc_generated_files/mcc.h"

void main(void) {
  SYSTEM_Initialize();

  // 電流制御用オペアンプが無効のときに 0V を出力する
  TRISBbits.TRISB1 = 0;
  LATBbits.LATB1 = 0;
  
  // 電流制御用オペアンプの - 入力の電圧を 0V とする
  DAC1_SetOutput(0);

  while (1) {
    // 電流制御用オペアンプを有効にする
    OPA2CONbits.OPA2EN = 1;
    for (int i = 0; i < 2; i++) {
      IO_LED_LAT = 1;
      __delay_ms(500);
      IO_LED_LAT = 0;
      __delay_ms(500);
    }

    // 電流制御用オペアンプを無効にする
    OPA2CONbits.OPA2EN = 0;
    for (int i = 0; i < 2; i++) {
      IO_LED_LAT = 1;
      __delay_ms(100);
      IO_LED_LAT = 0;
      __delay_ms(900);
    }
  }
}
