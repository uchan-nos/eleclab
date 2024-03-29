/**
  @Generated Pin Manager Header File

  @Company:
    Microchip Technology Inc.

  @File Name:
    pin_manager.h

  @Summary:
    This is the Pin Manager file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  @Description
    This header file provides APIs for driver for .
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.81.7
        Device            :  PIC16F1786
        Driver Version    :  2.11
    The generated drivers are tested against the following:
        Compiler          :  XC8 2.31 and above
        MPLAB 	          :  MPLAB X 5.45	
*/

/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

#ifndef PIN_MANAGER_H
#define PIN_MANAGER_H

/**
  Section: Included Files
*/

#include <xc.h>

#define INPUT   1
#define OUTPUT  0

#define HIGH    1
#define LOW     0

#define ANALOG      1
#define DIGITAL     0

#define PULL_UP_ENABLED      1
#define PULL_UP_DISABLED     0

// get/set channel_HIVOLT aliases
#define channel_HIVOLT_TRIS                 TRISAbits.TRISA0
#define channel_HIVOLT_LAT                  LATAbits.LATA0
#define channel_HIVOLT_PORT                 PORTAbits.RA0
#define channel_HIVOLT_WPU                  WPUAbits.WPUA0
#define channel_HIVOLT_ANS                  ANSELAbits.ANSA0
#define channel_HIVOLT_SetHigh()            do { LATAbits.LATA0 = 1; } while(0)
#define channel_HIVOLT_SetLow()             do { LATAbits.LATA0 = 0; } while(0)
#define channel_HIVOLT_Toggle()             do { LATAbits.LATA0 = ~LATAbits.LATA0; } while(0)
#define channel_HIVOLT_GetValue()           PORTAbits.RA0
#define channel_HIVOLT_SetDigitalInput()    do { TRISAbits.TRISA0 = 1; } while(0)
#define channel_HIVOLT_SetDigitalOutput()   do { TRISAbits.TRISA0 = 0; } while(0)
#define channel_HIVOLT_SetPullup()          do { WPUAbits.WPUA0 = 1; } while(0)
#define channel_HIVOLT_ResetPullup()        do { WPUAbits.WPUA0 = 0; } while(0)
#define channel_HIVOLT_SetAnalogMode()      do { ANSELAbits.ANSA0 = 1; } while(0)
#define channel_HIVOLT_SetDigitalMode()     do { ANSELAbits.ANSA0 = 0; } while(0)

// get/set channel_BAT aliases
#define channel_BAT_TRIS                 TRISAbits.TRISA1
#define channel_BAT_LAT                  LATAbits.LATA1
#define channel_BAT_PORT                 PORTAbits.RA1
#define channel_BAT_WPU                  WPUAbits.WPUA1
#define channel_BAT_ANS                  ANSELAbits.ANSA1
#define channel_BAT_SetHigh()            do { LATAbits.LATA1 = 1; } while(0)
#define channel_BAT_SetLow()             do { LATAbits.LATA1 = 0; } while(0)
#define channel_BAT_Toggle()             do { LATAbits.LATA1 = ~LATAbits.LATA1; } while(0)
#define channel_BAT_GetValue()           PORTAbits.RA1
#define channel_BAT_SetDigitalInput()    do { TRISAbits.TRISA1 = 1; } while(0)
#define channel_BAT_SetDigitalOutput()   do { TRISAbits.TRISA1 = 0; } while(0)
#define channel_BAT_SetPullup()          do { WPUAbits.WPUA1 = 1; } while(0)
#define channel_BAT_ResetPullup()        do { WPUAbits.WPUA1 = 0; } while(0)
#define channel_BAT_SetAnalogMode()      do { ANSELAbits.ANSA1 = 1; } while(0)
#define channel_BAT_SetDigitalMode()     do { ANSELAbits.ANSA1 = 0; } while(0)

// get/set RA2 procedures
#define RA2_SetHigh()            do { LATAbits.LATA2 = 1; } while(0)
#define RA2_SetLow()             do { LATAbits.LATA2 = 0; } while(0)
#define RA2_Toggle()             do { LATAbits.LATA2 = ~LATAbits.LATA2; } while(0)
#define RA2_GetValue()              PORTAbits.RA2
#define RA2_SetDigitalInput()    do { TRISAbits.TRISA2 = 1; } while(0)
#define RA2_SetDigitalOutput()   do { TRISAbits.TRISA2 = 0; } while(0)
#define RA2_SetPullup()             do { WPUAbits.WPUA2 = 1; } while(0)
#define RA2_ResetPullup()           do { WPUAbits.WPUA2 = 0; } while(0)
#define RA2_SetAnalogMode()         do { ANSELAbits.ANSA2 = 1; } while(0)
#define RA2_SetDigitalMode()        do { ANSELAbits.ANSA2 = 0; } while(0)

