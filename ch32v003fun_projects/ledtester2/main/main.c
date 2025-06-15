/*
 * Copyright (c) 2025 Kota UCHIDA
 *
 * LED テスター 2 のファームウェア
 */
#include "ch32fun.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"

#define DMA_CNT 32
#define CSR 50 // 電流検出抵抗（Current Sensing Resistor）
static uint16_t adc_buf[DMA_CNT];
static uint8_t adc_current_ch;
static uint8_t led;

// VCC の電圧（10μV 単位）
// MeasureVCC() が設定する
uint32_t vcc_10uv;

// PWM=0 のときの各アンプの出力値（オペアンプのオフセット電圧に依存）
// MeasureGND() が設定する
uint16_t adc_gnd_x5;
uint16_t adc_gnd_x49;

#define AMPx49_PIN PD4
#define AMPx49_AN ANALOG_7
#define AMPx5_PIN PD6
#define AMPx5_AN ANALOG_6
#define VR_PIN PA2
#define VR_AN ANALOG_0
#define VF_AN ANALOG_5
#define SEL1_PIN PD3
#define SEL2_PIN PD7
// to use PD7=NRST, pass -D option to minichlink

#define EXTI_LINE_N(n) EXTI_Line # n

#define ENCA_PORT     PC
#define ENCA_LINE     0
#define ENCB_PORT     PC
#define ENCB_LINE     5
#define MODE_BTN_PORT PC
#define MODE_BTN_LINE 6
#define LED_BTN_PORT  PC
#define LED_BTN_LINE  7

#define CONCAT2(a, b) a ## b
#define CONCAT4(a, b, c, d) a ## b ## c ## d

#define PIN_NAME_RAW(port, line) CONCAT2(port, line)
#define PIN_NAME(id) PIN_NAME_RAW(id ## _PORT, id ## _LINE)

#define EXTI_LINE_RAW(line) CONCAT2(EXTI_Line, line)
#define EXTI_LINE(id) EXTI_LINE_RAW(id ## _LINE)

#define AFIO_EXTICR_VAL_RAW(port, line) CONCAT4(AFIO_EXTICR_EXTI, line, _, port)
#define AFIO_EXTICR_VAL(id) AFIO_EXTICR_VAL_RAW(id ## _PORT, id ## _LINE)

#define EXTI_MR_RAW(line) CONCAT2(EXTI_INTENR_MR, line)
#define EXTI_MR(id) EXTI_MR_RAW(id ## _LINE)

#define ENCA_PIN      PIN_NAME(ENCA)
#define ENCB_PIN      PIN_NAME(ENCB)
#define MODE_BTN_PIN  PIN_NAME(MODE_BTN)
#define LED_BTN_PIN   PIN_NAME(LED_BTN)

#define I2C_REMAP 0
// 0, GPIO_PartialRemap_I2C1, GPIO_FullRemap_I2C1
#define SCL_PIN PC2
#define SDA_PIN PC1

// ADC の読み取り値を 10μV 単位に変換する
#define ADC_TO_10UV(adc) (((uint32_t)vcc_10uv * (adc)) >> ADC_BITS)

void __assert_func(const char *file, int line, const char *func, const char *expr) {
  printf("assertion failed inside %s (%s:%d): %s\n", func, file, line, expr);
  while (1) {
    __WFE();
  }
}

// Vref を用いて VCC を測定する
void MeasureVCC(void) {
  funAnalogRead(ANALOG_8); // 1 回読み飛ばす
  uint16_t adc_vref = 0;
  for (int i = 0; i < 16; ++i) {
    adc_vref += funAnalogRead(ANALOG_8);
  }
  //adc_vref >>= 4;

  // ADC_BITS == 10
  // adc_vref ~= 246 * 16
  // bits(adc_vref) == 8 + 4
  // bits(1200000) == 21
  // bits(120000) == 17

  // Vref = 1.2V (typical)
  // adc_vref : ADC@Vcc = 1.2 : Vcc
  // Vcc = 1.2 * ADC@Vcc / adc_vref
  vcc_10uv = ((UINT32_C(120000) << (ADC_BITS + 4)) / adc_vref);
}

