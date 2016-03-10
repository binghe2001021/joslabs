#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H
#include <kern/pci.h>
#include <inc/x86.h>
#include <inc/mmu.h>
#include <kern/pmap.h>
#include <inc/assert.h>
#include <inc/error.h>
volatile uint32_t* head;

int e1000_attach(struct pci_func *pcif);

#endif	// JOS_KERN_E1000_H
