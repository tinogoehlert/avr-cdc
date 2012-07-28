
/* Name: main.c
 * Project: AVR USB driver for CDC interface on Low-Speed USB
 *              for ATtiny2313
 * Author: Osamu Tamura
 * Creation Date: 2007-10-03
 * Tabsize: 4
 * Copyright: (c) 2007-2009 by Recursion Co., Ltd.
 * License: Proprietary, free under certain conditions. See Documentation.
 *
 */

#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include "usbdrv.h"
#include "oddebug.h"

#ifdef _AVR_IO2313_H_
#define	UCSRA	USR
#define	UCSRB	UCR
#define	UBRRL	UBRR
#endif

#define HW_CDC_BULK_OUT_SIZE     8
#define HW_CDC_BULK_IN_SIZE      8

/*	Control signals works at 16/20MHz clocks. */
#ifdef USE_UART_CTRL
#define	UART_CTRL_PIN		PINB
#define	UART_CTRL_DDR		DDRB
#define	UART_CTRL_PORT		PORTB
#define	UART_CTRL_DTR		5
#define	UART_CTRL_RTS		6
#define	UART_CTRL_CTS		7
#endif

enum {
    SEND_ENCAPSULATED_COMMAND = 0,
    GET_ENCAPSULATED_RESPONSE,
    SET_COMM_FEATURE,
    GET_COMM_FEATURE,
    CLEAR_COMM_FEATURE,
    SET_LINE_CODING = 0x20,
    GET_LINE_CODING,
    SET_CONTROL_LINE_STATE,
    SEND_BREAK
};


static PROGMEM char configDescrCDC[] = {   /* USB configuration descriptor */
    9,          /* sizeof(usbDescrConfig): length of descriptor in bytes */
    USBDESCR_CONFIG,    /* descriptor type */
    67,
    0,          /* total length of data returned (including inlined descriptors) */
    2,          /* number of interfaces in this configuration */
    1,          /* index of this configuration */
    0,          /* configuration name string index */
#if USB_CFG_IS_SELF_POWERED
    (1 << 7) | USBATTR_SELFPOWER,       /* attributes */
#else
    (1 << 7),                           /* attributes */
#endif
    USB_CFG_MAX_BUS_POWER/2,            /* max USB current in 2mA units */

    /* interface descriptor follows inline: */
    9,          /* sizeof(usbDescrInterface): length of descriptor in bytes */
    USBDESCR_INTERFACE, /* descriptor type */
    0,          /* index of this interface */
    0,          /* alternate setting for this interface */
    USB_CFG_HAVE_INTRIN_ENDPOINT,   /* endpoints excl 0: number of endpoint descriptors to follow */
    USB_CFG_INTERFACE_CLASS,
    USB_CFG_INTERFACE_SUBCLASS,
    USB_CFG_INTERFACE_PROTOCOL,
    0,          /* string index for interface */

    /* CDC Class-Specific descriptor */
    5,           /* sizeof(usbDescrCDC_HeaderFn): length of descriptor in bytes */
    0x24,        /* descriptor type */
    0,           /* header functional descriptor */
    0x10, 0x01,

    4,           /* sizeof(usbDescrCDC_AcmFn): length of descriptor in bytes */
    0x24,        /* descriptor type */
    2,           /* abstract control management functional descriptor */
    0x02,        /* SET_LINE_CODING,    GET_LINE_CODING, SET_CONTROL_LINE_STATE    */

    5,           /* sizeof(usbDescrCDC_UnionFn): length of descriptor in bytes */
    0x24,        /* descriptor type */
    6,           /* union functional descriptor */
    0,           /* CDC_COMM_INTF_ID */
    1,           /* CDC_DATA_INTF_ID */

    5,           /* sizeof(usbDescrCDC_CallMgtFn): length of descriptor in bytes */
    0x24,        /* descriptor type */
    1,           /* call management functional descriptor */
    3,           /* allow management on data interface, handles call management by itself */
    1,           /* CDC_DATA_INTF_ID */

    /* Endpoint Descriptor */
    7,           /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
    0x83,        /* IN endpoint number 3 */
    0x03,        /* attrib: Interrupt endpoint */
    8, 0,        /* maximum packet size */
    USB_CFG_INTR_POLL_INTERVAL,        /* in ms */

    /* Interface Descriptor  */
    9,           /* sizeof(usbDescrInterface): length of descriptor in bytes */
    USBDESCR_INTERFACE,           /* descriptor type */
    1,           /* index of this interface */
    0,           /* alternate setting for this interface */
    2,           /* endpoints excl 0: number of endpoint descriptors to follow */
    0x0A,        /* Data Interface Class Codes */
    0,
    0,           /* Data Interface Class Protocol Codes */
    0,           /* string index for interface */

    /* Endpoint Descriptor */
    7,           /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
    0x01,        /* OUT endpoint number 1 */
    0x02,        /* attrib: Bulk endpoint */
    HW_CDC_BULK_OUT_SIZE, 0,        /* maximum packet size */
    0,           /* in ms */

    /* Endpoint Descriptor */
    7,           /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
    0x81,        /* IN endpoint number 1 */
    0x02,        /* attrib: Bulk endpoint */
    HW_CDC_BULK_IN_SIZE, 0,        /* maximum packet size */
    0,           /* in ms */
};


