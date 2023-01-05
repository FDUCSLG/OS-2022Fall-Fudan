#ifndef _PIPE_H
#define _PIPE_H
#include <common/spinlock.h>
#include <common/defines.h>
#include <fs/file.h>
#include <common/sem.h>
#define PIPESIZE 512
typedef struct pipe {
    SpinLock lock;
    Semaphore wlock,rlock;
    char data[PIPESIZE];
    u32 nread;  // number of bytes read
    u32 nwrite;  // number of bytes written
    int readopen;  // read fd is still open
    int writeopen;  // write fd is still open
} Pipe;
int pipeAlloc(File** f0, File** f1);
void pipeClose(Pipe* pi, int writable);
int pipeWrite(Pipe* pi, u64 addr, int n);
int pipeRead(Pipe* pi, u64 addr, int n);
#endif