uint16_t MeasureADCMin(uint8_t adc_ch) {
  funAnalogRead(adc_ch); // 1 回読み飛ばす
  uint16_t adc_min = UINT16_MAX;
  for (int i = 0; i < 16; ++i) {
    uint16_t adc = funAnalogRead(adc_ch);
    if (adc_min > adc) {
      adc_min = adc;
    }
  }
  return adc_min;
}

void MeasureGND(void) {
  adc_gnd_x5 = MeasureADCMin(AMPx5_AN);
  adc_gnd_x49 = MeasureADCMin(AMPx49_AN);
}

// ADC の読み取り値から電流値（μA 単位）を計算する
uint16_t CalcIF(uint16_t adc_vr, uint8_t adc_ch) {
  switch (adc_ch) {
  case AMPx49_AN:
    if (adc_vr >= adc_gnd_x49) {
      adc_vr -= adc_gnd_x49;
    } else {
      adc_vr = 0;
    }
    return 10 * (ADC_TO_10UV(adc_vr) / CSR) / 49;
  case AMPx5_AN:
    if (adc_vr >= adc_gnd_x5) {
      adc_vr -= adc_gnd_x5;
    } else {
      adc_vr = 0;
    }
    return 100 * (ADC_TO_10UV(adc_vr) / CSR) / 49;
  case VR_AN:
    return 10 * (ADC_TO_10UV(adc_vr) / CSR);
  default:
    assert(0);
  }
}

// 使用するアンプ倍率を決定し、対応する ADC チャンネルを選択
void DetermineADCChForVR() {
  ADC1->CTLR2 &= ~ADC_DMA;
  funAnalogRead(VR_AN); // スイッチ切替直後は 1 回読み飛ばす必要がある
  uint16_t adc_vr = funAnalogRead(VR_AN);
  if (adc_vr < 20) {
    adc_current_ch = AMPx49_AN;
  } else if (adc_vr < 200) {
    adc_current_ch = AMPx5_AN;
  } else {
    adc_current_ch = VR_AN;
  }
  funAnalogRead(adc_current_ch); // スイッチ切替直後は 1 回読み飛ばす必要がある

  ADC1->CTLR2 |= ADC_DMA;
}

void SelectLEDCh(uint8_t ch) {
  funDigitalWrite(SEL1_PIN, ch & 1);
  funDigitalWrite(SEL2_PIN, (ch >> 1) & 1);
}

void TIM2_Updated(void) {
  static uint8_t cnt = 0;
  cnt += CTRL_PERIOD_MS;
  if (cnt >= TICK_MS) {
    cnt -= TICK_MS;
    Queue_Push(MSG_TICK);
  }

  ++led;
  if (led == LED_NUM) {
    led = 0;
  }
  SelectLEDCh(led);
  Delay_Us(1);

  DetermineADCChForVR();

  DMA1_Channel1->CNTR = DMA_CNT;
  ADC1_StartContinuousConv();
}

void DMA1_Channel1_Transferred(void) {
  static uint8_t prev_adc_ch = 0;
  static uint16_t prev_if_ua = 0;

  ADC1_StopContinuousConv();
  uint16_t adc_sum = 0;
  for (int i = 0; i < DMA_CNT; ++i) {
    adc_sum += adc_buf[i];
  }
  uint16_t adc_avg = adc_sum / DMA_CNT;

  switch (adc_current_ch) {
  case AMPx49_AN:
    // fall through
  case AMPx5_AN:
    // fall through
  case VR_AN:
    {
      uint16_t if_ua = CalcIF(adc_avg, adc_current_ch);
      uint16_t pw = NextPW(led, if_ua);
      TIM1_SetPulseWidth(led, pw);
    }
    break;
  case VF_AN:
    // something
    break;
  default:
    assert(0);
  }
}

uint16_t LEDIfToPWM(uint32_t if_ua) {
  return (69905 * if_ua) / 100000 + 7864;
}

#define MAKE_ADC_SAMPTR(ch, sample_time) \
  ((sample_time) << (3 * (ch)))

