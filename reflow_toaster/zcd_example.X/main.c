#include "mcc_generated_files/mcc.h"

/*
 * ## ZCD の外部抵抗値の計算
 * R_SERIES = V_PEAK / 3E-4
 * 
 * 本来は V_PEAK = 141V だけど、実験では V_PEAK = 3V とする。
 * R_SERIES = 3 / 3E-4 = 10kΩ
 * 
 * ## ZCD の検出遅れ・進み時間の計算
 * V_CPINV が 0 ではない影響で、検出タイミングがズレる。
 * ずれる幅 T0 は
 * T0 = asin(V_CPINV / V_PEAK) / 2πf
 * 
 * V_CPINV = 0.75V, V_PEAK = 141V, f = 50Hz とすると
 * T0 = 1.69E-5 = 16.9μs
 * 
 * ## 補正用プルアップ抵抗の計算
 * R_PULLUP = R_SERIES * (V_PULLUP - V_CPINV) / V_CPINV
 * 
 * R_SERIES = 10kΩ, V_PULLUP = 5V, V_CPINV = 0.75V とすると
 * R_PULLUP = 10E3 * 5.667 = 56.67E3 ≒ 56kΩ
 * 
 * 
 * Vcpinv   Vpeak   Vpullup   f   Rseries   T0(us)  Rpullup
 * 0.75     3       5         50  10000     804.3   56667
 * 0.75     5       5         50  16666     479.3   94444
 * 0.75     141     5         50  470000    16.93   2663333
 * 0.75     141     5         50  330000    16.93   1870000
 * 
 * ## ZCD 信号の出力
 * RC0～RC5: ZCD 割り込みの回数をデジタル値で出力
 * RB5: ZCD 割り込み 50 回につき 1 回反転（f_in/25 Hz）
 * RB4: ZCD 割り込みで反転
 * RA0: ZCD 出力信号を CLC 経由で出力
 * 
 * RA0 と RB4 はほとんど同じ信号だが、RB4 は割り込み処理の分の遅延がある
 */

volatile unsigned int zcd_count;

void ZCD_ISR(void) {
  IO_DETECT_LAT = ZCDCONbits.ZCDOUT;
  zcd_count++;
  if (zcd_count == 50) {
    zcd_count = 0;
    IO_LED_LAT = !IO_LED_LAT;
  }
  LATC = (zcd_count & 0x3fu) | (LATC & 0xc0u);
  PIR2bits.ZCDIF = 0;
}

void main(void) {
  SYSTEM_Initialize();
  INTERRUPT_GlobalInterruptEnable();
  INTERRUPT_PeripheralInterruptEnable();

  while (1) {
    __delay_ms(500);
  }
}