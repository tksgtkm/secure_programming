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

    char *StringTable;
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

typedef struct calldata {
    char *symname;
    char *string;
    unsigned long vaddr;
    unsigned long retaddr;
    breakpoint_t breakpoint;
} calldata_t;

typedef struct callstack {
    calldata_t *calldata;
    unsigned int depth;
} callstack_t;

struct call_list {
    char *callstring;
    struct call_list *next;
};

#define MAX_SHRDS 256

struct handle {
    char *path;
    char **args;
    uint8_t *map;
    struct elf32 *elf32;
    struct elf64 *elf64;
    struct elf_section_range sh_range[MAX_SHRDS];
    struct syms lsyms[MAX_SYMS];
    struct syms dsyms[MAX_SYMS];
    char *libnames[256];
    int lsc;
    int dsc;
    int lnc;
    int shdr_count;
    int pid;
};

int global_pid;

void *HeapAlloc(unsigned int);
char *xstrdup(const char *);

void set_breakpoint(callstack_t *callstack) {
    int status;
    long orig = ptrace(PTRACE_PEEKTEXT, global_pid, callstack->calldata[callstack->depth].retaddr);
    long trap;

    trap = (orig & ~0xff) | 0xcc;
    if (opts.verbose)
        printf("[+] Setting breakpoint on 0x%ls\n", callstack->calldata[callstack->depth].retaddr);
    
    ptrace(PTRACE_POKETEXT, global_pid, callstack->calldata[callstack->depth].retaddr, trap);
    callstack->calldata[callstack->depth].breakpoint.orig_code = orig;
    callstack->calldata[callstack->depth].breakpoint.vaddr = callstack->calldata[callstack->depth].retaddr;
}

void remove_breakpoint(callstack_t *callstack) {
    int status;
    if (opts.verbose)
        printf("[+] Removing breakpoint from 0x%lx\n", callstack->calldata[callstack->depth].retaddr);
    
    ptrace(
        PTRACE_POKETEXT, global_pid,
        callstack->calldata[callstack->depth].retaddr,
        callstack->calldata[callstack->depth].breakpoint.orig_code
    );
}

void callstack_init(callstack_t *callstack) {
    callstack->calldata = (calldata_t *)HeapAlloc(sizeof(calldata_t) * CALLSTACK_DEPTH);
    callstack->depth = -1;
}

void callstack_push(callstack_t *callstack, calldata_t *calldata) {
    memcpy(&callstack->calldata[++callstack->depth], calldata, sizeof(calldata_t));
    set_breakpoint(callstack);
}

calldata_t *calldata_pop(callstack_t *callstack) {
    if (callstack->depth == -1)
        return NULL;
    
    remove_breakpoint(callstack);
    return (&callstack->calldata[callstack->depth--]);
}

calldata_t *callstack_peek(callstack_t *callstack) {
    if (callstack->depth == -1)
        return NULL;
    
    return &callstack->calldata[callstack->depth];
}

struct call_list *add_call_string(struct call_list **head, const char *string) {
    struct call_list *tmp = (struct call_list *)HeapAlloc(sizeof(struct call_list));

    tmp->callstring = (char *)xstrdup(string);
    tmp->next = *head;
    *head = tmp;

    return *head;
}

void clear_call_list(struct call_list **head) {
    struct call_list *tmp;

    if (!head)
        return;
    
    while (*head != NULL) {
        tmp = (*head)->next;
        free(*head);
        *head = tmp;
    }
}

struct branch_instr *search_branch_instr(uint8_t instr) {
    int i;
    struct branch_instr *p, *ret;

    for (i = 0, p = branch_table; p->mnemonic != NULL; p++, i++) {
        if (instr == p->opcode)
            return p;
    }

    return NULL;
}

void print_call_list(struct call_list **head) {
    if (!head)
        return;
    
    while (*head != NULL) {
        fprintf(stdout, "%s", (*head)->callstring);
        head = &(*head)->next;
    }
}

void *HeapAlloc(unsigned int len) {
    uint8_t *mem = malloc(len);
    if (!mem) {
        perror("malloc");
        exit(-1);
    }

    return mem;
}

char *xstrdup(const char *s) {
    char *p = strdup(s);
    if (p == NULL) {
        perror("strdup");
        exit(-1);
    }
    return p;
}

char *xfmtstrdup(char *fmt, ...) {
    char *s, buf[512];
    va_list va;

    va_start(va, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va);
    s = xstrdup(buf);

    return s;
}

int pid_read(int pid, void *dst, const void *src, size_t len) {
    int sz = len / sizeof(void *);
    int rem = len % sizeof(void *);
    unsigned char *s = (unsigned char *)src;
    unsigned char *d = (unsigned char *)dst;
    long word;

    while (sz-- != 0) {
        word = ptrace(PTRACE_PEEKTEXT, pid, s, NULL);
        if (word == -1 && errno)
            return -1;
        
        *(long *)d = word;
        s += sizeof(long);
        d += sizeof(long);
    }

    return 0;
}

