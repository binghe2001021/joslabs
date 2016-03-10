// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include <kern/env.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line
typedef void(*func)();

struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "time", "Display the time",mon_time },
	{ "x", "Display the memory", mon_mem},
	{ "si", "stepi", mon_stepi},
	{ "c", "continue", mon_continue}
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

unsigned read_eip();

/***** Implementations of basic kernel monitor commands *****/

int 
ctoi(char c)
{
	if (c>'a' && c<'g') return 10+c-'a';
	else return c-'0';
}

int
mon_continue(int argc, char **argv, struct Trapframe *tf)
{
	tf->tf_eflags &= ~FL_TF;
	
	env_run(curenv);
	return 0;
}


int
mon_stepi(int argc, char **argv, struct Trapframe *tf)
{
	tf->tf_eflags |= FL_TF;
	cprintf("tf_eip=0x%x\n", tf->tf_eip);
	
	env_run(curenv);
	return 0;
}

int
mon_mem(int argc, char **argv, struct Trapframe *tf)
{
	if (argc != 2)
	{
		cprintf("usage: x [ADDR]\n");
		return 0;
	}
	char *ch;
	uintptr_t val = 0;
	ch = &argv[1][2];
	while ((*ch)!=0)
	{
		val = val*16 + (uintptr_t)ctoi(*ch);
	//	cprintf("%c %d %u\n",*ch,ctoi(*ch),val);
		ch++;
	}
//	cprintf("%u\n",val);
	cprintf("%u\n",*((uint32_t*)val));
	return 0;
}

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		(end-entry+1023)/1024);
	return 0;
}

int
mon_time(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t begin1=0;
 	uint32_t begin2=0;
	uint32_t end1=0;
	uint32_t end2=0;
	int i;
	for (i = 0; i < NCOMMANDS; i++) {
                if (strcmp(argv[1], commands[i].name) == 0)
			break;
        }
	__asm __volatile("rdtsc" : "=a" (begin1), "=d" (begin2));
        commands[i].func(argc-1, argv+1, tf);
	__asm __volatile("rdtsc" : "=a" (end1), "=d" (end2));
	uint64_t begintime = ((uint64_t)begin2 << 32) | begin1;
	uint64_t endtime = ((uint64_t)end2 << 32) | end1;
	cprintf("%s cycles: %llu\n",argv[1],begintime-endtime);
	return 0;
}

// Lab1 only
// read the pointer to the retaddr on the stack
static uint32_t
read_pretaddr() {
    uint32_t pretaddr;
    __asm __volatile("leal 4(%%ebp), %0" : "=r" (pretaddr)); 
    return pretaddr;
}

void
do_overflow(void)
{
    cprintf("Overflow success\n");
}

void
start_overflow(void)
{
	// You should use a techique similar to buffer overflow
	// to invoke the do_overflow function and
	// the procedure must return normally.

    // And you must use the "cprintf" function with %n specifier
    // you augmented in the "Exercise 9" to do this job.

    // hint: You can use the read_pretaddr function to retrieve 
    //       the pointer to the function call return address;

    char str[256] = {};
    int nstr = 0;
    char *pret_addr;

	// Your code here.
    pret_addr = (char*)read_pretaddr();
    func getaddr = do_overflow;
    uint32_t newaddr = (uint32_t)getaddr + 3;     
    cprintf("newaddr:%x\n",newaddr);

    for (nstr = 0;nstr < 256;nstr++){
    	str[nstr] = '1';
    }
    while(newaddr>0){
	uint32_t addrbyte = newaddr & 0xff;
	str[addrbyte] = 0;
	cprintf("%s%n\n",str,pret_addr);
	str[addrbyte] = '1';
	pret_addr += 1;
	newaddr >>= 8;
    }
}

void
overflow_me(void)
{
        start_overflow();
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
    cprintf("Stack backtrace:\n");
    uint32_t* ebp = (uint32_t*)read_ebp();
    while (ebp > 0){
	cprintf("  eip %08x  ebp %08x  args %08x %08x %08x %08x %08x\n",ebp[1],ebp,
		ebp[2],ebp[3],ebp[4],ebp[5],ebp[6]);
 	struct Eipdebuginfo info;
	debuginfo_eip(ebp[1],&info);
	char name[info.eip_fn_namelen+1];
        char *p = name;
 	char *q = (char*)info.eip_fn_name;
	for (;p!=name+info.eip_fn_namelen;p++,q++){
//		cprintf("%c ",*q);
		*p=*q;
	}	
  	*p=0;
//	cprintf("%s\n",name);
	name[info.eip_fn_namelen] = 0;
	cprintf("%s:%d: %s+%x\n",info.eip_file,info.eip_line,name,ebp[1]-info.eip_fn_addr);
	ebp = (uint32_t*)ebp[0];
    }    

    overflow_me();
    cprintf("Backtrace success\n");
    return 0;
    
}


/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf!=NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}

// return EIP of caller.
// does not work if inlined.
// putting at the end of the file seems to prevent inlining.
unsigned
read_eip()
{
	uint32_t callerpc;
	__asm __volatile("movl 4(%%ebp), %0" : "=r" (callerpc));
	return callerpc;
}
