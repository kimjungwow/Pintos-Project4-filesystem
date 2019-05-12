#include "vm/page.h"
extern struct pool user_pool;
extern struct frame_table_entry** frametable;
struct sup_page_table_entry** suppagetable;
size_t suppagetableindex=1024;

/*
 * Initialize supplementary page table
 */
void
page_init (void)
{
  thread_current()->suppagetable = (struct sup_page_table_entry**)calloc(sizeof(struct sup_page_table_entry),suppagetableindex);
  hash_init(&thread_current()->hash,hash_spte,compare_spte,NULL);
  thread_current()->next_mapid=3;
  thread_current()->nextmmapfd=3;
  // list_init(&thread_current()->mmaplist);
  size_t i;
}


/*
 * Make new supplementary page table entry for addr
 */
struct sup_page_table_entry *
allocate_page (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
  struct sup_page_table_entry** given = thread_current()->suppagetable;
  if(given==NULL)
  {
    page_init();
    given = thread_current()->suppagetable;
  }
  struct sup_page_table_entry* myspte = (struct sup_page_table_entry*)malloc(sizeof(struct sup_page_table_entry));
  if(myspte==NULL)
    return NULL;
  myspte->user_vaddr=(uint32_t *)upage;
  // myspte->access_time=(uint64_t)timer_ticks();
  myspte->mapid=0;
  myspte->mmapfd=0;
  myspte->inswap=false;
  myspte->swapindex=-1;
  if(file!=NULL)
  {
    myspte->filerelated = true;
    myspte->file = file;
    myspte->ofs = ofs;
    myspte->read_bytes = read_bytes;
    myspte->zero_bytes = zero_bytes;
  }
  else
  {
    myspte->filerelated = false;
  }
  myspte->writable=writable;
  hash_insert(&thread_current()->hash,&myspte->hash_elem);
  return myspte;


}

struct sup_page_table_entry *
select_spte_for_evict(void)
{
  struct hash_iterator i;


  hash_first (&i, &thread_current()->hash);
  hash_next(&i);
  struct sup_page_table_entry* evictspte = hash_entry (hash_cur (&i), struct sup_page_table_entry, hash_elem);
  while (hash_cur (&i))
  {
    struct sup_page_table_entry *spte = hash_entry (hash_cur (&i), struct sup_page_table_entry, hash_elem);
    if(spte->access_time<evictspte->access_time)
      evictspte=spte;
    hash_next(&i);

  }
  return evictspte;
}

void
before_munmap(struct sup_page_table_entry* spte)
{
  if(spte->mapid)
  {
    if(pagedir_is_dirty(thread_current()->pagedir,spte->user_vaddr))
    {
      spte_set_dirty(spte->user_vaddr,true);
      void* target = (void *)((char *)spte->file + spte->ofs);
      lock_acquire(&handlesem);
      struct file* filetowrite = thread_current()->fdtable[spte->mmapfd];
      off_t size = strlen((char *)spte->user_vaddr);
      file_seek(spte->file,0);
      off_t temppos = file_tell(spte->file);
      off_t writebytes = file_write(spte->file,spte->user_vaddr, strlen((char *)spte->user_vaddr));
      file_seek(spte->file,temppos);
      lock_release(&handlesem);
    }
  }

}

void
destroy_spt(void) // In fact, only munmap.
{

  struct thread *curr = thread_current();
  struct hash_iterator i;
  hash_first (&i, &curr->hash);
  hash_next(&i);
  while (hash_cur (&i))
  {
    struct sup_page_table_entry *spte = hash_entry (hash_cur (&i), struct sup_page_table_entry, hash_elem);
    before_munmap(spte);
    hash_next(&i);
    // hash_delete(&thread_current()->hash,&spte->hash_elem);
    // free(spte);
  }
}


unsigned
hash_spte(const struct hash_elem* he,void *aux UNUSED)
{
  const struct sup_page_table_entry* spte = hash_entry(he,struct sup_page_table_entry, hash_elem);
  return hash_bytes(&spte->user_vaddr, sizeof spte->user_vaddr);
}

bool
compare_spte(const struct hash_elem *a, const struct hash_elem *b, void *aux UNUSED)
{
  const struct sup_page_table_entry* afte = hash_entry(a, struct sup_page_table_entry, hash_elem);
  const struct sup_page_table_entry* bfte = hash_entry(b, struct sup_page_table_entry, hash_elem);
  return afte->user_vaddr < bfte->user_vaddr;
}
