#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lib/kernel/stdio.h"
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "pagedir.h"
#include "threads/pte.h"
#include "../src/devices/shutdown.h"
#include "../src/devices/input.h"
#include "../src/filesys/filesys.h"
#include "../src/filesys/file.h"
#include "vm/page.h"

static void syscall_handler (struct intr_frame *);

//System call functions
void halt (void) NO_RETURN;
void exit (int status) NO_RETURN;
pid_t exec (const char *file,void *);
int wait (pid_t);
bool create (const char *file, unsigned initial_size,void *);
bool remove (const char *file,void *);
int open (const char *file,void *);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size, void *);
int write (int fd, const void *buffer, unsigned size,void *);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  vm_init();
}

static void
syscall_handler (struct intr_frame *f) 
{
  uint32_t sys_call_num;
  void **param_1;
  void **param_2;
  void **param_3;  
  bool param_1_valid;
  bool param_2_valid;
  bool param_3_valid;
  if(!user_readable(f->esp, 4,f->esp))
  {
    exit(-1);
    return;
  }

  sys_call_num = *(uint32_t*)f->esp;

  param_1 = (void **)(f->esp + 4);
  param_2 = (void **)(f->esp + 8);
  param_3 = (void **)(f->esp + 12);
  
  param_1_valid = user_readable(param_1, 4,f->esp);
  param_2_valid = user_readable(param_2, 4,f->esp);
  param_3_valid = user_readable(param_3, 4,f->esp);


  switch(sys_call_num)
  {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      if(param_1_valid)
      {
        exit((int)*param_1);
        return;
      }
      break;
    case SYS_EXEC:
      if(param_1_valid) 
      {
        f->eax = exec((char *)*param_1,f->esp);
        return;
      }
      break;  
    case SYS_WAIT:
      if(param_1_valid) 
      {
        f->eax = wait((pid_t)*param_1);
        return;
      }   
      break; 
    case SYS_CREATE:
      if(param_1_valid && param_2_valid) 
      {
        f->eax = create((char *)*param_1, (uint32_t)*param_2,f->esp);
        return;
      }    
      break;
    case SYS_REMOVE:
      if(param_1_valid) 
      {
        f->eax = remove((char *)*param_1,f->esp);
        return;
      }      
      break; 
    case SYS_OPEN:
      if(param_1_valid) 
      {
        f->eax = open((char *)*param_1,f->esp);
        return;
      }    
      break; 
    case SYS_FILESIZE:
      if(param_1_valid) 
      {
        f->eax = filesize((int)*param_1);
        return;
      }    
      break;
    case SYS_READ:
      if(param_1_valid && param_2_valid && param_3_valid) 
      {
        f->eax = read((int)*param_1, *param_2, (uint32_t)*param_3,f->esp);
        return;
      }    
      break;    
    case SYS_WRITE:
      if(param_1_valid && param_2_valid && param_3_valid) 
      {
        f->eax = write((int)*param_1, *param_2, (uint32_t)*param_3,f->esp);
        return;
      }       
      break;
    case SYS_SEEK:
      if(param_1_valid && param_2_valid) 
      {
        seek((int)*param_1, (uint32_t)*param_2);
        return;
      }       
      break; 
    case SYS_TELL:
      if(param_1_valid) 
      {
        f->eax = tell((int)*param_1);
        return;
      }   
      break; 
    case SYS_CLOSE:
      if(param_1_valid) 
      {
        close((int)*param_1);
        return;
      }     
      break; 
    case SYS_MMAP:
      if(param_1_valid && param_2_valid)
      {
        f->eax = mmap((int)*param_1, *param_2);
        return;
      }
      break;
    case SYS_MUNMAP:
      if(param_1_valid){
        munmap((int)*param_1);
        return;
      }
      break;
  }
  exit(-1);
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
exec (const char *file, void *esp)
{
	if (user_readable_string(file,esp))
	{
		int a = process_execute(file);
		if (a == TID_ERROR) return -1;
		struct thread * t = get_thread(a);
		thread_current()->waiting_on = a;
		enum intr_level old_level = intr_disable();
		thread_block();
		intr_set_level(old_level);
		thread_current()->waiting_on = 0;
		if (t->status_code == -1)
		{
			t->parent = NULL;
			list_remove(&t->parentelem);
			cleanup_thread(t,true);
			return -1;
		}
		return a;
	}
	else exit(-1);
}

int 
wait (pid_t pid)
{
	return process_wait(pid);
}

bool 
create (const char *file, unsigned initial_size, void *esp)
{
  int len;
  if(user_readable_string(file,esp))
  {
    len = strlen(file);
    if(len > 0 && len <= 14)
    {
      return filesys_create(file, initial_size);
    }
  }
  else 
  {
    exit(-1);
  }
  return false;
}

bool 
remove (const char *file,void *esp)
{
  int len;
  if(user_readable_string(file,esp))
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
open (const char *file,void *esp)
{
   if(user_readable_string(file,esp))
  {
    if(strcmp(file, "") == 0)
    {
      return -1;
    }
    struct file *open = filesys_open(file);
    if(open != NULL)
    {
      struct open_file_struct *ofs;
      int fd = thread_current()->cur_fd_num++;
      ofs = malloc(sizeof(struct open_file_struct));
      ofs->fd = fd;
      ofs->file = open;
      list_push_back(&thread_current()->open_files, &ofs->elem);
      return fd;
    }else
    {
      return -1;
    }
  }else
  {
    exit(-1);
  }
  return -1;
}

int 
filesize (int fd)
{
  struct file *f = get_open_file(fd);
  return file_length(f);
}

int 
read (int fd, void *buffer, unsigned size, void *esp)
{
  if(user_writable(buffer, size,esp))
  {
    if(fd < 0 || fd >= thread_current()->cur_fd_num || fd == 1)
    {
      return -1;
    }
    else if(fd == 0)
    {
      return input_getc();
    }
    else
    {
       struct file *f = get_open_file(fd);
       int i = file_read(f, buffer, size);
	   return i;
    }
  }
  else
  {
    exit(-1);
    return 0;
  }
}

int
write (int fd, const void *buffer, unsigned size, void *esp)
{
  if(user_readable(buffer, size,esp)) 
  {
    if(fd <= 0 || fd >= thread_current()->cur_fd_num)
    {
      return -1;
    }
    else if(fd == 1)
    {
      putbuf(buffer, size);
      return size;
    }
    else{
      struct file *f = get_open_file(fd);
      return file_write(f, buffer, size);
    }
  }
  else
  {
    exit(-1);
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
  if(fd <= 0 || fd >= thread_current()->cur_fd_num)
  {
    return -1;
  }
  if(fd != 0 && fd != 1)
  {
    struct list *l = &thread_current()->open_files;
    for(struct list_elem * e = list_begin(l); e != list_end(l); e = list_next(e))
    {
      struct open_file_struct *elem = list_entry (e, struct open_file_struct, elem);
      if (elem->fd == fd)
      {
        list_remove(e);
		file_close(elem->file);
        free(elem);
        break;
      }
    }
  }else
  {
    exit(-1);
  }
}

mapid_t 
mmap (int fd, void *addr)
{
  if(fd <= 1 || fd >= thread_current()->cur_fd_num)
  {
    return -1;
  }
  int size = filesize(fd);
  if(size != 0 && addr != NULL && addr != 0)
  {
    //TODO: Check addr page alignment and valid page range
    
    int mid = thread_current()->cur_mapid++;

    struct file *file = get_open_file(fd);
    if(file == NULL)
    {
      return -1;
    }

    off_t length = file_length (file);

    if (length <= 0) 
    {
      return -1;
    }

    int pag_num = length / PGSIZE + 1;
    off_t offset = 0;
    uint32_t read_bytes = length;

    while (read_bytes > 0)
    {
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      if(vm_install_page(addr, file, offset, page_read_bytes, true))
      {
        struct map_item *m = malloc(sizeof(*m));
        m->map_id = mid;

        struct thread * t = thread_current();
	      struct spte to_find;
	      to_find.pte = lookup_page(t->pagedir, addr, false);
	      struct spte * spte = hash_entry(hash_find(&t->spt, &to_find.elem), struct spte, elem);
        
        m->page = addr;
        m->spt_entry = spte;

        list_push_front(&thread_current()->mapped_list, &m->elem);
      }

      read_bytes -= page_read_bytes;
      addr += PGSIZE;
      offset += PGSIZE;
    } 
  }
  else 
  {
    return -1;
  }
}

void 
munmap (mapid_t mapping)
{
  if(mapping <= 0)
  {
    return;
  }

  struct thread * t = thread_current();
  struct list_elem * e;

  for(e = list_begin(&t->mapped_list); e != list_end(&t->mapped_list); e = list_next(e))
  {
    struct map_item * item = list_entry(e, struct map_item, elem);

    if(item->map_id == mapping)
    {
      struct spte *spt_entry = item->spt_entry;
      file_seek(spt_entry->file, 0);
      if(pagedir_is_dirty(thread_current()->pagedir, item->page)) 
      {

      }

      vm_free_page(item->page);

      e = list_remove(&item->elem);
      free(item);
    }

  }
}