/*
 * Copyright (c) 2025 Kota UCHIDA
 *
 * CH32V003 の PD7=NRST ピンが出力できることを検証する
 */
#include "ch32fun.h"

int main() {
  SystemInit();
  funGpioInitAll();
  funPinMode(PD6, GPIO_CFGLR_OUT_2Mhz_PP);
  funPinMode(PD7, GPIO_CFGLR_OUT_2Mhz_PP);

  while (1) {
    funDigitalWrite(PD6, 1);
    funDigitalWrite(PD7, 1);
    Delay_Ms(500);

    funDigitalWrite(PD6, 0);
    funDigitalWrite(PD7, 0);
    Delay_Ms(500);
  }
}
