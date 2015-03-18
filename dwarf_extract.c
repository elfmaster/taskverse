/*
 * This code is an absolute mess and was
 * hacked together in an hour-- needs some
 * attention.
 *
 * kernelDetective 2014 (C) Bitlackeys
 * elfmaster [at] zoho.com
 */

#include "./includes/common.h"
#include <libdwarf.h>
#include <dwarf.h>

struct srcfilesdata {
	char **srcfiles;
	Dwarf_Signed srcfilescount;
	int srcfilesres;
};

struct {
	void *mem;
	char *name;
	size_t totsize;
} globals;

struct list_head {
        struct list_head *next, *prev;
};

int member_count = 0;
struct some_data_struct {
	char *name;
	unsigned long offset;
	size_t member_size;
	int bit_size;
} data_structure[4096] __attribute__((section(".data"))) = { [0 ... 4095] = 0 };


static ssize_t get_dwarf_attr_size(Dwarf_Debug dbg, Dwarf_Die die, int level)
{
	Dwarf_Attribute attrib = 0;
	Dwarf_Half attr = DW_AT_byte_size;
	Dwarf_Unsigned uval = 0;
	ssize_t retval;
	int res;
	
	Dwarf_Error err = 0;
	dwarf_attr(die, DW_AT_byte_size, &attrib, &err);
	
	res = dwarf_whatattr(attrib, &attr, &err); 
	res = dwarf_formudata(attrib, &uval, &err);
	retval = (ssize_t)uval;
	if (res != DW_DLV_OK)
		return -1;
	return retval;
}

static int get_dwarf_attr_bitsize(Dwarf_Debug dbg, Dwarf_Die die, int level)
{
        Dwarf_Attribute attrib = 0;
        Dwarf_Half attr = DW_AT_byte_size;
        Dwarf_Unsigned uval = 0;
        ssize_t retval;
        int res;

        Dwarf_Error err = 0;
        dwarf_attr(die, DW_AT_byte_size, &attrib, &err);

        res = dwarf_whatattr(attrib, &attr, &err);
        res = dwarf_formudata(attrib, &uval, &err);
        retval = uval;
        if (res != DW_DLV_OK)
                return -1;
        return retval;
}

static unsigned long get_dwarf_attr_member_offset(Dwarf_Debug dbg, Dwarf_Die die, int level)
{
	Dwarf_Attribute attrib = 0;
        Dwarf_Half attr = DW_AT_data_member_location;
        Dwarf_Unsigned uval = 0;
        unsigned long retval;
        int res;

	Dwarf_Error err = 0;
	dwarf_attr(die, attr, &attrib, &err);
	res = dwarf_whatattr(attrib, &attr, &err);
	res = dwarf_formudata(attrib, &uval, &err);
	retval = (unsigned long)uval;
	if (res != DW_DLV_OK)
		return -1;
	return retval;
}



static void
print_die_data(Dwarf_Debug dbg, Dwarf_Die print_me, int level,
	       struct srcfilesdata *sf)
{
	char *name = 0;
	Dwarf_Error error = 0;
	Dwarf_Half tag = 0;
	const char *tagname = 0;
	int localname = 0;
	static int depth = 0;
	static int too_far = 0;

	depth++;

	int res = dwarf_diename(print_me, &name, &error);
	if (res == DW_DLV_ERROR) {
		printf("Error in dwarf_diename , level %d \n", level);
		exit(1);
	}
	if (res == DW_DLV_NO_ENTRY) {
		name = "<no DW_AT_name attr>";
		localname = 1;
	}
	res = dwarf_tag(print_me, &tag, &error);
	if (res != DW_DLV_OK) {
		printf("Error in dwarf_tag , level %d \n", level);
		exit(1);
	}
	res = dwarf_get_TAG_name(tag, &tagname);
	if (res != DW_DLV_OK) {
		printf("Error in dwarf_get_TAG_name , level %d \n", level);
		exit(1);
	}
	
	unsigned long member_offset = get_dwarf_attr_member_offset(dbg, print_me, level);
	ssize_t size = get_dwarf_attr_size(dbg, print_me, level);
	if (size == -1)
		size = 0;
	int bsize = get_dwarf_attr_bitsize(dbg, print_me, level);
	
	/*
	 * If we are past DW_TAG_structure_type "task_struct"
	 * and we are also now past the members of it, then we
	 * are done.
	 */
	if (depth > 1 && tag != 13) {
		too_far = 1; // 13 is DW_TAG_member
		goto done;
	}
	if (too_far)
		goto done;

