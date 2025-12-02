#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <sys/reg.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>

#define WHITE         "\x1B[37m"
#define RED           "\x1B[31m"
#define GREEN         "\x1B[32m"
#define YELLOW        "\x1B[33m"
#define DEFAULT_COLOR "\x1B[0m"

#define MAX_SYMS 8192 * 2

#define FTRACE_ENV "FTRACE_ARCH"

#define MAX_ADDR_SPACE 256
#define MAXSTR 512

#define TEXT_SPACE  0
#define DATA_SPACE  1
#define STACK_SPACE 2
#define HEAP_SPACE  3

#define CALLSTACK_DEPTH 0xf4240

struct branch_instr {
    char *mnemonic;
    uint8_t opcode;
};

#define BRANCH_INSTR_LEN_MAX 5

struct branch_instr branch_table[64] = {
    {"jo",  0x70}, 
	{"jno", 0x71},  {"jb", 0x72},  {"jnae", 0x72},  {"jc", 0x72},  {"jnb", 0x73},
	{"jae", 0x73},  {"jnc", 0x73}, {"jz", 0x74},    {"je", 0x74},  {"jnz", 0x75},
	{"jne", 0x75},  {"jbe", 0x76}, {"jna", 0x76},   {"jnbe", 0x77}, {"ja", 0x77},
	{"js",  0x78},  {"jns", 0x79}, {"jp", 0x7a},	{"jpe", 0x7a}, {"jnp", 0x7b},
	{"jpo", 0x7b},  {"jl", 0x7c},  {"jnge", 0x7c},  {"jnl", 0x7d}, {"jge", 0x7d},
	{"jle", 0x7e},  {"jng", 0x7e}, {"jnle", 0x7f},  {"jg", 0x7f},  {"jmp", 0xeb},
	{"jmp", 0xe9},  {"jmpf", 0xea}, {NULL, 0}
};

struct elf_section_range {
    char *sh_name;
    unsigned long sh_addr;
    unsigned int sh_size;
};

struct {
    int stripped;
    int callsite;
    int showret;
    int attach;
    int verbose;
    int elfinfo;
    int typeinfo;
    int getstr;
    int arch;
    int cflow;
} opts;

struct elf64 {
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdr;
    Elf64_Shdr *shdr;
    Elf64_Sym  *sym;
    Elf64_Dyn  *dyn;

    char *StringTable;
    char *SymStringTable;
};

struct elf32 {
    Elf32_Ehdr *ehdr;
    Elf32_Phdr *phdr;
    Elf32_Shdr *shdr;
    Elf32_Sym  *sym;
    Elf32_Dyn  *dyn;

    char *stringTable;
    char *SymStringTable;
};

struct address_space {
    unsigned long svaddr;
    unsigned long evaddr;
    unsigned int size;
    int count;
};

struct syms {
    char *name;
    unsigned long value;
};

typedef struct breakpoint {
    unsigned long vaddr;
    long orig_code;
} breakpoint_t;