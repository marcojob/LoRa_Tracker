CC=arm-none-eabi-g++
CC_OBJCOPY=arm-none-eabi-objcopy
STM32_FLASH=stm32flash

main:
	${CC} -c -mcpu=cortex-m3 -mthumb --std=c++14 -O2 -fno-rtti -fno-exceptions main.cpp -o main.o
	${CC} -mcpu=cortex-m3 -mthumb -Tlinkerscript.ld -nostartfiles main.o -o main.elf
	${CC_OBJCOPY} -O binary main.elf main.bin

flash: main
	${STM32_FLASH} -w main.bin /dev/tty.usbserial-A506BPL8

clean:
	-rm -f *.o *.bin *.elf