// get/set channel_BATN aliases
#define channel_BATN_TRIS                 TRISAbits.TRISA5
#define channel_BATN_LAT                  LATAbits.LATA5
#define channel_BATN_PORT                 PORTAbits.RA5
#define channel_BATN_WPU                  WPUAbits.WPUA5
#define channel_BATN_ANS                  ANSELAbits.ANSA5
#define channel_BATN_SetHigh()            do { LATAbits.LATA5 = 1; } while(0)
#define channel_BATN_SetLow()             do { LATAbits.LATA5 = 0; } while(0)
#define channel_BATN_Toggle()             do { LATAbits.LATA5 = ~LATAbits.LATA5; } while(0)
#define channel_BATN_GetValue()           PORTAbits.RA5
#define channel_BATN_SetDigitalInput()    do { TRISAbits.TRISA5 = 1; } while(0)
#define channel_BATN_SetDigitalOutput()   do { TRISAbits.TRISA5 = 0; } while(0)
#define channel_BATN_SetPullup()          do { WPUAbits.WPUA5 = 1; } while(0)
#define channel_BATN_ResetPullup()        do { WPUAbits.WPUA5 = 0; } while(0)
#define channel_BATN_SetAnalogMode()      do { ANSELAbits.ANSA5 = 1; } while(0)
#define channel_BATN_SetDigitalMode()     do { ANSELAbits.ANSA5 = 0; } while(0)

// get/set IO_LCD_RW aliases
#define IO_LCD_RW_TRIS                 TRISAbits.TRISA6
#define IO_LCD_RW_LAT                  LATAbits.LATA6
#define IO_LCD_RW_PORT                 PORTAbits.RA6
#define IO_LCD_RW_WPU                  WPUAbits.WPUA6
#define IO_LCD_RW_SetHigh()            do { LATAbits.LATA6 = 1; } while(0)
#define IO_LCD_RW_SetLow()             do { LATAbits.LATA6 = 0; } while(0)
#define IO_LCD_RW_Toggle()             do { LATAbits.LATA6 = ~LATAbits.LATA6; } while(0)
#define IO_LCD_RW_GetValue()           PORTAbits.RA6
#define IO_LCD_RW_SetDigitalInput()    do { TRISAbits.TRISA6 = 1; } while(0)
#define IO_LCD_RW_SetDigitalOutput()   do { TRISAbits.TRISA6 = 0; } while(0)
#define IO_LCD_RW_SetPullup()          do { WPUAbits.WPUA6 = 1; } while(0)
#define IO_LCD_RW_ResetPullup()        do { WPUAbits.WPUA6 = 0; } while(0)

// get/set IO_LED aliases
#define IO_LED_TRIS                 TRISAbits.TRISA7
#define IO_LED_LAT                  LATAbits.LATA7
#define IO_LED_PORT                 PORTAbits.RA7
#define IO_LED_WPU                  WPUAbits.WPUA7
#define IO_LED_ANS                  ANSELAbits.ANSA7
#define IO_LED_SetHigh()            do { LATAbits.LATA7 = 1; } while(0)
#define IO_LED_SetLow()             do { LATAbits.LATA7 = 0; } while(0)
#define IO_LED_Toggle()             do { LATAbits.LATA7 = ~LATAbits.LATA7; } while(0)
#define IO_LED_GetValue()           PORTAbits.RA7
#define IO_LED_SetDigitalInput()    do { TRISAbits.TRISA7 = 1; } while(0)
#define IO_LED_SetDigitalOutput()   do { TRISAbits.TRISA7 = 0; } while(0)
#define IO_LED_SetPullup()          do { WPUAbits.WPUA7 = 1; } while(0)
#define IO_LED_ResetPullup()        do { WPUAbits.WPUA7 = 0; } while(0)
#define IO_LED_SetAnalogMode()      do { ANSELAbits.ANSA7 = 1; } while(0)
#define IO_LED_SetDigitalMode()     do { ANSELAbits.ANSA7 = 0; } while(0)

