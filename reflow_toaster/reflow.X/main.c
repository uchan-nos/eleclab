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

void FlushStdin() {
  int c;
  while ((c = getchar()) != '\n' && c != EOF);
}

void main(void) {
  SYSTEM_Initialize();
  IOCBF0_SetInterruptHandler(PhaseISR);
  TMR0_SetInterruptHandler(Tmr0ISR);
  INTERRUPT_GlobalInterruptEnable();
  INTERRUPT_PeripheralInterruptEnable();
  
  printf("hello, reflow toaster!\n");
  
  TMR0_StartTimer();

  char s[32];
  unsigned long v = 6000;
  for (;;) {
    printf("%lu,%lu>\n", phase_cnt, tmr0_cnt);
    FlushStdin();
    scanf("%lu", &v);
    tmr0_reload_value = 0xfffflu - v + 1;
    printf("set delay to %lu\n", v);
  }
}