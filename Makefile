 # = 5 sectors into boot image - 4 bytes = 512 * 5 - 4 update if number of stage
 # 2 bootsectors increases.
ELF_SIZE_OFFSET=2556

# The BIOS ELF loader is currently limited to 255 sectors.
MAX_KERNEL_ELF_SIZE=130560

BOOT_CFLAGS=--std=gnu2x -fno-pic -fno-pie -fno-builtin -fno-stack-protector -nostdinc -Wall -Wextra -Werror
CFLAGS=$(BOOT_CFLAGS) -O2 -g -fno-omit-frame-pointer -mcmodel=large
INCLUDES=-I. -Iinclude/
HEADERS=include/*.h
BOOTSECTOR_HEADERS=$(HEADERS) arch/x86_64/include/*.h
TEST_EARLY_HEADERS=test/include/test_helpers.h test/include/test_early.h
BOOTSECTOR_CFILES=arch/x86_64/boot/*.c
BOOTSECTOR_FILES=arch/x86_64/boot/*.S arch/x86_64/boot/*.ld $(BOOTSECTOR_CFILES)
KERNEL_CFILES=kernel/*.c lib/*.c early/*.c
KERNEL_FILES=$(KERNEL_CFILES) kernel/kernel.ld
TEST_EARLY_CFILES=lib/*.c early/*.c test/early/*.c
TEST_EARLY_FILES=$(TEST_EARLY_CFILES) kernel/kernel.ld
TEST_USER_CFILES=test/user/*.cpp
TEST_USER_FILES=$(TEST_USER_CFILES) $(BOOTSECTOR_HEADERS)

ALL_CSOURCE=$(BOOTSECTOR_HEADERS) $(TEST_EARLY_HEADERS) $(BOOTSECTOR_CFILES) $(KERNEL_CFILES) $(TEST_EARLY_CFILES)
QEMU_OPT=-serial mon:stdio -smp 1

KERNEL_OBJ_FILES=format.o early_serial.o early_video.o early_init.o
TEST_OBJ_FILES=test_format.o test_string.o test_misc.o

all: pre_step zeptux.img

pre_step:
	./scripts/check_build_env.sh
	clang-format -style=file -i $(shell find $(ALL_CSOURCE) -type f)

boot.bin: $(BOOTSECTOR_FILES) $(BOOTSECTOR_HEADERS) $(ADDITIONAL_SOURCES)
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
	gcc $(CFLAGS) -c $(INCLUDES) early/init.c -o early_init.o

	ld -T kernel/kernel.ld -o kernel.elf main.o $(KERNEL_OBJ_FILES)

	find kernel.elf -size -$(MAX_KERNEL_ELF_SIZE)c | grep -q . # Assert less than maximum size

zeptux.img: boot.bin kernel.elf
	scripts/patch_bin_int.py boot.bin $(ELF_SIZE_OFFSET) $(shell stat -c%s kernel.elf)

	dd if=/dev/zero of=zeptux.img count=2000 2>/dev/null
	dd if=boot.bin of=zeptux.img conv=notrunc 2>/dev/null
	dd if=kernel.elf of=zeptux.img seek=5 conv=notrunc 2>/dev/null

test-early.elf: zeptux.img $(TEST_EARLY_FILES) $(HEADERS) $(TEST_EARLY_HEADERS) Makefile
	gcc $(CFLAGS) -c $(INCLUDES) -I test/include -Wno-main test/early/test_main.c -o test_main.o
	gcc $(CFLAGS) -c $(INCLUDES) -I test/include test/early/test_format.c -o test_format.o
	gcc $(CFLAGS) -c $(INCLUDES) -I test/include test/early/test_string.c -o test_string.o
	gcc $(CFLAGS) -c $(INCLUDES) -I test/include test/early/test_misc.c -o test_misc.o

	ld -T kernel/kernel.ld -o test-early.elf test_main.o $(KERNEL_OBJ_FILES) $(TEST_OBJ_FILES)

test-early.img: boot.bin test-early.elf
	dd if=/dev/zero of=test-early.img count=2000 2>/dev/null
	dd if=boot.bin of=test-early.img conv=notrunc 2>/dev/null
	dd if=test-early.elf of=test-early.img seek=5 conv=notrunc 2>/dev/null

clean:
	rm -f *.o *.img *.bin *.elf

qemu: zeptux.img
	qemu-system-x86_64 -nographic $(QEMU_OPT) -drive file=zeptux.img,format=raw

qemu-vga: zeptux.img
	qemu-system-x86_64 $(QEMU_OPT) -drive file=zeptux.img,format=raw

test-early: test-early.img
	@qemu-system-x86_64 -nographic $(QEMU_OPT) -drive file=test-early.img,format=raw

test-user: $(TEST_USER_FILES) Makefile
	@g++ -g -Iinclude -Iarch/x86-64/include -Itest/include test/user/test_main.cpp -o test_user
	@./test_user

test: test-early test-user

.PHONY: all clean pre_step qemu qemu-vga test-early test-user test
