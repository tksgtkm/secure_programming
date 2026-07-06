#include <stddef.h>
#include <stdint.h>

static void outb(uint16_t port, uint8_t value) {
    asm("outb %0,%1" : /* empty */ : "a" (value), "Nd" (port) : "memory");
}

void
__attribute__((noreturn))
__attribute__((section(".start")))
_start(void) {
    const char *p;

    for (p = "Hello world!\n"; *p; ++p)
        outb(0xE9, *p);
    
    // // 副作用のあるメモリアクセスであることを明示
    // *(volatile long *) 0x400 = 42;

    // インラインアセンブリでアドレス経由で0x400への書き込みを行う
    void *addr = (void *)0x400;
    __asm__ volatile ("" : "+r"(addr));
    *(volatile long *) addr = 42;

    for (;;)
        asm("hlt" : /* empty */ : "a" (42) : "memory");
}