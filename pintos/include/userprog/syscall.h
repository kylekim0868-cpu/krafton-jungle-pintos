#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"

/* 선언 : lock filesys_lock 이라는 이름과 타입의 변수가 어딘가에 있다는 약속  */
extern struct lock filesys_lock;

void syscall_init (void);

int syscall_open(char *file);
int syscall_filesize(int fd);
bool fd_list_init(struct thread* t);
/* read  system call */
int syscall_read (int fd, void *buffer, unsigned size);
/* write system call */
int syscall_write (int fd, void *buffer, unsigned size);
/* exit  system call */
void syscall_exit(int status);

#endif /* userprog/syscall.h */
