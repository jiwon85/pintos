#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "filesys/filesys.h"



static void syscall_handler (struct intr_frame *);
bool create(const char *file, unsigned initial_size);
int open(const char *file);
int write (int fd, const void *buffer, unsigned size);
int wait (pid_t pid);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
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

//open system call
int open(const char *file){
	printf("we're about to open the file\n");
	struct file * fileObj = filesys_open(*file); 
	printf("we're about to return from open bitches\n");
	return -1; //if failed
}

int write (int fd, const void *buffer, unsigned size){
	return -1;
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
