/*
 * Copyright (c) 2025 Kota UCHIDA
 *
 * LED テスター 2 のファームウェア（LCD コントローラ用）
 */
#include "ch32fun.h"
#include <assert.h>

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

  funAnalogInit();
  funGpioInitAll();

  funPinMode(PA1, GPIO_CFGLR_IN_ANALOG); // Opamp Input
  funPinMode(PA2, GPIO_CFGLR_IN_ANALOG); // Opamp Input
  //funPinMode(PD4, GPIO_CFGLR_IN_ANALOG); // Opamp Output

  OPA1_Init(1, PA1, PA2);

  while (1) {
  }
}
