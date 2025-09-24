#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <sys/stat.h>
#include <sys/mman.h>

static int elfdump(void *head) {
    Elf64_Ehdr *ehdr;
    Elf64_Shdr *shdr, *str, *sym, *rel;
    Elf64_Phdr *phdr;
    Elf64_Sym *symp;
    Elf64_Rel *relp;
    int i, j, size;
    char *sname;

    // ehdr = (Elf64_Ehdr *)head;
    unsigned char *p = (unsigned char*)head;

    if (p[EI_MAG0] != ELFMAG0 || p[EI_MAG1] != ELFMAG1 ||
        p[EI_MAG2] != ELFMAG2 || p[EI_MAG3] != ELFMAG3) {
        fprintf(stderr, "This is not ELF file\n");
        return 1;
    }

    if (p[EI_CLASS] != ELFCLASS64) {
        fprintf(stderr, "unknown class. (%d)\n", (int)p[EI_CLASS]);
    }

    // if (p[EI_DATA] != ELFDATA2MSB)
}

int main(int argc, char *argv[]) {
    int fd;
    struct stat sb;

    fd = open(argv[1], O_RDONLY);
    if (fd < 0)
        exit(1);
    fstat(fd, &sb);
    void *head = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    elfdump(head);
    munmap(head, sb.st_size);
    close(fd);

    exit(1);
}