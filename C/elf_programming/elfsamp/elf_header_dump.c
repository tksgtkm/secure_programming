#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <elf.h>

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void print_ident(const unsigned char *id) {
    printf("Magic: %02x %c %c %c\n", id[EI_MAG0], id[EI_MAG1], id[EI_MAG2], id[EI_MAG3]);
    printf("Class: %s\n", id[EI_CLASS] == ELFCLASS64 ? "ELF64" :
                           id[EI_CLASS] == ELFCLASS32 ? "ELF32" : "Unknown");
    printf("Data : %s-endian\n", id[EI_DATA] == ELFDATA2LSB ? "Little" :
                                  id[EI_DATA] == ELFDATA2MSB ? "Big" : "Unknown");
    printf("Ver  : %u\n", id[EI_VERSION]);
}

static const char* etype_str(uint16_t t){
    switch(t){
        case ET_NONE: return "NONE";
        case ET_REL:  return "REL";
        case ET_EXEC: return "EXEC";
        case ET_DYN:  return "DYN";
        case ET_CORE: return "CORE";
        default: return "OTHER";
    }
}

static void dump64(const Elf64_Ehdr *h){
    print_ident(h->e_ident);
    printf("Type : %s (%u)\n", etype_str(h->e_type), h->e_type);
    printf("Mach : 0x%04x\n", h->e_machine);
    printf("Entry: 0x%016lx\n", (unsigned long)h->e_entry);
    printf("PHoff: %lu\n", (unsigned long)h->e_phoff);
    printf("SHoff: %lu\n", (unsigned long)h->e_shoff);
    printf("Flags: 0x%08x\n", h->e_flags);
    printf("EHsiz: %u\n", h->e_ehsize);
    printf("PHsiz: %u  PHnum: %u\n", h->e_phentsize, h->e_phnum);
    printf("SHsiz: %u  SHnum: %u  SHstrNdx: %u\n", h->e_shentsize, h->e_shnum, h->e_shstrndx);
}

static void dump32(const Elf32_Ehdr *h){
    print_ident(h->e_ident);
    printf("Type : %s (%u)\n", etype_str(h->e_type), h->e_type);
    printf("Mach : 0x%04x\n", h->e_machine);
    printf("Entry: 0x%08x\n", h->e_entry);
    printf("PHoff: %u\n", h->e_phoff);
    printf("SHoff: %u\n", h->e_shoff);
    printf("Flags: 0x%08x\n", h->e_flags);
    printf("EHsiz: %u\n", h->e_ehsize);
    printf("PHsiz: %u  PHnum: %u\n", h->e_phentsize, h->e_phnum);
    printf("SHsiz: %u  SHnum: %u  SHstrNdx: %u\n", h->e_shentsize, h->e_shnum, h->e_shstrndx);
}

int main(int argc, char **argv){
    const char *path = (argc >= 2) ? argv[1] : "/mnt/data/elfsamp";

    int fd = open(path, O_RDONLY);
    if (fd < 0) die("open");

    struct stat st;
    if (fstat(fd, &st) < 0) die("fstat");
    if (st.st_size < (off_t)sizeof(Elf64_Ehdr)) {
        fprintf(stderr, "File too small to be an ELF\n");
        close(fd);
        return EXIT_FAILURE;
    }

    // 指定したファイルをメモリにマップして、その先頭(ファイルのオフセット0)を指すポインタを使うようにする
    // mmap()でファイルを読み取り専用でマッピングする
    void *base = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (base == MAP_FAILED) die("mmap");

    // ここがポイント：ファイルの「オフセット0」に対応するアドレスが base
    unsigned char *p = (unsigned char*)base;

    // マジック確認
    if (p[EI_MAG0] != ELFMAG0 || p[EI_MAG1] != ELFMAG1 ||
        p[EI_MAG2] != ELFMAG2 || p[EI_MAG3] != ELFMAG3) {
        fprintf(stderr, "Not an ELF file: %s\n", path);
        munmap(base, st.st_size);
        close(fd);
        return EXIT_FAILURE;
    }

    if (p[EI_CLASS] == ELFCLASS64) {
        const Elf64_Ehdr *ehdr = (const Elf64_Ehdr*)p;  // ← オフセット0を指す
        dump64(ehdr);
    } else if (p[EI_CLASS] == ELFCLASS32) {
        const Elf32_Ehdr *ehdr = (const Elf32_Ehdr*)p;  // ← オフセット0を指す
        dump32(ehdr);
    } else {
        fprintf(stderr, "Unknown ELF class\n");
        munmap(base, st.st_size);
        close(fd);
        return EXIT_FAILURE;
    }

    munmap(base, st.st_size);
    close(fd);
    return EXIT_SUCCESS;
}