#include "userprog/syscall.h"
#include <stdio.h>
#include "lib/kernel/stdio.h"
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "pagedir.h"
#include "threads/pte.h"

static void syscall_handler (struct intr_frame *);

//System call functions
void halt (void) NO_RETURN;
void exit (int status,struct intr_frame *) NO_RETURN;
pid_t exec (const char *file);
int wait (pid_t);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned length);
int write (int fd, const void *buffer, unsigned length);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  uint32_t sys_call_num;
  void *param_1;
  void *param_2;
  void *param_3;  
  
  sys_call_num = *(uint32_t*)f->esp;

  param_1 = *(void **)(f->esp + 4);
  param_2 = *(void **)(f->esp + 8);
  param_3 = *(void **)(f->esp + 12);
  switch(sys_call_num)
  {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      exit((int)param_1,f);
      break;
    case SYS_EXEC:
      f->eax = exec((char *)param_1);
      break;  
    case SYS_WAIT:
      f->eax = wait((pid_t)param_1);
      break; 
    case SYS_CREATE:
      create((char *)param_1, (uint32_t)param_2);
      break;
    case SYS_REMOVE:
      remove((char *)param_1);
      break; 
    case SYS_OPEN:
      open((char *)param_1);
      break; 
    case SYS_FILESIZE:
      filesize((int)param_1);
      break;
    case SYS_READ:
      read((int)param_1, param_2, (uint32_t)param_3);
      break;    
    case SYS_WRITE:
      write((int)param_1, param_2, (uint32_t)param_3);
      break;
    case SYS_SEEK:
      seek((int)param_1, (uint32_t)param_2);
      break; 
    case SYS_TELL:
      tell((int)param_1);
      break; 
    case SYS_CLOSE:
      close((int)param_1);
      break; 
  }
}

void 
halt (void) 
{

}

void 
exit (int status,struct intr_frame *f)
{
	struct thread *t = thread_current();
	t->status_code = status;
  thread_exit();
}

pid_t 
exec (const char *file)
{
	if (user_readable_string(file))
	{
		int a = process_execute(file);
		if (a == TID_ERROR) return -1;
		return a;
	}
	else return -1;
}

/*
int
wait (pid_t pid)
{
int a = syscall1(SYS_WAIT, pid);
if (thread_current()->status == THREAD_BLOCKED)
{
thread_block();
return syscall1(SYS_WAIT, pid);
}
else return a;
}
*/

int 
wait (pid_t pid)
{
	return process_wait(pid);
}

bool 
create (const char *file, unsigned initial_size)
{

}

bool 
remove (const char *file)
{

}

int 
open (const char *file)
{

}

int 
filesize (int fd)
{

}

int 
read (int fd, void *buffer, unsigned length)
{


}

int
write (int fd, const void *buffer, unsigned size)
{
  if(user_readable_string(buffer)) 
  {
    if(fd == 1)
    {
      putbuf(buffer, size);
      return size;
    }
    //Other stuff
  }
  else
  {
    thread_exit();
    return 0;
  }
}

void 
seek (int fd, unsigned position)
{

}

unsigned 
tell (int fd)
{

}

void 
close (int fd)
{

}