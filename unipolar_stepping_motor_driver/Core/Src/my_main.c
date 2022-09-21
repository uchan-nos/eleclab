/*
 * my_main.c
 *
 *  Created on: Sep 19, 2022
 *      Author: uchan
 */

#include "main.h"

#include <stdatomic.h>
#include <stdio.h>

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern UART_HandleTypeDef huart2;

void SystemClock_Config(void);

atomic_int pulsetimer_tick; // 現在時刻 [ms]

struct MotorStatus {
  int current_pps;  // 現在速さ [pulse/sec] の 1000 倍の値（固定小数）
  int target_pps;   // 目標速さ [pulse/sec] の 1000 倍の値（固定小数）
  int acceleration; // 加減速度 [pulse/sec^2] の 1000 倍の値（固定小数）

  uint32_t period_us; // 1 パルスの周期
  uint32_t next_us; // 次にパルスを出す時刻
  int8_t phase;        // 励磁フェーズ（0, 1, 2, 3）
  int8_t dir;          // 1: 正転、-1: 逆転
  uint8_t pins[4]; // モータ制御ピン番号（A1, A2, B1, B2）
};

struct MotorStatus motor_right, motor_left;

void InitMotorStatus(struct MotorStatus *stat, uint8_t pin_a1, uint8_t pin_a2, uint8_t pin_b1, uint8_t pin_b2) {
  stat->current_pps = stat->target_pps = stat->acceleration = 0;
  stat->period_us = stat->next_us = 0;
  stat->phase = 0;
  stat->dir = 1;
  stat->pins[0] = pin_a1;
  stat->pins[1] = pin_a2;
  stat->pins[2] = pin_b1;
  stat->pins[3] = pin_b2;
}

void SetMotorTargetSpeed(struct MotorStatus *stat, int target_pps, int acc) {
  // 割り込みを禁止する必要がある気がする
  stat->target_pps = target_pps;
  int diff_pps = target_pps - stat->current_pps;
  if (diff_pps < 0 && acc > 0) {
    acc = -acc;
  }
  stat->acceleration = acc;

  if (stat->current_pps == 0) {
    stat->period_us = 0;
  } else {
    stat->period_us = 1000000000 / stat->current_pps;
  }
  stat->next_us = pulsetimer_tick * 1000;
}

void UpdateMotorSpeed(struct MotorStatus *stat) {
  const int diff_pps = stat->target_pps - stat->current_pps;
  const int acc_10ms = stat->acceleration / 100;
  if (diff_pps > 0 && acc_10ms > 0) {
    stat->current_pps += diff_pps < acc_10ms ? diff_pps : acc_10ms;
  } else if (diff_pps < 0 && acc_10ms < 0) {
    stat->current_pps += diff_pps > acc_10ms ? diff_pps : acc_10ms;
  }
  if (stat->current_pps == 0) {
    stat->period_us = 0;
  } else {
    stat->period_us = 1000000000 / stat->current_pps;
  }

  if (diff_pps == 0) {
    HAL_GPIO_WritePin(UserLED_GPIO_Port, UserLED_Pin, 1);
  } else {
    HAL_GPIO_WritePin(UserLED_GPIO_Port, UserLED_Pin, 0);
  }
}

static const uint8_t motor_excitation[4] = {
    0b0101, 0b0110, 0b1010, 0b1001
};

void StepOne(struct MotorStatus *stat) {
  uint8_t e = motor_excitation[stat->phase];
  stat->phase = (stat->phase + stat->dir) & 3u;
  for (int i = 0; i < 4; i++) {
    HAL_GPIO_WritePin(GPIOB, 1u << stat->pins[i], (e >> i) & 1u);
  }
}

void HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef *htim) {
  if (htim->Instance == htim3.Instance) {
    // 制御周期 1ms
    pulsetimer_tick++;
    if (motor_right.next_us <= pulsetimer_tick * 1000) {
      StepOne(&motor_right);
      motor_right.next_us += motor_right.period_us;

      static int count = 0;
      if ((count++ % 100) == 0) {
        printf("TIM3 right: pulsetimer_tick=%4d count=%d\n", pulsetimer_tick, count);
      }
    }
    if (motor_left.next_us <= pulsetimer_tick * 1000) {
      StepOne(&motor_left);
      motor_left.next_us += motor_left.period_us;
    }
  } else if (htim->Instance == htim2.Instance) {
    static int count = 0;
    if ((count % 500) == 0) {
      printf("TIM2 callback: %d\n", count);
    }
    count++;
    // 制御周期 10ms
    UpdateMotorSpeed(&motor_right);
    UpdateMotorSpeed(&motor_left);
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
  SetMotorTargetSpeed(&motor_right, 500 * 1000, 25 * 1000);
  InitMotorStatus(&motor_left, 6, 7, 9, 8);
  SetMotorTargetSpeed(&motor_left, 500 * 1000, 20 * 1000);

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 0);  // パラレル制御

  while (1) {
    for (int d = 15; d >= 2; d--) {
      for (int i = 0; i < 120; i++) {
        StepOne(&motor_right);
        StepOne(&motor_left);
        HAL_Delay(d);
      }
      motor_right.dir = motor_left.dir = -motor_right.dir;
    }
  }

  //HAL_TIM_Base_Start_IT(&htim2);
  //HAL_TIM_Base_Start_IT(&htim3);

  while (1) {
    printf("%05d: %8d->%8d @%8d %8d;%d [%d]\n", pulsetimer_tick,
           motor_right.current_pps, motor_right.target_pps, motor_right.acceleration,
           motor_right.period_us, motor_right.next_us, motor_right.phase);
    HAL_Delay(500);
  }

  while (1) {
    int acc = 0;
    printf("acc> ");
    scanf("%d", &acc);
    printf("acc == %d\n", acc);

    printf("hello\n");
    if (motor_right.acceleration > 0) {
      motor_right.target_pps = 0;
      motor_right.acceleration = -acc * 1000;
    } else {
      motor_right.target_pps = 500 * 1000;
      motor_right.acceleration = acc * 1000;
    }
  }
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
