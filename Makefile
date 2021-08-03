CFLAGS=--std=gnu2x -fno-pic -fno-pie -fno-builtin -fno-stack-protector -nostdinc -Wall -O2 -mcmodel=large

EARLY_HEADERS=include/types.h arch/x86_64/include/bootsector.h arch/x86_64/include/x86-consts.h arch/x86_64/include/io.h include/elf.h
HEADERS=include/types.h include/mem.h arch/x86_64/include/x86-consts.h include/elf.h

BOOTSECTOR_FILES=arch/x86_64/boot/bootsector.S arch/x86_64/boot/bootsector.ld arch/x86_64/boot/loader.c

KERNEL_FILES=kernel/main.c kernel/kernel.ld

all: zeptux.img

boot.bin: $(BOOTSECTOR_FILES) $(EARLY_HEADERS) $(ADDITIONAL_SOURCES)
	gcc $(CFLAGS) -c arch/x86_64/boot/bootsector.S -Iarch/x86_64/include -o bootsector.o
	objcopy --remove-section .note.gnu.property bootsector.o
	gcc $(CFLAGS) -c arch/x86_64/boot/loader.c -Os -Iinclude -Iarch/x86_64/include -o loader.o
	objcopy --remove-section=* --keep-section=.text loader.o
	ld -T arch/x86_64/boot/bootsector.ld -o boot.bin bootsector.o loader.o

kernel.elf: $(KERNEL_FILES) $(HEADERS) Makefile
	gcc $(CFLAGS) -c kernel/main.c -o kernel.o
	ld -T kernel/kernel.ld -o kernel.elf kernel.o

zeptux.img: boot.bin kernel.elf
	dd if=/dev/zero of=zeptux.img count=2000
	dd if=boot.bin of=zeptux.img conv=notrunc
	dd if=kernel.elf of=zeptux.img seek=1 conv=notrunc

clean:
	rm -f *.o *.img *.bin

qemu: zeptux.img
	qemu-system-x86_64 -drive file=zeptux.img,format=raw \
		-serial mon:stdio -smp 1

.PHONY: all clean qemu
