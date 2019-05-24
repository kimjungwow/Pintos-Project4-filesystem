#include "filesys/cache.h"
#include "filesys/filesys.h"
#include "devices/timer.h"
#include "threads/thread.h"

void buffer_cache_init(void) {
  list_init(&buffer_cache_list);
  lock_init(&buffersem); //Need it
  thread_create("writebehind",PRI_DEFAULT,buffer_cache_write_dirties, filesys_disk);
  // thread_create("readahead",PRI_DEFAULT,buffer_cache_read_ahead,filesys_disk);
  return;

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
          buffer->accessed = true;
          lock_release(&buffersem);
          return buffer;
        }
      }
  }
  if(list_size(&buffer_cache_list)>=64)
  {
    //Implement Clock Algorithm
    struct buffer_cache_entry *victim = select_bce_for_evict();
    if(victim->dirty)
      disk_write (filesys_disk, victim->sec_no, victim->buffer);
    list_remove(&victim->list_elem);
    free(victim);
  }
  struct buffer_cache_entry* newbce = (struct buffer_cache_entry*)calloc(1,sizeof (struct buffer_cache_entry));
  if(newbce==NULL)
  {
    lock_release(&buffersem);
    return NULL;
  }
  lock_init(&newbce->entry_lock);
  lock_acquire(&newbce->entry_lock);
  cond_init(&newbce->entry_cond);
  newbce->sec_no = sec_no;
  list_push_back(&buffer_cache_list,&newbce->list_elem);
  lock_release(&buffersem);
  // if(!write)
  disk_read(disk, sec_no, newbce->buffer);
  newbce->accessed=true;
  lock_release(&newbce->entry_lock);

  if(write)
    buffer_cache_check(disk,sec_no+1,false);

  // PANIC("HERE5\n");
  return newbce;

}


struct buffer_cache_entry*
buffer_cache_write(struct disk *disk, disk_sector_t sec_no, void* towrite
  , off_t offset, size_t size)
{
  // printf("required sec_no (%u) | disk capacity (%u)\n",sec_no,disk_size(disk));
  struct buffer_cache_entry* bce = buffer_cache_check(disk, sec_no,true);
  // lock_acquire(&buffersem);
  lock_acquire(&bce->entry_lock);
  while(bce->readers>0)
    cond_wait(&bce->entry_cond,&bce->entry_lock);
  memcpy(bce->buffer+offset,towrite, size);
  bce->dirty=true;
  lock_release(&bce->entry_lock);
  // lock_release(&buffersem);

}

void
buffer_cache_write_dirties(struct disk *filesys_disk)
{
  while(true)
  {
    timer_sleep(20000);
    buffer_cache_write_dirties_once(filesys_disk);
  }
  return;
}

void
buffer_cache_write_dirties_once(struct disk *filesys_disk)
{
  lock_acquire(&buffersem);
  struct list_elem *e;
  if(!list_empty(&buffer_cache_list))
  {
    for (e = list_begin (&buffer_cache_list); e != list_end (&buffer_cache_list);
         e = list_next (e))
      {
        struct buffer_cache_entry *buffer = list_entry (e, struct buffer_cache_entry, list_elem);
        if(buffer->dirty)
        {
          disk_write (filesys_disk, buffer->sec_no, buffer->buffer);
          buffer->dirty=false;
        }
      }
  }
  lock_release(&buffersem);
  return;
}

struct buffer_cache_entry*
select_bce_for_evict(void)
{
  if(!list_empty(&buffer_cache_list))
  {
    struct list_elem* e;
    struct buffer_cache_entry *bce;
    while(true)
    {
      e = list_begin (&buffer_cache_list);
      bce = list_entry (e, struct buffer_cache_entry, list_elem);
      if(bce->accessed)
      {
        bce->accessed=false;
        list_remove(&bce->list_elem);
        list_push_back(&buffer_cache_list,&bce->list_elem);
      }
      else
      {
        break;
      }
    }
    return bce;
  }
  return NULL;
}
