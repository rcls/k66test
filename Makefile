
all: blink.hex

.PHONY: all clean
.SECONDARY:

clean:
	-rm -f *.elf *.bin *.flasher *.boot *.o *.a *.s .deps/*.d

CC=arm-linux-gnu-gcc
LD=$(CC)
LDFLAGS=-nostdlib -Wl,--build-id=none -fwhole-program
OBJCOPY=arm-linux-gnu-objcopy
CFLAGS=-Os -flto -ffreestanding \
	-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -MMD -MP -MF.deps/$@.d \
	-fno-common -Wall -Werror
UTILS=../utils

-include .deps/*.d

# Kill this rule.
%: %.c

blink.elf: blink.ld blink.o
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

.PHONY: blinkp
blinkp: blink.hex
	sudo /home/mirror/teensy_loader_cli/teensy_loader_cli --mcu mk66fx1m0 -w blink.hex