	printf("<%d> tag: %d %s  name: \"%s\" size: 0x%lx offset: %lx\n", level, tag, tagname, name, size, member_offset);
	if (member_count == 0)
		globals.totsize = size;
	data_structure[member_count].name = strdup(name);
	data_structure[member_count].member_size = size; // only task_struct itself will have appropriate size here, otherwise look 2 lines down
	data_structure[member_count].offset = member_offset;
	data_structure[member_count].bit_size = bsize;
	member_count++;
	
done:
	if (!localname) {
		dwarf_dealloc(dbg, name, DW_DLA_STRING);
	}
}

static void get_die_and_siblings(Dwarf_Debug dbg, Dwarf_Die in_die,
				 int in_level, struct srcfilesdata *sf)
{
	int res = DW_DLV_ERROR;
	Dwarf_Die cur_die = in_die;
	Dwarf_Die child = 0;
	Dwarf_Error error;
	static ssize_t found_struct = 0;
	Dwarf_Half tag = 0;

	//print_die_data(dbg, in_die, in_level, sf);
	
	for (;;) {
		Dwarf_Die sib_die = 0;
		res = dwarf_child(cur_die, &child, &error);
		if (res == DW_DLV_ERROR) {
			printf("Error in dwarf_child , level %d \n", in_level);
			exit(1);
		}
		if (res == DW_DLV_OK) {
			get_die_and_siblings(dbg, child, in_level + 1, sf);
		}
		res = dwarf_siblingof(dbg, cur_die, &sib_die, &error);
		if (res == DW_DLV_ERROR) {
			printf("Error in dwarf_siblingof , level %d \n",
			       in_level);
			exit(1);
		}
		if (res == DW_DLV_NO_ENTRY) {
			break;
		}
		if (cur_die != in_die) {
			dwarf_dealloc(dbg, cur_die, DW_DLA_DIE);
		}
		cur_die = sib_die;
		char *name = 0;
		dwarf_diename(cur_die, &name, &error);
		dwarf_tag(cur_die, &tag, &error);
		if (name == NULL)
			continue;
		if (found_struct == 0)
			if (!strcmp(name, globals.name)) 
				found_struct++;
		//printf("found_struct: %d in_level: %d\n", found_struct, in_level);
		if (found_struct == 1 || (found_struct > 1 && tag == 13 && in_level == 2)) 
			print_die_data(dbg, cur_die, in_level, sf); 
		else
			found_struct = 0;
	}

	return;
}

static int read_cu_list(Dwarf_Debug dbg)
{
	Dwarf_Unsigned cu_header_length = 0;
	Dwarf_Half version_stamp = 0;
	Dwarf_Unsigned abbrev_offset = 0;
	Dwarf_Half address_size = 0;
	Dwarf_Unsigned next_cu_header = 0;
	Dwarf_Error error;
	int cu_number = 0;

	for (;; ++cu_number) {
		struct srcfilesdata sf;
		sf.srcfilesres = DW_DLV_ERROR;
		sf.srcfiles = 0;
		sf.srcfilescount = 0;
		Dwarf_Die no_die = 0;
		Dwarf_Die cu_die = 0;
		int res = DW_DLV_ERROR;
		res =
		    dwarf_next_cu_header(dbg, &cu_header_length,
					 &version_stamp, &abbrev_offset,
					 &address_size, &next_cu_header,
					 &error);
		if (res == DW_DLV_ERROR) {
			printf("Error in dwarf_next_cu_header\n");
			exit(1);
		}
		if (res == DW_DLV_NO_ENTRY) {
			/* Done. */
			return 0;
		}
		/* The CU will have a single sibling, a cu_die. */
		res = dwarf_siblingof(dbg, no_die, &cu_die, &error);
		if (res == DW_DLV_ERROR) {
			printf("Error in dwarf_siblingof on CU die \n");
			exit(1);
		}
		if (res == DW_DLV_NO_ENTRY) {
			/* Impossible case. */
			printf("no entry! in dwarf_siblingof on CU die \n");
			exit(1);
		}
		get_die_and_siblings(dbg, cu_die, 0, &sf);
		dwarf_dealloc(dbg, cu_die, DW_DLA_DIE);
		//resetsrcfiles(dbg, &sf);
	}
}

static int get_int_size(int int_sz)
{
	switch(int_sz) {
		case 1:
			return 8;
		case 2:
			return 16;
		case 4:
			return 32;
		case 8:
			return 64;
	}
	printf("int_sz: %d\n", int_sz);
	return 0;
}

