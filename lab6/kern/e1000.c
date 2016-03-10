#include <kern/e1000.h>

// LAB 6: Your driver code here
int e1000_attach(struct pci_func* pcif){
	pci_func_enable(pcif);
	head = (uint32_t*)KSTACKTOP+PGSIZE;
	boot_map_region(kern_pgdir,(uintptr_t)head,pcif->reg_size[0],pcif->reg_base[0],PTE_W | PTE_PCD | PTE_PWT);
	cprintf("device status register: %x\n",head[0x8/4]);
	return 0;
}
