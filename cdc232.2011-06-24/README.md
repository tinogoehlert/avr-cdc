

                                    CDC-232


    This is the Readme file to firmware-only CDC driver for Atmel AVR
    microcontrollers. For more information please visit
    http://www.recursion.jp/avrcdc/


SUMMARY
=======
    The CDC-232 performs the CDC (Communication Device Class) connection over
    low-speed USB. It provides the RS-232C interface through virtual COM
    port. The AVR-CDC is originally developed by Osamu Tamura.
    Akira Kitazawa has significantly contributed to improve this software. 


SPECIFICATION
=============
    AVR-CDC with USART (ATmega8/48/88/168)
        speed:     600 - 38400bps
        datasize: 5-8
        parity:   none/even/odd
        stopbit:  1/2
        controls: DTR, RTS, CTS

    AVR-CDC with USART (ATtiny2313)
        speed:     600 - 38400bps
        datasize: 8
        parity:   none
        stopbit:  1

    AVR-CDC without USART (ATtiny45/85)
        speed:    1200 -  4800bps
        datasize: 8
        parity:   none
        stopbit:  1
        supply current: 8-15mA

    The RTS indicates that the receive buffer is not full, and the CTS stops
    sending data at '0' input. These controls cannot be controlled/read by the
    host PC (ATmega). 

    Internal RC Oscillator is calibrated at startup time on ATtiny45/85.
    When the other low speed device is connected under the same host 
    controller, the ATtiny45/85 may fail to be recognized by the downstream
    broadcast packet.

    Although the CDC protocol is supported by Windows 2000/XP/(Vista/7), Mac
    OS 9.1/X, and Linux 2.4 or 2.6.31-, low-speed bulk transfer is not allowed
    by the USB standard. Use CDC-232 at your own risk.


USAGE
=====
    [Windows XP/2000/Vista/7]
    Download "avrcdc_inf.zip" and read the instruction carefully.
 
    [Mac OS X]
    You'll see the device /dev/cu.usbmodem***.

    [Linux]
    The device will be /dev/ttyACM*.
    Linux <2.6.31 does not accept low-speed CDC without patching the kernel.
    Replace the kernel to 2.6.31 or higher.


DEVELOPMENT
===========
    Build your circuit and write firmware (cdcmega*.hex/cdctiny*.hex) into it.
    C1:104 means 0.1uF, R3:1K5 means 1.5K ohms.

    This firmware has been developed on AVR Studio 4.18 and WinAVR 20100110.
    If you couldn't invoke the project from cdc*.aps, create new GCC project
    named "at***" under "cdc232.****-**-**/" without creating initial file. 
    Select each "default/Makefile" at "Configuration Options" menu.

    There are several options you can configure in
    "Project/Configuration Options" menu, or in Makefile

    (General)
    Device      Select MCU type.   
    Frequency   Select clock. 16.5MHz is the internal RC oscillator.
                (ATtiny45/85)
                3.3V Vcc may not be enough for the higher clock operation.

    (Custom Options) add -D*** to select options below.
    UART_INVERT Reverse the polarity of TXD and RXD to connect to RS-232C
                directly (ATtiny45/85).
                Enables software-inverters (PC0 -|>o- PB0, PC1 -|>o- PB1).
                Connect RXD to PB0 and TXD to PC1. The baudrate should be
                <=2400bps (ATmega48/88/168).

    Rebuild all the codes after modifying Makefile.

    Fuse bits
                          ext  H-L
        ATtiny2313         FF CD-FF
        ATtiny45/85        FF CE-F1
        ATtiny45/85(Xtal)  FF 6E-FF / FF 6E-F1 (PLL)
        ATmega8               8F-FF
        ATmega48/88/168    FF CE-FF

	SPIEN=0, WDTON=0, CKOPT(mega8)=0,
	Crystal: Ex.8MHz/PLL(45,461), BOD: 1.8-2.7V

    * Detach the ISP programmer before restarting the device.

    The code size of AVR-CDC is 2-3KB, and 128B RAM is required at least.


USING AVR-CDC FOR FREE
======================
    The AVR-CDC is published under an Open Source compliant license.
    See the file "License.txt" for details.

    You may use this driver in a form as it is. However, if you want to
    distribute a system with your vendor name, modify these files and recompile
    them;
        1. Vendor String in usbconfig.h
        2. COMPANY and MFGNAME strings in avrcdc.inf/lowbulk.inf 



    Osamu Tamura @ Recursion Co., Ltd.
    http://www.recursion.jp/avrcdc/
    26 June 2006
    7 April 2007
    7 July 2007
    27 January 2008
    25 August 2008
    10 April 2009
    18 July 2009
    28 February 2010

