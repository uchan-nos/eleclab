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
        Device            :  PIC16F18857
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

// get/set IO_PHASE aliases
#define IO_PHASE_TRIS                 TRISBbits.TRISB0
#define IO_PHASE_LAT                  LATBbits.LATB0
#define IO_PHASE_PORT                 PORTBbits.RB0
#define IO_PHASE_WPU                  WPUBbits.WPUB0
#define IO_PHASE_OD                   ODCONBbits.ODCB0
#define IO_PHASE_ANS                  ANSELBbits.ANSB0
#define IO_PHASE_SetHigh()            do { LATBbits.LATB0 = 1; } while(0)
#define IO_PHASE_SetLow()             do { LATBbits.LATB0 = 0; } while(0)
#define IO_PHASE_Toggle()             do { LATBbits.LATB0 = ~LATBbits.LATB0; } while(0)
#define IO_PHASE_GetValue()           PORTBbits.RB0
#define IO_PHASE_SetDigitalInput()    do { TRISBbits.TRISB0 = 1; } while(0)
#define IO_PHASE_SetDigitalOutput()   do { TRISBbits.TRISB0 = 0; } while(0)
#define IO_PHASE_SetPullup()          do { WPUBbits.WPUB0 = 1; } while(0)
#define IO_PHASE_ResetPullup()        do { WPUBbits.WPUB0 = 0; } while(0)
#define IO_PHASE_SetPushPull()        do { ODCONBbits.ODCB0 = 0; } while(0)
#define IO_PHASE_SetOpenDrain()       do { ODCONBbits.ODCB0 = 1; } while(0)
#define IO_PHASE_SetAnalogMode()      do { ANSELBbits.ANSB0 = 1; } while(0)
#define IO_PHASE_SetDigitalMode()     do { ANSELBbits.ANSB0 = 0; } while(0)

// get/set IO_LOAD0 aliases
#define IO_LOAD0_TRIS                 TRISCbits.TRISC1
#define IO_LOAD0_LAT                  LATCbits.LATC1
#define IO_LOAD0_PORT                 PORTCbits.RC1
#define IO_LOAD0_WPU                  WPUCbits.WPUC1
#define IO_LOAD0_OD                   ODCONCbits.ODCC1
#define IO_LOAD0_ANS                  ANSELCbits.ANSC1
#define IO_LOAD0_SetHigh()            do { LATCbits.LATC1 = 1; } while(0)
#define IO_LOAD0_SetLow()             do { LATCbits.LATC1 = 0; } while(0)
#define IO_LOAD0_Toggle()             do { LATCbits.LATC1 = ~LATCbits.LATC1; } while(0)
#define IO_LOAD0_GetValue()           PORTCbits.RC1
#define IO_LOAD0_SetDigitalInput()    do { TRISCbits.TRISC1 = 1; } while(0)
#define IO_LOAD0_SetDigitalOutput()   do { TRISCbits.TRISC1 = 0; } while(0)
#define IO_LOAD0_SetPullup()          do { WPUCbits.WPUC1 = 1; } while(0)
#define IO_LOAD0_ResetPullup()        do { WPUCbits.WPUC1 = 0; } while(0)
#define IO_LOAD0_SetPushPull()        do { ODCONCbits.ODCC1 = 0; } while(0)
#define IO_LOAD0_SetOpenDrain()       do { ODCONCbits.ODCC1 = 1; } while(0)
#define IO_LOAD0_SetAnalogMode()      do { ANSELCbits.ANSC1 = 1; } while(0)
#define IO_LOAD0_SetDigitalMode()     do { ANSELCbits.ANSC1 = 0; } while(0)

// get/set RC6 procedures
#define RC6_SetHigh()            do { LATCbits.LATC6 = 1; } while(0)
#define RC6_SetLow()             do { LATCbits.LATC6 = 0; } while(0)
#define RC6_Toggle()             do { LATCbits.LATC6 = ~LATCbits.LATC6; } while(0)
#define RC6_GetValue()              PORTCbits.RC6
#define RC6_SetDigitalInput()    do { TRISCbits.TRISC6 = 1; } while(0)
#define RC6_SetDigitalOutput()   do { TRISCbits.TRISC6 = 0; } while(0)
#define RC6_SetPullup()             do { WPUCbits.WPUC6 = 1; } while(0)
#define RC6_ResetPullup()           do { WPUCbits.WPUC6 = 0; } while(0)
#define RC6_SetAnalogMode()         do { ANSELCbits.ANSC6 = 1; } while(0)
#define RC6_SetDigitalMode()        do { ANSELCbits.ANSC6 = 0; } while(0)

// get/set RC7 procedures
#define RC7_SetHigh()            do { LATCbits.LATC7 = 1; } while(0)
#define RC7_SetLow()             do { LATCbits.LATC7 = 0; } while(0)
#define RC7_Toggle()             do { LATCbits.LATC7 = ~LATCbits.LATC7; } while(0)
#define RC7_GetValue()              PORTCbits.RC7
#define RC7_SetDigitalInput()    do { TRISCbits.TRISC7 = 1; } while(0)
#define RC7_SetDigitalOutput()   do { TRISCbits.TRISC7 = 0; } while(0)
#define RC7_SetPullup()             do { WPUCbits.WPUC7 = 1; } while(0)
#define RC7_ResetPullup()           do { WPUCbits.WPUC7 = 0; } while(0)
#define RC7_SetAnalogMode()         do { ANSELCbits.ANSC7 = 1; } while(0)
#define RC7_SetDigitalMode()        do { ANSELCbits.ANSC7 = 0; } while(0)

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


/**
 * @Param
    none
 * @Returns
    none
 * @Description
    Interrupt on Change Handler for the IOCBF0 pin functionality
 * @Example
    IOCBF0_ISR();
 */
void IOCBF0_ISR(void);

/**
  @Summary
    Interrupt Handler Setter for IOCBF0 pin interrupt-on-change functionality

  @Description
    Allows selecting an interrupt handler for IOCBF0 at application runtime
    
  @Preconditions
    Pin Manager intializer called

  @Returns
    None.

  @Param
    InterruptHandler function pointer.

  @Example
    PIN_MANAGER_Initialize();
    IOCBF0_SetInterruptHandler(MyInterruptHandler);

*/
void IOCBF0_SetInterruptHandler(void (* InterruptHandler)(void));

/**
  @Summary
    Dynamic Interrupt Handler for IOCBF0 pin

  @Description
    This is a dynamic interrupt handler to be used together with the IOCBF0_SetInterruptHandler() method.
    This handler is called every time the IOCBF0 ISR is executed and allows any function to be registered at runtime.
    
  @Preconditions
    Pin Manager intializer called

  @Returns
    None.

  @Param
    None.

  @Example
    PIN_MANAGER_Initialize();
    IOCBF0_SetInterruptHandler(IOCBF0_InterruptHandler);

*/
extern void (*IOCBF0_InterruptHandler)(void);

/**
  @Summary
    Default Interrupt Handler for IOCBF0 pin

  @Description
    This is a predefined interrupt handler to be used together with the IOCBF0_SetInterruptHandler() method.
    This handler is called every time the IOCBF0 ISR is executed. 
    
  @Preconditions
    Pin Manager intializer called

  @Returns
    None.

  @Param
    None.

  @Example
    PIN_MANAGER_Initialize();
    IOCBF0_SetInterruptHandler(IOCBF0_DefaultInterruptHandler);

*/
void IOCBF0_DefaultInterruptHandler(void);



#endif // PIN_MANAGER_H
/**
 End of File
*/