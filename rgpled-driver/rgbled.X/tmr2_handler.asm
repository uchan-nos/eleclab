#include <xc.inc>
#include "common.h"

; When assembly code is placed in a psect, it can be manipulated as a
; whole by the linker and placed in memory.  
;
; In this example, barfunc is the program section (psect) name, 'local' means
; that the section will not be combined with other sections even if they have
; the same name.  class=CODE means the barfunc must go in the CODE container.
; PIC18's should have a delta (addressible unit size) of 1 (default) since they
; are byte addressible.  PIC10/12/16's have a delta of 2 since they are word
; addressible.  PIC18's should have a reloc (alignment) flag of 2 for any
; psect which contains executable code.  PIC10/12/16's can use the default
; reloc value of 1.  Use one of the psects below for the device you use:

psect   barfunc,local,class=CODE,delta=2 ; PIC10/12/16
; psect   barfunc,local,class=CODE,reloc=2 ; PIC18

global _led_index, _led_bit_index, _led_curdata, _led_status, _led_data_bytes, _led_data
global _TMR2_StopTimer

global _tmr2_handler_asm ; extern of bar function goes in the C source file
_tmr2_handler_asm:
#ifdef SHOW_HANDLER_TIMING
    bsf PORTA, 0
#endif
    ; if (led_status & LED_STATUS_FINISHING_POSN) goto finish_pwm;
    btfsc _led_status, LED_STATUS_FINISHING_POSN
    goto finish_pwm

    ; PWMDCH = (led_curdata & 0x80) ? T1H_DCH : T0H_DCH
    movlw T0H_DCH
    btfsc _led_curdata, 7
    movlw T1H_DCH
    BANKSEL(PWMDCH)
    movwf PWMDCH

    ; led_curdata <<= 1;
    BANKSEL(_led_curdata)
    lslf _led_curdata, f

    ; led_bit_index >>= 1;
    lsrf _led_bit_index, f ; 結果がゼロなら Z=1

    ; if (led_bit_index != 0) goto tmr2_handler_end;
    btfss STATUS, STATUS_Z_POSN
    goto tmr2_handler_end

    ; _led_bit_index = 0x80
    bsf _led_bit_index, 7

    ; led_index++;
    incf _led_index, f

    ; if (led_index >= led_data_bytes) led_finishing = 1;
    movf _led_data_bytes, w
    subwf _led_index, w   ; W = led_index - W
    btfsc STATUS, STATUS_C_POSN
    bsf _led_status, LED_STATUS_FINISHING_POSN

    ; led_curdata = led_data[led_index];
    movf _led_index, w
    addlw _led_data
    movwf FSR1L
    clrf FSR1H
    movf INDF1, w
    movwf _led_curdata
    goto tmr2_handler_end

finish_pwm:
    BANKSEL(PWMDCH)
    clrf PWMDCL
    clrf PWMDCH
    call _TMR2_StopTimer
    bcf _led_status, LED_STATUS_SENDING_POSN

tmr2_handler_end:
#ifdef SHOW_HANDLER_TIMING
    bcf PORTA, 0
#endif
    return

;32:            void Tmr2TimeoutHandler() {
;33:            #ifdef SHOW_HANDLER_TIMING
;34:              IO_RA0_PORT = 1;
;0188  140C     BSF PORTA, 0x0
;35:            #endif
;36:              if (led_index < 3 * NUM_LED) {
;0189  3015     MOVLW 0x15
;018A  0244     SUBWF led_index, W
;018B  1803     BTFSC STATUS, 0x0
;018C  29B1     GOTO 0x1B1
;37:                PWMDCH = (led_curdata & 0x80u) ? T1H_DCH : T0H_DCH;
;018D  1BD7     BTFSC led_curdata, 0x7
;018E  2991     GOTO 0x191
;018F  3002     MOVLW 0x2
;0190  2992     GOTO 0x192
;0191  3005     MOVLW 0x5
;0192  00F1     MOVWF 0x71
;0193  3000     MOVLW 0x0
;0194  00F2     MOVWF 0x72
;0195  0871     MOVF 0x71, W
;0196  002C     MOVLB 0xC
;0197  0098     MOVWF PWM5DCH
;38:                led_curdata <<= 1;
;0198  1003     BCF STATUS, 0x0
;0199  0020     MOVLB 0x0
;019A  0DD7     RLF led_curdata, F
;39:                led_bit_index = (led_bit_index + 1) & 7;
;019B  0861     MOVF led_bit_index, W
;019C  3E01     ADDLW 0x1
;019D  3907     ANDLW 0x7
;019E  00F0     MOVWF __pcstackCOMMON
;019F  0870     MOVF __pcstackCOMMON, W
;01A0  00E1     MOVWF led_bit_index
;40:                if (led_bit_index == 0) {
;01A1  0861     MOVF led_bit_index, W
;01A2  1D03     BTFSS STATUS, 0x2
;01A3  29B5     GOTO 0x1B5
;41:                  led_index++;
;01A4  3001     MOVLW 0x1
;01A5  00F0     MOVWF __pcstackCOMMON
;01A6  0870     MOVF __pcstackCOMMON, W
;01A7  07C4     ADDWF led_index, F
;42:                  led_curdata = led_data[led_index];
;01A8  0844     MOVF led_index, W
;01A9  3EA0     ADDLW 0xA0
;01AA  0086     MOVWF FSR1
;01AB  0187     CLRF FSR1H
;01AC  0801     MOVF INDF1, W
;01AD  00F0     MOVWF __pcstackCOMMON
;01AE  0870     MOVF __pcstackCOMMON, W
;01AF  00D7     MOVWF led_curdata
;01B0  29B5     GOTO 0x1B5
;43:                }
;44:              } else {
;45:                PWMDCL = 0;
;01B1  002C     MOVLB 0xC
;01B2  0197     CLRF PWM5DCL
;46:                PWMDCH = 0;
;01B3  0198     CLRF PWM5DCH
;47:                TMR2_StopTimer();
;01B4  2305     CALL 0x305
;48:              }
;49:            #ifdef SHOW_HANDLER_TIMING
;50:              IO_RA0_PORT = 0;
;01B5  100C     BCF 0x60C, 0x0
;51:            #endif
;52:            }
;01B6  0008     RETURN