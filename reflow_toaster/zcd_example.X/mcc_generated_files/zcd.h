/**
  ZCD Generated Driver API Header File

  @Company
    Microchip Technology Inc.

  @File Name
    zcd.h

  @Summary
    This is the generated header file for the ZCD driver using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  @Description
    This header file provides APIs for driver for ZCD.
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

#ifndef ZCD_H
#define ZCD_H

/**
  Section: Included Files
*/

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus  // Provide C++ Compatibility

    extern "C" {

#endif

/**
  Section: ZCD Module APIs
*/

/**
  @Summary
    Initializes the ZCD_Initialize.

  @Description
    This routine initializes the ZCD_Initialize.
    This routine must be called before any other ZCD routine is called.
    This routine should only be called once during system initialization.

  @Preconditions
    None

  @Param
    None

  @Returns
    None

  @Comment
    

 @Example
    <code>
    ZCD_Initialize();
    </code>
 */
void ZCD_Initialize(void);

/**
  @Summary
    Determines if current is sinking or sourcing

  @Description
    This routine is used to determine if current is sinking or sourcing
    depending on output polarity.

    For non inverted polarity:
    high - Indicates current is sinking
    low - Indicates current is sourcing

    For inverted polarity:
    high - Indicates current is sourcing
    low - Indicates current is sinking

  @Preconditions
    ZCD_Initialize() function should have been called before calling this function.

  @Param
    None

  @Returns
   high or low

  @Example
    <code>
    ZCD_Initialize();

    if(ZCD_IsLogicLevel())
    {
        // User code..
    }
    else
    {
        // User code..
    }
    </code>
 */
bool ZCD_IsLogicLevel(void);

/**
  @Summary
    Implements ISR.

  @Description
    This routine is used to implement the ISR for the interrupt-driven
    implementations.

  @Returns
    None

  @Param
    None
*/
void ZCD_ISR(void);

#ifdef __cplusplus  // Provide C++ Compatibility

    }

#endif

#endif  //ZCD_H
/**
 End of File
*/

