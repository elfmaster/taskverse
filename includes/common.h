#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <elf.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <link.h>
#include <sys/stat.h>
#include <stdarg.h>
#include "elfhelper.h"



#define STRUCT_MEMBER_OFFSET(STRUCT, ELEMENT) (unsigned int)(uintptr_t) &((STRUCT *)NULL)->ELEMENT;


#define KSYMS "/proc/kallsyms"
#define PAGE_SIZE 4096

typedef struct node {
        struct node *next;
        struct node *prev;
        void *data;
} node_t;

typedef struct list {
        node_t *head;
        node_t *tail;
} list_t;

void * heapAlloc(size_t);
void assert_write(ssize_t);
void assert_lseek(ssize_t);
void assert_read(ssize_t); 
void assert_mmap(uint8_t *);
unsigned long get_symbol(const char *, char *);
unsigned long sysmap_lookup(char *, char *);
unsigned long kallsyms_lookup(char *);
void *memdup(void *, size_t);
char * xfmtstrdup(char *fmt, ...);
char * xstrdup(const char *s);

