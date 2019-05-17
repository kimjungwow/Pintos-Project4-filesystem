#include "threads/palloc.h"
#include <bitmap.h>
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "threads/init.h"
#include "threads/loader.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include <hash.h>

#include "filesys/file.h"


#ifndef VM_PAGE_H
#define VM_PAGE_H

struct sup_page_table_entry
{
	uint32_t* user_vaddr;
	uint64_t access_time;
	struct hash_elem hash_elem;
	int mapid;
	int mmapfd;

	bool filerelated;
	struct file* file;
	off_t ofs;
	uint32_t read_bytes;
	uint32_t zero_bytes;
	bool writable;
	bool dirty;
	bool accessed;
	bool inswap;
	int swapindex;
};

void page_init (void);
//struct sup_page_table_entry *allocate_page (void *uaddr, void *kaddr, bool writable);
struct sup_page_table_entry *
allocate_page (struct file *file, off_t ofs, uint8_t *upage,
uint32_t read_bytes, uint32_t zero_bytes, bool writable);
unsigned hash_spte(const struct hash_elem* he, void* aux UNUSED);
bool compare_spte(const struct hash_elem* a,const struct hash_elem* b,void* aux UNUSED);
void before_munmap(struct sup_page_table_entry* spte);
void destroy_spt(void);
struct sup_page_table_entry* select_spte_for_evict(void);
#endif /* vm/page.h */
