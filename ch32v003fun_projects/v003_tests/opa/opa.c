/*
 * Copyright (c) 2025 Kota UCHIDA
 *
 * CH32V003 のオペアンプを検証するプログラム。
 */
#include "ch32fun.h"
#include <assert.h>
#include <stdio.h>

void OPA1_Init(int enable, int neg_pin, int pos_pin) {
  assert(neg_pin == PA1 || neg_pin == PD0);
  assert(pos_pin == PA2 || pos_pin == PD7);

  uint32_t ctr = EXTEN->EXTEN_CTR;
  ctr |= enable ? EXTEN_OPA_EN : 0;
  ctr |= neg_pin == PD0 ? EXTEN_OPA_NSEL : 0;
  ctr |= pos_pin == PD7 ? EXTEN_OPA_PSEL : 0;
  EXTEN->EXTEN_CTR = ctr;
}

int main() {
  SystemInit();
  funGpioInitAll();
  funAnalogInit();
  funPinMode(PD4, GPIO_CFGLR_IN_ANALOG); // A7
  OPA1_Init(1, PA1, PA2);

  // Read Vref = 1.2V (typ)
  int vref_adc = 0;
  for (int i = 0; i < 8; i++) {
    vref_adc += funAnalogRead(ANALOG_8);
  }
  vref_adc = (vref_adc + 4) >> 3;
  int vdd100 = 120 * 1023 / vref_adc;
  printf("ADC (Vref): %d = 1.2(V)\n", vref_adc);
  printf("VDD calculated: %d.%02d(V)\n", vdd100 / 100, vdd100 % 100);

  while (1) {
    int adc = funAnalogRead(ANALOG_7);
    int vol100 = adc * vdd100 / 1023;
    printf("ADC (PD4 = OPO): %d = %d.%02d(V)\n", adc, vol100 / 100, vol100 % 100);

    Delay_Ms(500);
  }
}
