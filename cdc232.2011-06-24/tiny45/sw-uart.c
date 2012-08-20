/* Name: sw-uart.c
 * Project: AVR USB driver for CDC interface on Low-Speed USB
 * Author: Osamu Tamura
 * Creation Date: 2006-06-22
 * Tabsize: 4
 * Copyright: (c) 2006 by Recursion Co., Ltd.
 * License: Proprietary, free under certain conditions. See Documentation.
 *
 *  2006-07-10 software-UART interrupt handling time reduced.
 */

/*
    General Description:
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>   /* needed by usbdrv.h */
#include "oddebug.h"
#include "usbdrv.h"
#include "uart.h"


/* UART buffer */
uchar    urptr, uwptr, irptr, iwptr;
uchar    rx_buf[RX_SIZE], tx_buf[TX_SIZE];


void uartInit(uint baudrate)
{

    PRR     = (1<<PRADC);
    ACSR    = (1<<ACD);


    UART_DDR    |= (1<<UART_CFG_TXD);
    UART_DDR    &= ~(1<<UART_CFG_RXD);

    UART_INTR_ENABLE    &= ~_BV(UART_INTR_ENABLE_BIT);

    TCCR0A   = 0;
    TCCR0B   = 0;
    TCCR1    = 0;

	USISR   = 0xe0;		          /* clear all interrupt flags */
	USICR   = (1<<USIOIE)|(1<<USIWM0)|(1<<USICS0);	/* 3 wire mode       */

#ifdef UART_INVERT
	USIDR   = 0;
#else
	USIDR   = 0xff;
#endif

    OCR0A    =
    OCR1A    = ((F_CPU>>6)+(baudrate>>1)) / baudrate - 1;
    OCR1C    = 0;
    RX_DELAY = -((OCR1A+1)>>2);   /* 1.25 sample bit */
	if( RX_DELAY<=OCR1A )
		RX_DELAY	= OCR1A + 1;  /* for 1200 bps	 */


    TCCR0A   = 2;                 /* CTC */
    RX_READY = 1;
    TIMSK    = (1<<OCIE1A);

#if UART_CFG_RXD==2
	UART_INTR_CFG       = (UART_INTR_CFG&0xfe)|UART_INTR_CFG_SET;
#else
	UART_INTR_CFG       = UART_INTR_CFG_SET;
#endif
    UART_INTR_PENDING   = _BV(UART_INTR_PENDING_BIT);
    UART_INTR_ENABLE    |= _BV(UART_INTR_ENABLE_BIT);


    irptr    = 0;
    iwptr    = 0;
    urptr    = 0;
    uwptr    = 0;
}

void uartPoll(void)
{

    /*  device <= rs232c : receive  */
    if( RX_READY==0 && iwptr<HW_CDC_BULK_IN_SIZE ) {
	    rx_buf[iwptr++] = EEDR;
        RX_READY     = 1;
    }

    /*  host <= device : transmit   */
    if( usbInterruptIsReady() && (iwptr||sendEmptyFrame) ) {
        usbSetInterrupt(rx_buf, iwptr);
        sendEmptyFrame    = iwptr & HW_CDC_BULK_IN_SIZE;
        iwptr    = 0;
    }

    /*  device => rs232c : transmit */
    if( TCCR0B==0 && uwptr!=irptr ) {
        uchar       data;

        data    = bit_reverse( tx_buf[irptr] );
        irptr   = (irptr+1) & TX_MASK;

        TCNT0   = 0;
		USISR   = 0x4b;						/* interrupt at D4  */
        EEARL   = 0;  	    				/* usi_phase  */

#ifdef UART_INVERT
		OCR0B   = ~((data<<4) | 0x0f);
#else
		OCR0B   = ((data<<4) | 0x0f);	    /* D4-7, stop bit   */
#endif

        data    >>= 1;
        cli();
#ifdef UART_INVERT
		USIDR   = ~data;
#else
		USIDR   = data;						/* startbit, D0-3   */
#endif
        TCCR0B  = 3;                        /* start timer0: 1/64 clk */
        sei();

        /*  host => device : accept     */
        if( usbAllRequestsAreDisabled() && uartTxBytesFree()>=HW_CDC_BULK_OUT_SIZE ) {
            usbEnableAllRequests();
        }
    }
}