void EXTI7_0_IRQHandler(void) __attribute__((interrupt));
void EXTI7_0_IRQHandler(void) {
  static uint32_t last_enca_change_tick = 0;
  static uint32_t last_encb_change_tick = 0;
  static int last_encb = 0;
  static uint32_t last_mode_change_tick = 0;
  static uint32_t last_led_change_tick = 0;
  uint32_t current_tick = SysTick->CNT;

  uint16_t intfr = EXTI->INTFR;
  if (intfr & EXTI_LINE(ENCA)) { // ENCA
    const int enca = funDigitalRead(ENCA_PIN);
    // ENCA の立ち下がりエッジで情報を記憶し、立ち上がりエッジでキューに積む。
    // 立ち下がりから立ち上がりまでの時間が短すぎるときは誤動作と判断し、キューに積まない。
    // ただ、両エッジの間に ENCB の変化があれば、正常な信号とみなす。
    if (enca) { // 立ち上がりエッジ
      if (last_enca_change_tick + Ticks_from_Ms(3) <= current_tick ||
          last_enca_change_tick < last_encb_change_tick) {
        Queue_Push(MSG_CW + (last_encb ? 0 : 1));
      }
    } else { // 立ち下がりエッジ
      last_encb = funDigitalRead(ENCB_PIN);
    }
    last_enca_change_tick = current_tick;
  }
  if (intfr & EXTI_LINE(ENCB)) { // ENCB
    last_encb_change_tick = current_tick;
  }
  if (intfr & EXTI_LINE(MODE_BTN)) { // Button
    if (last_mode_change_tick + Ticks_from_Ms(50) <= current_tick) {
      Queue_Push(MSG_MODE_PRS);
    }
    last_mode_change_tick = current_tick;
  }
  if (intfr & EXTI_LINE(LED_BTN)) { // Button
    if (last_led_change_tick + Ticks_from_Ms(50) <= current_tick) {
      Queue_Push(MSG_LED_PRS);
    }
    last_led_change_tick = current_tick;
  }

  EXTI->INTFR = intfr;
}

