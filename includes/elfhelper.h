
typedef struct elf_type {
        uint8_t *mem;
 	uint8_t *vmalloc_mem;
	uint16_t e_type;
        ElfW(Ehdr) *ehdr;
        ElfW(Phdr) *phdr;
        ElfW(Shdr) *shdr;
        ElfW(Sym) *sym;
        ElfW(Off) textOff, vmOff, kmOff;
        size_t textSiz, vmSiz, kmSiz;
        ElfW(Addr) entry;
	ElfW(Addr) textVaddr, vmVaddr, kmVaddr;
        char *StringTable;
        size_t size;
        int fd;
        struct stat st;
        char *path;
} Elf_t;


ElfW(Addr) GetSymAddr(const char *name, Elf_t *target);
char * get_symbol_name(Elf_t *target, ElfW(Addr) addr);
uint32_t GetSymSize(const char *name, Elf_t *target);
int section_exists(Elf_t *target, const char *name);
char * get_section_name(Elf_t *target, ElfW(Addr) addr);
int load_live_kcore(Elf_t *elf);
int load_elf_image(const char *path, Elf_t *elf);