// get/set RB1 procedures
#define RB1_SetHigh()            do { LATBbits.LATB1 = 1; } while(0)
#define RB1_SetLow()             do { LATBbits.LATB1 = 0; } while(0)
#define RB1_Toggle()             do { LATBbits.LATB1 = ~LATBbits.LATB1; } while(0)
#define RB1_GetValue()              PORTBbits.RB1
#define RB1_SetDigitalInput()    do { TRISBbits.TRISB1 = 1; } while(0)
#define RB1_SetDigitalOutput()   do { TRISBbits.TRISB1 = 0; } while(0)
#define RB1_SetPullup()             do { WPUBbits.WPUB1 = 1; } while(0)
#define RB1_ResetPullup()           do { WPUBbits.WPUB1 = 0; } while(0)
#define RB1_SetAnalogMode()         do { ANSELBbits.ANSB1 = 1; } while(0)
#define RB1_SetDigitalMode()        do { ANSELBbits.ANSB1 = 0; } while(0)

// get/set RB2 procedures
#define RB2_SetHigh()            do { LATBbits.LATB2 = 1; } while(0)
#define RB2_SetLow()             do { LATBbits.LATB2 = 0; } while(0)
#define RB2_Toggle()             do { LATBbits.LATB2 = ~LATBbits.LATB2; } while(0)
#define RB2_GetValue()              PORTBbits.RB2
#define RB2_SetDigitalInput()    do { TRISBbits.TRISB2 = 1; } while(0)
#define RB2_SetDigitalOutput()   do { TRISBbits.TRISB2 = 0; } while(0)
#define RB2_SetPullup()             do { WPUBbits.WPUB2 = 1; } while(0)
#define RB2_ResetPullup()           do { WPUBbits.WPUB2 = 0; } while(0)
#define RB2_SetAnalogMode()         do { ANSELBbits.ANSB2 = 1; } while(0)
#define RB2_SetDigitalMode()        do { ANSELBbits.ANSB2 = 0; } while(0)

// get/set IO_MODE aliases
#define IO_MODE_TRIS                 TRISBbits.TRISB3
#define IO_MODE_LAT                  LATBbits.LATB3
#define IO_MODE_PORT                 PORTBbits.RB3
#define IO_MODE_WPU                  WPUBbits.WPUB3
#define IO_MODE_ANS                  ANSELBbits.ANSB3
#define IO_MODE_SetHigh()            do { LATBbits.LATB3 = 1; } while(0)
#define IO_MODE_SetLow()             do { LATBbits.LATB3 = 0; } while(0)
#define IO_MODE_Toggle()             do { LATBbits.LATB3 = ~LATBbits.LATB3; } while(0)
#define IO_MODE_GetValue()           PORTBbits.RB3
#define IO_MODE_SetDigitalInput()    do { TRISBbits.TRISB3 = 1; } while(0)
#define IO_MODE_SetDigitalOutput()   do { TRISBbits.TRISB3 = 0; } while(0)
#define IO_MODE_SetPullup()          do { WPUBbits.WPUB3 = 1; } while(0)
#define IO_MODE_ResetPullup()        do { WPUBbits.WPUB3 = 0; } while(0)
#define IO_MODE_SetAnalogMode()      do { ANSELBbits.ANSB3 = 1; } while(0)
#define IO_MODE_SetDigitalMode()     do { ANSELBbits.ANSB3 = 0; } while(0)

// get/set channel_TEMP aliases
#define channel_TEMP_TRIS                 TRISBbits.TRISB5
#define channel_TEMP_LAT                  LATBbits.LATB5
#define channel_TEMP_PORT                 PORTBbits.RB5
#define channel_TEMP_WPU                  WPUBbits.WPUB5
#define channel_TEMP_ANS                  ANSELBbits.ANSB5
#define channel_TEMP_SetHigh()            do { LATBbits.LATB5 = 1; } while(0)
#define channel_TEMP_SetLow()             do { LATBbits.LATB5 = 0; } while(0)
#define channel_TEMP_Toggle()             do { LATBbits.LATB5 = ~LATBbits.LATB5; } while(0)
#define channel_TEMP_GetValue()           PORTBbits.RB5
#define channel_TEMP_SetDigitalInput()    do { TRISBbits.TRISB5 = 1; } while(0)
#define channel_TEMP_SetDigitalOutput()   do { TRISBbits.TRISB5 = 0; } while(0)
#define channel_TEMP_SetPullup()          do { WPUBbits.WPUB5 = 1; } while(0)
#define channel_TEMP_ResetPullup()        do { WPUBbits.WPUB5 = 0; } while(0)
#define channel_TEMP_SetAnalogMode()      do { ANSELBbits.ANSB5 = 1; } while(0)
#define channel_TEMP_SetDigitalMode()     do { ANSELBbits.ANSB5 = 0; } while(0)

