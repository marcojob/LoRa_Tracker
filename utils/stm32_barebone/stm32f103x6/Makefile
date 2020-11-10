# Define the linker script location and chip architecture.
LD_SCRIPT = linkerscript.ld
MCU_SPEC  = cortex-m3

# Toolchain definitions (ARM bare metal defaults)
CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-ld
OC = arm-none-eabi-objcopy
OD = arm-none-eabi-objdump
OS = arm-none-eabi-size
FLASH = stm32flash

# Assembly directives.
ASFLAGS += -c
ASFLAGS += -O0
ASFLAGS += -mcpu=$(MCU_SPEC)
ASFLAGS += -mthumb
ASFLAGS += -Wall
ASFLAGS += -fmessage-length=0 # (Set error messages to appear on a single line.)

# C compilation directives
CFLAGS += -mcpu=$(MCU_SPEC)
CFLAGS += -mthumb
CFLAGS += -Wall
CFLAGS += -g
INCLUDE = -I common/onewire_uart/src/include \
          -I common/stm_include/

CFLAGS += -fmessage-length=0 # (Set error messages to appear on a single line.)
CFLAGS += --specs=nosys.specs # (Set system to ignore semihosted junk)
CFLAGS += -DUSE_FULL_LL_DRIVER

# Linker directives.
LSCRIPT = ./$(LD_SCRIPT)
LFLAGS += -mcpu=$(MCU_SPEC)
LFLAGS += -mthumb
LFLAGS += -Wall
LFLAGS += --specs=nosys.specs
LFLAGS += -nostdlib
LFLAGS += -lgcc
LFLAGS += -T$(LSCRIPT)

OBJS = main.o \
       core.o \
       common/stm_source/system_stm32f1xx.o \
       common/stm_source/stm32f1xx_ll_gpio.o \
       common/stm_source/stm32f1xx_ll_usart.o \
       common/stm_source/stm32f1xx_ll_rcc.o \
       common/stm_source/syscalls.o \
       common/onewire_uart/src/devices/ow_device_ds18x20.o \
       common/onewire_uart/src/ow/ow.o \
       common/onewire_uart/src/system/ow_ll_stm32.o \
       common/onewire_uart/src/scan_devices.o

.PHONY: all
all: main.bin

core.o: core.s
	$(CC) -x assembler-with-cpp $(ASFLAGS) $< -o $@

main.o: main.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -O3 --std=c++14 $(CINCLUDE) $< -o $@
%.o: %.c
	$(CC) -c $(CFLAGS) $(INCLUDE) $< -o $@

main.elf: $(OBJS)
	$(CC) $^ $(LFLAGS) -o $@

main.bin: main.elf
	$(OC) -S -O binary $< $@
	$(OS) $<

flash: main.bin
	${FLASH} -w $< /dev/tty.usbserial-A506BPL8

.PHONY: clean
clean:
	rm *.elf *.bin *.o 
	rm common/stm_source/*.o 
	rm common/onewire_uart/src/devices/*.o 
	rm common/onewire_uart/src/ow/*.o
	rm common/onewire_uart/src/system/*.o
	rm common/onewire_uart/src/*.o
