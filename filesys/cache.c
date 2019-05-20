#include "filesys/cache.h"
#include "filesys/filesys.h"

void buffer_cache_init(void) {
  list_init(&buffer_cache_list);
  lock_init(&buffersem); //Need it
  thread_create("writebehind",PRI_DEFAULT,buffer_cache_write_dirties, filesys_disk);
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
          buffer->accessed = true;
          lock_release(&buffersem);
          return buffer;
        }
      }
  }
  if(list_size(&buffer_cache_list)>=64)
  {
    //Implement Clock Algorithm

    // struct list_elem *e = list_begin(&buffer_cache_list);
    // struct buffer_cache_entry *victim = list_entry (e, struct buffer_cache_entry, list_elem);

    struct buffer_cache_entry *victim = select_bce_for_evict();

    if(victim->dirty)
      disk_write (filesys_disk, victim->sec_no, victim->buffer);
    list_remove(&victim->list_elem);
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
  // newbce->buffer = calloc(1,DISK_SECTOR_SIZE);
  /*if(newbce->buffer==NULL)
  {
    lock_release(&buffersem);
    return NULL;
  }*/
  // newbce->owner = thread_current();
  newbce->sec_no = sec_no;
  list_push_back(&buffer_cache_list,&newbce->list_elem);
  // if(!write)
  disk_read(disk, sec_no, newbce->buffer);
  newbce->accessed=true;
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
  memcpy(bce->buffer+offset,towrite, size);
  bce->dirty=true;
  // disk_write (disk, sec_no, bce->buffer);
  lock_release(&buffersem);

}

void
buffer_cache_write_dirties(struct disk *filesys_disk)
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
