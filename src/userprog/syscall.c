#include "userprog/syscall.h"
#include <syscall.h>
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{   
  uint32_t sys_call_num;
  uint32_t param_1;
  uint32_t param_2;
  uint32_t param_3;
  
  sys_call_num = f->ebp + 4;
  param_1 = f->ebp + 8;
  param_2 = f->ebp + 12;
  param_3 = f->ebp + 16;
  switch(sys_call_num)
  {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      exit(param_1);
      break;
    case SYS_EXEC:
      exec(param_1);
      break;  
    case SYS_WAIT:
      wait(param_1);
      break; 
    case SYS_CREATE:
      create(param_1, param_2);
      break;
    case SYS_REMOVE:
      remove(param_1);
      break; 
    case SYS_OPEN:
      open(param_1);
      break; 
    case SYS_FILESIZE:
      filesize(param_1);
      break;
    case SYS_READ:
      read(param_1, param_2, param_3);
      break;    
    case SYS_WRITE:
      write(param_1, param_2, param_3);
      break;
    case SYS_SEEK:
      seek(param_1, param_2);
      break; 
    case SYS_TELL:
      tell(param_1);
      break; 
    case SYS_CLOSE:
      close(param_1);
      break; 
  }
}

void 
halt (void) 
{

}

void 
exit (int status)
{

}

pid_t 
exec (const char *file)
{

}

int 
wait (pid_t pid)
{

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
  if(fd == 1)
  {
    putbuf(buffer, size);
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


