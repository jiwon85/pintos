#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "lib/syscall-nr.h"
#include "threads/vaddr.h"
#include "threads/synch.h"


static struct file ** fd_table;
int fd_table_size;
struct lock *filesys_lock; //is this all i need?
struct condition * cond;




static void syscall_handler (struct intr_frame *);
bool create(const char *file, unsigned initial_size);
int open(const char *file);
int write (int fd, const void *buffer, unsigned size);
int wait (pid_t pid);
int assign_fd(struct file* filePtr);
bool remove(const char * file);
int filesize(int fd);
int read(int fd, const void* buffer, unsigned size);
void seek(int fd, unsigned position);
unsigned tell(int fd);
void close(int fd);
void exit(int status);
pid_t exec(const char *cmd_line);
void halt();
void clearTable(int* fd, int size);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  fd_table_size = 100;
  fd_table = malloc(sizeof(struct file * )*fd_table_size);
  filesys_lock = malloc(sizeof(struct lock));
  cond = malloc(sizeof(struct condition));
  lock_init(filesys_lock);
  cond_init(cond);

}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{ 


  
  // SYS_HALT,                   /* Halt the operating system. */
  //   SYS_EXIT,                   /* Terminate this process. */
  //   SYS_EXEC,                   /* Start another process. */
  //   SYS_WAIT,                   /* Wait for a child process to die. */
  //   SYS_CREATE,                 /* Create a file. */
  //   SYS_REMOVE,                 /* Delete a file. */
  //   SYS_OPEN,                   /* Open a file. */
  //   SYS_FILESIZE,               /* Obtain a file's size. */
  //   SYS_READ,                   /* Read from a file. */
  //   SYS_WRITE,                  /* Write to a file. */
  //   SYS_SEEK,                   /* Change position in a file. */
  //   SYS_TELL,                   /* Report current position in a file. */
  //   SYS_CLOSE, 


  int syscallnum = getSysCallNumber(f->esp);
  //printf("i'm in syscall_handler about to go to %d\n", syscallnum);
  void* arg0;
  void* arg1;
  void* arg2;
  if(syscallnum == SYS_EXEC || syscallnum == SYS_EXIT || syscallnum == SYS_WAIT || syscallnum == SYS_REMOVE || 
    syscallnum == SYS_OPEN  || syscallnum == SYS_FILESIZE  || syscallnum == SYS_TELL  || syscallnum == SYS_CLOSE){
    //retrieve
    arg0 = (f->esp + 4);
  }
  else if(syscallnum == SYS_CREATE || syscallnum == SYS_SEEK){
    arg0 = (f->esp + 4);
    arg1 = (f->esp + 8);
  }
  else if(syscallnum == SYS_READ || syscallnum == SYS_WRITE){
    arg0 = (f->esp + 4);
    arg1 = (f->esp + 8);
    arg2 = (f->esp + 12);
    // printf("arg0 is %p\n", arg0);
    // printf("arg1 is %p\n", arg1);
    // printf("arg2 is %p\n", arg2);
  }

  int tempNum;
  unsigned tempUnsigned;
  const char * tempStr;

  switch(syscallnum){
    case SYS_HALT:
      halt();
      break;
    case SYS_EXEC:
      tempNum = *(int *) arg0;
      f->eax = exec(tempNum);
      break;
    case SYS_EXIT:
      tempNum = *(int *) arg0;
      exit(tempNum);
      break;
    case SYS_WAIT:
      tempNum = *(int *) arg0;
      f->eax = wait(tempNum);
      break;
    case SYS_CREATE:
      tempNum = *(int *) arg0;
      tempStr = (char *) tempNum;
      tempUnsigned = *((unsigned *) arg1);
      // if(!tempStr)
      //   exit(-1);
      //lock_acquire(filesys_lock);
      f->eax = create(tempStr, tempUnsigned);
      break;
    case SYS_REMOVE:
      tempNum = *(int *) arg0;
      tempStr = (char *) tempNum;
      f->eax = remove(tempStr);
      break;
    case SYS_OPEN:
      tempNum = *(int *) arg0;
      tempStr = (char *) tempNum;
      f->eax = open(tempStr);
      break;
    case SYS_FILESIZE:
      tempNum = *(int *) arg0;
      f->eax = filesize(tempNum);
      break;
    case SYS_READ:
      tempNum = *(int *) arg0;
      tempUnsigned = *((unsigned *) arg2);
      tempStr = (char *) *(int *) arg1;
      f->eax = read(tempNum, tempStr, tempUnsigned);
      break;
    case SYS_WRITE:
      tempNum = *(int *) arg0;
      tempUnsigned = *((unsigned *) arg2);
      tempStr = (char *) *(int *) arg1;
      f->eax = write(tempNum, tempStr, tempUnsigned);
      break;
    case SYS_SEEK:
      tempNum = *(int *) arg0;
      tempUnsigned = *((unsigned *) arg1);
      seek(tempNum, tempUnsigned);
      break;
    case SYS_TELL:
      tempNum = *(int *) arg0;
      f->eax = tell(tempNum);
      break;
    case SYS_CLOSE:
      tempNum = *(int *) arg0;
      close(tempNum);
      break;
    default: 
      thread_exit();
      break;
  }


}