// get/set IO_RC0 aliases
#define IO_RC0_TRIS                 TRISCbits.TRISC0
#define IO_RC0_LAT                  LATCbits.LATC0
#define IO_RC0_PORT                 PORTCbits.RC0
#define IO_RC0_WPU                  WPUCbits.WPUC0
#define IO_RC0_SetHigh()            do { LATCbits.LATC0 = 1; } while(0)
#define IO_RC0_SetLow()             do { LATCbits.LATC0 = 0; } while(0)
#define IO_RC0_Toggle()             do { LATCbits.LATC0 = ~LATCbits.LATC0; } while(0)
#define IO_RC0_GetValue()           PORTCbits.RC0
#define IO_RC0_SetDigitalInput()    do { TRISCbits.TRISC0 = 1; } while(0)
#define IO_RC0_SetDigitalOutput()   do { TRISCbits.TRISC0 = 0; } while(0)
#define IO_RC0_SetPullup()          do { WPUCbits.WPUC0 = 1; } while(0)
#define IO_RC0_ResetPullup()        do { WPUCbits.WPUC0 = 0; } while(0)

// get/set IO_RC1 aliases
#define IO_RC1_TRIS                 TRISCbits.TRISC1
#define IO_RC1_LAT                  LATCbits.LATC1
#define IO_RC1_PORT                 PORTCbits.RC1
#define IO_RC1_WPU                  WPUCbits.WPUC1
#define IO_RC1_SetHigh()            do { LATCbits.LATC1 = 1; } while(0)
#define IO_RC1_SetLow()             do { LATCbits.LATC1 = 0; } while(0)
#define IO_RC1_Toggle()             do { LATCbits.LATC1 = ~LATCbits.LATC1; } while(0)
#define IO_RC1_GetValue()           PORTCbits.RC1
#define IO_RC1_SetDigitalInput()    do { TRISCbits.TRISC1 = 1; } while(0)
#define IO_RC1_SetDigitalOutput()   do { TRISCbits.TRISC1 = 0; } while(0)
#define IO_RC1_SetPullup()          do { WPUCbits.WPUC1 = 1; } while(0)
#define IO_RC1_ResetPullup()        do { WPUCbits.WPUC1 = 0; } while(0)

// get/set IO_RC2 aliases
#define IO_RC2_TRIS                 TRISCbits.TRISC2
#define IO_RC2_LAT                  LATCbits.LATC2
#define IO_RC2_PORT                 PORTCbits.RC2
#define IO_RC2_WPU                  WPUCbits.WPUC2
#define IO_RC2_SetHigh()            do { LATCbits.LATC2 = 1; } while(0)
#define IO_RC2_SetLow()             do { LATCbits.LATC2 = 0; } while(0)
#define IO_RC2_Toggle()             do { LATCbits.LATC2 = ~LATCbits.LATC2; } while(0)
#define IO_RC2_GetValue()           PORTCbits.RC2
#define IO_RC2_SetDigitalInput()    do { TRISCbits.TRISC2 = 1; } while(0)
#define IO_RC2_SetDigitalOutput()   do { TRISCbits.TRISC2 = 0; } while(0)
#define IO_RC2_SetPullup()          do { WPUCbits.WPUC2 = 1; } while(0)
#define IO_RC2_ResetPullup()        do { WPUCbits.WPUC2 = 0; } while(0)

// get/set IO_RC3 aliases
#define IO_RC3_TRIS                 TRISCbits.TRISC3
#define IO_RC3_LAT                  LATCbits.LATC3
#define IO_RC3_PORT                 PORTCbits.RC3
#define IO_RC3_WPU                  WPUCbits.WPUC3
#define IO_RC3_SetHigh()            do { LATCbits.LATC3 = 1; } while(0)
#define IO_RC3_SetLow()             do { LATCbits.LATC3 = 0; } while(0)
#define IO_RC3_Toggle()             do { LATCbits.LATC3 = ~LATCbits.LATC3; } while(0)
#define IO_RC3_GetValue()           PORTCbits.RC3
#define IO_RC3_SetDigitalInput()    do { TRISCbits.TRISC3 = 1; } while(0)
#define IO_RC3_SetDigitalOutput()   do { TRISCbits.TRISC3 = 0; } while(0)
#define IO_RC3_SetPullup()          do { WPUCbits.WPUC3 = 1; } while(0)
#define IO_RC3_ResetPullup()        do { WPUCbits.WPUC3 = 0; } while(0)

