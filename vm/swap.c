#include "vm/swap.h"
#include "vm/page.h"
#include "vm/frame.h"
#include "devices/disk.h"
#include "threads/synch.h"
#include <bitmap.h>


/* The swap device */
static struct disk *swap_device;

/* Tracks in-use and free swap slots */
static struct bitmap *swap_table;

/* Protects swap_table */
static struct lock swap_lock;

struct lock framesem;

/*
 * Initialize swap_device, swap_table, and swap_lock.
 */
void
swap_init (void)
{
  swap_device = disk_get(1,1);
  printf("DISK SECTOR # : %d\n",disk_size(swap_device));
  swap_table = bitmap_create(disk_size(swap_device)/8);

  lock_init(&swap_lock);

  return;
}

// Implement order : write_to_disk -> swap_out ->  read_from_disk ->swap_in

/*
 * Reclaim a frame from swap device.
 * 1. Check that the page has been already evicted.
 * 2. You will want to evict an already existing frame
 * to make space to read from the disk to cache.
 * 3. Re-link the new frame with the corresponding supplementary
 * page table entry.
 * 4. Do NOT create a new supplementray page table entry. Use the
 * already existing one.
 * 5. Use helper function read_from_disk in order to read the contents
 * of the disk into the frame.
 */

 /*
 1. spte마다 swap에 있는지 아닌지를 판단할 수 있게 변수 (bool?) 하나 넣어줌.
 현재 swap에 있다 -> already evicted.
 2. swap_in 안에서 swap_out 호출되는듯!
 (i) existing frame의 내용을 swap disk에 쓴다. 이 때 swap disk의 어느 주소에 쓰는지를
 spte에 기록. 동시에 bitmap도 변경.
 (ii) spte의 swap 판단 bool을 true로
 (iii) 해당 frame을 free하고 pagedir_clear_page도 해줘야하나?
 palloc_free + pagedir 정리 + 해당 frame 메모리를 zero화 시켜서 전 내용 안 남아있게
 3~4 page fault가 나게 한 fault_addr과 관련된 spte를 allocate_frame 시켜줌.
 당연히 이 spte는 현재 swap에 있는 상태

 */

bool
swap_in (void *addr)
{
   // * 1. Check that the page has been already evicted.
  uint32_t* fault_addr = pg_round_down(addr);
  struct sup_page_table_entry check;
  struct hash_elem *he;
  struct sup_page_table_entry* spte;
  check.user_vaddr=fault_addr;
  he= hash_find(&thread_current()->hash, &check.hash_elem);
  spte = he !=NULL ? hash_entry(he, struct sup_page_table_entry, hash_elem) : NULL;
  if(!spte->inswap)
    return false;
  lock_acquire(&framesem);
  uint32_t* frame = allocate_frame(spte->user_vaddr,PAL_USER|PAL_ZERO);
  while(frame==NULL)
    frame = allocate_frame(spte->user_vaddr,PAL_USER|PAL_ZERO);

  read_from_disk(frame,spte->swapindex);
  spte->swapindex= -1;
  spte->inswap=false;

  return true;
}

/*
 * Evict a frame to swap device.
 * 1. Choose the frame you want to evict.
 * (Ex. Least Recently Used policy -> Compare the timestamps when each
 * frame is last accessed)
 * 2. Evict the frame. Unlink the frame from the supplementray page table entry
 * Remove the frame from the frame table after freeing the frame with
 * pagedir_clear_page.
 * 3. Do NOT delete the supplementary page table entry. The process
 * should have the illusion that they still have the page allocated to
 * them.
 * 4. Find a free block to write you data. Use swap table to get track
 * of in-use and free swap slots.
 */

 /*
 0. 얘는 모든frame 사용중일때 호출. palloc_get_page==NULL일때
1. 정책을 정한 뒤 frame 사용중인 spte하나 고름
2.
(i) existing frame의 내용을 swap disk에 쓴다. 이 때 swap disk의 어느 주소에 쓰는지를
spte에 기록. 동시에 bitmap도 변경.
(ii) spte의 swap 판단 bool을 true로
(iii) 해당 frame을 free하고 pagedir_clear_page도 해줘야하나?
palloc_free + pagedir 정리 + 해당 frame 메모리를 zero화 시켜서 전 내용 안 남아있게
3. page fault가 나게 한 fault_addr과 관련된 spte를 allocate_frame 시켜줌.
+ 이거는 page_fault다시 나면 알아서하지않을까?
 */
bool
swap_out (void)
{
  struct frame_table_entry* evictfte = select_fte_for_evict();
  size_t swapindex = bitmap_scan_and_flip (swap_table, 0,1,false);
  if (swapindex ==BITMAP_ERROR)
    return false;
  //다른 process의 frame도 evict가능? 그러면 여기서 pd는 해당 frame의 주인의 pd여야함.
  pagedir_clear_page(evictfte->owner->pagedir,evictfte->spte->user_vaddr);
  evictfte->spte->swapindex=swapindex;
  evictfte->spte->inswap=true;

  write_to_disk(evictfte->frame, swapindex);


  //free_page는 user_addr인가? physical인가? = physical(frame)
  list_remove(&evictfte->global_list_elem);
  list_remove(&evictfte->perprocess_list_elem);

  palloc_free_page(evictfte->frame);

  free(evictfte);
  lock_release(&framesem);
  return true;
}

/*
 * Read data from swap device to frame.
 * Look at device/disk.c
 */
void read_from_disk (uint8_t *frame, int index)
{
  int i, realindex=index*8;
  for (i=0;i<8;i++)
  {
    disk_read(swap_device, realindex, frame);
    realindex++;
    frame+=DISK_SECTOR_SIZE;
  }
  return;
}
//index 기준을 bitmap으로 생각.
/* Write data to swap device from frame */
void write_to_disk (uint8_t *frame, int index)
{
  int i, realindex=index*8;
  for (i=0;i<8;i++)
  {
    disk_write(swap_device, realindex, frame);
    realindex++;
    frame+=DISK_SECTOR_SIZE;
  }
  //disk_write


  return;
}
