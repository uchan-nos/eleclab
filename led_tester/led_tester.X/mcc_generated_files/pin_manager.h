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
        Device            :  PIC16F18323
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

// get/set AN_LEDVF aliases
#define AN_LEDVF_TRIS                 TRISAbits.TRISA2
#define AN_LEDVF_LAT                  LATAbits.LATA2
#define AN_LEDVF_PORT                 PORTAbits.RA2
#define AN_LEDVF_WPU                  WPUAbits.WPUA2
#define AN_LEDVF_OD                   ODCONAbits.ODCA2
#define AN_LEDVF_ANS                  ANSELAbits.ANSA2
#define AN_LEDVF_SetHigh()            do { LATAbits.LATA2 = 1; } while(0)
#define AN_LEDVF_SetLow()             do { LATAbits.LATA2 = 0; } while(0)
#define AN_LEDVF_Toggle()             do { LATAbits.LATA2 = ~LATAbits.LATA2; } while(0)
#define AN_LEDVF_GetValue()           PORTAbits.RA2
#define AN_LEDVF_SetDigitalInput()    do { TRISAbits.TRISA2 = 1; } while(0)
#define AN_LEDVF_SetDigitalOutput()   do { TRISAbits.TRISA2 = 0; } while(0)
#define AN_LEDVF_SetPullup()          do { WPUAbits.WPUA2 = 1; } while(0)
#define AN_LEDVF_ResetPullup()        do { WPUAbits.WPUA2 = 0; } while(0)
#define AN_LEDVF_SetPushPull()        do { ODCONAbits.ODCA2 = 0; } while(0)
#define AN_LEDVF_SetOpenDrain()       do { ODCONAbits.ODCA2 = 1; } while(0)
#define AN_LEDVF_SetAnalogMode()      do { ANSELAbits.ANSA2 = 1; } while(0)
#define AN_LEDVF_SetDigitalMode()     do { ANSELAbits.ANSA2 = 0; } while(0)

// get/set AN_SENSE aliases
#define AN_SENSE_TRIS                 TRISAbits.TRISA4
#define AN_SENSE_LAT                  LATAbits.LATA4
#define AN_SENSE_PORT                 PORTAbits.RA4
#define AN_SENSE_WPU                  WPUAbits.WPUA4
#define AN_SENSE_OD                   ODCONAbits.ODCA4
#define AN_SENSE_ANS                  ANSELAbits.ANSA4
#define AN_SENSE_SetHigh()            do { LATAbits.LATA4 = 1; } while(0)
#define AN_SENSE_SetLow()             do { LATAbits.LATA4 = 0; } while(0)
#define AN_SENSE_Toggle()             do { LATAbits.LATA4 = ~LATAbits.LATA4; } while(0)
#define AN_SENSE_GetValue()           PORTAbits.RA4
#define AN_SENSE_SetDigitalInput()    do { TRISAbits.TRISA4 = 1; } while(0)
#define AN_SENSE_SetDigitalOutput()   do { TRISAbits.TRISA4 = 0; } while(0)
#define AN_SENSE_SetPullup()          do { WPUAbits.WPUA4 = 1; } while(0)
#define AN_SENSE_ResetPullup()        do { WPUAbits.WPUA4 = 0; } while(0)
#define AN_SENSE_SetPushPull()        do { ODCONAbits.ODCA4 = 0; } while(0)
#define AN_SENSE_SetOpenDrain()       do { ODCONAbits.ODCA4 = 1; } while(0)
#define AN_SENSE_SetAnalogMode()      do { ANSELAbits.ANSA4 = 1; } while(0)
#define AN_SENSE_SetDigitalMode()     do { ANSELAbits.ANSA4 = 0; } while(0)

