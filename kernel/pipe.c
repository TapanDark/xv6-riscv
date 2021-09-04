#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"

#define PIPESIZE 2048

struct pipe {
  struct spinlock lock;
  char data[PIPESIZE];
  uint nread;     // number of bytes read
  uint nwrite;    // number of bytes written
  int readopen;   // read fd is still open
  int writeopen;  // write fd is still open
};

int
pipealloc(struct file **f0, struct file **f1)
{
  struct pipe *pi;

  pi = 0;
  *f0 = *f1 = 0;
  if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
    goto bad;
  if((pi = (struct pipe*)kalloc()) == 0)
    goto bad;
  pi->readopen = 1;
  pi->writeopen = 1;
  pi->nwrite = 0;
  pi->nread = 0;
  initlock(&pi->lock, "pipe");
  (*f0)->type = FD_PIPE;
  (*f0)->readable = 1;
  (*f0)->writable = 0;
  (*f0)->pipe = pi;
  (*f1)->type = FD_PIPE;
  (*f1)->readable = 0;
  (*f1)->writable = 1;
  (*f1)->pipe = pi;
  return 0;

 bad:
  if(pi)
    kfree((char*)pi);
  if(*f0)
    fileclose(*f0);
  if(*f1)
    fileclose(*f1);
  return -1;
}

void
pipeclose(struct pipe *pi, int writable)
{
  acquire(&pi->lock);
  if(writable){
    pi->writeopen = 0;
    wakeup(&pi->nread);
  } else {
    pi->readopen = 0;
    wakeup(&pi->nwrite);
  }
  if(pi->readopen == 0 && pi->writeopen == 0){
    release(&pi->lock);
    kfree((char*)pi);
  } else
    release(&pi->lock);
}

int
pipewrite(struct pipe *pi, uint64 addr, int n)
{
  int writesize = 0;
  int pipespace = 0;
  struct proc *pr = myproc();

  acquire(&pi->lock);
  if(pi->readopen == 0 || pr->killed){
    release(&pi->lock);
    return -1;
  }
  pipespace = PIPESIZE - (pi->nwrite - pi->nread);
  if(pipespace == 0){ //DOC: pipewrite-full
    wakeup(&pi->nread);
    sleep(&pi->nwrite, &pi->lock);
  } else {
    writesize = n < pipespace ? n : pipespace;
    int pipewallspace = PIPESIZE - (pi->nwrite % PIPESIZE);
    if (writesize <= pipewallspace)
    {
      if (copyin(pr->pagetable, &pi->data[pi->nwrite % PIPESIZE], addr, writesize) == -1)
        writesize = 0;
    }
    else
    {
      if (copyin(pr->pagetable, &pi->data[pi->nwrite % PIPESIZE], addr, pipewallspace) == -1)
      {
        writesize = 0;
        goto exit;
      }
      if (copyin(pr->pagetable, &pi->data[(pi->nwrite + pipewallspace) % PIPESIZE], addr + pipewallspace, writesize - pipewallspace) == -1)
        writesize = pipewallspace;
    }
  }
  pi->nwrite += writesize;
  exit:
  wakeup(&pi->nread);
  release(&pi->lock);

  return writesize;
}

int
piperead(struct pipe *pi, uint64 addr, int n)
{
  int readsize = 0;
  int pipespace = 0;
  struct proc *pr = myproc();

  acquire(&pi->lock);
  while(pi->nread == pi->nwrite && pi->writeopen){  //DOC: pipe-empty
    if(pr->killed){
      release(&pi->lock);
      return -1;
    }
    sleep(&pi->nread, &pi->lock); //DOC: piperead-sleep
  }
  pipespace = pi->nwrite - pi->nread;
  if (pipespace > 0)
  {
    readsize = n < pipespace ? n : pipespace;
    int pipewallspace = PIPESIZE - (pi->nread % PIPESIZE);
    if (readsize <= pipewallspace)
    {
      if (copyout(pr->pagetable, addr, &pi->data[pi->nread % PIPESIZE], readsize) == -1)
        readsize = 0;
    }
    else
    {
      if (copyout(pr->pagetable, addr, &pi->data[pi->nread % PIPESIZE], pipewallspace) == -1)
      {
        readsize = 0;
        goto exit;
      }
      if (copyout(pr->pagetable, addr + pipewallspace, &pi->data[(pi->nread + pipewallspace) % PIPESIZE], readsize-pipewallspace) == -1)
        readsize = pipewallspace;
    }
  }
  pi->nread += readsize;
  exit:
  wakeup(&pi->nwrite);  //DOC: piperead-wakeup
  release(&pi->lock);
  return readsize;
}
