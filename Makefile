 # = 5 sectors into boot image - 4 bytes = 512 * 5 - 4 update if number of stage
 # 2 bootsectors increases.
ELF_SIZE_OFFSET=2556

# The BIOS ELF loader is currently limited to 255 sectors.
MAX_KERNEL_ELF_SIZE=130560

BOOT_CFLAGS=--std=gnu2x -fno-pic -fno-pie -fno-builtin -fno-stack-protector -nostdinc -Wall -Wextra -Werror
CFLAGS=$(BOOT_CFLAGS) -O2 -g -fno-omit-frame-pointer -mcmodel=large
INCLUDES=-I. -Iinclude/
HEADERS=include/*.h
EARLY_HEADERS=$(HEADERS) arch/x86_64/include/*.h
TEST_HEADERS=test/include/*.h
BOOTSECTOR_CFILES=arch/x86_64/boot/*.c
BOOTSECTOR_FILES=arch/x86_64/boot/*.S arch/x86_64/boot/*.ld $(BOOTSECTOR_CFILES)
KERNEL_CFILES=kernel/*.c lib/*.c early/*.c
KERNEL_FILES=$(KERNEL_CFILES) kernel/kernel.ld
TEST_CFILES=lib/*.c early/*.c test/early_kernel/*.c
TEST_FILES=$(TEST_CFILES) kernel/kernel.ld

ALL_CSOURCE=$(EARLY_HEADERS) $(TEST_HEADERS) $(BOOTSECTOR_CFILES) $(KERNEL_CFILES) $(TEST_CFILES)
QEMU_OPT=-serial mon:stdio -smp 1

all: pre_step zeptux.img

pre_step:
	./scripts/check_build_env.sh
	clang-format -style=file -i $(shell find $(ALL_CSOURCE) -type f)

boot.bin: $(BOOTSECTOR_FILES) $(EARLY_HEADERS) $(ADDITIONAL_SOURCES)
	gcc $(BOOT_CFLAGS) -c arch/x86_64/boot/boot1.S -Iarch/x86_64/include -o boot1.o
	objcopy --remove-section .note.gnu.property boot1.o
	gcc $(BOOT_CFLAGS) -c arch/x86_64/boot/boot2.S -Iinclude -Iarch/x86_64/include -o boot2.o
	objcopy --remove-section .note.gnu.property boot2.o
	gcc $(BOOT_CFLAGS) -c -Os arch/x86_64/boot/loader.c $(INCLUDES) -Iarch/x86_64/include -o loader.o
	objcopy --only-section=.text loader.o

	ld -T arch/x86_64/boot/boot1.ld -o boot1.bin boot1.o
	ld -T arch/x86_64/boot/boot2.ld -o boot2.bin boot2.o loader.o
	cat boot1.bin boot2.bin > boot.bin

kernel.elf: $(KERNEL_FILES) $(HEADERS) Makefile
	gcc $(CFLAGS) -c $(INCLUDES) -Wno-main kernel/main.c -o main.o
	gcc $(CFLAGS) -c $(INCLUDES) lib/format.c -o format.o
	gcc $(CFLAGS) -c $(INCLUDES) early/serial.c -o early_serial.o
	gcc $(CFLAGS) -c $(INCLUDES) early/video.c -o early_video.o

	ld -T kernel/kernel.ld -o kernel.elf main.o format.o early_serial.o early_video.o

	find kernel.elf -size -$(MAX_KERNEL_ELF_SIZE)c | grep -q . # Assert less than maximum size

zeptux.img: boot.bin kernel.elf
	scripts/patch_bin_int.py boot.bin $(ELF_SIZE_OFFSET) $(shell stat -c%s kernel.elf)

	dd if=/dev/zero of=zeptux.img count=2000 2>/dev/null
	dd if=boot.bin of=zeptux.img conv=notrunc 2>/dev/null
	dd if=kernel.elf of=zeptux.img seek=5 conv=notrunc 2>/dev/null

test.elf: zeptux.img $(TEST_FILES) $(HEADERS) $(TEST_HEADERS) Makefile
	gcc $(CFLAGS) -c $(INCLUDES) -I test/include -Wno-main test/early_kernel/test_main.c -o test_main.o
	gcc $(CFLAGS) -c $(INCLUDES) -I test/include test/early_kernel/test_format.c -o test_format.o
	gcc $(CFLAGS) -c $(INCLUDES) -I test/include test/early_kernel/test_string.c -o test_string.o
	gcc $(CFLAGS) -c $(INCLUDES) -I test/include test/early_kernel/test_misc.c -o test_misc.o

	ld -T kernel/kernel.ld -o test.elf test_main.o test_format.o test_string.o test_misc.o format.o early_serial.o early_video.o

test.img: boot.bin test.elf
	dd if=/dev/zero of=test.img count=2000 2>/dev/null
	dd if=boot.bin of=test.img conv=notrunc 2>/dev/null
	dd if=test.elf of=test.img seek=5 conv=notrunc 2>/dev/null

clean:
	rm -f *.o *.img *.bin *.elf

qemu: zeptux.img
	qemu-system-x86_64 -nographic $(QEMU_OPT) -drive file=zeptux.img,format=raw

qemu-vga: zeptux.img
	qemu-system-x86_64 $(QEMU_OPT) -drive file=zeptux.img,format=raw


test: test.img
	qemu-system-x86_64 -nographic $(QEMU_OPT) -drive file=test.img,format=raw

.PHONY: all clean pre_step qemu qemu-vga test
