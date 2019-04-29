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

  //thread_current()->suppagetable = (struct sup_page_table_entry**)malloc(sizeof(struct sup_page_table_entry)*suppagetableindex);
  hash_init(&thread_current()->hash,hash_spte,compare_spte,NULL);
  size_t i;
/*
  for(i=0;i<suppagetableindex;i++)
  {
    thread_current()->suppagetable[i] = malloc(sizeof(struct sup_page_table_entry));
    memset(thread_current()->suppagetable[i],0,sizeof(struct sup_page_table_entry));
  }*/
}


/*
 * Make new supplementary page table entry for addr
 */
struct sup_page_table_entry *
allocate_page (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{

  size_t i,j;
  for(i=0;i<suppagetableindex;i++)
  {
    struct sup_page_table_entry** given = thread_current()->suppagetable;
    struct sup_page_table_entry* myspte = given[i];
    if(myspte->user_vaddr==NULL)
    {
      struct hash_elem hash_elem;


      myspte->user_vaddr=(uint32_t *)upage;
      myspte->access_time=(uint64_t)timer_ticks();
      myspte->hash_elem = hash_elem;
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


      size_t frametableindex=user_pool.used_map->bit_cnt;
      memcpy(thread_current()->suppagetable[i],myspte,sizeof (struct sup_page_table_entry));
      if(upage!=NULL)
      {
        struct frame_table_entry fte;
        struct hash_elem* e;
        fte.uaddr=upage;
        e=hash_find(&ftehash,&fte.hash_elem);
        if(e!=NULL)
        {
          struct frame_table_entry* goalfte= hash_entry(e, struct frame_table_entry,hash_elem);
          goalfte->spte=thread_current()->suppagetable[i];
        }

      }
      return suppagetable[i];
    }
  }
  return NULL;

}



unsigned
hash_spte(struct hash_elem he)
{
  struct sup_page_table_entry* spte = hash_entry(&he,struct sup_page_table_entry, hash_elem);
  return hash_bytes(spte->user_vaddr, sizeof spte->user_vaddr);
}

bool
compare_spte(struct hash_elem a, struct hash_elem b)
{
  const struct sup_page_table_entry* afte = hash_entry(&a, struct sup_page_table_entry, hash_elem);
  const struct sup_page_table_entry* bfte = hash_entry(&b, struct sup_page_table_entry, hash_elem);
  return afte->user_vaddr < bfte->user_vaddr ;
}




/*
struct sup_page_table_entry*
filerelated_page (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable)
{
  if(file==NULL)
    return NULL;
  struct sup_page_table_entry* spte;
  spte = allocate_page(upage,NULL,writable);
  spte->filerelated=true;
  spte->file = file;
  spte->ofs = ofs;
  spte->read_bytes = read_bytes;
  spte->zero_bytes = zero_bytes;
  return spte;
}*/
