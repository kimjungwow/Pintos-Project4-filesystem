#include "vm/page.h"

struct sup_page_table_entry** suppagetable;
size_t suppagetableindex=1024;
/*
 * Initialize supplementary page table
 */
void
page_init (void)
{
  suppagetable= (struct sup_page_table_entry**)malloc(sizeof(struct sup_page_table_entry)*suppagetableindex);
  size_t i;
  for(i=0;i<suppagetableindex;i++)
  {
    suppagetable[i] = malloc(sizeof(struct sup_page_table_entry));
    suppagetable[i]->user_vaddr = NULL;
    suppagetable[i]->access_time = NULL;
    suppagetable[i]->dirty = false;
    suppagetable[i]->accessed = false;
  }

}

/*
 * Make new supplementary page table entry for addr
 */
struct sup_page_table_entry *
allocate_page (void *addr)
{
  return NULL;

}