int main() {
  SystemInit();

  funAnalogInit();
  funGpioInitAll();

  AFIO->PCFR1 |= I2C_REMAP;

  funPinMode(AMPx49_PIN, GPIO_CFGLR_IN_ANALOG);
  funPinMode(AMPx5_PIN, GPIO_CFGLR_IN_ANALOG);
  funPinMode(VR_PIN, GPIO_CFGLR_IN_ANALOG);
  funPinMode(SEL1_PIN, GPIO_CFGLR_OUT_50Mhz_PP);
  funPinMode(SEL2_PIN, GPIO_CFGLR_OUT_50Mhz_PP);
  funPinMode(PD2, GPIO_CFGLR_OUT_50Mhz_AF_PP); // T1CH1
  funPinMode(PA1, GPIO_CFGLR_OUT_50Mhz_AF_PP); // T1CH2
  funPinMode(PC3, GPIO_CFGLR_OUT_50Mhz_AF_PP); // T1CH3
  funPinMode(PC4, GPIO_CFGLR_OUT_50Mhz_AF_PP); // T1CH4

  funDigitalWrite(SCL_PIN, 1);
  funDigitalWrite(SDA_PIN, 1);
  funPinMode(SCL_PIN, GPIO_CFGLR_OUT_50Mhz_AF_OD);
  funPinMode(SDA_PIN, GPIO_CFGLR_OUT_50Mhz_AF_OD);

  funPinMode(ENCA_PIN, GPIO_CFGLR_IN_PUPD); // ENCA
  funPinMode(ENCB_PIN, GPIO_CFGLR_IN_PUPD); // ENCB
  funPinMode(MODE_BTN_PIN, GPIO_CFGLR_IN_PUPD); // Button 'MODE'
  funPinMode(LED_BTN_PIN, GPIO_CFGLR_IN_PUPD); // Button 'LED'
  funDigitalWrite(ENCA_PIN, 1); // pull up
  funDigitalWrite(ENCB_PIN, 1); // pull up
  funDigitalWrite(MODE_BTN_PIN, 1); // pull up
  funDigitalWrite(LED_BTN_PIN, 1); // pull up

  // ENCA/ENCB と Button の変化で割り込み
  AFIO->EXTICR =
    AFIO_EXTICR_VAL(ENCA) | AFIO_EXTICR_VAL(ENCB) |
    AFIO_EXTICR_VAL(MODE_BTN) | AFIO_EXTICR_VAL(LED_BTN);
  EXTI->INTENR =
    EXTI_MR(ENCA) | EXTI_MR(ENCB) |
    EXTI_MR(MODE_BTN) | EXTI_MR(LED_BTN);

  // ENCA/ENCB は両エッジで割り込み、ボタン類は押下（立ち下がりエッジ）で割り込み
  EXTI->RTENR = EXTI_MR(ENCA) | EXTI_MR(ENCB);
  EXTI->FTENR = EXTI_MR(ENCA) | EXTI_MR(ENCB) | EXTI_MR(MODE_BTN) | EXTI_MR(LED_BTN);
  NVIC_EnableIRQ(EXTI7_0_IRQn);

  OPA1_Init(1, PD0, PA2);

  TIM1_InitForPWM(0, 65535, 15, 0);
  TIM1->CH1CVR = 0;
  TIM1_Start();

  I2C1_Init();

  // LCD コントローラ側と通信が確立するまで送り続ける
  int set_bl_cnt = 0;
  for (; set_bl_cnt < 100; ++set_bl_cnt) {
    if (LCD_SetBLBrightness(10) == 0) {
      break;
    }
  }
  if (set_bl_cnt == 100) {
    printf("Failed to connect to the LCD controller\n");
  }

  Delay_Ms(800);

  MeasureVCC();
  MeasureGND();

  printf("adc_gnd: x5=%u x49=%u\n", adc_gnd_x5, adc_gnd_x49);
  printf("Vcc=%d.%02d(mV)\n", vcc_10uv / 100, vcc_10uv % 100);
  SelectLEDCh(led);

  InitUI();

  for (uint8_t i = 0; i < LED_NUM; ++i) {
    SetGoalCurrent(i, 0);
    TIM1_SetPulseWidth(i, 5000);
  }

  DMA1_InitForADC1(adc_buf, DMA_CNT, DMA_IT_TC);
  NVIC_EnableIRQ(DMA1_Channel1_IRQn);

  ADC1_InitForDMA(ADC_ExternalTrigConv_None);
  ADC1->SAMPTR2 =
    MAKE_ADC_SAMPTR(VR_AN, ADC_SampleTime_15Cycles) |
    MAKE_ADC_SAMPTR(AMPx5_AN, ADC_SampleTime_15Cycles) |
    MAKE_ADC_SAMPTR(AMPx49_AN, ADC_SampleTime_15Cycles);
  ADC1->RSQR1 = 0; // 1 チャンネル
  ADC1->RSQR3 = AMPx49_AN;

  TIM2_InitForSimpleTimer(FUNCONF_SYSTEM_CORE_CLOCK / 2000 - 1, CTRL_PERIOD_MS * 2 - 1);
  TIM2->DMAINTENR |= TIM_IT_Update;
  NVIC_EnableIRQ(TIM2_IRQn);

  printf("Starting TIM2\n");
  TIM2_Start();

  uint32_t tick = 0;
  uint32_t mode_press_tick = 0;
  uint8_t led_press_tick = 0;

  while (1) {
    __disable_irq();
    if (Queue_IsEmpty()) {
      __enable_irq();
      continue;
    }
    MessageType msg = Queue_Pop();
    __enable_irq();

    if (msg == MSG_TICK) {
      ++tick;
      HandleUIEvent(tick, MSG_TICK);

      if (mode_press_tick > 0 && mode_press_tick + 50/TICK_MS < tick) {
        if (funDigitalRead(MODE_BTN_PIN)) { // released
          mode_press_tick = 0;
          HandleUIEvent(tick, MSG_MODE_REL);
        }
      }
      if (led_press_tick > 0 && led_press_tick + 50/TICK_MS < tick) {
        if (funDigitalRead(LED_BTN_PIN)) { // released
          led_press_tick = 0;
          HandleUIEvent(tick, MSG_LED_REL);
        }
      }
    } else if (msg == MSG_CW || msg == MSG_CCW) {
      HandleUIEvent(tick, msg);
    } else if (msg == MSG_MODE_PRS) {
      if (mode_press_tick == 0) {
        mode_press_tick = tick;
        HandleUIEvent(tick, MSG_MODE_PRS);
      }
    } else if (msg == MSG_LED_PRS) {
      if (led_press_tick == 0) {
        led_press_tick = tick;
        HandleUIEvent(tick, MSG_LED_PRS);
      }
    }
  }
}
