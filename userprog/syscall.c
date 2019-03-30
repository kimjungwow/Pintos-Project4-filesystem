#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "lib/user/syscall.h"
#include "threads/vaddr.h"
#include "lib/kernel/console.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
   occurred. */
static int
get_user (const uint8_t *uaddr)
{
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}

/* Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
       : "=&a" (error_code), "=m" (*udst) : "q" (byte));
  return error_code != -1;
}


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
      if (tempesp<PHYS_BASE)
        thread_current()->returnstatus=get_user((uint8_t *)tempesp);
      else
        thread_current()->returnstatus=-1;

      // if(tempesp>=PHYS_BASE)
      //   {
      //     thread_current()->returnstatus=-1;
      //   }
      // else
      //   {
      //     int status = *(int *)tempesp;
      //     thread_current()->returnstatus=status;
      //   }
      thread_exit ();
      break;
    }
    case SYS_EXEC:
    {
      char* tempesp = (char *)f->esp;
      tempesp+=4;
      if(*tempesp>PHYS_BASE)
        thread_exit();
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
      if(*(uint32_t *)tempesp>PHYS_BASE)
        thread_exit();

      if(get_user(*(uint8_t **)tempesp)==-1)
        {
          thread_current()->returnstatus=-1;
          thread_exit();
        }


      bool answer=false;
      if (*tempesp=='\0')
        {
          thread_current()->returnstatus=-1;
          thread_exit ();
        }
      else
        {
          char *file;

          file = palloc_get_page (0);
          if (file == NULL)
            return TID_ERROR;
          strlcpy (file, *(char **)tempesp, PGSIZE);

          // char* file = *tempesp;
          tempesp+=4;
          unsigned initial_size = *(unsigned *)tempesp;

          if((strlen(file)<=14)&&(strlen(file)>0))
          {
            answer = filesys_create(file, initial_size);
            answer=true;
          }
        }


      f->eax=answer;
      // f->esp-=4;
      // *(bool *)(f->esp) = answer;
      break;
    }
    case SYS_REMOVE:
    {
      char* tempesp = (char *)f->esp;
      tempesp+=4;
      if(*tempesp>PHYS_BASE)
        thread_exit();
      char* file = *tempesp;

      break;
    }
    case SYS_OPEN:
    {
      char* tempesp = (char *)f->esp;
      tempesp+=4;
      if(*tempesp>PHYS_BASE)
        thread_exit();
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
        thread_exit();
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
        thread_exit();
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
