#include "../includes/common.h"

ElfW(Addr) GetSymAddr(const char *name, Elf_t *target)
{
        ElfW(Sym) *symtab;
        char *SymStrTable;
        int i, j, symcount;

        for (i = 0; i < target->ehdr->e_shnum; i++)
                if (target->shdr[i].sh_type == SHT_SYMTAB || target->shdr[i].sh_type == SHT_DYNSYM) {
                        SymStrTable = (char *)&target->mem[target->shdr[target->shdr[i].sh_link].sh_offset];
                        symtab = (ElfW(Sym) *)&target->mem[target->shdr[i].sh_offset];
                        for (j = 0; j < target->shdr[i].sh_size / sizeof(ElfW(Sym)); j++, symtab++) {
                                if(strcmp(&SymStrTable[symtab->st_name], name) == 0)
                                        return (symtab->st_value);
                        }
                }
        return 0;
}

char * get_symbol_name(Elf_t *target, ElfW(Addr) addr)
{
        ElfW(Sym) *symtab;
        char *SymStrTable;
        int i, j, symcount;
        for (i = 0; i < target->ehdr->e_shnum; i++) {
                if (target->shdr[i].sh_type == SHT_SYMTAB || target->shdr[i].sh_type == SHT_DYNSYM) {
                        SymStrTable = (char *)&target->mem[target->shdr[target->shdr[i].sh_link].sh_offset];
                        symtab = (ElfW(Sym) *)&target->mem[target->shdr[i].sh_offset];
                        for (j = 0; j < target->shdr[i].sh_size / sizeof(ElfW(Sym)); j++, symtab++) {
                                if (symtab->st_value == addr)
                                        return (char *)&SymStrTable[symtab->st_name];
                        }
                }
        }
        return NULL;
}

uint32_t GetSymSize(const char *name, Elf_t *target)
{
        ElfW(Sym) *symtab;
        char *SymStrTable;
        int i, j, symcount;

        for (i = 0; i < target->ehdr->e_shnum; i++)
                if (target->shdr[i].sh_type == SHT_SYMTAB || target->shdr[i].sh_type == SHT_DYNSYM) {
                        SymStrTable = (char *)&target->mem[target->shdr[target->shdr[i].sh_link].sh_offset];
                        symtab = (ElfW(Sym) *)&target->mem[target->shdr[i].sh_offset];

                        for (j = 0; j < target->shdr[i].sh_size / sizeof(ElfW(Sym)); j++, symtab++) {
                                if(strcmp(&SymStrTable[symtab->st_name], name) == 0)
                                        return (symtab->st_size);
                        }
                }
        return 0;

}

int section_exists(Elf_t *target, const char *name)
{
        char *StringTable = target->StringTable;
        ElfW(Shdr) *shdr = target->shdr;
        ElfW(Ehdr) *ehdr = target->ehdr;
        int i;

        for (i = 0; i < ehdr->e_shnum; i++)
                if (!strcmp(&StringTable[shdr[i].sh_name], name))
                        return 1;
        return 0;
}


/*
 * XXX move ELF functions int libs/elf.c or something.
 */
char * get_section_name(Elf_t *target, ElfW(Addr) addr)
{
        char *StringTable = target->StringTable;
        ElfW(Shdr) *shdr = target->shdr;
        int i;

        for (i = 0; i < target->ehdr->e_shnum; i++)
                if (shdr[i].sh_addr == addr)
                        return (char *)&StringTable[shdr[i].sh_name];
        return NULL;
}

