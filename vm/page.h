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
#include "vm/frame.h"
#include "filesys/file.h"


#ifndef VM_PAGE_H
#define VM_PAGE_H

struct sup_page_table_entry
{
	uint32_t* user_vaddr;
	//uint32_t* kernel_vaddr;
	uint64_t access_time;
	struct hash_elem hash_elem;

	bool filerelated;
	struct file* file;
	off_t ofs;
	uint32_t read_bytes;
	uint32_t zero_bytes;
	bool writable;
	bool dirty;
	bool accessed;
};

void page_init (void);
//struct sup_page_table_entry *allocate_page (void *uaddr, void *kaddr, bool writable);
struct sup_page_table_entry *
allocate_page (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable);
							unsigned
							hash_spte(struct hash_elem he);
bool compare_spte(struct hash_elem a, struct hash_elem b);
/*struct sup_page_table_entry*
filerelated_page (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable);
*/
#endif /* vm/page.h */
