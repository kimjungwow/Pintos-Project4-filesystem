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
		char* tempesp = (char *)f->esp;
		tempesp+=4;
		if (*(uint32_t *)tempesp<PHYS_BASE)
			thread_current()->returnstatus=get_user((uint8_t *)tempesp);
		else
			thread_current()->returnstatus=-1;
		thread_exit ();
		break;
	}
	case SYS_EXEC:
	{
		char* tempesp = (char *)f->esp;
		tempesp+=4;
		if(*(uint32_t *)tempesp>PHYS_BASE)
			thread_exit();
		char* cmd_line = *tempesp;
		f->eax = (uint32_t)process_execute(cmd_line);
		break;
	}

/*Waits for a child process pid and retrieves the child's exit status.

   If pid is still alive, waits until it terminates. Then, returns the status
   that pid passed to exit. If pid did not call exit(), but was terminated by the
   kernel (e.g. killed due to an exception), wait(pid) must return -1. It is perfectly legal for a parent process to wait for child processes that have already terminated by the time the parent calls wait, but the kernel must still allow the parent to retrieve its child's exit status, or learn that the child was terminated by the kernel.

   wait must fail and return -1 immediately if any of the following conditions is true:

   pid does not refer to a direct child of the calling process. pid is a direct child of the calling process if and only if the calling process received pid as a return value from a successful call to exec.
   Note that children are not inherited: if A spawns child B and B spawns child process C, then A cannot wait for C, even if B is dead. A call to wait(C) by process A must fail. Similarly, orphaned processes are not assigned to a new parent if their parent process exits before they do.
   The process that calls wait has already called wait on pid. That is, a process may wait for any given child at most once.
   Processes may spawn any number of children, wait for them in any order, and may even exit without having waited for some or all of their children. Your design should consider all the ways in which waits can occur. All of a process's resources, including its struct thread, must be freed whether its parent ever waits for it or not, and regardless of whether the child exits before or after its parent.

   You must ensure that Pintos does not terminate until the initial process exits. The supplied Pintos code tries to do this by calling process_wait()(in userprog/process.c) from main() (in threads/init.c). We suggest that you implement process_wait() according to the comment at the top of the function and then implement the wait system call in terms of process_wait().

   Implementing this system call requires considerably more work than any of the rest.*/

	case SYS_WAIT:
	{
		break;
	}
	case SYS_CREATE:
	{
		char* tempesp = (char *)f->esp;
		tempesp+=4;
		if(*(uint32_t *)tempesp>PHYS_BASE)
		{
			thread_current()->returnstatus=-1;
			thread_exit();
		}

		if((get_user(*(uint8_t **)tempesp)==-1)||(*tempesp==NULL))
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
			tempesp+=4;
			unsigned initial_size = *(unsigned *)tempesp;
			if((strlen(file)<=14)&&(strlen(file)>0))
			{
				answer = filesys_create(file, initial_size);

			}
		}
		f->eax=answer;
		break;
	}
	case SYS_REMOVE:
	{
		char* tempesp = (char *)f->esp;
		tempesp+=4;
		if(*(uint32_t *)tempesp>PHYS_BASE)
			thread_exit();
		char *file;

		file = palloc_get_page (0);
		if (file == NULL)
			return TID_ERROR;
		strlcpy (file, *(char **)tempesp, PGSIZE);
		bool answer = filesys_remove(file);
		f->eax=answer;

		break;
	}
	case SYS_OPEN:
	{
		char* tempesp = (char *)f->esp;
		tempesp+=4;
		if(*(uint32_t *)tempesp>PHYS_BASE)
		{
			thread_current()->returnstatus=-1;
			thread_exit();
		}

		if((get_user(*(uint8_t **)tempesp)==-1)||(*tempesp==NULL))
		{
			thread_current()->returnstatus=-1;
			thread_exit();
		}
		char *file;

		file = palloc_get_page (0);
		if (file == NULL)
			return TID_ERROR;
		strlcpy (file, *(char **)tempesp, PGSIZE);
		struct file* openedfile = filesys_open(file);
		int fd;
		if (openedfile==NULL)
		{
			fd=-1;
		}
		else
		{
			fd= thread_current()->nextfd;
			thread_current()->fdtable[fd]=openedfile;
			thread_current()->nextfd++;
		}
		f->eax=fd;

		break;
	}
	case SYS_FILESIZE:
	{
		char* tempesp = (char *)f->esp;
		tempesp+=4;
		int fd = *(int *)tempesp;
		struct file* filetoread = thread_current()->fdtable[fd];
		int filesize = (int)(file_length(filetoread));
		f->eax=filesize;
		break;
	}

	/*Reads size bytes from the file open as fd into buffer.
	   Returns the number of bytes actually read (0 at end of file), or -1
	   if the file could not be read (due to a condition other than end of file).
	   Fd 0 reads from the keyboard using input_getc().*/
	case SYS_READ:
	{

		char* tempesp = (char *)f->esp;

		tempesp+=4;
		int fd = *(int *)tempesp;
		tempesp+=4;
		if(*(uint32_t *)tempesp>PHYS_BASE)
		{
			thread_current()->returnstatus=-1;
			thread_exit();
		}
		char *buffer;
		buffer = *(char **)tempesp;
		tempesp+=4;
		unsigned size = *(unsigned *)tempesp;
		unsigned i;
		uint8_t fdzero;
		if(fd == 0)
		{
			for (i=0; i<size; i++)
			{
				fdzero = input_getc();
				*buffer = (char)fdzero;
				buffer++;
			}
			f->eax=size;
		}
		else if (fd>1)
		{
			if (size==0)
			{
				f->eax=0;
			}
			else
			{
				struct file* filetoread = thread_current()->fdtable[fd];
				if(get_user(*(uint8_t **)filetoread)==-1)
				{
					f->eax=-1;
				}
				else
				{
					off_t readbytes = file_read(filetoread,(void *) buffer, (off_t)size);
					f->eax=(int)readbytes;
				}
			}
		}
		break;
	}
	case SYS_WRITE:
	{
		char* tempesp = (char *)f->esp;

		tempesp+=4;
		int fd = *(int *)tempesp;
		tempesp+=4;
		if(*(uint32_t *)tempesp>PHYS_BASE)
		{
			thread_current()->returnstatus=-1;
			thread_exit();
		}

		char *buffer = *(char **)tempesp;
		tempesp+=4;
		unsigned size = *(unsigned *)tempesp, temp;
		if(fd==1)
		{
			while(size>100)
			{
				putbuf(buffer,100);
				buffer+=100;
				size-=100;
			}
			putbuf(buffer,size);
		}
		else
		{
			if (fd<=0)
			{
				thread_current()->returnstatus=-1;
				thread_exit();
			}
			else if(fd>sizeof(thread_current()->fdtable)/sizeof(struct file*))
			{
				thread_current()->returnstatus=-1;
				thread_exit();
			}
			else if(thread_current()->fdtable[fd]==NULL)
			{
				thread_current()->returnstatus=-1;
				thread_exit();
			}
			else if((get_user((uint8_t *)buffer)==-1)||(buffer==NULL))
			{
				thread_current()->returnstatus=-1;
				thread_exit();
			}
			struct file* filetowrite = thread_current()->fdtable[fd];
			off_t writebytes = file_write(filetowrite,(void *) buffer, (off_t)size);
			f->eax=(int)writebytes;
		}
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
		char* tempesp = (char *)f->esp;

		tempesp+=4;
		int fd = *(int *)tempesp;

		struct file* filetoclose = thread_current()->fdtable[fd];
		if(filetoclose!=NULL)
		{
			thread_current()->fdtable[fd]=NULL;
			file_close(filetoclose);
		}
		else
		{
			thread_current()->returnstatus=-1;
			thread_exit();
		}
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
