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

struct hash ftehash;


#ifndef VM_FRAME_H
#define VM_FRAME_H


struct frame_table_entry
{
	uint32_t* frame;
	struct thread* owner;
	struct sup_page_table_entry* spte;

	uint32_t* uaddr;
	struct hash_elem hash_elem;
};
/*
void* get_frame(void);
void* get_frame_zero(void);*/
void frame_init (void);
void* allocate_frame (void *uaddr, enum palloc_flags flags);
unsigned hash_fte(struct hash_elem he);
bool compare_fte(struct hash_elem a, struct hash_elem b);

#endif /* vm/frame.h */
