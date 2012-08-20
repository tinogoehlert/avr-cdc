
/* Name: uart.h
 * Project: AVR USB driver for CDC interface on Low-Speed USB
 * Author: Osamu Tamura
 * Creation Date: 2006-06-18
 * Tabsize: 4
 * Copyright: (c) 2006 by Recursion Co., Ltd.
 * License: Proprietary, free under certain conditions. See Documentation.
 */

#ifndef __uart_h_included__
#define __uart_h_included__

#include <avr/io.h> /* for TXEN or TXEN0 resp., if available */

#ifndef uchar
#define uchar   unsigned char
#endif

#ifndef ulong
#define ulong   unsigned long
#endif

#define HW_CDC_BULK_OUT_SIZE     8
#define HW_CDC_BULK_IN_SIZE      8


#if !(defined TXEN || defined TXEN0)
#   error "MCU has no UART"
#endif

#ifndef UART_DEFAULT_BPS
#define UART_DEFAULT_BPS     9600
#endif
/*
   The baud rate will be automatically configured after opening device anyway.
*/

/* These are the USART port and TXD, RXD bit numbers.
*/
/* ATmega8/48/88/168 */
#define UART_CFG_PORTNAME   D
#define UART_CFG_RXD        0
#define UART_CFG_TXD        1

#define	UART_CTRL_PORTNAME	C
#define	UART_CTRL_DTR		3
#define	UART_CTRL_RTS		4
#define	UART_CTRL_CTS		5

#define	RX_SIZE		128      /* UART receive buffer size (must be 2^n, 16-128)  */
#define	TX_SIZE		256      /* UART transmit buffer size (must be 2^n, 16-256) */
#define	RX_MASK		(RX_SIZE-1)
#define	TX_MASK		(TX_SIZE-1)


#ifndef URSEL
#define URSEL_MASK   0
//#define	UART_INVERT
/* enables software-inverter (PC0 -|>o- PB0, PC1 -|>o- PB1)	*/
/* to connect to RS-232C line directly. ( <= 2400 bps )		*/
#else
#define URSEL_MASK   (1 << URSEL)
#ifdef UART_INVERT
#error "ATmega8 doesn't support UART_INVERT"
#undef	UART_INVERT
#endif
#endif

/* ------------------------------------------------------------------------- */
/* ------------------------ General Purpose Macros ------------------------- */
/* ------------------------------------------------------------------------- */
#define OD_CONCAT(a, b)             a ## b
#define UART_OUTPORT(name)          OD_CONCAT(PORT, name)
#define UART_INPORT(name)           OD_CONCAT(PIN, name)
#define UART_DDRPORT(name)          OD_CONCAT(DDR, name)

#define UART_CFG_PORT     UART_OUTPORT(UART_CFG_PORTNAME)
#define UART_PIN          UART_INPORT(UART_CFG_PORTNAME)
#define UART_DDR          UART_DDRPORT(UART_CFG_PORTNAME)

#define UART_CTRL_PORT    UART_OUTPORT(UART_CTRL_PORTNAME)
#define UART_CTRL_PIN     UART_INPORT(UART_CTRL_PORTNAME)
#define UART_CTRL_DDR     UART_DDRPORT(UART_CTRL_PORTNAME)

#ifndef __ASSEMBLER__

/* allow ATmega8 compatibility */
#ifdef URSEL
#define UBRR0H    UBRRH
#define UBRR0L    UBRRL
#define UCSR0A    UCSRA
#define UCSR0B    UCSRB
#define UCSR0C    UCSRC
#define UDR0      UDR

/* UCSR0A */
#define RXC0      RXC
#define UDRE0     UDRE
#define FE0       FE
#define DOR0      DOR
#define UPE0      PE
#define U2X0      U2X

/* UCSR0B */
#define RXCIE0    RXCIE
#define UDRIE0    UDRIE
#define RXEN0     RXEN
#define TXEN0     TXEN

/* UCSR0C */
#define UPM00     UPM0
#define USBS0     USBS
#define UPBS0     UPBS
#define UCSZ00    UCSZ0
#endif

/* ------------------------------------------------------------------------- */
/*	---------------------- Type Definition --------------------------------- */
/* ------------------------------------------------------------------------- */
typedef union usbDWord {
    ulong	dword;
    uchar   bytes[4];
} usbDWord_t;


extern uchar    urptr, uwptr, irptr, iwptr;
extern uchar    rx_buf[], tx_buf[]; 

extern void uartInit(ulong baudrate, uchar parity, uchar stopbits, uchar databits);
extern void uartPoll(void);


/* The following function returns the amount of bytes available in the TX
 * buffer before we have an overflow.
 */
static inline uchar uartTxBytesFree(void)
{
    return (irptr - uwptr - 1) & TX_MASK;
}


#endif	/*  #ifndef __ASSEMBLER__  */
#endif  /*  __uart_h_included__  */

