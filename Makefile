CFLAGS += -g -O2 -Wall -I./libdwarf/libdwarf
LIBS	= ./lib/kdlib.a -ldwarf -lelf


all: klib dextract taskverse

klib: 
	make -C lib
dextract: dwarf_extract.c
	$(CC) $(CFLAGS) dwarf_extract.c -o dextract $(LIBDIR) $(LIBS)
	./run.sh
	#./dextract kernobj/kernobjs.ko file_struct
	./dextract kernobj/kernobjs.ko task_struct
taskverse: taskverse.c
	$(CC) $^ $(LIBS) $(LDFLAGS) $(CFLAGS) -o $@
	./run.sh
install:
	sudo cp taskverse /usr/bin
clean:
	$(RM) *.o taskverse dextract data.task_struct.h
