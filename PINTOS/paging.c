#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "paging.h"
#include "fs.h"


#define NOT_PTE_P 0xffe
#define BLOCK_BIT 0x080

// #include <stdio.h>

/* Allocate eight consecutive disk blocks.
 * Save the content of the physical page in the pte
 * to the disk blocks and save the block-id into the
 * pte.
 */
void
swap_page_from_pte(pte_t *pte)
{
	uint first_block_address = balloc_page(1);
	// char *page_value;
	pte_t temp=* pte;
	for(int i=0;i<12;i++){
		temp/=2;
	}
	for(int i=0;i<12;i++){
		temp=temp*2;
	}
	write_page_to_disk(1,(char*)(temp+KERNBASE),first_block_address);
		//uint converted to void *
		//we can also pass an array of char as second arg after copying
	
	*pte&=(NOT_PTE_P);

	//first 20 bits will be set to 0's,to avoid that we can and with ~PTE_P
	*pte |= BLOCK_BIT;
	//set block bit
	uint temp2=first_block_address;
	for(int i=0;i<12;i++){
		temp2*=2;
	}
	*pte|=(temp2);
	// invlpg((void *)temp);
	//invalidate thisss

	//uint converted to void *
	kfree((KERNBASE + temp));
}

/* Select a victim and swap the contents to the disk.
 */
int
swap_page(pde_t *pgdir)
{
	pte_t *pg_to_swap = select_a_victim(pgdir);
	while(pg_to_swap == 0){
		//possible error: maybe change pg_to_swap to *pg_to_swap
		if(pg_to_swap < 0) break;
		clearaccessbit(pgdir);
		pg_to_swap = select_a_victim(pgdir);
		if(pg_to_swap > 0) break;
	}
	if(*pg_to_swap & PTE_P){
		swap_page_from_pte(pg_to_swap);
		return 1;
	}
	panic("swap_page is not implemented");
	return 0;
}

/* Map a physical page to the virtual address addr.
 * If the page table entry points to a swapped block
 * restore the content of the page from the swapped
 * block and free the swapped block.
 */
void
map_address(pde_t *pgdir, uint addr)
{
	pte_t *pg_map;
	pde_t *pde;
  	pte_t *pgtab;
  	pde = &pgdir[PDX(void *)addr];
  	if(*pde & PTE_P){
    	pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
    	return;
  	}
  	else if(*pde & 0x080){
  		//i.e., the page was swapped
  		pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
  		uint *b_id = &pgtab[PTX(va)];
  		for(int i=0;i<12;i++){
  			*b_id = *b_id/2;
  		}


  		pgtab = (pte_t*)kalloc();
  		if(pgtab == 0){
  			swap_page(pgdir);
  			pgtab = (pte_t*)kalloc();
  		}
  		// memset(pgtab, 0, PGSIZE);
  		char page_in_block[4096];
  		read_page_from_disk(1,page_in_block,*b_id);
  		memmove(pgtab, page_in_block, 4096);
  		*pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
  		bfree_page(1,*b_id);
  		// *pde &= ~BLOCK_BIT;
  		// *pde |= PTE_P;

  		// pgtab[PTX(PTE_ADDR(*pde))] &= ~BLOCK_BIT;
  		// pgtab[PTX(PTE_ADDR(*pde))] |= PTE_P;
  	}
  	else{
  		pgtab = (pte_t*)kalloc();
  		if(pgtab == 0){
  			swap_page(pgdir);
  			pgtab = (pte_t*)kalloc();
  		}
  		memset(pgtab, 0, PGSIZE);
  		*pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
  	}

}

/* page fault handler */
void
handle_pgfault()
{
	unsigned addr;
	struct proc *curproc = myproc();

	asm volatile ("movl %%cr2, %0 \n\t" : "=r" (addr));
	addr &= ~0xfff;
	map_address(curproc->pgdir, addr);
}
