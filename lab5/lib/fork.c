// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	int r;
	uint32_t err = utf->utf_err;
	void *addr = (void *) utf->utf_fault_va;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at vpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if ((err & FEC_WR) == 0)
		panic("pgfault: not a write!");
	if ((vpt[PGNUM(addr)] & PTE_COW) == 0)
		panic("pgfault: not a cow!");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.

	//panic("pgfault not implemented");
	if ((r=sys_page_alloc(0, (void*)PFTEMP, PTE_U | PTE_W | PTE_P))<0)
		panic("pgfault: page_alloc failed!");
	memmove((void*)PFTEMP, ROUNDDOWN(addr,PGSIZE),PGSIZE);
	if ((r=sys_page_map(0, (void*)PFTEMP, 0, ROUNDDOWN(addr,PGSIZE), PTE_U | PTE_W | PTE_P))<0)
		panic("pgfault: page_map failed!");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	// LAB 4: Your code here.
	//panic("duppage not implemented");
	int r;
	void *addr = (void*)(pn*PGSIZE);
	if (vpt[pn] & (PTE_W | PTE_COW))
	{
		if ((r=sys_page_map(0, addr, envid, addr, PTE_U | PTE_P | PTE_COW))<0)
			return r;
		if ((r=sys_page_map(0, addr, 0, addr, PTE_U | PTE_P | PTE_COW))<0)
			return r;
	}else
		if ((r=sys_page_map(0, addr, envid, addr, PTE_U | PTE_P))<0)
			return r;
	return 0;
}

int
sduppage(envid_t envid, unsigned pn, int flag)
{
	int r;
	void *addr = (void*)(pn*PGSIZE);
	if (flag || (vpt[pn] & PTE_COW))
	{
		if ((r=sys_page_map(0, addr, envid, addr, PTE_U | PTE_P | PTE_COW))<0)
			return r;
		if ((r=sys_page_map(0, addr, 0, addr, PTE_U | PTE_P | PTE_COW))<0)
			return r;
	}else
	if (vpt[pn] & PTE_W)
	{
		if ((r=sys_page_map(0, addr, envid, addr, PTE_U | PTE_P | PTE_W))<0)
			return r;
	}
	else	
		if ((r=sys_page_map(0, addr, envid, addr, PTE_U | PTE_P))<0)
			return r;
	return 0;
}
//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	//panic("fork not implemented");
	extern void _pgfault_upcall(void);
	set_pgfault_handler(pgfault);
	envid_t id = sys_exofork();
	int r;
	if (id < 0)
		panic("exofork: child");
	if (id == 0)
	{
		thisenv = envs + ENVX(sys_getenvid());
		return 0;
	}
//	cprintf("fork id: %x",id);
	if ((r=sys_page_alloc(id, (void*)(UXSTACKTOP-PGSIZE), PTE_U | PTE_W | PTE_P))<0)
		return r;
	int i;
	for (i=0;i<UTOP-PGSIZE;i+=PGSIZE)
		if ((vpd[PDX(i)] & PTE_P) && (vpt[PGNUM(i)] & PTE_P))
		{
//			cprintf("i:%x ",i);
			if ((r=duppage(id,PGNUM(i)))<0)
				return r;
		}
	if ((r=sys_env_set_pgfault_upcall(id,(void*)_pgfault_upcall))<0)
		return r;
	if ((r=sys_env_set_status(id, ENV_RUNNABLE))<0)
		return r;
	return id;
}



// Challenge!
int
sfork(void)
{
/*	panic("sfork not implemented");
	return -E_INVAL;*/
	extern void _pgfault_upcall(void);
	set_pgfault_handler(pgfault);
	envid_t id = sys_exofork();
	int r;
	if (id < 0)
		panic("exofork: child");
	if (id == 0)
	{
		thisenv = envs + ENVX(sys_getenvid());
		return 0;
	}
	uint32_t i;
/*	for (i=0;i<UTOP-PGSIZE;i+=PGSIZE)
		if ((vpd[PDX(i)] & PTE_P) && (vpt[PGNUM(i)] & PTE_P))
			if ((r=duppage(id,PGNUM(i)))<0)
				return r;*/
	if ((r=sys_page_alloc(id, (void*)(UXSTACKTOP-PGSIZE), PTE_U | PTE_W | PTE_P))<0)
		return r;
//	cprintf("begin to map!\nUSTACKTOP-PGSIZE: %x\nUTEXT: %x\n",USTACKTOP-PGSIZE,UTEXT);
	for (i=USTACKTOP-PGSIZE;i>=UTEXT;i-=PGSIZE)
	{
		if ((vpd[PDX(i)] & PTE_P) && (vpt[PGNUM(i)] & PTE_P))
		{
			if ((r=sduppage(id,PGNUM(i),1))<0)
				return r;	
		}else
			break;
	}
	for (;i>=UTEXT;i-=PGSIZE)
		if ((vpd[PDX(i)] & PTE_P) && (vpt[PGNUM(i)] & PTE_P))
			if ((r=sduppage(id,PGNUM(i),0))<0)
				return r;

	if ((r=sys_env_set_pgfault_upcall(id,(void*)_pgfault_upcall))<0)
		return r;
	if ((r=sys_env_set_status(id, ENV_RUNNABLE))<0)
		return r;
//	cprintf("sfork succeed!\n");
	return id;
}
