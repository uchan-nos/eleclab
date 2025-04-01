#include "ch32fun.h"

/*
 * LEDs D2-5: PA4-7
 */
int main() {
  SystemInit();
  funGpioInitAll();
  funPinMode( PA4, GPIO_CFGLR_OUT_10Mhz_PP );
  funPinMode( PA5, GPIO_CFGLR_OUT_10Mhz_PP );
  funPinMode( PA6, GPIO_CFGLR_OUT_10Mhz_PP );
  funPinMode( PA7, GPIO_CFGLR_OUT_10Mhz_PP );

  uint8_t cnt = 0;

  while (1) {
    ++cnt;
    GPIOA->OUTDR = cnt << 4;
    Delay_Ms(1000);
  }
}
