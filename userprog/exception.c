#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include "userprog/gdt.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "userprog/pagedir.h"
/* Number of page faults processed. */
static long long page_fault_cnt;

static void kill (struct intr_frame *);
static void page_fault (struct intr_frame *);
struct lock handlesem;

/* Registers handlers for interrupts that can be caused by user
   programs.

   In a real Unix-like OS, most of these interrupts would be
   passed along to the user process in the form of signals, as
   described in [SV-386] 3-24 and 3-25, but we don't implement
   signals.  Instead, we'll make them simply kill the user
   process.

   Page faults are an exception.  Here they are treated the same
   way as other exceptions, but this will need to change to
   implement virtual memory.

   Refer to [IA32-v3a] section 5.15 "Exception and Interrupt
   Reference" for a description of each of these exceptions. */
void
exception_init (void)
{
  /* These exceptions can be raised explicitly by a user program,
     e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
     we set DPL==3, meaning that user programs are allowed to
     invoke them via these instructions. */
  intr_register_int (3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
  intr_register_int (4, 3, INTR_ON, kill, "#OF Overflow Exception");
  intr_register_int (5, 3, INTR_ON, kill,
                     "#BR BOUND Range Exceeded Exception");

  /* These exceptions have DPL==0, preventing user processes from
     invoking them via the INT instruction.  They can still be
     caused indirectly, e.g. #DE can be caused by dividing by
     0.  */
  intr_register_int (0, 0, INTR_ON, kill, "#DE Divide Error");
  intr_register_int (1, 0, INTR_ON, kill, "#DB Debug Exception");
  intr_register_int (6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
  intr_register_int (7, 0, INTR_ON, kill,
                     "#NM Device Not Available Exception");
  intr_register_int (11, 0, INTR_ON, kill, "#NP Segment Not Present");
  intr_register_int (12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
  intr_register_int (13, 0, INTR_ON, kill, "#GP General Protection Exception");
  intr_register_int (16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
  intr_register_int (19, 0, INTR_ON, kill,
                     "#XF SIMD Floating-Point Exception");

  /* Most exceptions can be handled with interrupts turned on.
     We need to disable interrupts for page faults because the
     fault address is stored in CR2 and needs to be preserved. */
  intr_register_int (14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
void
exception_print_stats (void)
{
  printf ("Exception: %lld page faults\n", page_fault_cnt);
}

/* Handler for an exception (probably) caused by a user process. */
static void
kill (struct intr_frame *f)
{
  /* This interrupt is one (probably) caused by a user process.
     For example, the process might have tried to access unmapped
     virtual memory (a page fault).  For now, we simply kill the
     user process.  Later, we'll want to handle page faults in
     the kernel.  Real Unix-like operating systems pass most
     exceptions back to the process via signals, but we don't
     implement them. */

  /* The interrupt frame's code segment value tells us where the
     exception originated. */
  switch (f->cs)
    {
    case SEL_UCSEG:
      /* User's code segment, so it's a user exception, as we
         expected.  Kill the user process.  */
      printf ("%s: dying due to interrupt %#04x (%s).\n",
              thread_name (), f->vec_no, intr_name (f->vec_no));
      intr_dump_frame (f);
      thread_exit ();

    case SEL_KCSEG:
      /* Kernel's code segment, which indicates a kernel bug.
         Kernel code shouldn't throw exceptions.  (Page faults
         may cause kernel exceptions--but they shouldn't arrive
         here.)  Panic the kernel to make the point.  */
      intr_dump_frame (f);
      PANIC ("Kernel bug - unexpected interrupt in kernel");

    default:
      /* Some other code segment?  Shouldn't happen.  Panic the
         kernel. */
      printf ("Interrupt %#04x (%s) in unknown segment %04x\n",
             f->vec_no, intr_name (f->vec_no), f->cs);
      thread_exit ();
    }
}

/* Page fault handler.  This is a skeleton that must be filled in
   to implement virtual memory.  Some solutions to project 2 may
   also require modifying this code.

   At entry, the address that faulted is in CR2 (Control Register
   2) and information about the fault, formatted as described in
   the PF_* macros in exception.h, is in F's error_code member.  The
   example code here shows how to parse that information.  You
   can find more information about both of these in the
   description of "Interrupt 14--Page Fault Exception (#PF)" in
   [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */
static void
page_fault (struct intr_frame *f)
{
  bool not_present;  /* True: not-present page, false: writing r/o page. */
  bool write;        /* True: access was write, false: access was read. */
  bool user;         /* True: access by user, false: access by kernel. */
  void *fault_addr;  /* Fault address. */




  /* Obtain faulting address, the virtual address that was
     accessed to cause the fault.  It may point to code or to
     data.  It is not necessarily the address of the instruction
     that caused the fault (that's f->eip).
     See [IA32-v2a] "MOV--Move to/from Control Registers" and
     [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception
     (#PF)". */
  asm ("movl %%cr2, %0" : "=r" (fault_addr));
  barrier();

  /* Turn interrupts back on (they were only off so that we could
     be assured of reading CR2 before it changed). */
  intr_enable ();

  /* Count page faults. */
  page_fault_cnt++;

  /* Determine cause. */
  not_present = (f->error_code & PF_P) == 0;
  write = (f->error_code & PF_W) != 0;
  user = (f->error_code & PF_U) != 0;

  // printf("fault_addr = %p\n",fault_addr);
  if(!user)
  {
    // printf("%p = fault_addr KERNEL\n",fault_addr);
    if(not_present)
    {
      if(!is_user_vaddr(fault_addr))
      {
        //Do Something
        PANIC("Not user vaddr %p\n",fault_addr);
        return;
      }
      else
      {
        if(
          (pagedir_get_page(thread_current()->pagedir, fault_addr)==NULL
         && thread_current()->process_stack>=fault_addr
         // && fault_addr >0x08058000
          )
         ||is_code_section(thread_current()->process_stack))
        // if(is_code_vaddr(fault_addr))
        // if(is_code_vaddr(fault_addr)&&pagedir_get_page(thread_current()->pagedir,pg_round_down(fault_addr-2*PGSIZE))==NULL) // To avoid allocate something on code section or below code section
        {
          thread_current()->returnstatus=-1;
          thread_exit();
        }
        //Why do we need this?
        struct sup_page_table_entry* stackgrow = allocate_page(NULL,0,pg_round_down(fault_addr),0,0,true);
        uint8_t *kpage;
        kpage =	allocate_frame (pg_round_down(fault_addr), PAL_USER | PAL_ZERO); //PAL_USER because it's for stack??

        return;
      }

    }
    else
    {
      f->eip=f->eax;
      f->eax =0xffffffff;
      // return;
      thread_current()->returnstatus=-1;
      if(lock_held_by_current_thread(&handlesem))
        lock_release(&handlesem);
      thread_exit();
      // return;

    }
  }
  else
  {

    if((fault_addr==NULL)||(is_kernel_vaddr(fault_addr)))
    {
      thread_current()->returnstatus=-1;
      thread_exit();
    }

    if(is_code_vaddr(f->esp))
    {
      thread_current()->returnstatus=-1;
      // thread_exit();
      return;
    }

    // fault_addr = pg_round_down(fault_addr);

    struct sup_page_table_entry check;
    struct hash_elem *he;
    struct sup_page_table_entry* spte;
    check.user_vaddr=(uint32_t *)(pg_round_down(fault_addr));
    he = hash_find(&thread_current()->hash, &check.hash_elem);

    spte = he !=NULL ? hash_entry(he, struct sup_page_table_entry, hash_elem) : NULL;



    if(spte==NULL)
    {
      // printf("%p = fault_addr SPTE NULL\n",fault_addr);
      void* currentesp = f->esp;
      if(is_code_vaddr(fault_addr))
      {
        thread_current()->returnstatus=-1;
        thread_exit();
      }
      else if((currentesp<=fault_addr)||(currentesp-fault_addr==32)||(currentesp-fault_addr==4))
            // if(((unsigned int*)currentesp-(unsigned int*)fault_addr<=0)||(currentesp-fault_addr==32)||(currentesp-fault_addr==4))
      {


        struct sup_page_table_entry* stackgrow = allocate_page(NULL,0,pg_round_down(fault_addr),0,0,true);
        uint8_t *kpage;
        kpage =	allocate_frame (pg_round_down(fault_addr), PAL_USER | PAL_ZERO);
        return;

      }
      else
      {
        thread_current()->returnstatus=-1;
        thread_exit();
        return;
        PANIC("INVALID - Not in SPT");
      }
    }
    else
    {

      if(spte->inswap)
      {
        // printf("%p = fault_addr inswap | spte->uservaddr = %p\n",fault_addr,spte->user_vaddr);
        swap_in(spte->user_vaddr);
        return;
      }
      if(spte->file==NULL)
      {


        PANIC("NOT FILE CASE");
        return;
      }
      else
      {
        // printf("%p = fault_addr |  SPTE file = %p\n",fault_addr,spte->file);
        // printf("%p = spte->user_vaddr | %p = fault_addr\n",spte->user_vaddr,fault_addr);
        ASSERT ((spte->read_bytes + spte->zero_bytes) % PGSIZE == 0);
      	ASSERT (pg_ofs (spte->user_vaddr) == 0);
      	// ASSERT (spte->ofs % PGSIZE == 0);
        lock_acquire(&handlesem);
      	file_seek (spte->file, spte->ofs);
        lock_release(&handlesem);

    		/* Do calculate how to fill this page.
    		   We will read PAGE_READ_BYTES bytes from FILE
    		   and zero the final PAGE_ZERO_BYTES bytes. */
    		size_t page_read_bytes = spte->read_bytes;
    		size_t page_zero_bytes = spte->zero_bytes;

    		/* Get a page of memory. */

    		uint8_t *kpage = allocate_frame (spte->user_vaddr, PAL_USER | PAL_ZERO);
    		// uint8_t *kpage = palloc_get_page (PAL_USER);

    		if (kpage == NULL)
        {
          thread_current()->returnstatus=-1;
          thread_exit();
          // printf("CODE SECTION\n");
        }


    		/* Load this page. */
        lock_acquire(&handlesem);
    		if (file_read (spte->file, kpage, page_read_bytes) != (int) page_read_bytes)
    		{
    			palloc_free_page (kpage);
          lock_release(&handlesem);
    			return;
    		}
    		// memset (kpage + page_read_bytes, 0, page_zero_bytes);
        pagedir_set_writable(thread_current()->pagedir,spte->user_vaddr,spte->writable);
        /*uint32_t* pte = lookup_page(thread_current()->pagedir,spte->user_vaddr,false);
        if(pte!=NULL)
        {

          *pte&=PF_W;
          if (active_pd () == pd)
            {

              pagedir_activate (pd);
            }
        }*/
        lock_release(&handlesem);
        return;
      }
    }
    return;
  }

  thread_current()->returnstatus=-1;
  thread_exit();
  /* To implement virtual memory, delete the rest of the function
     body, and replace it with code that brings in the page to
     which fault_addr refers. */
  printf ("Page fault at %p: %s error %s page in %s context.\n",
          fault_addr,
          not_present ? "not present" : "rights violation",
          write ? "writing" : "reading",
          user ? "user" : "kernel");
  kill (f);

}
