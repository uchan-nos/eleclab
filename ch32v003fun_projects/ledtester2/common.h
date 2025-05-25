#pragma once

// LCD コントローラのアドレス
#define LCD_I2C_ADDR 0x18

// I2C 通信速度（I2C モジュールのクロック周波数より低くなければならない）
#define I2C_CLKRATE 100000

// I2C モジュールのクロック周波数（2～36MHz で、通信速度より高くなければならない）
#define I2C_PRERATE 2000000

// 高速クロックの際に 36% デューティにする場合にコメントを外す（デフォルトは 33%）
//#define I2C_DUTY_16_9

// I2C タイムアウト回数
#define I2C_TIMEOUT_MAX 20000000

/****************
 * LCD Commands *
 ****************/

// 引数なし
#define LCD_HIDE_CURSOR 0x02
#define LCD_SHOW_CURSOR 0x03

// 引数 1 バイト
#define LCD_MOVE_CURSOR 0x10
#define LCD_PUT_SPACES  0x11

// 可変長引数（引数のバイト数はコマンドバイトの下位 5 ビットで表現）
#define LCD_PUT_STRING  0x20
