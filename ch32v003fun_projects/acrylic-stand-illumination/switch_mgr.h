#include "ch32fun.h"
#include <stdint.h>

#define SW_RELEASED    0
#define SW_RELEASE_NOW 1
#define SW_PUSH_NOW    2
#define SW_PUSHED      3

#define SW_BIT_INDEX(pin) ((pin) << 1)
#define SW_GET_STATE(st, pin) (((st) >> SW_BIT_INDEX(pin)) & UINT32_C(0x03))

#define SW_PORT GPIOC
#define SW1_PIN GPIO_Pin_1
#define SW2_PIN GPIO_Pin_2

#define SW_LONGPUSH_PIN SW1_PIN

uint32_t UpdateSwitchState(void);
int IsSwitchPushedLongerThan(uint32_t ms);
