
/*
 * This code is responsible for finding hidden processes. It does this
 * by compiling a kernel module 'kernobj/kernobjs.ko' that has 'struct task_struct'
 * in it. It then parses the dwarf information and reconstructs that
 * kernel structure into userland by creating data.task_struct.h which contains
 * an equivalent version of this that can be used from userspace. It then
 * opens and parses /proc/kcore (or a kcore snapshot) and walks the tasklist
 * using a modified version of the kernels linked list that works with kcore
 * offsets in userland.
 * elfmaster[at]zoho.com - 2014
*/

#define _GNU_SOURCE

#include "./includes/common.h"
#include "./includes/list.h"
#include "data.task_struct.h"
#include <dirent.h>

#define INIT_TASK "init_task"
#define TASK_COMM_LEN 16


struct proc_pids {
	int pid;
	char *name;
}; 

static struct {
	Elf_t *kcore;
} global;

int kfd;
void *kcore_translate_km(ElfW(Addr));
	
/*
 * Memory within text and data of kernel 
 * NOTE: On x86_32 this segment is much larger
 * in kcore and contains text, data, kernel modules, and even some malloc'd memory
 *
 */
void * kcore_translate_kernel(ElfW(Addr) addr)
{
	Elf_t *kcore = global.kcore;
	uint8_t *ptr = &kcore->mem[kcore->textOff + (addr - kcore->textVaddr)];
	return ptr;
}

/* memory within vmalloc()'d areas
 */
void * kcore_translate_vm(ElfW(Addr) addr)
{
        Elf_t *kcore = global.kcore;
	static char *vmem = NULL;
	
	
	/*
	 * If the address isn't in vmalloc memory then try kmalloc
	 */
	if (addr < kcore->vmVaddr || addr > (kcore->vmVaddr + kcore->vmSiz)) {
		return (kcore_translate_km(addr));
	}
	if (vmem)
		free(vmem);
	vmem = (char *)heapAlloc(8192);
	
	ssize_t b = lseek(kfd, kcore->vmOff + (addr - kcore->vmVaddr), SEEK_SET);
	assert_lseek(b);
	b = read(kfd, vmem, 8192);
	assert_read(b);
	return vmem;
}

/*
 * kmalloc'd memory areas
 */

void * kcore_translate_km(ElfW(Addr) addr)
{
        Elf_t *kcore = global.kcore;
        static char *vmem = NULL;

        if (addr < kcore->kmVaddr || addr > (kcore->kmVaddr + kcore->kmSiz)) {
                printf("Address %lx is out of range\n", addr);
                exit(-1);
        }

        if (vmem)
                free(vmem);
        vmem = (char *)heapAlloc(8192);

        ssize_t b = lseek(kfd, kcore->kmOff + (addr - kcore->kmVaddr), SEEK_SET);
        assert_lseek(b);
        b = read(kfd, vmem, 8192);
        assert_read(b);
        return vmem;
}

#define TASK_UNRUNNABLE		-1
#define TASK_RUNNING            0
#define TASK_INTERRUPTIBLE      1
#define TASK_UNINTERRUPTIBLE    2
#define TASK_ZOMBIE             4
#define TASK_STOPPED            8 

#if 0
               D    uninterruptible sleep (usually IO)
               R    running or runnable (on run queue)
               S    interruptible sleep (waiting for an event to complete)
               T    stopped, either by a job control signal or because it is being traced
               W    paging (not valid since the 2.6.xx kernel)
               X    dead (should never be seen)
               Z    defunct ("zombie") process, terminated but not reaped by its parent
#endif

static char task_state(int state)
{
	switch(state) {
		case TASK_UNINTERRUPTIBLE:
			return 'D';
		case TASK_RUNNING:
			return 'R';
		case TASK_INTERRUPTIBLE:
			return 'S';
		case TASK_STOPPED:
			return 'T';
		case TASK_UNRUNNABLE:
			return 'X';
		case TASK_ZOMBIE:
			return 'Z';
		default:
			return '?';
	}
	return '?';
}

static int check_for_match(int pid, struct proc_pids *pids, int count)
{
	int i;
	for (i = 0; i < count; i++)
		if (pid == pids[i].pid)
			return 1;
	return 0;
}

