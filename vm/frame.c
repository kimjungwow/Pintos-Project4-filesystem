#include "vm/frame.h"

extern struct pool user_pool;

struct frame_table_entry** frametable;
size_t frametableindex;
/*
 * Initialize frame table
 */
void
frame_init (void)
{
  frametableindex=user_pool.used_map->bit_cnt;
  frametable = (struct frame_table_entry**)malloc(sizeof(struct frame_table_entry) *frametableindex);
  size_t i;
  for(i=0;i<frametableindex;i++)
  {
    frametable[i] = malloc(sizeof(struct frame_table_entry));
    memset(frametable[i],0,sizeof(struct frame_table_entry));
  }
  hash_init(&ftehash,hash_fte,compare_fte,NULL);
  return;
}
/*
void*
get_frame(void)
{
  uint8_t *upage = palloc_get_page (PAL_USER);
  if (allocate_frame((void *)upage))
    return (void *)upage;
  else
  {
    PANIC("FRAME_TABLE_RUNOUT");
    return NULL;
  }
}

void* get_frame_zero(void)
{
  uint8_t *upage = palloc_get_page (PAL_USER|PAL_ZERO);
  if (allocate_frame((void *)upage))
    return (void *)upage;
  else
    {
      PANIC("FRAME_TABLE_RUNOUT");
      return NULL;
    }
}*/

/*
 * Make a new frame table entry for addr.
 */
void *
allocate_frame (void *uaddr, enum palloc_flags flags)
{
  uint32_t *frame = palloc_get_page(flags);
  size_t i;
  for(i=0;i<frametableindex;i++)
  {
    if(frametable[i]->owner==NULL)
    {
      struct hash_elem elem;
      frametable[i]->frame = frame;
      frametable[i]->owner = thread_current();
      frametable[i]->uaddr = (uint32_t*)uaddr;
      frametable[i]->hash_elem = elem;
      return frame;
    }
  }
  palloc_free_page(frame);
  PANIC("RUN OUT OF FRAME");
  return NULL;

}
//


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
