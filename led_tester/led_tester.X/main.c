#include <string.h>

#include "mcc_generated_files/mcc.h"
#include "sc1602.h"

#define FVR_ADC_N 4

void format_dec(char* s, long v, int n, int dp) {
    for (int i = n - 1; i >= 0; --i) {
        if (i == dp) {
            s[i] = '.';
        } else if (v == 0) {
            if (i == dp - 1) {
                s[i] = '0';
            }
            break;
        } else {
            s[i] = '0' + v % 10;
            v /= 10;
        }
    }
}

void main(void) {
    SYSTEM_Initialize();
    lcd_init();
    char s[17];
    
    for (;;) {
        // I[mA] = (adc_value * 1/1000)[V] / 50[Ω] * 1000
        //       = adc_value / 50
        // 100I = adc_value * 2
        ADCON1bits.ADPREF = 3; // Vref+ = FVR (1.024V)
        int current = ADC_GetConversion(AN_SENSE) * 2; // 10uA 単位

        ADCON1bits.ADPREF = 0; // Vref+ = VDD (5V)
        // Vref+ = VDD としたときの FVR の ADC 値（1.024V あたりの ADC 値）
        int fvr_adc_refvdd = 0;
        for (int i = 0; i < FVR_ADC_N; i++) { // FVR_ADC_N 回の平均を取る
          fvr_adc_refvdd += ADC_GetConversion(channel_FVR);
        }
        // VDD の電圧 mV を求める
        // fvr_adc_refvdd/N : Vref+=VDD としたときの、1024mV あたりの ADC 値
        // 1024 / (fvr_adc_refvdd/N) : ADC の 1 ステップあたりの電圧 mV
        // 1024 * (1024 / (fvr_adc_refvdd/N)) : フルスケールに相当する電圧 mV
        int vdd_mv = (1024L * 1024L * FVR_ADC_N) / fvr_adc_refvdd;
        // LED の VF mV
        int vf_mv = vdd_mv - (ADC_GetConversion(AN_LEDVF) * (long)vdd_mv) / 1024;
        int vf = vf_mv / 10; // 10mV 単位

        strcpy(s, " 0.00mA Vf=0.000");
        format_dec(s + 0, current, 5, 2);
        format_dec(s + 11, vf_mv, 5, 1);
        lcd_cursor_at(0, 0);
        lcd_puts(s);

        lcd_cursor_at(0, 1);
        if (vf > 480) {
            // Vf > 4.8V のときは LED が抜かれたと考え、省電力モードとする
            IO_OPAMP_EN_TRIS = 0;
            IO_OPAMP_EN_LAT = 0;
            lcd_puts("LED \xa6 \xbb\xbc\xc3 \xb8\xc0\xde\xbb\xb2 ");
        } else {
            IO_OPAMP_EN_TRIS = 1;
            strcpy(s, "5:      33:     ");
            if (current == 0) {
                strncpy(s + 3, "+Inf", 4);
                strncpy(s + 12, "+Inf", 4);
            } else {
                // Vdd=5[V] のときの抵抗両端の電圧 Rv[V] = 5 - Vf = 5 - vf/100
                // 抵抗の電流 Ir[mA] = I = current/100
                // R[ohm] = Rv / (Ir/1000)
                //        = (5 - vf/100) / (current/100000)
                //        = 100000 * (5 - vf/100) / current
                long reg5v = (500000l - 1000l * vf) / current;
                long reg3v3 = (330000l - 1000l * vf) / current;
                if (reg5v >= 0) {
                    format_dec(s + 2, reg5v, 5, 5);
                } else {
                    s[6] = '-';
                }
                if (reg3v3 >= 0) {
                    format_dec(s + 11, reg3v3, 5, 5);
                } else {
                    s[15] = '-';
                }
            }
            lcd_puts(s);
        }

        __delay_ms(500);
    }
}