int main(int argc, char **argv)
{
	int fd = -1;
	int res = DW_DLV_ERROR;
	Dwarf_Error error;
	Dwarf_Handler errhand = 0;
	Dwarf_Ptr errarg = 0;
	Dwarf_Debug dbg = 0;
	FILE *fp;
	
	if (argc < 3) {
		printf("Usage: %s <file> <struct>\n", argv[0]);
		exit(0);
	}
	globals.name = strdup(argv[2]);

	if ((fd = open(argv[1], O_RDONLY)) < 0) {
		perror("open");
		exit(-1);
	}

	res = dwarf_init(fd, DW_DLC_READ, errhand, errarg, &dbg, &error);
	if (res != DW_DLV_OK) {
		printf("Giving up, cannot do DWARF processing\n");
		exit(1);
	}

	read_cu_list(dbg);
	res = dwarf_finish(dbg, &error);
	if (res != DW_DLV_OK) {
		printf("dwarf_finish failed!\n");
	}
	close(fd);
	int i;
	for (i = 0; i < member_count; i++) {
		if (i > 0)
			data_structure[i].member_size = data_structure[i + 1].offset - data_structure[i].offset;
		if (i == member_count - 1)
			data_structure[i].member_size = globals.totsize - data_structure[i].offset;
	}
	char data_struct_file[256];
	snprintf(data_struct_file, sizeof(data_struct_file), "data.%s.h", argv[2]);
	if ((fp = fopen(data_struct_file, "w+")) == NULL) {
		perror("fopen");
		exit(-1);
	}
	
	fprintf(fp, "typedef struct { char x[1]; } packed_pad_t;\n");
	fprintf(fp, "/* task_struct is %d bytes */\nstruct %s {\n", (int)globals.totsize, globals.name);

	fflush(fp);
	unsigned int padding_count, array_count, flag_count;
	if (!strcmp(argv[2], "task_struct")) {
		if (!strcmp(data_structure[1].name, "stack"))
                	fprintf(fp, "\tvolatile long state;\n");

		for (flag_count = 0, padding_count = 0, array_count = 0, i = 0; i < member_count; i++) {
#if DEBUG
			printf("Name: %s offset: %lx size: %d\n", data_structure[i].name, data_structure[i].offset, (int)data_structure[i].member_size);
#endif
			if (i == 0)
				continue;
		/*
	 	 * Types that we need to have correct (Instead of just uintSIZ_t)
	 	 * we will directly create with the correct type, such as char or struct list_head
		 */
			if (strcmp(data_structure[i].name, "comm") == 0)
				fprintf(fp, "\tchar comm[%d];\n", (int)data_structure[i].member_size);
			else
			if (strcmp(data_structure[i].name, "tasks") == 0)
				fprintf(fp, "\tstruct list_head tasks;\n");
			else
			if (strcmp(data_structure[i].name, "children") == 0)
				fprintf(fp, "\tstruct list_head children;\n");
			else
			if (strcmp(data_structure[i].name, "sibling") == 0)
				fprintf(fp, "\tstruct list_head sibling;\n");
			else
			if (strcmp(data_structure[i].name, "active_mm") == 0)
				fprintf(fp, "\t//uint%d_t %s;\t/* We pad this out to counteract a dwarf code bug */\n", 
				(int)get_int_size(data_structure[i].member_size), data_structure[i].name);
			else
			if (data_structure[i].member_size == 0) // must be a bit flag I.E unsigned var:1
				fprintf(fp, "\tunsigned gcc_flag_ext_%d:%d;\n", flag_count++, data_structure[i].bit_size);
			else
			if (data_structure[i].member_size <= 8) 
				//fprintf(fp, "\tuint%d_t padding_%d;\n", get_int_size(data_structure[i].member_size), padding_count++);
				fprintf(fp, "\tuint%d_t %s;\n", (int)get_int_size(data_structure[i].member_size), data_structure[i].name);
			else
				fprintf(fp, "\tpacked_pad_t array_pad_%d[%d];\n", array_count++, (int)data_structure[i].member_size);
			//fprintf(fp, "%s:%d:%d\n", data_structure[i].name, data_structure[i].offset, data_structure[i].member_size);
			fflush(fp);
		}
	} 
	fflush(fp);
	fprintf(fp, "\n};");
	fclose(fp);
	printf("data structure has been written to %s\n", data_struct_file);
	return 0;
}
