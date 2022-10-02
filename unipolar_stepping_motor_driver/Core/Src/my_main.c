/*
 * my_main.c
 *
 *  Created on: Sep 19, 2022
 *      Author: uchan
 */

#include "main.h"

#include <inttypes.h>
#include <stdatomic.h>
#include <stdio.h>

extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart2;

void SystemClock_Config(void);

// 固定少数の桁位置
#define DP (INT32_C(100000))

// 1 Mega
#define MEG (INT32_C(1000000))

struct MotorStatus {
  uint32_t steps; // 残りのステップ数
  uint8_t phase;  // 励磁フェーズ
  int8_t dir;     // 1: 正転、-1: 逆転
  uint8_t pins[4]; // モータ制御ピン番号（A1, A2, B1, B2）

  int32_t current_pps;  // 現在の速度 [pulse/sec] の DP 倍の値
  int32_t target_pps;   // 目標の速度 [pulse/sec] の DP 倍の値
  int32_t acceleration; // 加速度 [pulse/sec^2] の DP 倍の値
};

struct MotorStatus motor_right, motor_left;

void InitMotorStatus(struct MotorStatus *stat, uint8_t pin_a1, uint8_t pin_a2, uint8_t pin_b1, uint8_t pin_b2) {
  stat->steps = 0;
  stat->phase = 0;
  stat->dir = 1;
  stat->pins[0] = pin_a1;
  stat->pins[1] = pin_a2;
  stat->pins[2] = pin_b1;
  stat->pins[3] = pin_b2;

  stat->current_pps = stat->target_pps = stat->acceleration = 0;
}

const uint32_t kTriangularWave = 0x8ceff731;

// stat->steps の下位 4 ビットが 0 になったときに 0 を返す
int StepOne(struct MotorStatus *stat) {
  if (stat->steps == 0) {
    for (int i = 0; i < 4; i++) {
      HAL_GPIO_WritePin(GPIOB, 1u << stat->pins[i], 0);
    }
    return 0;
  }

  uint8_t sub_phase = stat->phase & 15u;
  const uint32_t pulse_pattern = stat->current_pps < 100 * DP ? kTriangularWave : 0xffffffff;

  if (stat->phase <= 15) {
    HAL_GPIO_WritePin(GPIOB, 1u << stat->pins[1], 0); // A2
    HAL_GPIO_WritePin(GPIOB, 1u << stat->pins[2], 0); // B1
    HAL_GPIO_WritePin(GPIOB, 1u << stat->pins[0], (pulse_pattern >> sub_phase) & 1);
    HAL_GPIO_WritePin(GPIOB, 1u << stat->pins[3], (pulse_pattern >> (15 - sub_phase)) & 1);
  } else if (stat->phase <= 31) {
    HAL_GPIO_WritePin(GPIOB, 1u << stat->pins[1], 0); // A2
    HAL_GPIO_WritePin(GPIOB, 1u << stat->pins[3], 0); // B2
    HAL_GPIO_WritePin(GPIOB, 1u << stat->pins[0], (pulse_pattern >> (15 - sub_phase)) & 1);
    HAL_GPIO_WritePin(GPIOB, 1u << stat->pins[2], (pulse_pattern >> sub_phase) & 1);
  } else if (stat->phase <= 47) {
    HAL_GPIO_WritePin(GPIOB, 1u << stat->pins[0], 0); // A1
    HAL_GPIO_WritePin(GPIOB, 1u << stat->pins[3], 0); // B2
    HAL_GPIO_WritePin(GPIOB, 1u << stat->pins[1], (pulse_pattern >> sub_phase) & 1);
    HAL_GPIO_WritePin(GPIOB, 1u << stat->pins[2], (pulse_pattern >> (15 - sub_phase)) & 1);
  } else {
    HAL_GPIO_WritePin(GPIOB, 1u << stat->pins[0], 0); // A1
    HAL_GPIO_WritePin(GPIOB, 1u << stat->pins[2], 0); // B1
    HAL_GPIO_WritePin(GPIOB, 1u << stat->pins[1], (pulse_pattern >> (15 - sub_phase)) & 1);
    HAL_GPIO_WritePin(GPIOB, 1u << stat->pins[3], (pulse_pattern >> sub_phase) & 1);
  }

  stat->phase = (stat->phase + stat->dir) & 63u;
  sub_phase = (stat->phase & 15u);

  if (sub_phase == 0) {
    stat->steps--;
  }
  return sub_phase;
}

volatile atomic_int motor_right_step_add, motor_left_step_add; // ステップ数加算要求

void HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef *htim) {
  if (htim->Instance == htim3.Instance) {
    // 制御周期はステッピングモータの 1 ステップの 1/16
    motor_right.steps += atomic_exchange(&motor_right_step_add, 0);
    motor_left.steps += atomic_exchange(&motor_left_step_add, 0);

    if (motor_right.current_pps == 0 && motor_right.acceleration > 0) {
      motor_right.current_pps = motor_right.acceleration / 10;
      uint16_t new_arr = ((int64_t)(MEG) * DP / 16) / motor_right.current_pps;
      __HAL_TIM_SET_AUTORELOAD(htim, new_arr);
    }

    if (StepOne(&motor_right) == 0 && motor_right.current_pps != motor_right.target_pps) {
      uint16_t arr = __HAL_TIM_GET_AUTORELOAD(htim);
      // タイマクロック = 1MHz なので、タイマ周期 = arr+1 [us]
      // したがって、モータの 1 ステップ = 16*(arr+1) [us] = 16*(arr+1)/MEG [s]
      int32_t acc_diff = ((int64_t)(motor_right.acceleration) * 16*(arr+1)) / MEG;
      motor_right.current_pps += acc_diff;
      if (motor_right.current_pps > motor_right.target_pps) {
        motor_right.current_pps = motor_right.target_pps;
      } else if (motor_right.current_pps < 0) {
        motor_right.current_pps = 0;
      }

      uint16_t new_arr = ((int64_t)(MEG) * DP / 16) / motor_right.current_pps;
      __HAL_TIM_SET_AUTORELOAD(htim, new_arr);
    }
  }
}

