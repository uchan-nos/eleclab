#include "mcc_generated_files/mcc.h"

#include "sc1602.h"

static void lcd_out4(uint8_t rs, uint8_t val) {
    LCD_RS = rs;
    LCD_E = 1;
    
    LCD_DB4 = val & 0b0001u ? 1 : 0;
    LCD_DB5 = val & 0b0010u ? 1 : 0;
    LCD_DB6 = val & 0b0100u ? 1 : 0;
    LCD_DB7 = val & 0b1000u ? 1 : 0;
    
    __delay_us(1);
    LCD_E = 0;
    __delay_us(1);
}

void lcd_out8(uint8_t rs, uint8_t val) {
    lcd_out4(rs, val >> 4u);
    lcd_out4(rs, val);
}

void lcd_exec(uint8_t cmd) {
    lcd_out8(0, cmd);
    if (cmd == 0x01u || cmd == 0x02u) {
        __delay_us(1640);
    } else {
        __delay_us(40);
    }
}

void lcd_putc(char c) {
    lcd_out8(1, c);
    __delay_us(40);
}

void lcd_init() {
    __delay_ms(40);
    lcd_out4(0, 0x3u);
    __delay_us(4100);
    lcd_out4(0, 0x3u);
    __delay_us(40);
    lcd_out4(0, 0x3u);
    __delay_us(40);
    lcd_out4(0, 0x2u);  // 4bit mode after this line
    __delay_us(40);

    lcd_exec(0x28u); // function set: 4bit, 2lines, 5x7dots
    lcd_exec(0x08u); // display on/off: display off, cursor off, no blink
    lcd_exec(0x01u); // clear display
    lcd_exec(0x06u); // entry mode set: increment, with display shift
    lcd_exec(0x0cu); // display on/off: display on, cursor off, no blink
}

void lcd_puts(const char* s) {
    while (*s) {
        lcd_putc(*s);
        ++s;
    }
}

void lcd_cursor_at(uint8_t x, uint8_t y) {
    if (y == 0) {
        lcd_exec(0x80u | x);
    } else if (y == 1) {
        lcd_exec(0x80u | 0x40u | x);
    }
}
