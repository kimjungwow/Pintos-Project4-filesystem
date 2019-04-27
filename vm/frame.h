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


#ifndef VM_FRAME_H
#define VM_FRAME_H


struct frame_table_entry
{
	uint32_t* frame;
	struct thread* owner;
	struct sup_page_table_entry* spte;
};

void* get_frame(void);
void* get_frame_zero(void);
void frame_init (void);
bool allocate_frame (void *addr);

#endif /* vm/frame.h */