// get/set IO_OPAMP_EN aliases
#define IO_OPAMP_EN_TRIS                 TRISAbits.TRISA5
#define IO_OPAMP_EN_LAT                  LATAbits.LATA5
#define IO_OPAMP_EN_PORT                 PORTAbits.RA5
#define IO_OPAMP_EN_WPU                  WPUAbits.WPUA5
#define IO_OPAMP_EN_OD                   ODCONAbits.ODCA5
#define IO_OPAMP_EN_ANS                  ANSELAbits.ANSA5
#define IO_OPAMP_EN_SetHigh()            do { LATAbits.LATA5 = 1; } while(0)
#define IO_OPAMP_EN_SetLow()             do { LATAbits.LATA5 = 0; } while(0)
#define IO_OPAMP_EN_Toggle()             do { LATAbits.LATA5 = ~LATAbits.LATA5; } while(0)
#define IO_OPAMP_EN_GetValue()           PORTAbits.RA5
#define IO_OPAMP_EN_SetDigitalInput()    do { TRISAbits.TRISA5 = 1; } while(0)
#define IO_OPAMP_EN_SetDigitalOutput()   do { TRISAbits.TRISA5 = 0; } while(0)
#define IO_OPAMP_EN_SetPullup()          do { WPUAbits.WPUA5 = 1; } while(0)
#define IO_OPAMP_EN_ResetPullup()        do { WPUAbits.WPUA5 = 0; } while(0)
#define IO_OPAMP_EN_SetPushPull()        do { ODCONAbits.ODCA5 = 0; } while(0)
#define IO_OPAMP_EN_SetOpenDrain()       do { ODCONAbits.ODCA5 = 1; } while(0)
#define IO_OPAMP_EN_SetAnalogMode()      do { ANSELAbits.ANSA5 = 1; } while(0)
#define IO_OPAMP_EN_SetDigitalMode()     do { ANSELAbits.ANSA5 = 0; } while(0)

// get/set IO_RC0 aliases
#define IO_RC0_TRIS                 TRISCbits.TRISC0
#define IO_RC0_LAT                  LATCbits.LATC0
#define IO_RC0_PORT                 PORTCbits.RC0
#define IO_RC0_WPU                  WPUCbits.WPUC0
#define IO_RC0_OD                   ODCONCbits.ODCC0
#define IO_RC0_ANS                  ANSELCbits.ANSC0
#define IO_RC0_SetHigh()            do { LATCbits.LATC0 = 1; } while(0)
#define IO_RC0_SetLow()             do { LATCbits.LATC0 = 0; } while(0)
#define IO_RC0_Toggle()             do { LATCbits.LATC0 = ~LATCbits.LATC0; } while(0)
#define IO_RC0_GetValue()           PORTCbits.RC0
#define IO_RC0_SetDigitalInput()    do { TRISCbits.TRISC0 = 1; } while(0)
#define IO_RC0_SetDigitalOutput()   do { TRISCbits.TRISC0 = 0; } while(0)
#define IO_RC0_SetPullup()          do { WPUCbits.WPUC0 = 1; } while(0)
#define IO_RC0_ResetPullup()        do { WPUCbits.WPUC0 = 0; } while(0)
#define IO_RC0_SetPushPull()        do { ODCONCbits.ODCC0 = 0; } while(0)
#define IO_RC0_SetOpenDrain()       do { ODCONCbits.ODCC0 = 1; } while(0)
#define IO_RC0_SetAnalogMode()      do { ANSELCbits.ANSC0 = 1; } while(0)
#define IO_RC0_SetDigitalMode()     do { ANSELCbits.ANSC0 = 0; } while(0)

