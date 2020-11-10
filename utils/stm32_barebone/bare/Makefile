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
FLASH = st-flash

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
INCLUDE = -I common/cmsis_include \
		  -I common/stm_include

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
# LFLAGS += -Wl,--print-map

OBJS = main.o \
       startup_stm32f103xb.o \
       common/stm_source/system_stm32f1xx.o \

.PHONY: all
all: main.bin

startup_stm32f103xb.o: startup_stm32f103xb.s
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
	${FLASH} write $< 0x08000000

.PHONY: clean
clean:
	rm *.elf *.bin *.o 
