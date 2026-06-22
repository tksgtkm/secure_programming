.intel_syntax noprefix
.code16

.text
.globl _start
_start:
    mov dx, 0x3FB
    in al, dx

loop:
    out dx, al
    dec al
    cmp al, 0
    jnz loop
    hlt