zeptux.img: arch/x86_64/boot/bootsector.S arch/x86_64/boot/bootsector.ld Makefile
	# Amusingly we must compile for 32-bit despite writing 16-bit code.
	as --32 arch/x86_64/boot/bootsector.S -o bootsector.o
	ld -melf_i386 -T arch/x86_64/boot/bootsector.ld -o zeptux.img bootsector.o

clean:
	rm -f *.o hello.img

qemu: zeptux.img
	qemu-system-x86_64 -nographic -drive file=zeptux.img,format=raw \
		-serial mon:stdio -smp 1

.PHONY: clean qemu
