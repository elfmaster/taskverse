#include "../includes/common.h"

void assert_write(ssize_t b)
{
        if (b < 0) {
                perror("write()");
                exit(-1);
        }
}

void assert_lseek(ssize_t b)
{
        if (b < 0) {
                perror("lseek()");
                exit(-1);
        }
}

void assert_read(ssize_t b)
{
        if (b < 0) {
                perror("read()");
                exit(-1);
        }
}

void assert_mmap(uint8_t *m)
{
        if (m == MAP_FAILED) {
                perror("mmap()");
                exit(-1);
        }
}