// get/set IO_RC4 aliases
#define IO_RC4_TRIS                 TRISCbits.TRISC4
#define IO_RC4_LAT                  LATCbits.LATC4
#define IO_RC4_PORT                 PORTCbits.RC4
#define IO_RC4_WPU                  WPUCbits.WPUC4
#define IO_RC4_SetHigh()            do { LATCbits.LATC4 = 1; } while(0)
#define IO_RC4_SetLow()             do { LATCbits.LATC4 = 0; } while(0)
#define IO_RC4_Toggle()             do { LATCbits.LATC4 = ~LATCbits.LATC4; } while(0)
#define IO_RC4_GetValue()           PORTCbits.RC4
#define IO_RC4_SetDigitalInput()    do { TRISCbits.TRISC4 = 1; } while(0)
#define IO_RC4_SetDigitalOutput()   do { TRISCbits.TRISC4 = 0; } while(0)
#define IO_RC4_SetPullup()          do { WPUCbits.WPUC4 = 1; } while(0)
#define IO_RC4_ResetPullup()        do { WPUCbits.WPUC4 = 0; } while(0)

// get/set IO_RC5 aliases
#define IO_RC5_TRIS                 TRISCbits.TRISC5
#define IO_RC5_LAT                  LATCbits.LATC5
#define IO_RC5_PORT                 PORTCbits.RC5
#define IO_RC5_WPU                  WPUCbits.WPUC5
#define IO_RC5_SetHigh()            do { LATCbits.LATC5 = 1; } while(0)
#define IO_RC5_SetLow()             do { LATCbits.LATC5 = 0; } while(0)
#define IO_RC5_Toggle()             do { LATCbits.LATC5 = ~LATCbits.LATC5; } while(0)
#define IO_RC5_GetValue()           PORTCbits.RC5
#define IO_RC5_SetDigitalInput()    do { TRISCbits.TRISC5 = 1; } while(0)
#define IO_RC5_SetDigitalOutput()   do { TRISCbits.TRISC5 = 0; } while(0)
#define IO_RC5_SetPullup()          do { WPUCbits.WPUC5 = 1; } while(0)
#define IO_RC5_ResetPullup()        do { WPUCbits.WPUC5 = 0; } while(0)

// get/set RC6 procedures
#define RC6_SetHigh()            do { LATCbits.LATC6 = 1; } while(0)
#define RC6_SetLow()             do { LATCbits.LATC6 = 0; } while(0)
#define RC6_Toggle()             do { LATCbits.LATC6 = ~LATCbits.LATC6; } while(0)
#define RC6_GetValue()              PORTCbits.RC6
#define RC6_SetDigitalInput()    do { TRISCbits.TRISC6 = 1; } while(0)
#define RC6_SetDigitalOutput()   do { TRISCbits.TRISC6 = 0; } while(0)
#define RC6_SetPullup()             do { WPUCbits.WPUC6 = 1; } while(0)
#define RC6_ResetPullup()           do { WPUCbits.WPUC6 = 0; } while(0)

// get/set RC7 procedures
#define RC7_SetHigh()            do { LATCbits.LATC7 = 1; } while(0)
#define RC7_SetLow()             do { LATCbits.LATC7 = 0; } while(0)
#define RC7_Toggle()             do { LATCbits.LATC7 = ~LATCbits.LATC7; } while(0)
#define RC7_GetValue()              PORTCbits.RC7
#define RC7_SetDigitalInput()    do { TRISCbits.TRISC7 = 1; } while(0)
#define RC7_SetDigitalOutput()   do { TRISCbits.TRISC7 = 0; } while(0)
#define RC7_SetPullup()             do { WPUCbits.WPUC7 = 1; } while(0)
#define RC7_ResetPullup()           do { WPUCbits.WPUC7 = 0; } while(0)

/**
   @Param
    none
   @Returns
    none
   @Description
    GPIO and peripheral I/O initialization
   @Example
    PIN_MANAGER_Initialize();
 */
void PIN_MANAGER_Initialize (void);

/**
 * @Param
    none
 * @Returns
    none
 * @Description
    Interrupt on Change Handling routine
 * @Example
    PIN_MANAGER_IOC();
 */
void PIN_MANAGER_IOC(void);



#endif // PIN_MANAGER_H
/**
 End of File
*/