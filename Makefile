CFLAGS=--std=gnu2x -fno-pic -fno-pie -fno-builtin -nostdinc -Wall -O2

BOOTSECTOR_FILES=arch/x86_64/boot/bootsector.S arch/x86_64/boot/bootsector.ld arch/x86_64/boot/loader.c
EARLY_HEADERS=arch/x86_64/include/bootsector.h arch/x86_64/include/x86-consts.h

zeptux.img: $(BOOTSECTOR_FILES) $(EARLY_HEADERS) Makefile
	gcc $(CFLAGS) -c arch/x86_64/boot/bootsector.S -Iarch/x86_64/include -o bootsector.o
	objcopy --remove-section .note.gnu.property bootsector.o
	gcc $(CFLAGS) -fno-stack-protector -c arch/x86_64/boot/loader.c -Os -Iinclude -Iarch/x86_64/include -o loader.o
	objcopy --remove-section=* --keep-section=.text loader.o
	ld -T arch/x86_64/boot/bootsector.ld -o zeptux.img bootsector.o loader.o

clean:
	rm -f *.o *.img

qemu: zeptux.img
	qemu-system-x86_64 -drive file=zeptux.img,format=raw \
		-serial mon:stdio -smp 1

.PHONY: clean qemu
