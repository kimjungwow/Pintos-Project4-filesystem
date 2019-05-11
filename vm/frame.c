#include "vm/frame.h"
#include "vm/page.h"
extern struct pool user_pool;

// struct frame_table_entry** frametable;
size_t frametableindex;
// struct lock framesem;
struct lock framesem;
/*
 * Initialize frame table
 */
void
frame_init (void)
{
  frametableindex=user_pool.used_map->bit_cnt;
  hash_init(&ftehash,hash_fte,compare_fte,NULL);
  lock_init(&framesem);
  return;
}
/*
 * Make a new frame table entry for addr.
 */
void *
allocate_frame (void *uaddr, enum palloc_flags flags)
{


  lock_acquire(&framesem);


  uint32_t *frame = palloc_get_page(flags);
  if(frame==NULL)
  {
    printf("Frame table full!\n");
    lock_release(&framesem);
    PANIC("FRAMETABLEFULL");
    return NULL;
  }
  size_t i;
  struct frame_table_entry* newfte = (struct frame_table_entry*)malloc(sizeof(struct frame_table_entry));
  newfte->frame = frame;
  newfte->owner = thread_current();
  newfte->uaddr = (uint32_t*)uaddr;

  hash_insert(&ftehash,&newfte->hash_elem);
  uint32_t* fault_addr = pg_round_down(uaddr);
  struct sup_page_table_entry check;
  struct hash_elem *he;
  struct sup_page_table_entry* spte;
  check.user_vaddr=fault_addr;
  he= hash_find(&thread_current()->hash, &check.hash_elem);
  spte = he !=NULL ? hash_entry(he, struct sup_page_table_entry, hash_elem) : NULL;
  if(spte!=NULL)
  {
    newfte->spte=spte;
    if(pagedir_get_page (thread_current()->pagedir, uaddr) == NULL)
    {
      pagedir_set_page (thread_current()->pagedir, uaddr, frame, spte->writable);
      lock_release(&framesem);
      return frame;
    }
    else
    {
      palloc_free_page(frame);
      lock_release(&framesem);
      return NULL;
    }
  }
  else{
    printf("FAIL TO FIND SPTE\n");
    palloc_free_page(frame);
    lock_release(&framesem);
    return NULL;
  }

  printf("RUN OUT OF FRAME\n");
  palloc_free_page(frame);
  lock_release(&framesem);
  return NULL;

}

struct frame_table_entry *
select_fte_for_evict(void)
{
  struct hash_iterator i;


  hash_first (&i, &ftehash);
  hash_next(&i);
  struct frame_table_entry* evictfte = hash_entry (hash_cur (&i), struct frame_table_entry, hash_elem);
  while (hash_cur (&i))
  {
    struct frame_table_entry *fte = hash_entry (hash_cur (&i), struct frame_table_entry, hash_elem);
    if(fte->spte->access_time<evictfte->spte->access_time)
      evictfte=fte;
    hash_next(&i);

  }
  return evictfte;
}



unsigned
hash_fte(struct hash_elem he)
{
  struct frame_table_entry* fte = hash_entry(&he,struct frame_table_entry, hash_elem);
  return hash_bytes(fte->uaddr, sizeof fte->uaddr);
}

bool
compare_fte(struct hash_elem a, struct hash_elem b)
{
  const struct frame_table_entry* afte = hash_entry(&a, struct frame_table_entry, hash_elem);
  const struct frame_table_entry* bfte = hash_entry(&b, struct frame_table_entry, hash_elem);
  return afte->uaddr < bfte->uaddr ;
}