bool isValidPtr(void * ptr){
  if(!ptr || !is_user_vaddr(ptr) || !pagedir_get_page(thread_current()->pagedir, (void *) ptr)){ //  || is_kernel_vaddr(ptov(file)) 
    return false;
  }
  return true;
}

//create system call
bool create(const char *file, unsigned initial_size){
  //const char *temp = "";
  //printf("%s is the file name with size %d\n", file, initial_size);
  if(!isValidPtr((void *) file)){ //  || is_kernel_vaddr(ptov(file)) 
    exit(-1);
  }


  lock_acquire(filesys_lock);
  int success = filesys_create(file, initial_size);
  lock_release(filesys_lock);
  //printf("success: %d\n", success);
  return success;
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
  if(!isValidPtr((void *) file))
    exit(-1);
  lock_acquire(filesys_lock);
	struct file * fileObj = filesys_open(file); 
  lock_release(filesys_lock);
  if(fileObj == NULL)
    return -1;
  int fd = assign_fd(fileObj);
  thread_current()->fd_list[thread_current()->fd_index] = fd;
  thread_current()->fd_index++;
	return assign_fd(fileObj); //if failed
}

int write (int fd, const void *buffer, unsigned size){
  //printf("Write function: fd value is %d\n", fd);
  if(!isValidPtr(buffer))
    exit(-1);

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
	//printf("hello we are in wait thank you\n");
  
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

//returns size of file
int filesize (int fd) { 
  struct file * file = fd_table[fd];
  return file_length (file); 
}

//Returns the position of the next byte to be read or written in open file fd
unsigned tell (int fd) {
  struct file * file = fd_table[fd];
  return file_tell (file);
}

int getSysCallNumber(void * address){
  if(!is_user_vaddr(address) || address < 0x08048000){
    exit(-1);
  }
  else{
    void * pointer = pagedir_get_page(thread_current()->pagedir, address);
    if(!pointer){
      exit(-1);
    }
    return *((int *) pointer);

  }
}

void close(int fd){
  if(fd<3 || fd>fd_table_size)
    exit(-1);
  if(fd_table[fd] != NULL){
    file_close(fd_table[fd]);
    fd_table[fd] = NULL;
  }
}

int read(int fd, const void *buffer, unsigned size){

 if(fd < 0 || fd > fd_table_size || !isValidPtr(buffer)){ //fd not correct
    exit(-1);
  }
  else if(fd != 0){
    struct file * filePtr = (struct file *) fd_table[fd];
    if(filePtr == NULL || file_is_writable(filePtr)){ //fd does not actually point to a file
      exit(-1);
    }
    int bytes_read = file_read(filePtr, buffer, size);
    return bytes_read;
  }
  else{
    return input_getc();
  }
}

bool remove(const char * file){
  if(!isValidPtr((void *) file))
    exit(-1);
  lock_acquire(filesys_lock);
  bool success = filesys_remove(file);
  lock_release(filesys_lock);
  return success;
}

pid_t exec(const char * cmd_line){
  if(!isValidPtr((void *)cmd_line))
    exit(-1);
  int childTid = process_execute(cmd_line);
  while(thread_current()->tid != childTid){
    printf("in the while loop\n");
  }

  return childTid; 
}

void exit(int status){
  if(status < -1 || status > 255)
    status = -1;
  printf("%s: exit(%d)\n", thread_current()->thread_name, status);
  thread_exit();
}

void halt(){
  shutdown_power_off();
}

void seek(int fd, unsigned position){
  if(fd < 0 || fd > fd_table_size){ //fd not correct
    return;
  }
  else if(fd != 0){
    struct file * filePtr = (struct file *) fd_table[fd];
    if(filePtr == NULL || file_is_writable(filePtr)){ //fd does not actually point to a file
      return;
    }
    file_seek(filePtr, position);
  }
  
}

void clearTable(int* fd, int size){
  int i;
  for(i=0; i<size; i++){
    close(fd[i]);
  }
}



