###############################################################################
# Makefile for the project cdc2313
###############################################################################

## General Flags
PROJECT = cdc2313

MCU = attiny2313
#MCU = at90s2313

CLK = 12000000UL
#CLK = 16000000UL
#CLK = 20000000UL

TARGET = cdc2313.elf
CC = avr-gcc

## Options common to compile, link and assembly rules
COMMON = -mmcu=$(MCU)

## Enables DTR, RTS and CTS signals.
ifneq ($(CLK),12000000UL)
COMMON += -DUSE_UART_CTRL
endif

## Compile options common for all C compilation units.
CFLAGS = $(COMMON)
CFLAGS += -Wall -gdwarf-2 -std=gnu99 -DF_CPU=$(CLK) -Os -fsigned-char
CFLAGS += -MD -MP -MT $(*F).o -MF dep/$(@F).d 

## Assembly specific flags
ASMFLAGS = $(COMMON)
ASMFLAGS += $(CFLAGS)
ASMFLAGS += -x assembler-with-cpp -Wa,-gdwarf2

## Linker flags
LDFLAGS = $(COMMON)
LDFLAGS += 


## Intel Hex file production flags
HEX_FLASH_FLAGS = -R .eeprom -R .fuse -R .lock -R .signature

HEX_EEPROM_FLAGS = -j .eeprom
HEX_EEPROM_FLAGS += --set-section-flags=.eeprom="alloc,load"
HEX_EEPROM_FLAGS += --change-section-lma .eeprom=0 --no-change-warnings


## Include Directories
INCLUDES = -I".." -I"../../usbdrv" 

## Objects that must be built in order to link
OBJECTS = main.o usbdrv.o usbdrvasm.o oddebug.o 

## Objects explicitly added by the user
LINKONLYOBJECTS = 

## Build
all: $(TARGET) cdc2313.hex cdc2313.eep cdc2313.lss size

## Compile
usbdrvasm.o: ../../usbdrv/usbdrvasm.S
	$(CC) $(INCLUDES) $(ASMFLAGS) -c  $<

main.o: ../main.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

usbdrv.o: ../../usbdrv/usbdrv.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

oddebug.o: ../../usbdrv/oddebug.c
	$(CC) $(INCLUDES) $(CFLAGS) -c  $<

##Link
$(TARGET): $(OBJECTS)
	 $(CC) $(LDFLAGS) $(OBJECTS) $(LINKONLYOBJECTS) $(LIBDIRS) $(LIBS) -o $(TARGET)

%.hex: $(TARGET)
	avr-objcopy -O ihex $(HEX_FLASH_FLAGS)  $< $@

%.eep: $(TARGET)
	-avr-objcopy $(HEX_EEPROM_FLAGS) -O ihex $< $@ || exit 0

%.lss: $(TARGET)
	avr-objdump -h -S $< > $@

size: ${TARGET}
	@echo
	@avr-size -C --mcu=${MCU} ${TARGET}

## Clean target
.PHONY: clean
clean:
	-rm -rf $(OBJECTS) cdc2313.elf dep/* cdc2313.hex cdc2313.eep cdc2313.lss

## Other dependencies
-include $(shell mkdir dep 2>/dev/null) $(wildcard dep/*)

