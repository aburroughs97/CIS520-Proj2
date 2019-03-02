#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

typedef int pid_t;
void syscall_init (void);
int user_readable(void * uddr);
int user_writable(void * uddr);

#endif /* userprog/syscall.h */
