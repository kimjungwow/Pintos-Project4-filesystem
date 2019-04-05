#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
extern struct lock handlesem;
extern int filenum;
// typedef extern bool jungwoo;
#endif /* userprog/syscall.h */