/*
 * PB4: Motor1 A1
 * PB5: Motor1 A2
 * PB0: Motor1 B1
 * PB1: Motor1 B2
 *
 * PB6: Motor2 A1
 * PB7: Motor2 A2
 * PB8: Motor2 B1
 * PB9: Motor2 B2
 */
int main(void) {
  InitMotorStatus(&motor_right, 4, 5, 0, 1);
  motor_right.target_pps = 500 * DP;
  motor_right.acceleration = 250 * DP;
  motor_right.steps = 120 * 10;
  InitMotorStatus(&motor_left, 6, 7, 9, 8);

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 0);  // パラレル制御

  HAL_TIM_Base_Start_IT(&htim3);

  /*
  while (1) {
    kMicrostepPattern = 0xffffffff;
    __HAL_TIM_SET_AUTORELOAD(&htim3, 999);
    motor_right_step_add = 120 * 3;
    HAL_Delay(2000);
    kMicrostepPattern = 0x8ceff731;
    __HAL_TIM_SET_AUTORELOAD(&htim3, 999);
    motor_right_step_add = 120 * 3;
    HAL_Delay(3000);

    kMicrostepPattern = 0xffffffff;
    __HAL_TIM_SET_AUTORELOAD(&htim3, 1499);
    motor_right_step_add = 120 * 3;
    HAL_Delay(2000);
    kMicrostepPattern = 0x8ceff731;
    __HAL_TIM_SET_AUTORELOAD(&htim3, 1499);
    motor_right_step_add = 120 * 3;
    HAL_Delay(3000);

    kMicrostepPattern = 0xffffffff;
    __HAL_TIM_SET_AUTORELOAD(&htim3, 1999);
    motor_right_step_add = 120 * 2;
    HAL_Delay(2000);
    kMicrostepPattern = 0x8ceff731;
    __HAL_TIM_SET_AUTORELOAD(&htim3, 1999);
    motor_right_step_add = 120 * 2;
    HAL_Delay(3000);

    kMicrostepPattern = 0xffffffff;
    __HAL_TIM_SET_AUTORELOAD(&htim3, 2999);
    motor_right_step_add = 120 * 1;
    HAL_Delay(2000);
    kMicrostepPattern = 0x8ceff731;
    __HAL_TIM_SET_AUTORELOAD(&htim3, 2999);
    motor_right_step_add = 120 * 1;
    HAL_Delay(3000);

    kMicrostepPattern = 0xffffffff;
    __HAL_TIM_SET_AUTORELOAD(&htim3, 3999);
    motor_right_step_add = 120 * 1;
    HAL_Delay(2000);
    kMicrostepPattern = 0x8ceff731;
    __HAL_TIM_SET_AUTORELOAD(&htim3, 3999);
    motor_right_step_add = 120 * 1;
    HAL_Delay(3000);
  }
  */

  while (1) {
    HAL_Delay(1000);
    HAL_GPIO_TogglePin(UserLED_GPIO_Port, UserLED_Pin);
  }
  /*
   * ARR   1ステップ周期
   * 499   1ms
   * 999   2ms
   * 1499  3ms
   * 1999  4ms
   * 9999  20ms
   * 24999 50ms
   * 49999 100ms
   */

  /*
  while (1) {
    uint16_t arr;
    int pat, steps;
    printf("arr pat(0/1) steps: ");
    scanf("%" PRIu16 " %d %d", &arr, &pat, &steps);
    if (pat) {
      kMicrostepPattern = 0x8ceff731;
    } else {
      kMicrostepPattern = 0xffffffff;
    }
    __HAL_TIM_SET_AUTORELOAD(&htim3, arr);

    motor_right_step_add = steps;
    motor_left_step_add = steps;
  }
  */
}

/********************************
 * 標準入出力のサポート用関数群 *
 ********************************/
int __io_putchar(int ch) {
  uint8_t c = ch;
  if (HAL_UART_Transmit(&huart2, &c, 1, 100 /* ms */) == HAL_OK) {
    return ch;
  }
  return EOF;
}

int __io_getchar(void) {
  uint8_t c;
  if (HAL_UART_Receive(&huart2, &c, 1, 100 /* ms */) == HAL_OK) {
    return c;
  }
  return EOF;
}

// 最低 1 バイトを読めるまでブロックし、
// 1 バイト読めた後は EOF が来たらすぐに return する
int _read(int file, char *ptr, int n) {
  int c;
  while ((c = __io_getchar()) == EOF);
  ptr[0] = c;
  for (int i = 1; i < n; i++) {
    if ((c = __io_getchar()) == EOF) {
      return i;
    }
    ptr[i] = c;
  }
  return n;
}
