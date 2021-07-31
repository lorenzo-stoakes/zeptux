hello.img: early/hello.S early/bootsector.ld Makefile
	# Amusingly we must compile for 32-bit despite writing 16-bit code.
	as --32 early/hello.S -o hello.o
	ld -melf_i386 -T early/bootsector.ld -o hello.img hello.o

clean:
	rm -f *.o hello.img

qemu: hello.img
	qemu-system-x86_64 -nographic -drive file=hello.img,format=raw \
		-serial mon:stdio -smp 1

.PHONY: clean qemu
