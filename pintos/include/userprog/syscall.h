#include "threads/synch.h"

/* 선언 : lock filesys_lock 이라는 이름과 타입의 변수가 어딘가에 있다는 약속  */
extern struct lock filesys_lock;

#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);

/* write system call */
int syscall_write (int fd, void *buffer, unsigned size);

#endif /* userprog/syscall.h */
