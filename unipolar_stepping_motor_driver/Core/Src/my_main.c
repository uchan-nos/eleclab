/*
 * my_main.c
 *
 *  Created on: Sep 17, 2022
 *      Author: uchan
 */

int main(void) {
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, 0);  // パラレル制御
  HAL_TIM_Base_Start_IT(&htim3);

  int delay_ms = 2;

  while (1) {
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
    HAL_Delay(delay_ms);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
    HAL_Delay(delay_ms);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    HAL_Delay(delay_ms);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
    HAL_Delay(delay_ms);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
  }
}
