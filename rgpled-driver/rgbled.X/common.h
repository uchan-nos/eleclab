#ifndef COMMON_H_
#define COMMON_H_

#define PWMDCH PWM5DCH
#define PWMDCL PWM5DCL
#define T0H_DCH 2
#define T1H_DCH 5

//#define SHOW_HANDLER_TIMING

//#define USE_MCC_GENERATED_ISR

// led_status のビット定義

// bit0 sending: LED 信号を送信中なら 1
#define LED_STATUS_SENDING_POSN   0
// bit1 finishing: 次の周期で LED 信号の送信が最後なら 1
#define LED_STATUS_FINISHING_POSN 1

#endif