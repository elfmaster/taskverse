ARFLAGS	= -rcs
CFLAGS	+= -g
ARLIBS	= kallsyms.o errchk.o common.o list.o elf.o 
LIBS	= kallsyms.c errchk.c common.c list.c elf.c

all: libs kdlib

libs: $(LIBS)
	$(CC) $(CFLAGS) -c $(LIBS)
kdlib: $(ARLIBS)
	$(AR) $(ARFLAGS) $@.a $(ARLIBS)
clean:
	$(RM) *.a *.o
