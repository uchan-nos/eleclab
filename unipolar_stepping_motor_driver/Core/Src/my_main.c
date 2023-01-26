/*
 * my_main.c
 *
 *  Created on: Sep 19, 2022
 *      Author: uchan
 */

#include "main.h"
#include "stepper_motor.h"

#include <inttypes.h>
#include <math.h>
#include <stdatomic.h>
#include <stdio.h>

#include <core_cm4.h>

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern UART_HandleTypeDef huart2;

void SystemClock_Config(void);

// 固定少数の桁位置
#define DP (INT32_C(100000))

// 1 Mega
#define MEG (INT32_C(1000000))

#define MOTOR_R 0
#define MOTOR_L 1

unsigned int mcrowne_isqrt(unsigned long val);
unsigned julery_isqrt(unsigned long val);
uint32_t julery_isqrt64(uint64_t val);
//#define ISQRT mcrowne_isqrt

void HAL_TIM_PeriodElapsedCallback (TIM_HandleTypeDef *htim) {
  if (htim->Instance == htim3.Instance) {
    uint16_t new_arr = StepMotor(MOTOR_R);
    if (new_arr == 0) {
      PowerOffMotor(MOTOR_R);
      // 停止時の制御周期を 1ms とする
      __HAL_TIM_SET_AUTORELOAD(htim, 999);
    } else {
      __HAL_TIM_SET_AUTORELOAD(htim, new_arr);
    }
  } else if (htim->Instance == htim4.Instance) {
    uint16_t new_arr = StepMotor(MOTOR_L);
    if (new_arr == 0) {
      PowerOffMotor(MOTOR_L);
      // 停止時の制御周期を 1ms とする
      __HAL_TIM_SET_AUTORELOAD(htim, 999);
    } else {
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
  InitMotor(MOTOR_R, 4, 5, 0, 1);
  InitMotor(MOTOR_L, 6, 7, 9, 8);

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 0);  // パラレル制御

  HAL_TIM_Base_Start_IT(&htim3);
  HAL_TIM_Base_Start_IT(&htim4);

  int dir = 1;
  while (1) {
    StartMotor(MOTOR_R, 500, 500);
    StartMotor(MOTOR_L, 500, 500);
    HAL_Delay(500);
    HAL_GPIO_TogglePin(UserLED_GPIO_Port, UserLED_Pin);
    StopMotor(MOTOR_R, 500);
    StopMotor(MOTOR_L, 500);
    HAL_Delay(500);
    HAL_GPIO_TogglePin(UserLED_GPIO_Port, UserLED_Pin);
    dir = -dir;
    SetMotorDirection(MOTOR_R, dir);
    SetMotorDirection(MOTOR_L, dir);
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
