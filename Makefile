BOOT_CFLAGS=--std=gnu2x -fno-pic -fno-pie -fno-builtin -fno-stack-protector -nostdinc -Wall
CFLAGS=$(BOOT_CFLAGS) -O2 -g -fno-omit-frame-pointer -mcmodel=large
HEADERS=include/*.h
EARLY_HEADERS=$(HEADERS) arch/x86_64/include/*.h
BOOTSECTOR_FILES=arch/x86_64/boot/boot1.S arch/x86_64/boot/boot1.ld arch/x86_64/boot/boot2.S arch/x86_64/boot/boot2.ld arch/x86_64/boot/loader.c
KERNEL_FILES=kernel/main.c lib/format.c early/fixups.c early/serial.c kernel/kernel.ld
INCLUDES=-I. -Iinclude/

all: zeptux.img

check_build_env:
	./scripts/check_build_env.sh

boot.bin: check_build_env $(BOOTSECTOR_FILES) $(EARLY_HEADERS) $(ADDITIONAL_SOURCES)
	gcc $(BOOT_CFLAGS) -c arch/x86_64/boot/boot1.S -Iarch/x86_64/include -o boot1.o
	objcopy --remove-section .note.gnu.property boot1.o
	gcc $(BOOT_CFLAGS) -c arch/x86_64/boot/boot2.S -Iarch/x86_64/include -o boot2.o
	objcopy --remove-section .note.gnu.property boot2.o
	gcc $(BOOT_CFLAGS) -c -Os arch/x86_64/boot/loader.c $(INCLUDES) -Iarch/x86_64/include -o loader.o
	objcopy --only-section=.text loader.o

	ld -T arch/x86_64/boot/boot1.ld -o boot1.bin boot1.o
	ld -T arch/x86_64/boot/boot2.ld -o boot2.bin boot2.o loader.o
	cat boot1.bin boot2.bin > boot.bin

kernel.elf: check_build_env $(KERNEL_FILES) $(HEADERS) Makefile
	gcc $(CFLAGS) -c $(INCLUDES) kernel/main.c -o main.o
	gcc $(CFLAGS) -c $(INCLUDES) lib/format.c -o format.o
	gcc $(CFLAGS) -c $(INCLUDES) early/fixups.c -o early_fixups.o
	gcc $(CFLAGS) -c $(INCLUDES) early/serial.c -o early_serial.o

	ld -T kernel/kernel.ld -o kernel.elf main.o format.o early_serial.o early_fixups.o

zeptux.img: boot.bin kernel.elf
	dd if=/dev/zero of=zeptux.img count=2000 2>/dev/null
	dd if=boot.bin of=zeptux.img conv=notrunc 2>/dev/null
	dd if=kernel.elf of=zeptux.img seek=5 conv=notrunc 2>/dev/null

clean:
	rm -f *.o *.img *.bin

qemu: zeptux.img
	qemu-system-x86_64 -nographic -drive file=zeptux.img,format=raw \
		-serial mon:stdio -smp 1

.PHONY: all clean qemu check_build_env