int load_live_kcore(Elf_t *elf)
{
        ElfW(Ehdr) *ehdr;
        ElfW(Phdr) *phdr;
        uint8_t *tmp;
        int fd, i, j;
        ssize_t b;
        loff_t real_text_offset, real_vm_offset;
        unsigned long text_base = get_text_base();
        if (text_base == 0) 
                text_base = GetSymAddr("_text", elf);
        
        if ((fd = open("/proc/kcore", O_RDONLY)) < 0) {
                perror("open");
                exit(-1);
        }
        
        elf->mem = malloc(4096);
        b = read(fd, elf->mem, sizeof(ElfW(Ehdr)));
        ehdr = (ElfW(Ehdr) *)elf->mem;

        /*
         * Get phdr table
         */
         
        lseek(fd, ehdr->e_phoff, SEEK_SET);
        b = read(fd, &elf->mem[ehdr->e_phoff], sizeof(ElfW(Phdr)) * ehdr->e_phnum);
        assert_read(b);
        phdr = (ElfW(Phdr) *)(elf->mem + ehdr->e_phoff);
        for (j = 0, i = 0; i < ehdr->e_phnum; i++) {
                switch(phdr[i].p_type) {
                        case PT_LOAD:
#ifdef __x86_64__
                                if (phdr[i].p_vaddr == text_base) {
#else
                                if ((phdr[i].p_vaddr & ~0x0fffffff) == 0xc0000000) {
#endif
                                        elf->textSiz = phdr[i].p_memsz;
                                        elf->textOff = ehdr->e_phoff + sizeof(ElfW(Phdr)) * ehdr->e_phnum;
                                        elf->textVaddr = phdr[i].p_vaddr;
					real_text_offset = phdr[i].p_offset;
                                        j++;
                                }
#ifdef __x86_64__
				if (phdr[i].p_vaddr == 0xffff880000100000) {
					elf->kmSiz = phdr[i].p_memsz;
					elf->kmOff = phdr[i].p_offset;
					elf->kmVaddr = phdr[i].p_vaddr;
					j++;
				}
#endif

#ifdef __x86_64__
				if (phdr[i].p_vaddr == 0xffff880100000000) {
#else
				if ((phdr[i].p_vaddr & ~0x00ffffff) == 0xf8000000) {
#endif
					elf->vmSiz = phdr[i].p_memsz;
					elf->vmOff = phdr[i].p_offset;
					real_vm_offset = phdr[i].p_offset;
					elf->vmVaddr = phdr[i].p_vaddr;
					j++;
				}
                                break;
                        case PT_NOTE:
                                break;
                }
        }
        tmp = alloca(4096);
        memcpy(tmp, elf->mem, sizeof(ElfW(Ehdr)) + sizeof(ElfW(Phdr)) * ehdr->e_phnum);
        elf->mem = mmap(NULL, elf->textSiz + 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        assert_mmap(elf->mem);
        memcpy(elf->mem, tmp, sizeof(ElfW(Ehdr)) + sizeof(ElfW(Phdr)) * ehdr->e_phnum);
        lseek(fd, real_text_offset, SEEK_SET);
        b = read(fd, &elf->mem[elf->textOff], elf->textSiz);
        assert_read(b);
	close(fd);
        elf->ehdr = ehdr;
        elf->phdr = phdr;
        elf->e_type = ehdr->e_type;
        return 0;
        
}

int load_elf_image(const char *path, Elf_t *elf)
{
        int fd;
        struct stat st;
        uint8_t *mem;
        int i, j, use_phdrs = 0;
	ElfW(Phdr) *phdr;
	ElfW(Ehdr) *ehdr;
	loff_t real_text_offset, real_vm_offset;
	unsigned long text_base = get_text_base();
        if (text_base == 0)
                text_base = GetSymAddr("_text", elf);

        elf->path = strdup(path);
        
        if ((fd = open(path, O_RDONLY)) < 0) {
                perror("open");
                return -1;
        }
        
        if (fstat(fd, &st) < 0) {       
                perror("fstat");
                return -1;
        }

        elf->mem = mem = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (mem == MAP_FAILED ) {
                perror("mmap");
                return -1;
        }

        elf->ehdr = (ElfW(Ehdr) *)mem;
        elf->phdr = (ElfW(Phdr) *)(mem + elf->ehdr->e_phoff);
        elf->shdr = (ElfW(Shdr) *)(mem + elf->ehdr->e_shoff);
	phdr = elf->phdr;
	ehdr = elf->ehdr;

	if (elf->ehdr->e_shstrndx == SHN_UNDEF || elf->ehdr->e_shnum == 0 || elf->ehdr->e_shoff == 0)
		use_phdrs++;
	
	if (use_phdrs) 
		goto parse_phdr;
		
        elf->StringTable = (char *)&elf->mem[elf->shdr[elf->ehdr->e_shstrndx].sh_offset];
	for (i = 0; i < elf->ehdr->e_shnum; i++) {
                if (!strcmp(&elf->StringTable[elf->shdr[i].sh_name], ".text") || !strcmp(&elf->StringTable[elf->shdr[i].sh_name], ".kernel")) {
                        elf->textOff = elf->shdr[i].sh_offset;
                        break;
                }
        }
	goto done;
	
	parse_phdr:
	        for (j = 0, i = 0; i < elf->ehdr->e_phnum; i++) {
                	switch(phdr[i].p_type) {
                        case PT_LOAD:
#ifdef __x86_64__
                                if (phdr[i].p_vaddr == text_base) {
#else
                                if ((phdr[i].p_vaddr & ~0x0fffffff) == 0xc0000000) {
#endif
                                        elf->textSiz = phdr[i].p_memsz;
                                        elf->textOff = ehdr->e_phoff + sizeof(ElfW(Phdr)) * ehdr->e_phnum;
                                        elf->textVaddr = phdr[i].p_vaddr;
                                        real_text_offset = phdr[i].p_offset;
                                        j++;
                                }
#ifdef __x86_64__
                                if (phdr[i].p_vaddr == 0xffff880000100000) {
                                        elf->kmSiz = phdr[i].p_memsz;
                                        elf->kmOff = phdr[i].p_offset;
                                        elf->kmVaddr = phdr[i].p_vaddr;
                                        j++;
                                }
#endif

#ifdef __x86_64__
                                if (phdr[i].p_vaddr == 0xffff880100000000) {
#else
                                if ((phdr[i].p_vaddr & ~0x00ffffff) == 0xf8000000) {
#endif
                                        elf->vmSiz = phdr[i].p_memsz;
                                        elf->vmOff = phdr[i].p_offset;
                                        real_vm_offset = phdr[i].p_offset;
                                        elf->vmVaddr = phdr[i].p_vaddr;
                                        j++;
                                }
                                break;
                        case PT_NOTE:
                                break;
                }
        }

done:
        elf->fd = fd;
        elf->st = st;
        elf->e_type = elf->ehdr->e_type;
        return 0;
}

