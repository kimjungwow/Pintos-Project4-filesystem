#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "lib/user/syscall.h"
#include "threads/vaddr.h"
#include "lib/kernel/console.h"


static void syscall_handler (struct intr_frame *);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");

}

static void
syscall_handler (struct intr_frame *f)
{

  uint32_t num = *(uint32_t *)f->esp;
  switch(num)
  {
    case SYS_HALT:
    {
      power_off();
      break;
    }
    case SYS_EXIT:
    {
      char* tempesp = (char *)f-> esp;

      tempesp+=4;
      int status = *(int *)tempesp;
      thread_current()->returnstatus=status;
      // printf("exit: exit(%d)\n",status);
      thread_exit ();
      break;
    }
    case SYS_EXEC:
    {
      char* tempesp = (char *)f->esp;
      tempesp+=4;
      if(*tempesp>PHYS_BASE)
        process_exit();
      char* cmd_line = *tempesp;
      f->eax = (uint32_t)process_execute(cmd_line);
      break;
    }
    case SYS_WAIT:
    {
      break;
    }
    case SYS_CREATE:
    {
      char* tempesp = (char *)f->esp;
      tempesp+=4;
      if(*tempesp>PHYS_BASE)
        process_exit();
      char* file = *tempesp;
      tempesp+=4;
      unsigned initial_size = *(unsigned *)tempesp;
      break;
    }
    case SYS_REMOVE:
    {
      char* tempesp = (char *)f->esp;
      tempesp+=4;
      if(*tempesp>PHYS_BASE)
        process_exit();
      char* file = *tempesp;

      break;
    }
    case SYS_OPEN:
    {
      char* tempesp = (char *)f->esp;
      tempesp+=4;
      if(*tempesp>PHYS_BASE)
        process_exit();
      char* file = *tempesp;

      break;
    }
    case SYS_FILESIZE:
    {
      int* tempesp = (int *)f->esp;
      tempesp+=1;
      int fd = *tempesp;
      break;
    }
    case SYS_READ:
    {

      char* tempesp = (char *)f->esp;

      tempesp+=4;
      int fd = *(int *)tempesp;
      tempesp+=4;
      if(*tempesp>PHYS_BASE)
        process_exit();
      // void* buffer = *(void *)tempesp;

      break;
    }
    case SYS_WRITE:
    {
      char* tempesp = (char *)f-> esp;

      tempesp+=4;
      int fd = *(int *)tempesp;
      tempesp+=4;
      if(*(char**)tempesp>PHYS_BASE)
        process_exit();
      char *buffer = *(char **)tempesp;
      tempesp+=4;
      unsigned size = *(unsigned *)tempesp;
      if(fd==1)
        putbuf(buffer,size);

      // char* enter="\n";
      // putbuf(enter,1);

      break;
    }
    case SYS_SEEK:
    {
      break;
    }
    case SYS_TELL:
    {
      break;
    }
    case SYS_CLOSE:
    {
      break;
    }
    case SYS_MMAP:
    {
      break;
    }
    case SYS_MUNMAP:
    {
      break;
    }
    case SYS_CHDIR:
    {
      break;
    }
    case SYS_MKDIR:
    {
      break;
    }
    case SYS_READDIR:
    {
      break;
    }
    case SYS_ISDIR:
    {
      break;
    }
    case SYS_INUMBER:
    {
      break;
    }
    default:
      break;

  }
  // thread_exit ();
}
