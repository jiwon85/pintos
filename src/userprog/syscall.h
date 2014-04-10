#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
bool create(const char *file, unsigned initial_size);
int open(const char *file);
int write (int fd, const void *buffer, unsigned size);
int wait (pid_t pid);

#endif /* userprog/syscall.h */
