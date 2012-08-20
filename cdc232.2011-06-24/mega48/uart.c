/* Name: uart.c
 * Project: AVR USB driver for CDC interface on Low-Speed USB
 * Author: Osamu Tamura
 * Creation Date: 2006-06-18
 * Tabsize: 4
 * Copyright: (c) 2006 by Recursion Co., Ltd.
 * License: Proprietary, free under certain conditions. See Documentation.
 *
 * 2006-07-08   adapted to higher baudrate by T.Kitazawa
 */
/*
General Description:
    This module implements the UART rx/tx system of the USB-CDC driver.
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>   /* needed by usbdrv.h */
#include "oddebug.h"
#include "usbdrv.h"
#include "uart.h"

extern uchar    sendEmptyFrame;

/* UART buffer */
uchar    urptr, uwptr, irptr, iwptr;
uchar    rx_buf[RX_SIZE+HW_CDC_BULK_IN_SIZE], tx_buf[TX_SIZE];


void uartInit(ulong baudrate, uchar parity, uchar stopbits, uchar databits)
{
usbDWord_t   br;

    br.dword = ((F_CPU>>3)+(baudrate>>1)) / baudrate - 1;
	UCSR0A  |= (1<<U2X0);

#if DEBUG_LEVEL < 1
    /*    USART configuration    */
    UCSR0B  = 0;
    UCSR0C  = URSEL_MASK | ((parity==1? 3:parity)<<UPM00) | ((stopbits>>1)<<USBS0) | ((databits-5)<<UCSZ00);
    UBRR0L  = br.bytes[0];
    UBRR0H  = br.bytes[1];
#else
    DBG1(0xf0, br.bytes, 2);
#endif /* DEBUG_LEVEL */

    UCSR0B  = (1<<RXEN0) | (1<<TXEN0);

	UART_CTRL_DDR	= (1<<UART_CTRL_DTR) | (1<<UART_CTRL_RTS);
	UART_CTRL_PORT	= 0xff;

#ifdef UART_INVERT
	DDRB	|= (1<<PB1)|(1<<PB0);
	PCMSK1	|= (1<<PCINT9)|(1<<PCINT8);
	PCICR	|= (1<<PCIE1);
#endif
}

void uartPoll(void)
{
	uchar		next;

	/*  device => RS-232C  */
	while( (UCSR0A&(1<<UDRE0)) && uwptr!=irptr && (UART_CTRL_PIN&(1<<UART_CTRL_CTS)) ) {
        UDR0    = tx_buf[irptr];
        irptr   = (irptr+1) & TX_MASK;

        if( usbAllRequestsAreDisabled() && uartTxBytesFree()>HW_CDC_BULK_OUT_SIZE ) {
            usbEnableAllRequests();
        }
    }

	/*  device <= RS-232C  */
	while( UCSR0A&(1<<RXC0) ) {
	    next = (iwptr+1) & RX_MASK;
		if( next!=urptr ) {
	        uchar   status, data;

	        status  = UCSR0A;
	        data    = UDR0;
	        status  &= (1<<FE0) | (1<<DOR0) | (1<<UPE0);
	        if(status == 0) { /* no receiver error occurred */
	            rx_buf[iwptr] = data;
	            iwptr = next;
	        }
		}
		else {
			UART_CTRL_PORT	&= ~(1<<UART_CTRL_RTS);
			break;
		}
    }

	/*  USB <= device  */
    if( usbInterruptIsReady() && (iwptr!=urptr || sendEmptyFrame) ) {
        uchar   bytesRead, i;

        bytesRead = (iwptr-urptr) & RX_MASK;
        if(bytesRead>HW_CDC_BULK_IN_SIZE)
            bytesRead = HW_CDC_BULK_IN_SIZE;
		next	= urptr + bytesRead;
		if( next>=RX_SIZE ) {
			next &= RX_MASK;
			for( i=0; i<next; i++ )
				rx_buf[RX_SIZE+i]	= rx_buf[i];
		}
        usbSetInterrupt(rx_buf+urptr, bytesRead);
        urptr   = next;
		if( bytesRead )
			UART_CTRL_PORT	|= (1<<UART_CTRL_RTS);

        /* send an empty block after last data block to indicate transfer end */
        sendEmptyFrame = (bytesRead==HW_CDC_BULK_IN_SIZE && iwptr==urptr)? 1:0;
    }
}


#ifdef UART_INVERT
/*
	enables software-inverter (PC0 -|>o- PB0, PC1 -|>o- PB1)
	to connect to RS-232C line directly. ( <= 2400 bps )
	(atmega48/88 only)
*/
ISR( PCINT1_vect, ISR_NAKED )
{
	asm volatile(
		"out	%0, r16"    	"\n\t"
		"in		r16, __SREG__"	"\n\t"
		"out	%1, r16"    	"\n\t"

		"in		r16, %2"    	"\n\t"
        "com    r16"    		"\n\t"
		"out	%3, r16"    	"\n\t"

		"in		r16, %1"    	"\n\t"
		"out	__SREG__, r16"	"\n\t"
		"in		r16, %0"    	"\n\t"
         :
         : "I" (_SFR_IO_ADDR(GPIOR0)),
           "I" (_SFR_IO_ADDR(GPIOR1)),
           "I" (_SFR_IO_ADDR(PINC)),
           "I" (_SFR_IO_ADDR(PORTB))
        );

	reti();
}
#endif