// get/set IO_RC1 aliases
#define IO_RC1_TRIS                 TRISCbits.TRISC1
#define IO_RC1_LAT                  LATCbits.LATC1
#define IO_RC1_PORT                 PORTCbits.RC1
#define IO_RC1_WPU                  WPUCbits.WPUC1
#define IO_RC1_OD                   ODCONCbits.ODCC1
#define IO_RC1_ANS                  ANSELCbits.ANSC1
#define IO_RC1_SetHigh()            do { LATCbits.LATC1 = 1; } while(0)
#define IO_RC1_SetLow()             do { LATCbits.LATC1 = 0; } while(0)
#define IO_RC1_Toggle()             do { LATCbits.LATC1 = ~LATCbits.LATC1; } while(0)
#define IO_RC1_GetValue()           PORTCbits.RC1
#define IO_RC1_SetDigitalInput()    do { TRISCbits.TRISC1 = 1; } while(0)
#define IO_RC1_SetDigitalOutput()   do { TRISCbits.TRISC1 = 0; } while(0)
#define IO_RC1_SetPullup()          do { WPUCbits.WPUC1 = 1; } while(0)
#define IO_RC1_ResetPullup()        do { WPUCbits.WPUC1 = 0; } while(0)
#define IO_RC1_SetPushPull()        do { ODCONCbits.ODCC1 = 0; } while(0)
#define IO_RC1_SetOpenDrain()       do { ODCONCbits.ODCC1 = 1; } while(0)
#define IO_RC1_SetAnalogMode()      do { ANSELCbits.ANSC1 = 1; } while(0)
#define IO_RC1_SetDigitalMode()     do { ANSELCbits.ANSC1 = 0; } while(0)

// get/set IO_RC2 aliases
#define IO_RC2_TRIS                 TRISCbits.TRISC2
#define IO_RC2_LAT                  LATCbits.LATC2
#define IO_RC2_PORT                 PORTCbits.RC2
#define IO_RC2_WPU                  WPUCbits.WPUC2
#define IO_RC2_OD                   ODCONCbits.ODCC2
#define IO_RC2_ANS                  ANSELCbits.ANSC2
#define IO_RC2_SetHigh()            do { LATCbits.LATC2 = 1; } while(0)
#define IO_RC2_SetLow()             do { LATCbits.LATC2 = 0; } while(0)
#define IO_RC2_Toggle()             do { LATCbits.LATC2 = ~LATCbits.LATC2; } while(0)
#define IO_RC2_GetValue()           PORTCbits.RC2
#define IO_RC2_SetDigitalInput()    do { TRISCbits.TRISC2 = 1; } while(0)
#define IO_RC2_SetDigitalOutput()   do { TRISCbits.TRISC2 = 0; } while(0)
#define IO_RC2_SetPullup()          do { WPUCbits.WPUC2 = 1; } while(0)
#define IO_RC2_ResetPullup()        do { WPUCbits.WPUC2 = 0; } while(0)
#define IO_RC2_SetPushPull()        do { ODCONCbits.ODCC2 = 0; } while(0)
#define IO_RC2_SetOpenDrain()       do { ODCONCbits.ODCC2 = 1; } while(0)
#define IO_RC2_SetAnalogMode()      do { ANSELCbits.ANSC2 = 1; } while(0)
#define IO_RC2_SetDigitalMode()     do { ANSELCbits.ANSC2 = 0; } while(0)

// get/set IO_RC3 aliases
#define IO_RC3_TRIS                 TRISCbits.TRISC3
#define IO_RC3_LAT                  LATCbits.LATC3
#define IO_RC3_PORT                 PORTCbits.RC3
#define IO_RC3_WPU                  WPUCbits.WPUC3
#define IO_RC3_OD                   ODCONCbits.ODCC3
#define IO_RC3_ANS                  ANSELCbits.ANSC3
#define IO_RC3_SetHigh()            do { LATCbits.LATC3 = 1; } while(0)
#define IO_RC3_SetLow()             do { LATCbits.LATC3 = 0; } while(0)
#define IO_RC3_Toggle()             do { LATCbits.LATC3 = ~LATCbits.LATC3; } while(0)
#define IO_RC3_GetValue()           PORTCbits.RC3
#define IO_RC3_SetDigitalInput()    do { TRISCbits.TRISC3 = 1; } while(0)
#define IO_RC3_SetDigitalOutput()   do { TRISCbits.TRISC3 = 0; } while(0)
#define IO_RC3_SetPullup()          do { WPUCbits.WPUC3 = 1; } while(0)
#define IO_RC3_ResetPullup()        do { WPUCbits.WPUC3 = 0; } while(0)
#define IO_RC3_SetPushPull()        do { ODCONCbits.ODCC3 = 0; } while(0)
#define IO_RC3_SetOpenDrain()       do { ODCONCbits.ODCC3 = 1; } while(0)
#define IO_RC3_SetAnalogMode()      do { ANSELCbits.ANSC3 = 1; } while(0)
#define IO_RC3_SetDigitalMode()     do { ANSELCbits.ANSC3 = 0; } while(0)

