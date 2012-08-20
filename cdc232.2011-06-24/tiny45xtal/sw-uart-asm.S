
/* Name: sw-uart.S
 * Project: AVR USB driver for CDC interface on USB1.1
 * Author: Osamu Tamura
 * Creation Date: 2006-05-12
 * Tabsize: 4
 * Copyright: (c) 2006 by Recursion Co., Ltd.
 * License: Proprietary, free under certain conditions. See Documentation.
 */

/*
General Description:
    This module implements the assembler part of the USB-CDC driver.

Note: This module violates the rule that interrupts must not be disabled for
longer than a couple of instructions (see usbdrv.h). Running UART interrupt
handlers with sei as the first instruction is not possible because it would
recurse immediately (the cause of the interrupt has not been removed). If
we collect the data and then call sei(), we win little. We therefore decide
to violate the rule. The effect on USB operation is, that packages may be
lost. This is equivalent to a package being dropped due to a CRC error. The
host will therefore retry the transfer after a timeout. It is therefore very
likely that no effect is seen at the application layer.
*/

#define __SFR_OFFSET 0      /* used by avr-libc's register definitions */
#include "usbportability.h"
#include "usbdrv.h" /* for common defs */

#include "uart.h"


/* register names */
#define x1      r16
#define x2      r17

/* Some assembler dependent definitions and declarations: */

#ifdef __IAR_SYSTEMS_ASM__

    public  bit_reverse

    COMMON  INTVEC
#   if USB_CFG_DPLUS_BIT==2
        ORG     PCINT0_vect
#   else  /* USB_INTR_VECTOR */
        ORG     INT0_vect
#   endif /* USB_INTR_VECTOR */
    rjmp    UART_INTR_VECTOR

    rjmp    UART_INTR_VECTOR
    ORG     TIM1_COMPA_vect
    rjmp    SIG_OUTPUT_COMPARE1A
    ORG     USI_OVF_vect
    rjmp    SIG_USI_OVERFLOW
    RSEG    CODE

#else /* __IAR_SYSTEMS_ASM__ */

    .text

    .global UART_INTR_VECTOR
    .type   UART_INTR_VECTOR, @function
    .global SIG_OUTPUT_COMPARE1A
    .type   SIG_OUTPUT_COMPARE1A, @function
    .global SIG_USI_OVERFLOW
    .type   SIG_USI_OVERFLOW, @function

    .global bit_reverse

#endif /* __IAR_SYSTEMS_ASM__ */


; ######################## RS-232C functions ########################

UART_INTR_VECTOR:
#ifdef UART_INVERT
    sbis    UART_PIN, UART_CFG_RXD         ;1/2
#else
    sbic    UART_PIN, UART_CFG_RXD         ;1/2
#endif
    reti                    ;4 

    out     GPIOR0, x1      ;1

    in      x1, RX_DELAY    ;1  1st bit sampling delay
    out     TCNT1, x1       ;1
    ldi     x1, 9           ;1  set rx_bitcounter = 9
    out     OCR1C, x1       ;1
    ldi     x1, 7           ;1  start timer
    out     TCCR1, x1       ;1

    ldi     x1, (1<<USB_INTR_ENABLE_BIT)  ;1
    out     USB_INTR_ENABLE, x1  ;1   stop rx_pin interrupt

    in      x1, GPIOR0      ;1
    reti                    ;4   {14,14}


SIG_OUTPUT_COMPARE1A:
    out     GPIOR0, x1      ;1
    in      x1, SREG        ;1
    out     GPIOR1, x1      ;1

    in      x1, OCR1C       ;1  rx_bitcounter--
    dec     x1              ;1
    out     OCR1C, x1       ;1
    breq    tm1_stopbit     ;1/2

    in      x1, OCR1B       ;1
    lsr     x1              ;1  data shift
#ifdef UART_INVERT
    sbis    UART_PIN, UART_CFG_RXD         ;1/2
#else
    sbic    UART_PIN, UART_CFG_RXD         ;1/2
#endif
    ori     x1, 0x80        ;1
    out     OCR1B, x1       ;1

    out     GPIOR2, x2      ;1
    in      x1, TCNT1       ;1
    in      x2, OCR1A       ;1
    sub     x1, x2          ;1
    out     TCNT1, x1       ;1

    in      x2, GPIOR2      ;1
    in      x1, GPIOR1      ;1
    out     SREG, x1        ;1
    in      x1, GPIOR0      ;1
    reti                    ;4

tm1_stopbit:
    out     TCCR1, x1       ;1  stop timer1

    out     RX_READY, x1    ;1  rx_data ready flag
    in      x1, OCR1B       ;1
    out     EEDR, x1        ;1  rx_data

    ldi     x1, (1<<UART_INTR_PENDING_BIT)   ;1
    out     UART_INTR_PENDING, x1        ;1
    ldi     x1, (1<<USB_INTR_ENABLE_BIT)|(1<<UART_INTR_ENABLE_BIT)  ;1
    out     USB_INTR_ENABLE, x1  ;1   enable rx_pin interrupt

    in      x1, GPIOR1      ;1
    out     SREG, x1        ;1
    in      x1, GPIOR0      ;1
    reti                    ;4   {23,25}


SIG_USI_OVERFLOW:
    out     GPIOR0, x1     	;1

	sbic	EEARL, 0		;1/2
	rjmp	usi_stopbit    	;2

	ldi		x1, 0x4b		;1
	out		USISR, x1		;1

	out		EEARL, x1		;1

	in		x1, OCR0B		;1
	out		USIDR, x1		;1  transfer upper 4 bits

    in      x1, GPIOR0     	;1
    reti                   	;4

usi_stopbit:
	ldi		x1, (1<<USIOIF)	;1
	out		USISR, x1		;1

    ldi     x1, 0           ;1
	out		TCCR0B, x1		;1  stop timer0

    in      x1, GPIOR0		;1
    reti					;4   {13,13}


;   extern uchar    bit_reverse( uchar x );

#ifdef __IAR_SYSTEMS_ASM__
#define inb      r16
#define outb     r17
#else
#define inb      r24
#define outb     r25
#endif

bit_reverse:
    rol     inb
    ror     outb
    rol     inb
    ror     outb
    rol     inb
    ror     outb
    rol     inb
    ror     outb
    rol     inb
    ror     outb
    rol     inb
    ror     outb
    rol     inb
    ror     outb
    rol     inb
    ror     outb
    mov     inb, outb
    ret


