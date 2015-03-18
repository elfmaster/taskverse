#include "../includes/common.h"



ElfW(Addr) get_text_end(void)
{
        return kallsyms_lookup("_etext");
}

ElfW(Addr) get_text_base(void)
{
        return kallsyms_lookup("_text");
}

void * heapAlloc(size_t len)
{
	uint8_t *p = malloc(len);
	if (p == NULL) {
		perror("malloc");
		exit(-1);
	}
	return (void *)p;
}

void * memdup(void *src, size_t len)
{
	uint8_t *ptr = heapAlloc(len);
	memcpy(ptr, src, len);
	return (void *)ptr;
}


void xfree(void *p)
{
	if (p != NULL)
		free(p);
}

char * _strdupa(const char *s)
{
	char *p = alloca(strlen(s) + 1);
	strcpy(p, s);
	return p;
}

char * xstrdup(const char *s)
{
        char *p = strdup(s);
        if (p == NULL) {
                perror("strdup");
                exit(-1);
        }
        return p;
}
        
char * xfmtstrdup(char *fmt, ...)
{
        char *s, buf[512];
        va_list va;
        
        va_start (va, fmt);
        vsnprintf (buf, sizeof(buf), fmt, va);
        s = xstrdup(buf);
        
        return s;
}

