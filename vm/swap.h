#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <bitmap.h>
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void swap_init (void);
bool swap_in (void *addr);
bool swap_out (void);
void read_from_disk (uint8_t *frame, int index);
void write_to_disk (uint8_t *frame, int index);

#endif /* vm/swap.h */