int BuildSyms(struct handle *h) {
    unsigned int i, j, k;
    char *SymStrTable;
    Elf32_Ehdr *ehdr32;
    Elf32_Shdr *shdr32;
    Elf32_Sym *symtab32;
    Elf64_Ehdr *ehdr64;
    Elf64_Shdr *shdr64;
    Elf64_Sym *symtab64;
    int st_type;

    h->lsc = 0;
    h->dsc = 0;

    switch(opts.arch) {
        case 32:
            ehdr32 = h->elf32->ehdr;
            shdr32 = h->elf32->shdr;

            for (i = 0; i < ehdr32->e_shnum; i++) {
                if (shdr32[i].sh_type == SHT_SYMTAB || shdr32[i].sh_type == SHT_DYNSYM) {
                    SymStrTable = (char *)&h->map[shdr32[shdr32[i].sh_link].sh_offset];
                    symtab32 = (Elf32_Sym *)&h->map[shdr32[i].sh_offset];

                    for (j = 0; j < shdr32[i].sh_size / sizeof(Elf32_Sym); j++, symtab32++) {
                        st_type = ELF32_ST_TYPE(symtab32->st_info);
                        if (st_type != STT_FUNC)
                            continue;

                        switch(shdr32[i].sh_type) {
                            case SHT_SYMTAB:
                                h->lsyms[h->lsc].name = xstrdup(&SymStrTable[symtab32->st_name]);
                                h->lsyms[h->lsc].value = symtab32->st_value;
                                h->lsc;
                                break;
                            case SHT_DYNSYM:
                                h->dsyms[h->dsc].name = xstrdup(&SymStrTable[symtab32->st_name]);
                                h->dsyms[h->dsc].value = symtab32->st_value;
                                h->dsc++;
                                break;
                        }
                    }
                }
            }

            h->elf32->StringTable = (char *)&h->map[shdr32[ehdr32->e_shstrndx].sh_offset];
            for (i = 0; i < ehdr32->e_shnum; i++) {
                if (!strcmp(&h->elf32->StringTable[shdr32[i].sh_name], ".plt")) {
                    for (k = 0, j = 0; j < shdr32[i].sh_size; j += 16) {
                        if (j >= 16) {
                            h->dsyms[k++].value = shdr32[i].sh_addr + j;
                        }
                    }
                    break;
                }
            }
            break;
    }
}

void sighandle(int sig) {
    fprintf(stdout, "Caught signal ctrl-C, detaching...\n");
    ptrace(PTRACE_DETACH, global_pid, NULL, NULL);
    exit(0);
}

int main(int argc, char **argv, char **envp) {
    int opt, i, pid, status, skip_getopt = 0;
    struct handle handle;
    char **p, *arch;

    struct sigaction act;
    sigset_t set;
    act.sa_handler = sighandle;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, NULL);
    sigemptyset(&set);
    sigaddset(&set, SIGINT);

    if (argc < 2) {
usage:
        printf("Usage: %s [-p <pid>] [-Sstve] <prog>\n", argv[0]);
		printf("[-p] Trace by PID\n");
		printf("[-t] Type detection of function args\n");
		printf("[-s] Print string values\n");
	//	printf("[-r] Show return values\n");
		printf("[-v] Verbose output\n");
		printf("[-e] Misc. ELF info. (Symbols,Dependencies)\n");
		printf("[-S] Show function calls with stripped symbols\n");
		printf("[-C] Complete control flow analysis\n");
		exit(0);
    }

    if (argc == 2 && argv[1][0] == '-')
        goto usage;
    
    memset(&opts, 0, sizeof(opts));

    opts.arch = 64;
    arch = getenv(FTRACE_ENV);
    if (arch != NULL) {
        switch(atoi(arch)) {
            case 32:
                opts.arch = 32;
                break;
            case 64:
                opts.arch = 64;
                break;
            default:
                fprintf(stderr, "Unknown architecture: %s\n", arch);
                break;
        }
    }

    if (argv[1][0] != '-') {
        handle.path = xstrdup(argv[1]);
        handle.args = (char **)HeapAlloc(sizeof(char *) * argc - 1);

        for (i = 0, p = &argv[1]; i != argc - 1; p++, i++) {
            *(handle.args + i) = xstrdup(*p);
        }
        *(handle.args + i) = NULL;
        skip_getopt = 1;
    } else {
        handle.path = xstrdup(argv[2]);
        handle.args = (char **)HeapAlloc(sizeof(char *) * argc - 1);

        for (i = 0, p = &argv[2]; i != argc - 2; p++, i++) {
            *(handle.args + i) = xstrdup(*p);
        }
        *(handle.args + i) = NULL;
    }

    while ((opt = getopt(argc, argv, "CSrhtvep:s")) != -1) {
        switch(opt) {
            case 'S':
                opts.stripped++;
                break;
            case 'r':
                opts.showret++;
                break;
            case 'v':
                opts.verbose;
                break;
            case 'e':
                opts.elfinfo;
                break;
            case 't':
                opts.typeinfo;
                break;
            case 'p':
                opts.attach++;
                handle.pid = atoi(optarg);
                break;
            case 's':
                opts.getstr++;
                break;
            case 'C':
                opts.cflow++;
                break;
            case 'h':
                goto usage;
            default:
                printf("Unknown option\n");
                exit(0);
        }
    }

begin:

    if (opts.verbose) {
        switch(opts.arch) {
            case 32:
                printf("[+] 32bit ELF mode enabled!\n");
                break;
            case 64:
                printf("[+] 64bit ELF mode enabled!\n");
                break;
        }
        if (opts.typeinfo) {
            printf("[+] Pointer type prediction enabled\n");
        }
    }

    if (opts.arch == 32 && opts.typeinfo) {
        printf("[!] Option -t may not be used on 32bit executables");
        exit(0);
    }

    if (opts.arch == 32 && opts.getstr) {
        printf("[!] Option -s may not be used on 32bit executables\n");
        exit(0);
	}

	if (opts.getstr && opts.typeinfo) {
		printf("[!] Options -t and -s may not be used together\n");
		exit(0);
	}
}