uchar usbFunctionDescriptor(usbRequest_t *rq)
{

    if(rq->wValue.bytes[1] == USBDESCR_DEVICE){
        usbMsgPtr = (uchar *)usbDescriptorDevice;
        return usbDescriptorDevice[0];
    }else{  /* must be config descriptor */
        usbMsgPtr = (uchar *)configDescrCDC;
        return sizeof(configDescrCDC);
    }
}

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

static uchar    sendEmptyFrame;
static uchar    modeBuffer[7];

uchar usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */

        if( rq->bRequest==GET_LINE_CODING || rq->bRequest==SET_LINE_CODING ){
            return 0xff;
        /*    GET_LINE_CODING -> usbFunctionRead()    */
        /*    SET_LINE_CODING -> usbFunctionWrite()   */
        }
#ifdef USE_UART_CTRL
        if(rq->bRequest==SET_CONTROL_LINE_STATE){
            UART_CTRL_PORT	= (UART_CTRL_PORT&~(1<<UART_CTRL_DTR))|((rq->wValue.word&1)<<UART_CTRL_DTR);
        }
#endif
    }
    return 0;
}

/*---------------------------------------------------------------------------*/
/* usbFunctionRead -  GET_LINE_CODING                                       */
/*---------------------------------------------------------------------------*/

uchar usbFunctionRead( uchar *data, uchar len )
{
    memcpy( data, modeBuffer, 7 );
    return 7;
}

/*---------------------------------------------------------------------------*/
/* usbFunctionWrite  -  SET_LINE_CODING                                      */
/*---------------------------------------------------------------------------*/

#define	BRMIN	600
#define	BRMAX	38400
#define	BSTEP	((F_CPU>>3)/BRMAX)
#define	BRVAL	((unsigned)BSTEP*BRMAX/BRMIN)

uchar usbFunctionWrite( uchar *data, uchar len )
{
	usbWord_t	br;
	unsigned 	baudrate;
	uchar		i;

	//	set baudrate generator
	baudrate	= *(unsigned *)data;
	for( i=0; baudrate; i++ )
		baudrate	-= BRMIN;
	for( br.word=BRVAL; i!=1; i>>=1 )
		br.word	>>= 1;
	br.word--;

#ifdef _AVR_IO2313_H_
    UBRRL	= br.bytes[0] << 1;
#else
    UBRRL	= br.bytes[0];
    UBRRH   = br.bytes[1];
    UCSRA   = (1<<U2X);
#endif
    UCSRB	= (1<<RXEN) | (1<<TXEN);

    memcpy( modeBuffer, data, 7 );
    return 1;
}

