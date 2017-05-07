
all: monitor.hex blink.bin

.PHONY: all clean
.SECONDARY:

clean:
	-rm -f *.elf *.bin *.o .*.d

CC=arm-linux-gnu-gcc
LD=$(CC)
LDFLAGS=-nostdlib -Wl,--build-id=none -fwhole-program
OBJCOPY=arm-linux-gnu-objcopy
CFLAGS=-Os -flto -ffreestanding \
	-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -MMD -MP -MF.$@.d \
	-fno-common -Wall -Werror
UTILS=../utils

-include .*.d

# Kill this rule.
%: %.c

monitor.elf: rom0.ld monitor.o
blink.elf: sramH.ld blink.o

%.elf:
	$(LINK.c) -T $^ $(LOADLIBES) $(LDLIBS) -o $@

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

%.hex: %.elf
	$(OBJCOPY) -O ihex $< $@

%.s: %.c
	$(CC) $(filter-out -flto,$(CFLAGS)) -S -o $@ $<

# Cancel...
%: %.c
%: %.o

.PHONY: monitorp
monitorp: monitor.hex
	sudo /home/mirror/teensy_loader_cli/teensy_loader_cli --mcu mk66fx1m0 -w monitor.hex
