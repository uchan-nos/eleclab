#include <string.h>

#include "mcc_generated_files/mcc.h"
#include "sc1602.h"

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
        int current = ADC_GetConversion(AN_SENSE) * 2;
        // 100Vf = 100(5[V] - adc_value * 5/1024)
        ADCON1bits.ADPREF = 0; // Vref+ = VDD (5V)
        int vf = 500 - (ADC_GetConversion(AN_LEDVF) * 500l) / 1024;

        strcpy(s, " 0.00mA Vf=0.00V");
        format_dec(s + 0, current, 5, 2);
        format_dec(s + 11, vf, 4, 1);
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
                strncpy(s + 2, "+Inf", 4);
                strncpy(s + 11, "+Inf", 4);
            } else {
                // Vdd=5[V] のときの抵抗両端の電圧 Rv[V] = 5 - Vf = 5 - vf/100
                // 抵抗の電流 Ir[mA] = I = current/10
                // R[ohm] = Rv / (Ir/1000)
                //        = (5 - vf/100) / (current/10000)
                //        = 10000 * (5 - vf/100) / current
                long reg5v = (50000l - 100l * vf) / current;
                long reg3v3 = (33000l - 100l * vf) / current;
                if (reg5v >= 0) {
                    format_dec(s + 2, reg5v, 5, 5);
                } else {
                    s[2] = '-';
                }
                if (reg3v3 >= 0) {
                    format_dec(s + 11, reg3v3, 5, 5);
                } else {
                    s[11] = '-';
                }
            }
            lcd_puts(s);
        }

        __delay_ms(500);
    }
}
