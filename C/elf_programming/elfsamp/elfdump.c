#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <sys/stat.h>
#include <sys/mman.h>

static int elfdump(void *head) {
    Elf64_Ehdr *ehdr;
    Elf64_Shdr *shdr, *shstr, *str, *sym, *rel;
    Elf64_Phdr *phdr;
    Elf64_Sym *symp;
    Elf64_Rel *relp;
    int i, j, size;
    char *sname;

    unsigned char *p = (unsigned char*)head;
    // ehdr = (const Elf64_Ehdr*)head;

    if (p[EI_MAG0] != ELFMAG0 || p[EI_MAG1] != ELFMAG1 ||
        p[EI_MAG2] != ELFMAG2 || p[EI_MAG3] != ELFMAG3) {
        fprintf(stderr, "This is not ELF file\n");
        return 1;
    }

    if (p[EI_CLASS] != ELFCLASS64) {
        fprintf(stderr, "unknown class. (%d)\n", (int)p[EI_CLASS]);
        return 1;
    }

    if (p[EI_DATA] != ELFDATA2MSB) {
        fprintf(stderr, "unknown endian. (%d)\n", (int)p[EI_DATA]);
    }

    shstr = (Elf64_Shdr *)(head + ehdr->e_shoff + ehdr->e_shentsize * ehdr->e_shstrndx);

    printf("Sections:\n");
    for (i = 0; i < ehdr->e_shnum; i++) {
        shdr = (Elf64_Phdr *)(head + ehdr->e_phoff + ehdr->e_phentsize * i);
        sname = (char *)(head + shstr->sh_offset + shdr->sh_name);
        printf("\t[%d]\t%s\n", i, sname);
        if (!strcmp(sname, ".strtab"))
            str = shdr;
    }

    printf("Segments:\n");
    for (i = 0; i < ehdr->e_phnum; i++) {
        phdr = (Elf64_Phdr *)(head + ehdr->e_phoff + ehdr->e_phentsize * i);
        printf("\t[%d]\t", i);
        for (j = 0; j < ehdr->e_shnum; j++) {
            shdr = (Elf64_Shdr *)(head + ehdr->e_shoff + ehdr->e_shentsize * j);
            size = (shdr->sh_type != SHT_NOBITS) ? shdr->sh_size : 0;
            if (shdr->sh_offset < phdr->p_offset)
                continue;
            if (shdr->sh_offset + size > phdr->p_offset + phdr->p_filesz)
                continue;
            sname = (char *)(head + shstr->sh_offset + shdr->sh_name);
            printf("%s ", sname);
        }
        printf("\n");
    }
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