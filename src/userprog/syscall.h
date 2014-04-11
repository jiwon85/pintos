#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <stdbool.h>
#include "filesys/file.h"

typedef int pid_t;

void syscall_init (void);
bool create(const char *file, unsigned initial_size);
int open(const char *file);
int write (int fd, const void *buffer, unsigned size);
int wait (pid_t pid);
int assign_fd(struct file* filePtr);
int filesize (int fd);
unsigned tell (int fd);


#endif /* userprog/syscall.h */
