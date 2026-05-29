#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <stdarg.h>
#define BUMP_ALLOC 4096
#define HASH_SIZE 51200 
#define M 0
#define F 1
#define R 2
#define C 3

typedef struct{
  size_t user_size;
  void *ptr;
  unsigned mode;
  void *old_ptr;
}mem_log;

static bool malloc_initialize;
static bool free_initialize;
static bool realloc_initialize;
static bool calloc_initialize;
static bool unistd_exit_initialize;
static bool stdlib_exit_initialize;
static bool syscall_exit_initialize;
static mem_log hash_t[HASH_SIZE];
void __attribute__((constructor)) initialize(){
  static mem_log init_log={0,NULL,1};
  for(int i=0;i<HASH_SIZE;i++){
    hash_t[i]=init_log;
  }
}
int hash_insert(mem_log *log){
  int slot=((uintptr_t)(log->ptr)) % HASH_SIZE;
  do{
    if(hash_t[slot].ptr==NULL){
      hash_t[slot]=(*log);
      return 0;
    }
    if(slot+1==HASH_SIZE)
      slot=0;
    else{
      slot++;
    }
  }while(slot<HASH_SIZE);
  return -1;
}

int hash_del(mem_log *log){
  int slot;
  unsigned search;
  search=0;
  if(log->ptr!=NULL){
    if(log->old_ptr==NULL){
      slot=((uintptr_t)(log->ptr)) % HASH_SIZE;
      do{
        if(hash_t[slot].ptr==log->ptr){
          hash_t[slot].ptr=NULL;
          return 0;
        }
        if(slot+1==HASH_SIZE){
          slot=0;
          search++;
        }else{
          slot++;
          search++;
        }
        if(search==4095)
          return -1;
      }while(slot<HASH_SIZE);
      return -1;
    }else{
      slot=((uintptr_t)(log->old_ptr)) % HASH_SIZE;
      do{
        if(hash_t[slot].ptr==log->old_ptr){
          hash_t[slot].ptr=NULL;
          return 0;
        }
        if(slot+1==HASH_SIZE){
          slot=0;
          search++;
        }else{
          slot++;
          search++;
        }
        if(search==4095)
          return -1;
      }while(slot<HASH_SIZE);
      return -1;
    }
  }
  return -1;
}

void *(*real_malloc)(size_t)=NULL;
void (*real_free)(void *)=NULL;
void *(*real_realloc)(void*,size_t)=NULL;
void *(*real_calloc)(size_t,size_t)=NULL;
void (*real_uniExit)(int)=NULL;
void (*real_stdExit)(int)=NULL;
long (*real_syscall)(long,...)=NULL;
void *calloc(size_t,size_t);

void custom_write(mem_log *log)
{
  static char bytes_allocated[14];
  static char memaddr_name[17];
  static char memaddr_oldname[17];
  if(log->mode==M){
    int bytes_char=snprintf(bytes_allocated,14,"%d",(int)(log->user_size));
    int memaddr_size=snprintf(memaddr_name,17,"%p",log->ptr);
    write(2,"[MALLOC ]\t",10);
    write(2,memaddr_name,memaddr_size);
    write(2,"\t",1);
    write(2,bytes_allocated,bytes_char);
    write(2," Bytes\n",7);
  }
  else if(log->mode==F){
    int memaddr_size=snprintf(memaddr_name,17,"%p",log->ptr);
    write(2,"[FREE   ]\t",10);
    write(2,memaddr_name,memaddr_size);
    write(2,"\n",1);
  }else if(log->mode==R){
    int memaddr_size=snprintf(memaddr_name,17,"%p",log->ptr);
    int memaddr_oldname_size=snprintf(memaddr_oldname,17,"%p",log->old_ptr);
    int bytes_size=snprintf(bytes_allocated,14,"%d",(int)log->user_size);
    write(2,"[REALLOC]\t",10);
    write(2,memaddr_oldname,memaddr_oldname_size);
    write(2," -> ",5);
    write(2,memaddr_name,memaddr_size);
    write(2,"\t(",2);
    write(2,bytes_allocated,bytes_size);
    write(2," Bytes)\n",8);
  }else if(log->mode==C){
    int memaddr_size=snprintf(memaddr_name,17,"%p",log->ptr);
    int bytes_size=snprintf(bytes_allocated,14,"%d",(int)log->user_size);
    write(2,"[CALLOC ]\t",10);
    write(2,memaddr_name,memaddr_size);
    write(2,"\t",1);
    write(2,bytes_allocated,bytes_size);
    write(2," Bytes\n",7);
  }
}

void *malloc(size_t size)
{
  mem_log malloc_log={0,NULL,M,NULL};
  if(malloc_initialize || free_initialize || realloc_initialize || calloc_initialize || unistd_exit_initialize || stdlib_exit_initialize || syscall_exit_initialize){
    static char buffer [BUMP_ALLOC];
    static int offset;
    void *ptr=&buffer[offset];
    offset+=size;
    return ptr;
  }
  if(!real_malloc){
      malloc_initialize=1;
      real_malloc= (void *(*)(size_t))dlsym(RTLD_NEXT,"malloc");
      malloc_initialize=0;
  }
  void *ptr=real_malloc(size);
  malloc_log.user_size=size;
  malloc_log.ptr=ptr;
  hash_insert(&malloc_log);
  custom_write(&malloc_log);
  return ptr;
}

