#include "switch_mgr.h"

static uint32_t prev_indr = 0xffu;

uint32_t MakeState(uint32_t indr, uint32_t pin) {
  unsigned sw = (indr & pin) == 0;
  unsigned prev_sw = (prev_indr & pin) == 0;
  return ((sw << 1) | prev_sw) << SW_BIT_INDEX(pin);
}

static uint32_t sw_pushed_tick;

uint32_t UpdateSwitchState(void) {
  uint32_t indr = SW_PORT->INDR;
  uint32_t st = MakeState(indr, SW1_PIN) | MakeState(indr, SW2_PIN);
  prev_indr = indr;

  if (SW_GET_STATE(st, SW_LONGPUSH_PIN) == SW_PUSH_NOW) {
    sw_pushed_tick = SysTick->CNT;
  }
  return st;
}

int IsSwitchPushedLongerThan(uint32_t ms) {
  if ((prev_indr & SW_LONGPUSH_PIN) != 0) {
    // スイッチが押されていなければ false
    return 0;
  }

  uint32_t after1s_tick = sw_pushed_tick + Ticks_from_Ms(ms);
  uint32_t cur_tick = SysTick->CNT;
  int is_long = after1s_tick <= cur_tick;
  if (sw_pushed_tick < after1s_tick) {
    is_long = is_long || cur_tick < sw_pushed_tick;
  } else { // overflow
    is_long = is_long && cur_tick < sw_pushed_tick;
  }
  return is_long;
}
