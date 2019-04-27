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
    frametable[i]->frame = NULL;
    frametable[i]->owner = NULL;
    frametable[i]->spte = NULL;
  }
  return;
}

void*
get_frame(void)
{
  uint8_t *upage = palloc_get_page (PAL_USER);
  if (allocate_frame((void *)upage))
    return (void *)upage;
  else
    return NULL;
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


}

/*
 * Make a new frame table entry for addr.
 */
bool
allocate_frame (void *addr)
{
  size_t i;
  for(i=0;i<frametableindex;i++)
  {
    if(frametable[i]->owner==NULL)
    {
      frametable[i]->frame = (uint32_t*)addr;
      frametable[i]->owner=thread_current();
      return true;
    }
  }
  return false;

}
//
