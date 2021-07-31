CFLAGS=-fno-pic -fno-pie -fno-builtin -nostdinc -Wall -O2

zeptux.img: arch/x86_64/boot/bootsector.S arch/x86_64/boot/bootsector.ld Makefile
	gcc $(CFLAGS) -c arch/x86_64/boot/bootsector.S -Iarch/x86_64/include -o bootsector.o
	ld -T arch/x86_64/boot/bootsector.ld -o zeptux.img bootsector.o

clean:
	rm -f *.o *.img

qemu: zeptux.img
	qemu-system-x86_64 -nographic -drive file=zeptux.img,format=raw \
		-serial mon:stdio -smp 1

.PHONY: clean qemu
