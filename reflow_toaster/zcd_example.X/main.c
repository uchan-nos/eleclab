#include "mcc_generated_files/mcc.h"

void main(void) {
  SYSTEM_Initialize();
  INTERRUPT_GlobalInterruptEnable();
  INTERRUPT_PeripheralInterruptEnable();

  while (1) {
    IO_LED_LAT = !IO_LED_LAT;
    __delay_ms(500);
  }
}