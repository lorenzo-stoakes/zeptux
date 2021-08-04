BOOT_CFLAGS=--std=gnu2x -fno-pic -fno-pie -fno-builtin -fno-stack-protector -nostdinc -Wall
CFLAGS=$(BOOT_CFLAGS) -O2 -g -fno-omit-frame-pointer -mcmodel=large
HEADERS=include/*.h
EARLY_HEADERS=$(HEADERS) arch/x86_64/include/*.h
BOOTSECTOR_FILES=arch/x86_64/boot/bootsector.S arch/x86_64/boot/bootsector.ld arch/x86_64/boot/loader.c
KERNEL_FILES=kernel/main.c kernel/format.c early/fixups.c early/serial.c kernel/kernel.ld
INCLUDES=-I. -Iinclude/

all: zeptux.img

boot.bin: $(BOOTSECTOR_FILES) $(EARLY_HEADERS) $(ADDITIONAL_SOURCES)
	gcc $(BOOT_CFLAGS) -c arch/x86_64/boot/bootsector.S -Iarch/x86_64/include -o bootsector.o
	objcopy --remove-section .note.gnu.property bootsector.o
	gcc $(BOOT_CFLAGS) -c -Os arch/x86_64/boot/loader.c $(INCLUDES) -Iarch/x86_64/include -o loader.o
	objcopy --remove-section=* --keep-section=.text loader.o
	ld -T arch/x86_64/boot/bootsector.ld -o boot.bin bootsector.o loader.o

kernel.elf: $(KERNEL_FILES) $(HEADERS) Makefile
	gcc $(CFLAGS) -c $(INCLUDES) kernel/main.c -o main.o
	gcc $(CFLAGS) -c $(INCLUDES) kernel/format.c -o format.o
	gcc $(CFLAGS) -c $(INCLUDES) early/fixups.c -o early_fixups.o
	gcc $(CFLAGS) -c $(INCLUDES) early/serial.c -o early_serial.o

	ld -T kernel/kernel.ld -o kernel.elf main.o format.o early_serial.o early_fixups.o

zeptux.img: boot.bin kernel.elf
	dd if=/dev/zero of=zeptux.img count=2000 2>/dev/null
	dd if=boot.bin of=zeptux.img conv=notrunc 2>/dev/null
	dd if=kernel.elf of=zeptux.img seek=1 conv=notrunc 2>/dev/null

clean:
	rm -f *.o *.img *.bin

qemu: zeptux.img
	qemu-system-x86_64 -nographic -drive file=zeptux.img,format=raw \
		-serial mon:stdio -smp 1

.PHONY: all clean qemu