static char * lookup_pid_name(int pid)
{
	char *name;
	char *path = xfmtstrdup("/proc/%d/cmdline", pid);
	FILE *fp = fopen(path, "r");
	if (fp == NULL)
		return strdup("_UNKNOWN_");
	fread(name = (char *)heapAlloc(TASK_COMM_LEN), 1, 16, fp);
	return name;
}

static int get_proc_pids(struct proc_pids **pids)
{
	struct proc_pids *pp;
	DIR *dp;
	struct dirent *dptr = NULL;
	char *p, *q;
	int ret = 0;

	*pids = (struct proc_pids *)heapAlloc(sizeof(struct proc_pids) * 8192);
	pp = *pids;
	
	for (dp = opendir("/proc"); dp != NULL;) {
		dptr = readdir(dp);
		if (dptr == NULL) 
			break;
		for (q = strchr(dptr->d_name, '\0'), p = dptr->d_name; *p != '\0'; p++)
			if (*p < '0' || *p > '9')
				break;
		if (!(ret += !(p != q)))  // if (p != q) then continue, otherwise ret++ :)
			continue; 
		(pp)->pid = atoi(dptr->d_name);
		(pp)->name = lookup_pid_name(pp->pid);	
		pp++;
	}
	closedir(dp);
	return ret;
}
		
int main(int argc, char **argv)
{
	Elf_t kcore;
	ElfW(Addr) init_task_vaddr;
	struct task_struct *init_task;
	int ret;
	char *sysmap = NULL, *str;
	struct proc_pids *pids;
	
	if (argc > 1) 
		sysmap = argv[1];

        if (getuid() != 0 && getgid() != 0) {
              	printf("Must be root (Or in wheel)\n");
        	exit(-1);
        }

	ret = load_live_kcore(&kcore);
	if (ret < 0) {
		fprintf(stderr, "Unable to load /proc/kcore\n");
		exit(-1);
	}
	
	kfd = open("/proc/kcore", O_RDONLY);
	assert_read(kfd);

	init_task_vaddr = sysmap ? sysmap_lookup("init_task", sysmap) : kallsyms_lookup("init_task");
	global.kcore = (Elf_t *)memdup((Elf_t *)&kcore, sizeof(Elf_t)); // do this before we can call kcore_translate
	init_task = (struct task_struct *)kcore_translate_kernel(init_task_vaddr);
	
	struct task_struct *task;
	
#if DEBUG
	/*
	 * Just debugging to see if tasks and children are in the right offset.
	 * early on this was thrown off by a bug in the dwarf code.
	 */
	unsigned long to, co;
	to = STRUCT_MEMBER_OFFSET(struct task_struct, tasks);
	co = STRUCT_MEMBER_OFFSET(struct task_struct, children);
	printf("tasks offset: %lx\n", to); 
	printf("children offset: %lx\n", co);
#endif
	
	int pcount = get_proc_pids(&pids);
	printf("[pid]\t[state]\t[comm]\n");
	
#if __x86_64__
	for (task = kcore_translate_vm((unsigned long)container_of(init_task->tasks.next, struct task_struct, tasks));
	    (task = kcore_translate_vm((unsigned long)container_of(task->tasks.next, struct task_struct, tasks))) != NULL; ) {
		ret = check_for_match(task->pid, pids, pcount);
		str = ret == 0 ? strdupa("<- Hidden process") : strdupa("");
		printf("[%d]\t[%c]\t[%s] %s\n", task->pid, task_state(task->state), task->comm, str);
		if (container_of(task->tasks.next, struct task_struct, tasks) == (void *)init_task_vaddr)
			break;
	}
#else
	/* For 32bit we use kcore_translate_kernel because kcore has its segments
	 * setup quite differently than on x86_64.
	 */
	for (task = kcore_translate_kernel((unsigned long)container_of(init_task->tasks.next, struct task_struct, tasks));
            (task = kcore_translate_kernel((unsigned long)container_of(task->tasks.next, struct task_struct, tasks))) != NULL; ) {
		ret = check_for_match(task->pid, pids, pcount);
		str = ret == 0 ? strdupa("<- Hidden process") : strdupa("");
                printf("[%d]\t[%c]\t[%s] %s\n", task->pid, task_state(task->state), task->comm, str);
                if (container_of(task->tasks.next, struct task_struct, tasks) == (void *)init_task_vaddr)
                        break;
        }
#endif

	
	exit(0);
}


