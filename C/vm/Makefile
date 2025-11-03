CC = gcc
AS = gcc

LDFLAGS += -nostdlib -T x86_64.lds -no-pie
CFLAGS  += -std=c99
ASFLAGS += -D__ASSEMBLY__

.PHONY: all

all: testkernel

testkernel: bootstrap.x86_64.o kernel.o
	$(CC) $(LDFLAGS) $^ -o $@

%.o: %.s
	$(AS) $(ASFLAGS) -c $< -o $@

clean:
	rm -f *.o
	rm -f testkernel