/*---------------------------------------------------------------------------*/
/* usbFunctionWriteOut					                                     */
/*---------------------------------------------------------------------------*/

#define    TX_SIZE        (HW_CDC_BULK_OUT_SIZE<<1)
#define    TX_MASK        (TX_SIZE-1)
#define    RX_SIZE        (HW_CDC_BULK_IN_SIZE)

static uchar    iwptr, uwptr, irptr;
static uchar    rx_buf[RX_SIZE], tx_buf[TX_SIZE];

void usbFunctionWriteOut( uchar *data, uchar len )
{

    /*  usb -> rs232c:  transmit char    */
    for( ; len; len-- ) {
        tx_buf[uwptr++] = *data++;
        uwptr	&= TX_MASK;
    }

    /*  postpone receiving next data    */
   	usbDisableAllRequests();
}


static void hardwareInit(void)
{
unsigned	i;
uchar		j;

    /* activate pull-ups except on USB lines */
    USB_CFG_IOPORT   = (uchar)~((1<<USB_CFG_DMINUS_BIT)|(1<<USB_CFG_DPLUS_BIT));
    /* all pins input except USB (-> USB reset) */
#ifdef USB_CFG_PULLUP_IOPORT    /* use usbDeviceConnect()/usbDeviceDisconnect() if available */
    USBDDR    = 0;    /* we do RESET by deactivating pullup */
    usbDeviceDisconnect();
#else
    USBDDR    = (1<<USB_CFG_DMINUS_BIT)|(1<<USB_CFG_DPLUS_BIT);
#endif

    j = 15;
    while(--j){          /* USB Reset by device only required on Watchdog Reset */
        i = 0;
        while(--i)
            wdt_reset();
    }

#ifdef USB_CFG_PULLUP_IOPORT
    usbDeviceConnect();
#else
    USBDDR    = 0;      /*  remove USB reset condition */
#endif

	PORTB	= 0xff;

#ifdef USE_UART_CTRL
	UART_CTRL_DDR	|= (1<<UART_CTRL_DTR) | (1<<UART_CTRL_RTS);
#endif
}


int main(void)
{
    odDebugInit();
    hardwareInit();
    usbInit();

    sei();
    for(;;){    /* main event loop */
        wdt_reset();
        usbPoll();

        /*    host => device    */
        if( (UCSRA&(1<<UDRE)) && uwptr!=irptr
#ifdef USE_UART_CTRL
		&& (UART_CTRL_PIN&(1<<UART_CTRL_CTS))
#endif
		) {
            UDR		= tx_buf[irptr++];
            irptr   &= TX_MASK;
        }
        if( usbAllRequestsAreDisabled() &&
            ((uwptr-irptr)&TX_MASK)<(TX_SIZE-HW_CDC_BULK_OUT_SIZE) ) {
            usbEnableAllRequests();
        }

        /*    host <= device    */
        if( UCSRA&(1<<RXC) && iwptr<HW_CDC_BULK_IN_SIZE ) {
            rx_buf[iwptr++]	= UDR;
#ifdef USE_UART_CTRL
			if( iwptr==HW_CDC_BULK_IN_SIZE )
				UART_CTRL_PORT &= ~(1<<UART_CTRL_RTS);
#endif
        }
        if( usbInterruptIsReady() && (iwptr||sendEmptyFrame) ) {
            usbSetInterrupt(rx_buf, iwptr);
            sendEmptyFrame	= iwptr & HW_CDC_BULK_IN_SIZE;
            iwptr    = 0;
#ifdef USE_UART_CTRL
			UART_CTRL_PORT |= (1<<UART_CTRL_RTS);
#endif
        }
    }
    return 0;
}

