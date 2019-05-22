#include <bitmap.h>
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <list.h>
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "devices/disk.h"


struct list buffer_cache_list;
struct lock buffersem;

#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

struct buffer_cache_entry
{
	// uint8_t* buffer;
	uint8_t buffer[DISK_SECTOR_SIZE];
	// struct thread* owner;
  bool accessed;
  bool dirty;
	int readers;
	disk_sector_t sec_no;
	struct list_elem list_elem;

	struct lock entry_lock;
	struct condition entry_cond;
};

void buffer_cache_init (void);
struct buffer_cache_entry* buffer_cache_check(struct disk *disk, disk_sector_t sec_no, bool write);
struct buffer_cache_entry* buffer_cache_write(struct disk *disk, disk_sector_t sec_no, void* towrite, off_t offset, size_t size);
struct buffer_cache_entry* buffer_cache_find(struct disk *disk, disk_sector_t sec_no);
void buffer_cache_write_dirties( struct disk *filesys_disk);
struct buffer_cache_entry* select_bce_for_evict(void);
void buffer_cache_write_dirties_once(struct disk *filesys_disk);
#endif /* filesys/cache.h */
