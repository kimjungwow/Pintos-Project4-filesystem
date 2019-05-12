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
#include "lib/kernel/hash.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"

struct list global_frame_list;
extern struct lock handlesem;

#ifndef VM_FRAME_H
#define VM_FRAME_H


struct frame_table_entry
{
	uint32_t* frame;
	struct thread* owner;
	struct sup_page_table_entry* spte;

	uint32_t* uaddr;
	struct list_elem global_list_elem;
	struct list_elem perprocess_list_elem;
};
/*
void* get_frame(void);
void* get_frame_zero(void);*/
void frame_init (void);
void* allocate_frame (void *uaddr, enum palloc_flags flags);
struct frame_table_entry * select_fte_for_evict(void);

#endif /* vm/frame.h */
