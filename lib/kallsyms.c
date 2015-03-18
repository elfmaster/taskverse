#include "../includes/common.h"


unsigned long get_symbol(const char *name, char *path)
{
        FILE *fd;
        char symbol_s[255];
        int i;
        unsigned long address, next_symaddr;
        char tmp[255], type, tmp_type;

        if ((fd = fopen(path, "r")) == NULL)
        {
                printf("[TOOL_FAILURE] fopen %s %s\n", path, strerror(errno));
                exit(-1);
        }

        while(!feof(fd))
        {
                bzero((char *)symbol_s, 255);
                fscanf(fd, "%lx %c %s", &address, &type, symbol_s);

                if (strcmp(symbol_s, name) == 0)
                {
                        fscanf(fd, "%lx %c %s", &next_symaddr, &tmp_type, tmp);
                        fclose(fd);
                        return address;
                }
                /* kallsyms isn't a regular file, meaning there is no EOF */
                if (!strcmp(symbol_s, ""))
                        break;
        }
        return 0;
}

		
unsigned long kallsyms_lookup(char *name)
{
	return get_symbol(name, KSYMS);
}

unsigned long sysmap_lookup(char *name, char *sysmap)
{
	return get_symbol(name, sysmap);
}

