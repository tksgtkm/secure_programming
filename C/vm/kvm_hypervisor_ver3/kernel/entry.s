.globl _start, hlt
.extern kernel_main
.intel_syntax noprefix
_start:
    mov rdx, [rsp]
    lea rcx, [rsp + 8]
    call kernel_main
hlt:
    hlt
    jmp hlt