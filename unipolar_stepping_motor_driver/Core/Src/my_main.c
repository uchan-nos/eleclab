/*
 * my_main.c
 *
 *  Created on: Sep 19, 2022
 *      Author: uchan
 */

#include "main.h"

#include <stdio.h>

extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern UART_HandleTypeDef huart2;

void SystemClock_Config(void);

struct MotorStatus {
  TIM_HandleTypeDef *htim; // このモータを駆動するタイマ
  int current_pps;  // 現在速さ [pulse/sec] の 1000 倍の値（固定小数）
  int target_pps;   // 目標速さ [pulse/sec] の 1000 倍の値（固定小数）
  int acceleration; // 加減速度 [pulse/sec^2] の 1000 倍の値（固定小数）
};
struct MotorStatus motor_right, motor_left;

void UpdateMotorSpeed(struct MotorStatus *stat) {
  const int diff_pps = stat->target_pps - stat->current_pps;
  const int acc_10ms = stat->acceleration / 100;
  if (diff_pps > 0 && acc_10ms > 0) {
    stat->current_pps += diff_pps < acc_10ms ? diff_pps : acc_10ms;
  } else if (diff_pps < 0 && acc_10ms < 0) {
    stat->current_pps += diff_pps > acc_10ms ? diff_pps : acc_10ms;
  }

  if (diff_pps == 0) {
    HAL_GPIO_WritePin(UserLED_GPIO_Port, UserLED_Pin, 1);
  } else {
    HAL_GPIO_WritePin(UserLED_GPIO_Port, UserLED_Pin, 0);
  }
}

void one_cycle_both_wheel(int delay_ms) {
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2); // A2
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4); // B2
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); // A1
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3); // B1

  HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2); // A2
  HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3); // B1
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1); // A1
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4); // B2

  HAL_Delay(delay_ms);
  HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1); // A1
  HAL_Delay(delay_ms);

  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1); // A1
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2); // A2

  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2); // A2

  HAL_Delay(delay_ms);
  HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4); // B2
  HAL_Delay(delay_ms);

  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3); // B1
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4); // B2

  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3); // B1

  HAL_Delay(delay_ms);
  HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2); // A2
  HAL_Delay(delay_ms);

  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2); // A2
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); // A1

  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1); // A1

  HAL_Delay(delay_ms);
  HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3); // B1
  HAL_Delay(delay_ms);
}

void one_cycle_2phase(int delay_ms) {
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2); // A2
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4); // B2
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); // A1
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3); // B1
  HAL_Delay(delay_ms);
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1); // A1
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2); // A2
  HAL_Delay(delay_ms);
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3); // B1
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4); // B2
  HAL_Delay(delay_ms);
  HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2); // A2
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); // A1
  HAL_Delay(delay_ms);
}

void one_cycle_halfphase(int delay_ms) {
  HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2); // A2
  HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4); // B2
  HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3); // B1
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1); // A1
  HAL_Delay(delay_ms);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3); // B1
  HAL_Delay(delay_ms);
  HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1); // A1
  HAL_Delay(delay_ms);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2); // A2
  HAL_Delay(delay_ms);
  HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_3); // B1
  HAL_Delay(delay_ms);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4); // B2
  HAL_Delay(delay_ms);
  HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_2); // A2
  HAL_Delay(delay_ms);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1); // A1
  HAL_Delay(delay_ms);
}


void HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef *htim) {
  if (htim->Instance != htim2.Instance) {
    return;
  }

  // 制御周期 10ms
  UpdateMotorSpeed(&motor_right);
  //UpdateMotorSpeed(&motor_left);
}

int main(void) {
  motor_right.htim = &htim3;
  motor_right.current_pps = 0;
  motor_right.target_pps = 500 * 1000;
  motor_right.acceleration = 250 * 1000;
  motor_left.htim = &htim4;
  motor_left.current_pps = 0;
  motor_left.target_pps = 500 * 1000;
  motor_left.acceleration = 0;

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_TIM2_Init();

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 0);  // パラレル制御
  HAL_TIM_Base_Start_IT(&htim3);

  for (int i = 0; i < 3; i++) {
    HAL_GPIO_WritePin(UserLED_GPIO_Port, UserLED_Pin, 1);
    HAL_Delay(250);
    HAL_GPIO_WritePin(UserLED_GPIO_Port, UserLED_Pin, 0);
    HAL_Delay(250);
  }

  HAL_TIM_Base_Start_IT(&htim2);

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
