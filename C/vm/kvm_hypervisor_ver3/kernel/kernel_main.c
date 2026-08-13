#define MSR_STAR 0xc0000081
#define MSR_LSTAR 0xc0000082
#define MSR_CSTAR 0xc0000083
#define MSR_SYSCALL_MASK 0xc0000084

int register_syscall() {
    asm(
        "xor rax, rax;"
        "mov rdx, 0x00200008;"
        "mov ecx, %[msr_star];"
        "wrmsr;"

        "mov eax, %[fmask];"
        "xor rdx, rdx;"
        "mov ecx, %[msr_fmask];"
        "wrmsr;"

        "lea rax, [rip + syscall_entry];"
        "mov rdx, %[base] >> 32;"
        "mov ecx, %[msr_syscall];"
        "wrmsr;"
        :: [msr_star]"i"(MSR_STAR),
           [fmask]"i"(0x3f7fd5), [msr_fmask]"i"(MSR_SYSCALL_MASK),
           [base]"i"
    );
    return 0;
}