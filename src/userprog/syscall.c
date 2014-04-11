#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "filesys/filesys.h"



static struct file ** fd_table;
int fd_table_size;


static void syscall_handler (struct intr_frame *);
bool create(const char *file, unsigned initial_size);
int open(const char *file);
int write (int fd, const void *buffer, unsigned size);
int wait (pid_t pid);
int assign_fd(struct file* filePtr);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  fd_table_size = 100;
  fd_table = malloc(sizeof(struct file * )*fd_table_size);

}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
}

//create system call
bool create(const char *file, unsigned initial_size){
	return filesys_create(file, initial_size);
}

int assign_fd(struct file* filePtr){
  //we're never going to assign 0,1,2 because they're reserved for STDIN, STDOUT, STDERR
  int i;
  for(i = 3; i<fd_table_size; i++){
    if(fd_table[i] == NULL){
      fd_table[i] = filePtr;
      return i;
    }
  }

  //resize the fd table because there is no more space
  fd_table_size *=2;
  fd_table = realloc (fd_table, fd_table_size);
  fd_table[i] = filePtr;
  return i;


}
//open system call
int open(const char *file){
	printf("we're about to open the file\n");
	struct file * fileObj = filesys_open(*file); 
	printf("we're about to return from open bitches\n");
  if(fileObj == NULL)
    return -1;
	return assign_fd(fileObj); //if failed
}

int write (int fd, const void *buffer, unsigned size){
  if(fd < 0 || fd > fd_table_size){ //fd not correct
    return 0;
  }
  else if(fd != 1){
    struct file * filePtr = (struct file *) fd_table[fd];
    if(filePtr == NULL || file_is_writable(filePtr)){ //fd does not actually point to a file
      return 0;
    }
    int bytes_written = file_write(filePtr, buffer, size);
    file_deny_write(filePtr);
    return bytes_written;
  }
  else{
    putbuf(buffer, size);
    return size;
  }
}

//only if the calling process received pid as a return value from a successful call to exec. 

int wait (pid_t pid) {
	printf("hello we are in wait thank you\n");
  
	//direct child of calling process
	struct thread * current = thread_current();
  	int i;
  	int childFound = 0; 
  	for(i=0; i< current->numChildren; i++){
    	if(current->children[i] == pid){
      		childFound = 1;
      	}
  	}
  	if(!childFound){
    	return -1;
    }

    if(current->calledWait){
    	return -1;
    }

    //wait for child to DIE
    struct thread *threadPtr = getChild(pid);
    if(threadPtr != NULL){
    	while(threadPtr->isDead == 0){} //wait for it to die
    }
	else{
		return -1;
	}

    return threadPtr->exitStatus;
}
