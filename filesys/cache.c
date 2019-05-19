#include "filesys/cache.h"

void buffer_cache_init(void) {
  list_init(&buffer_cache_list);
  lock_init(&buffersem); //Need it
  return;

}

struct buffer_cache_entry* buffer_cache_find(struct disk *disk, disk_sector_t sec_no)
{
  lock_acquire(&buffersem);
  struct list_elem *e;
  if(!list_empty(&buffer_cache_list))
  {
    for (e = list_begin (&buffer_cache_list); e != list_end (&buffer_cache_list);
         e = list_next (e))
      {
        struct buffer_cache_entry *buffer = list_entry (e, struct buffer_cache_entry, list_elem);
        if(buffer->sec_no == sec_no)
        {
          lock_release(&buffersem);
          return buffer;
        }
      }
  }


  lock_release(&buffersem);
  return NULL;
}

struct buffer_cache_entry* buffer_cache_check(struct disk *disk, disk_sector_t sec_no, bool write)
{

  lock_acquire(&buffersem);
  struct list_elem *e;
  if(!list_empty(&buffer_cache_list))
  {
    for (e = list_begin (&buffer_cache_list); e != list_end (&buffer_cache_list);
         e = list_next (e))
      {
        struct buffer_cache_entry *buffer = list_entry (e, struct buffer_cache_entry, list_elem);
        if(buffer->sec_no == sec_no)
        {
          lock_release(&buffersem);
          return buffer;
        }
      }
  }
  if(list_size(&buffer_cache_list)>=64)
  {
    //Implement Clock Algorithm
    struct list_elem *e = list_begin(&buffer_cache_list);
    struct buffer_cache_entry *victim = list_entry (e, struct buffer_cache_entry, list_elem);
    list_remove(e);
    //Check dirty bit
    free(victim);
  }

  struct buffer_cache_entry* newbce = (struct buffer_cache_entry*)calloc(1,sizeof (struct buffer_cache_entry));
  if(newbce==NULL)
  {
    PANIC("1");
    lock_release(&buffersem);
    return NULL;
  }

   newbce->buffer = calloc(1,DISK_SECTOR_SIZE);
  if(newbce->buffer==NULL)
  {

    lock_release(&buffersem);
    return NULL;
  }
  newbce->owner = thread_current();
  newbce->sec_no = sec_no;

  list_push_back(&buffer_cache_list,&newbce->list_elem);


  // if(!write)
  disk_read(disk, sec_no, newbce->buffer);
  lock_release(&buffersem);
  // PANIC("HERE5\n");
  return newbce;

}


struct buffer_cache_entry*
buffer_cache_write(struct disk *disk, disk_sector_t sec_no, void* towrite
  , off_t offset, size_t size)
{


  struct buffer_cache_entry* bce = buffer_cache_check(disk, sec_no,true);

  lock_acquire(&buffersem);

  memcpy(bce->buffer+offset,towrite+offset, size);
  bce->dirty=true;
  disk_write (disk, sec_no, bce->buffer);
  lock_release(&buffersem);

}
