#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include <stdbool.h>
#include "filesys/file.h"

typedef int pid_t;

void syscall_init (void);
void clearTable(int* fd, int size);


#endif /* userprog/syscall.h */
