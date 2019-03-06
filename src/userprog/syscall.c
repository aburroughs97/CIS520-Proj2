#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include "lib/kernel/stdio.h"
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "pagedir.h"
#include "threads/pte.h"
#include "../src/devices/shutdown.h"
#include "../src/filesys/filesys.h"
#include "../src/filesys/file.h"

static void syscall_handler (struct intr_frame *);

//System call functions
void halt (void) NO_RETURN;
void exit (int status) NO_RETURN;
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
  void **param_1;
  void **param_2;
  void **param_3;   
  
  if(!user_readable(f->esp, 4))
  {
    exit(-1);
    return;
  }

  sys_call_num = *(uint32_t*)f->esp;

  param_1 = (void **)(f->esp + 4);
  param_2 = (void **)(f->esp + 8);
  param_3 = (void **)(f->esp + 12);

  switch(sys_call_num)
  {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      if(!user_readable(param_1, 4)) 
      {
        exit(-1);
        return;
      }
      exit((int)*param_1);
      break;
    case SYS_EXEC:
      if(!user_readable(param_1, 4)) 
      {
        exit(-1);
        return;
      }
      f->eax = exec((char *)*param_1);
      break;  
    case SYS_WAIT:
      if(!user_readable(param_1, 4)) 
      {
        exit(-1);
        return;
      }    
      f->eax = wait((pid_t)*param_1);
      break; 
    case SYS_CREATE:
      if(!user_readable(param_1, 4) || !user_readable(param_2, 4)) 
      {
        exit(-1);
        return;
      }    
      f->eax = create((char *)*param_1, (uint32_t)*param_2);
      break;
    case SYS_REMOVE:
      if(!user_readable(param_1, 4)) 
      {
        exit(-1);
        return;
      }      
      f->eax = remove((char *)*param_1);
      break; 
    case SYS_OPEN:
      if(!user_readable(param_1, 4)) 
      {
        exit(-1);
        return;
      }    
      open((char *)*param_1);
      break; 
    case SYS_FILESIZE:
      if(!user_readable(param_1, 4)) 
      {
        exit(-1);
        return;
      }    
      f->eax = filesize((int)*param_1);
      break;
    case SYS_READ:
      if(!user_readable(param_1, 4) || !user_readable(param_2, 4) || !user_readable(param_3, 4)) 
      {
        exit(-1);
        return;
      }    
      read((int)*param_1, *param_2, (uint32_t)*param_3);
      break;    
    case SYS_WRITE:
      if(!user_readable(param_1, 4) || !user_readable(param_2, 4) || !user_readable(param_3, 4)) 
      {
        exit(-1);
        return;
      }       
      write((int)*param_1, *param_2, (uint32_t)*param_3);
      break;
    case SYS_SEEK:
      if(!user_readable(param_1, 4) || !user_readable(param_2, 4)) 
      {
        exit(-1);
        return;
      }       
      seek((int)*param_1, (uint32_t)*param_2);
      break; 
    case SYS_TELL:
      if(!user_readable(param_1, 4)) 
      {
        exit(-1);
        return;
      }   
      f->eax = tell((int)*param_1);
      break; 
    case SYS_CLOSE:
      if(!user_readable(param_1, 4)) 
      {
        exit(-1);
        return;
      }     
      close((int)*param_1);
      break; 
    default:
      exit(-1);
      break;
  }
}

void 
halt (void) 
{
  shutdown_power_off();
}

void 
exit (int status)
{
	struct thread *t = thread_current();
	t->status_code = status;
	printf("%s: exit(%i)\n", t->name, status);
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

int 
wait (pid_t pid)
{
	return process_wait(pid);
}

bool 
create (const char *file, unsigned initial_size)
{
  int len;
  if(user_readable_string(file))
  {
    len = strlen(file);
    if(len > 0 && len <= 14)
    {
      return filesys_create(file, initial_size);
    }
  }
  else {
    exit(-1);
  }
  return false;
}

bool 
remove (const char *file)
{
  int len;
  if(user_readable_string(file))
  {
    len = strlen(file);
    if(len > 0 && len <= 14)
    {
      return filesys_remove(file);
    }
  }
  return false;
}

int 
open (const char *file)
{

}

int 
filesize (int fd)
{
  struct file *f = get_open_file(fd);
  return file_length(f);
}

int 
read (int fd, void *buffer, unsigned length)
{


}

int
write (int fd, const void *buffer, unsigned size)
{
  if(user_readable(buffer, size)) 
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
  struct file *f = get_open_file(fd);
  file_seek(f, position);
}

unsigned 
tell (int fd)
{
  struct file *f = get_open_file(fd);
  return file_tell(f);
}

void 
close (int fd)
{

}