// get/set IO_RC4 aliases
#define IO_RC4_TRIS                 TRISCbits.TRISC4
#define IO_RC4_LAT                  LATCbits.LATC4
#define IO_RC4_PORT                 PORTCbits.RC4
#define IO_RC4_WPU                  WPUCbits.WPUC4
#define IO_RC4_OD                   ODCONCbits.ODCC4
#define IO_RC4_ANS                  ANSELCbits.ANSC4
#define IO_RC4_SetHigh()            do { LATCbits.LATC4 = 1; } while(0)
#define IO_RC4_SetLow()             do { LATCbits.LATC4 = 0; } while(0)
#define IO_RC4_Toggle()             do { LATCbits.LATC4 = ~LATCbits.LATC4; } while(0)
#define IO_RC4_GetValue()           PORTCbits.RC4
#define IO_RC4_SetDigitalInput()    do { TRISCbits.TRISC4 = 1; } while(0)
#define IO_RC4_SetDigitalOutput()   do { TRISCbits.TRISC4 = 0; } while(0)
#define IO_RC4_SetPullup()          do { WPUCbits.WPUC4 = 1; } while(0)
#define IO_RC4_ResetPullup()        do { WPUCbits.WPUC4 = 0; } while(0)
#define IO_RC4_SetPushPull()        do { ODCONCbits.ODCC4 = 0; } while(0)
#define IO_RC4_SetOpenDrain()       do { ODCONCbits.ODCC4 = 1; } while(0)
#define IO_RC4_SetAnalogMode()      do { ANSELCbits.ANSC4 = 1; } while(0)
#define IO_RC4_SetDigitalMode()     do { ANSELCbits.ANSC4 = 0; } while(0)

// get/set IO_RC5 aliases
#define IO_RC5_TRIS                 TRISCbits.TRISC5
#define IO_RC5_LAT                  LATCbits.LATC5
#define IO_RC5_PORT                 PORTCbits.RC5
#define IO_RC5_WPU                  WPUCbits.WPUC5
#define IO_RC5_OD                   ODCONCbits.ODCC5
#define IO_RC5_ANS                  ANSELCbits.ANSC5
#define IO_RC5_SetHigh()            do { LATCbits.LATC5 = 1; } while(0)
#define IO_RC5_SetLow()             do { LATCbits.LATC5 = 0; } while(0)
#define IO_RC5_Toggle()             do { LATCbits.LATC5 = ~LATCbits.LATC5; } while(0)
#define IO_RC5_GetValue()           PORTCbits.RC5
#define IO_RC5_SetDigitalInput()    do { TRISCbits.TRISC5 = 1; } while(0)
#define IO_RC5_SetDigitalOutput()   do { TRISCbits.TRISC5 = 0; } while(0)
#define IO_RC5_SetPullup()          do { WPUCbits.WPUC5 = 1; } while(0)
#define IO_RC5_ResetPullup()        do { WPUCbits.WPUC5 = 0; } while(0)
#define IO_RC5_SetPushPull()        do { ODCONCbits.ODCC5 = 0; } while(0)
#define IO_RC5_SetOpenDrain()       do { ODCONCbits.ODCC5 = 1; } while(0)
#define IO_RC5_SetAnalogMode()      do { ANSELCbits.ANSC5 = 1; } while(0)
#define IO_RC5_SetDigitalMode()     do { ANSELCbits.ANSC5 = 0; } while(0)

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