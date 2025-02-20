// This file was copied from ch32v003fun/examples_v20x/blink/blink.c
// and a pin number was modified.
#include "ch32fun.h"
#include <stdio.h>

int main()
{
	SystemInit();

	funGpioInitAll();

	funPinMode( PA4, GPIO_CFGLR_OUT_10Mhz_PP );

	while(1)
	{
		funDigitalWrite( PA4, FUN_HIGH );	 // Turn on GPIO
		Delay_Ms( 1000 );
		funDigitalWrite( PA4, FUN_LOW );	 // Turn off GPIO
		Delay_Ms( 1000 );
	}
}
