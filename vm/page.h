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

#ifndef VM_PAGE_H
#define VM_PAGE_H

struct sup_page_table_entry
{
	uint32_t* user_vaddr;
	uint64_t access_time;

	bool dirty;
	bool accessed;
};

void page_init (void);
struct sup_page_table_entry *allocate_page (void *addr);

#endif /* vm/page.h */
