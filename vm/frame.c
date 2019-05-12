#include "vm/frame.h"
#include "vm/page.h"
#include <list.h>
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
  list_init(&global_frame_list);
  lock_init(&framesem);
  return;
}
/*
 * Make a new frame table entry for addr.
 */
void *
allocate_frame (void *uaddr, enum palloc_flags flags)
{
  // printf("allocate_fram %p start\n",uaddr);

  if(!lock_held_by_current_thread(&framesem))
    lock_acquire(&framesem);
  uint32_t *frame = palloc_get_page(flags);
  while(frame==NULL)
  {
    // printf("Frame table full!\n");
    // lock_release(&framesem);
    // PANIC("FRAMETABLEFULL");
    swap_out();
    frame=palloc_get_page(flags);
  }
  size_t i;
  struct frame_table_entry* newfte = (struct frame_table_entry*)calloc(1,sizeof(struct frame_table_entry));
  // struct frame_table_entry* newfte = (struct frame_table_entry*)malloc(sizeof(struct frame_table_entry));
  newfte->frame = frame;
  newfte->owner = thread_current();
  newfte->uaddr = (uint32_t*)uaddr;

  // hash_insert(&ftehash,&newfte->hash_elem);
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
      list_push_back(&thread_current()->perprocess_frame_list,&newfte->perprocess_list_elem);
      list_push_back(&global_frame_list,&newfte->global_list_elem);
      if(lock_held_by_current_thread(&framesem))
        lock_release(&framesem);
      // printf("allocate_fram %p end\n",uaddr);
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
  if(!list_empty(&global_frame_list))
  {
    struct list_elem* e;
    struct frame_table_entry *fte;
    while(true)
    {
      e = list_begin (&global_frame_list);
      fte = list_entry (e, struct frame_table_entry, global_list_elem);
      if(pagedir_is_accessed (fte->owner->pagedir, fte->spte->user_vaddr))
      {
        pagedir_set_accessed (fte->owner->pagedir, fte->spte->user_vaddr,false);
        list_remove(&fte->global_list_elem);
        list_push_back(&global_frame_list,&fte->global_list_elem);
      }
      else
      {
        break;
      }
    }
    return fte;
  }
  /*
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
  return evictfte;*/
}