void free(void *ptr){
  mem_log free_log={0,NULL,F,NULL};
  free_log.ptr=ptr;
  if(!real_free){
    free_initialize=1;
    real_free=(void (*)(void *))dlsym(RTLD_NEXT,"free");
    free_initialize=0;
  }
  hash_del(&free_log);
  custom_write(&free_log);
  return real_free(ptr);
}

void *realloc(void *ptr,size_t bytes){
  if(!real_realloc){
    realloc_initialize=1;
    real_realloc=(void *(*)(void *,size_t))dlsym(RTLD_NEXT,"realloc");
    realloc_initialize=0;
  }
  void *ret_ptr=real_realloc(ptr,bytes);
  static mem_log realloc_log={0,NULL,R,NULL};
  if(ptr==NULL){
    realloc_log.user_size=bytes;
    realloc_log.ptr=ret_ptr;
    realloc_log.mode=M;
    hash_insert(&realloc_log);
    custom_write(&realloc_log);
    return ret_ptr;
  }
  else if(bytes==0){
    realloc_log.user_size=bytes;
    realloc_log.ptr=ptr;
    realloc_log.mode=F;
    hash_del(&realloc_log);
    custom_write(&realloc_log);
    return ret_ptr;
  }
  else{
    realloc_log.user_size=bytes;
    realloc_log.ptr=ret_ptr;
    realloc_log.mode=R;
    realloc_log.old_ptr=ptr;
    hash_del(&realloc_log);
    hash_insert(&realloc_log);
    custom_write(&realloc_log);
    return ret_ptr;
  }
}

void *calloc(size_t blocks,size_t bytes){
  static mem_log calloc_log={0,NULL,C,NULL};
  if(calloc_initialize || realloc_initialize || free_initialize || malloc_initialize || unistd_exit_initialize || stdlib_exit_initialize || syscall_exit_initialize){
    static char bump_alloc[BUMP_ALLOC];
    static int offset;
    void *send=&bump_alloc[offset];
    offset+=(int)(bytes * blocks);
    return send;
  }
  if(!real_calloc){
    calloc_initialize=1;
    real_calloc=(void *(*)(size_t,size_t))dlsym(RTLD_NEXT,"calloc");
    calloc_initialize=0;
  }
  void *ret_user=real_calloc(blocks,bytes);
  calloc_log.user_size=blocks*bytes;
  calloc_log.ptr=ret_user;
  hash_insert(&calloc_log);
  custom_write(&calloc_log);
  return ret_user;
}

void call_mem_leak(void){
  char *title="\n\n-------MEMORY LEAKS-------\n\n";
  write(2,title,strlen(title));
  int size_a;
  int size_s;
  char buffer_addr[17];
  char buffer_size[14];
  char *leak="[LEAK   ]\t\t";
  int leaks=0;
  for(int i=0;i<HASH_SIZE;i++){
    if(hash_t[i].ptr!=NULL){
      size_a=snprintf(buffer_addr,17,"%p",hash_t[i].ptr);
      size_s=snprintf(buffer_size,14,"%d",(int)hash_t[i].user_size);
      write(2,leak,strlen(leak));
      write(2,buffer_addr,size_a);
      write(2,"\t(",2);
      write(2,buffer_size,size_s);
      write(2," Bytes)\n",8);
      leaks++;
    }
  }

  if(leaks==0){
    write(2,"0 memory leaks\n",15);
  }
}

void _exit(int status){
  if(!real_uniExit){
    unistd_exit_initialize=1;
    real_uniExit=(void(*)(int))dlsym(RTLD_NEXT,"_exit");
    unistd_exit_initialize=0;
  }
  call_mem_leak();
  real_uniExit(status);
} 

void _Exit(int status){
  if(!real_stdExit){
    stdlib_exit_initialize=1;
    real_stdExit=(void(*)(int))dlsym(RTLD_NEXT,"_Exit");
    stdlib_exit_initialize=0;
  }
  call_mem_leak();
  real_stdExit(status);
}

long syscall(long number,...){
  va_list args;
  va_start(args,number);
  long arg1=va_arg(args,long);
  long arg2=va_arg(args,long);
  long arg3=va_arg(args,long);
  long arg4=va_arg(args,long);
  long arg5=va_arg(args,long);
  long arg6=va_arg(args,long);
  if(!real_syscall){
    syscall_exit_initialize=1;
    real_syscall=(long(*)(long,...))dlsym(RTLD_NEXT,"syscall");
    syscall_exit_initialize=0;
  }
  if(number==SYS_exit_group || number==SYS_exit){
    call_mem_leak();
    return real_syscall(number,arg1,arg2,arg3,arg4,arg5,arg6);
  }else{
    return real_syscall(number,arg1,arg2,arg3,arg4,arg5,arg6);
  }

}

void __attribute__((destructor)) mem_leak(){
  call_mem_leak();
}
