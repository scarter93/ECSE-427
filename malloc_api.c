#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "malloc_api.h"

#define FIRST_FIT 1
#define BEST_FIT 2

#define MAX_FREE_BLOCK 128000
#define MALLOC_BLOCKSIZE 2048
#define FREE_RM 20000

#define ALIGN(x) ((x/MALLOC_BLOCKSIZE + 1) * MALLOC_BLOCKSIZE)

typedef struct freeListNode
{
  void* startTag;
  void* endTag;
  size_t size;
  struct freeListNode *next;
  struct freeListNode *prev;

} freeListNode;

void updateContiguous();
void updateTopFreeBlock();

static freeListNode freeListHead = {NULL, NULL, 0, NULL, NULL};
extern char *my_malloc_error;

int currentPolicy = 1;
int bytesAlloc = 0;
int freeSpace = 0;
int largestSpace = 0;

void *my_malloc(int size)
{
  //int tempStartTag;
  void* bestTag;
  size_t bestSize = 128001;
  int alignSize = size + sizeof(freeListNode);
  //fprintf(stdout, "size of size: %d\n", size);
  int mallocSize = ALIGN(alignSize);
  //fprintf(stdout, "correct size is %d\n", mallocSize);
  void* currentLoc = sbrk(0);

  bytesAlloc += (size + sizeof(freeListNode));

  freeListNode *nextUp = freeListHead.next;
  freeListNode *previous = freeListHead.prev;
  freeListNode **nextAddr = &(freeListHead.next);
  freeListNode **prevAddr = &(freeListHead.prev);

  //check free list for open spot
  while(nextUp != NULL || previous != NULL)
  {
    if(currentPolicy == 1)  //First fit
    {

      if(nextUp != NULL)
      {
        if(nextUp->size >= (size + sizeof(freeListNode)))
        {
          freeSpace -= (size + sizeof(freeListNode));
          if(nextUp->size > (size + 2*sizeof(freeListNode)))
          {
            fprintf(stdout, "spliting free block that was found\n");
            freeListNode *newSpace = (void*)((char*)(nextUp->startTag));
            newSpace->startTag = nextUp->startTag;
            newSpace->size = (size + sizeof(freeListNode));
            newSpace->endTag = (void*)((char*)(nextUp->startTag) + (size + sizeof(freeListNode)));
            newSpace->next = NULL;
            newSpace->prev = NULL;

            nextUp->startTag = (void*)((char*)(nextUp->startTag) + (size + sizeof(freeListNode)));
            nextUp->size = nextUp->size - (size + sizeof(freeListNode));

            fprintf(stdout, "newspace startTag: %p\nnewspace endTag: %p\nnextUp startTag: %p\nnextUp endTag: %p\n",
                    newSpace->startTag, newSpace->endTag, nextUp->startTag, nextUp->endTag);


            // fprintf(stdout, "next: %p\nnewSpace: %p\n", freeListHead.next, &(newSpace));
            //
            // fprintf(stdout, "newSpace startTag: %p\nnewSpace endTag: %p\nnextUp startTag: %p\nnextUp endTag: %p\n",
            //         newSpace->startTag, newSpace->endTag, nextUp->startTag, nextUp->endTag);

            updateContiguous();
            return ((char*) newSpace) + sizeof(freeListNode);
          }else
          {
            if(nextUp->next != NULL)
              nextUp->next->prev = nextUp->prev;
            if(nextUp->prev != NULL)
              nextUp->prev->next = nextUp->next;
            *nextAddr = nextUp->next;

            nextUp->next == NULL;
            nextUp->prev == NULL;

            updateContiguous();
            return ((char*) nextUp) + sizeof(freeListNode);
          }
        }
      } else if(previous != NULL)
      {
        if (previous->size >= (size + sizeof(freeListNode)))
        {
          if(previous->size > (size + 2*sizeof(freeListNode)))
          {
            fprintf(stdout, "spliting free block that was found\n");
            freeListNode *newSpace = (void*)((char*)(previous->startTag));
            newSpace->startTag = previous->startTag;
            newSpace->size = (size + sizeof(freeListNode));
            newSpace->endTag = (void*)((char*)(previous->startTag) + (size + sizeof(freeListNode)));
            newSpace->next = NULL;
            newSpace->prev = NULL;

            previous->startTag = (void*)((char*)(previous->startTag) + (size + sizeof(freeListNode)));
            previous->size = previous->size - (size + sizeof(freeListNode));

            updateContiguous();
            return ((char*) newSpace) + sizeof(freeListNode);

          }else
          {
            if(previous->prev != NULL)
              previous->prev->next = previous->next;
            if(previous->next != NULL)
              previous->next->prev = previous->prev;
            *prevAddr = previous->prev;

            previous->prev == NULL;
            previous->next == NULL;

            updateContiguous();
            return ((char*) previous) + sizeof(freeListNode);
          }
        }
      }
    }else if(currentPolicy == 2) //Best Fit
    {
      if(nextUp != NULL)
      {
        if(nextUp->size >= (size + sizeof(freeListNode)))  //find best possible in next
        {
          if(nextUp->size == (size + sizeof(freeListNode)))
          {
            if(nextUp->next != NULL)
              nextUp->next->prev = nextUp->prev;
            if(nextUp->prev != NULL)
              nextUp->prev->next = nextUp->next;
            *nextAddr = nextUp->next;

            nextUp->next == NULL;
            nextUp->prev == NULL;

            updateContiguous();
            return ((char*) nextUp) + sizeof(freeListNode);

          } else if(nextUp->size < bestSize)
          {
            bestTag = nextUp->startTag;
            bestSize = nextUp->size;
          }
        }
      }
      //check previous free blocks if next yields no results
      if(previous != NULL)
      {
        if(previous->size >= (size + sizeof(freeListNode)))
        {
          if(previous->size == (size + sizeof(freeListNode)))
          {
            if(previous->prev != NULL)
              previous->prev->next = previous->next;
            if(previous->next != NULL)
              previous->next->prev = previous->prev;
            *prevAddr = previous->prev;

            previous->next == NULL;
            previous->prev == NULL;

            updateContiguous();
            return ((char*) previous) + sizeof(freeListNode);

          } else if(previous->size < bestSize)
          {
            bestTag = previous->startTag;
            bestSize = previous->size;
          }
        }
      }
    }
    if(nextUp != NULL)
    {
      nextAddr = &(nextUp->next);
      nextUp = nextUp->next;
    }
    if(previous != NULL)
    {
      prevAddr = &(previous->prev);
      previous = previous->prev;
    }
  }
  //if running a best fit approach, now search for the best result that was found and return that spot
  if(currentPolicy == 2)
  {
    freeListNode *next = freeListHead.next;
    freeListNode *prev = freeListHead.prev;
    freeListNode **nextOne = &(freeListHead.next);
    freeListNode **prevOne = &(freeListHead.prev);
    while(next != NULL || prev != NULL)
    {
      if(next != NULL)
      {
        if(next->startTag == bestTag)
        {
          freeSpace -= (size + sizeof(freeListNode));
          if(next->size > (size + 2*sizeof(freeListNode)))  //if space split block up
          {
            fprintf(stdout, "spliting free block that was found\n");
            freeListNode *newSpace = (void*)((char*)(next->startTag));
            newSpace->startTag = next->startTag;
            newSpace->size = (size + sizeof(freeListNode));
            newSpace->endTag = (void*)((char*)(next->startTag) + (size + sizeof(freeListNode)));
            newSpace->next = NULL;
            newSpace->prev = NULL;

            next->startTag = (void*)((char*)(next->startTag) + (size + sizeof(freeListNode)));
            next->size = next->size - (size + sizeof(freeListNode));

            updateContiguous();
            return ((char*) newSpace) + sizeof(freeListNode);
          }else //if not return this block
          {

            if(next->next != NULL)
              next->next->prev = next->prev;
            if(next->prev != NULL)
              next->prev->next = next->next;
            *nextOne = next->next;

            next->next == NULL;
            next->prev == NULL;

            updateContiguous();
            return ((char*) next) + sizeof(freeListNode);
          }
        }
      }
      if(prev != NULL)
      {
        if(prev->startTag == bestTag)
        {
          if(prev->size > (size + 2*sizeof(freeListNode)))  //check to see if free block can be split up
          {  fprintf(stdout, "spliting free block that was found\n");
            freeListNode *newSpace = (void*)((char*)(prev->startTag));
            newSpace->startTag = prev->startTag;
            newSpace->size = (size + sizeof(freeListNode));
            newSpace->endTag = (void*)((char*)(prev->startTag) + (size + sizeof(freeListNode)));
            newSpace->next = NULL;
            newSpace->prev = NULL;

            prev->startTag = (void*)((char*)(prev->startTag) + (size + sizeof(freeListNode)));
            prev->size = prev->size - (size + sizeof(freeListNode));

            updateContiguous();
            return ((char*) newSpace) + sizeof(freeListNode);
          }else
          {
            if(prev->prev != NULL)
              prev->prev->next = prev->next;
            if(prev->next != NULL)
              prev->next->prev = prev->prev;
            *prevOne = prev->prev;

            prev->next == NULL;
            prev->prev == NULL;

            updateContiguous();
            return ((char*) prev) + sizeof(freeListNode);
          }
        }
      }

      if(next != NULL)
      {
        nextOne = &(next->next);
        next = next->next;
      }
      if(prev != NULL)
      {
        prevOne = &(prev->prev);
        prev = prev->prev;
      }
    }
  }
  //if no free blocks set up a new block and return the address of the pointer



  if(nextUp == NULL)
  {

    freeListNode *nextNew;

    nextNew = (freeListNode*)sbrk(size + sizeof(freeListNode));
    nextNew->startTag = currentLoc;
    nextNew->endTag = sbrk(0);
    nextNew->size = size + sizeof(freeListNode);
    nextNew->next = NULL;
    nextNew->prev = NULL;

    freeSpace += (mallocSize - size + sizeof(freeListNode));


    //fprintf(stdout, "nextNew startTag: %p\nnextNew endTag: %p\nnextNew size: %d\n", nextNew->startTag, nextNew->endTag, nextNew->size);

    //put extra allocated memory into free list
    void* newLoc = sbrk(0);
    freeListNode *newNext = (freeListNode*)sbrk(mallocSize - size + sizeof(freeListNode));
    newNext->startTag = newLoc;
    newNext->endTag = sbrk(0);
    newNext->next = freeListHead.next;
    newNext->size = mallocSize - size + sizeof(freeListNode);
    if(newNext->next != NULL)
      newNext->next->prev = newNext;
    newNext->prev = &(freeListHead);
    freeListHead.next = newNext;

    fprintf(stdout, "newNext startTag: %p\nnewNext endTag: %p\nnewNext size: %d\n", newNext->startTag, newNext->endTag, newNext->size);

    updateContiguous();
    return ((char*)nextNew) + sizeof(freeListNode);
  }else if(previous == NULL)
  {
    freeListNode *prevNew;

    prevNew = (freeListNode*)sbrk(size + sizeof(freeListNode));
    prevNew->startTag = currentLoc;
    prevNew->endTag = sbrk(0);
    prevNew->size = size + sizeof(freeListNode);
    prevNew->next = NULL;
    prevNew->prev = NULL;

    fprintf(stdout, "prevNew startTag: %p\nprevNew endTag: %p\nprevNew size: %d\n", prevNew->startTag, prevNew->endTag, prevNew->size);

    //put extra allocated memory into free list
    void* newLoc = sbrk(0);
    freeListNode *newPrev = (freeListNode*)sbrk(mallocSize - size + sizeof(freeListNode));
    newPrev->startTag = newLoc;
    newPrev->endTag = sbrk(0);
    newPrev->prev = freeListHead.prev;
    newPrev->size= mallocSize - size + sizeof(freeListNode);
    if(newPrev->prev != NULL)
      newPrev->prev->next = newPrev;
    newPrev->next = &(freeListHead);
    freeListHead.next = newPrev;

    fprintf(stdout, "newPrev startTag: %p\nnewPrev endTag: %p\nnewPrev size: %d\n", newPrev->startTag, newPrev->endTag, newPrev->size);

    updateContiguous();
    return ((char*)prevNew) + sizeof(freeListNode);
  }
  updateContiguous();
  //error handling for my_malloc
  //my_malloc_error = "Error, mallocing required memory";
  //fprintf(stderr, "%s\n", my_malloc_error);
  return NULL;
}

void my_free(void *ptr)
{
  if(ptr == NULL)
    return;
  freeListNode *new = (freeListNode*)(((char*)ptr) - sizeof(freeListNode));
  new->next = freeListHead.next;

  freeSpace += new->size;
  bytesAlloc -= new->size;

  if(new->next != NULL)
    new->next->prev = new;

  new->prev = &(freeListHead);

  if(new->prev != NULL)
    new->prev->next = new;

  freeListHead.next = new;


  //set uo required data to check adjacent free blocks
  freeListNode *next = new->next;
  freeListNode *previous = new->prev;
  freeListNode **nextAddr = &(new->next);
  freeListNode **prevAddr = &(new->prev);

  int within = 0;
  //if the block we are freeing has adjacent free block combine them
  while(next != NULL || previous != NULL)
  {
    within = 0;
    if(next != NULL)
    {
      //check corresponding tags, if they are equal update free list
      if(next->startTag == new->endTag)
      {
        fprintf(stdout, "next startTag: %p\nnew endTag: %p\n", next->startTag, new->endTag);
        new->endTag = next->endTag;
        new->size += next->size;
        new->next = next->next;
        if(next->next != NULL)
          next->next->prev = next->prev;
        *nextAddr = next->next;

        next->size = 0;
        next->endTag = NULL;
        next->startTag = NULL;
        next->next = NULL;
        next->prev = NULL;
        fprintf(stdout, "new startTag: %p\nnew endTag: %p\nnew size: %d\n", new->startTag, new->endTag, new->size);

        next = new->next;
        previous = new->prev;
        nextAddr = &(new->next);
        prevAddr = &(new->prev);
        within = 1;
      //check corresponding tags, if they are equal update free list
    }else if(next->endTag == new->startTag)
      {
        fprintf(stdout, "next endTag: %p\nnew startTag: %p\n", next->endTag, new->startTag);
        new->startTag = next->startTag;
        new->size += next->size;
        new->next = next->next;

        if(next->next != NULL)
          next->next->prev = next->prev;
        *nextAddr = next->next;
        next->size = 0;
        next->endTag = NULL;
        next->startTag = NULL;
        next->next = NULL;
        next->prev = NULL;
        fprintf(stdout, "new startTag: %p\nnew endTag: %p\nnew size: %d\n", new->startTag, new->endTag, new->size);

        next = new->next;
        previous = new->prev;
        nextAddr = &(new->next);
        prevAddr = &(new->prev);
        within = 1;
      }
    }
    if(previous != NULL)
    {
      //check corresponding tags, if they are equal update free list
      if(previous->startTag == new->endTag)
      {
        fprintf(stdout, "previous startTag: %p\nnew endTag: %p\n", previous->startTag, new->endTag);
        new->endTag = previous->endTag;
        new->size += previous->size;
        if(previous->prev != NULL)
          previous->prev->next = previous->next;
        *prevAddr = previous->prev;
        previous->size = 0;
        previous->endTag = NULL;
        previous->startTag = NULL;
        previous->next = NULL;
        previous->prev = NULL;
        fprintf(stdout, "previous startTag: %p\nprevious endTag: %p\nprevious size: %d\n", previous->startTag, previous->endTag, previous->size);

        next = new->next;
        previous = new->prev;
        nextAddr = &(new->next);
        prevAddr = &(new->prev);
        within = 1;
      //check corresponding tags, if they are equal update free list
      }else if(previous->endTag == new->startTag)
      {
        fprintf(stdout, "previous endTag: %p\nnew startTag: %p\n", previous->endTag, new->startTag);

        new->startTag = previous->startTag;
        new->size += previous->size;
        if(previous->prev != NULL)
          previous->prev->next = previous->next;
        *prevAddr = previous->prev;
        previous->size = 0;
        previous->endTag = NULL;
        previous->startTag = NULL;
        previous->next = NULL;
        previous->prev = NULL;
        fprintf(stdout, "previous startTag: %p\nprevious endTag: %p\nprevious size: %d\n", previous->startTag, previous->endTag, previous->size);

        next = new->next;
        previous = new->prev;
        nextAddr = &(new->next);
        prevAddr = &(new->prev);
        within = 1;
      }
    }
    if(!within)
    {
      if(next != NULL)
      {
        nextAddr = &(next->next);
        next = next->next;
      }
      if(previous != NULL)
      {
        prevAddr = &(previous->prev);
        previous = previous->prev;
      }
    }
  }
  updateTopFreeBlock();
  updateContiguous();
}

void updateContiguous()
{
  freeListNode *next = freeListHead.next;
  freeListNode *prev = freeListHead.prev;
  freeListNode **nextAddr = &(freeListHead.next);
  freeListNode **prevAddr = &(freeListHead.prev);

  if(next != NULL)
    largestSpace = next->size;

  while(next != NULL || prev != NULL)
  {
    if(next != NULL)
    {
      if(next->size > largestSpace)
        largestSpace = next->size;
    }
    if(prev != NULL)
    {
      if(prev->size > largestSpace)
        largestSpace = prev->size;
    }

    if(next != NULL)
    {
      nextAddr = &(next->next);
      next = next->next;
    }
    if(prev != NULL)
    {
      prevAddr = &(prev->prev);
      prev = prev->prev;
    }
  }
}

void updateTopFreeBlock()
{
  void* topBlock = sbrk(0);

  freeListNode *next = freeListHead.next;
  freeListNode *prev = freeListHead.prev;
  freeListNode **nextAddr = &(freeListHead.next);
  freeListNode **prevAddr = &(freeListHead.prev);

  while(next != NULL || prev != NULL)
  {
    if(next != NULL)
    {
      if(next->endTag == topBlock)
      {
        if(next->size >= MAX_FREE_BLOCK)
        {
          sbrk(-20000);
          next->size -= FREE_RM;
          next->endTag = sbrk(0);
        }
      }
    }
    if(prev != NULL)
    {
      if(prev->endTag == topBlock)
      {
        if(prev->size >= MAX_FREE_BLOCK)
        {
          sbrk(-20000);
          prev->size -= FREE_RM;
          prev->endTag = sbrk(0);
        }
      }
    }

    if(next != NULL)
    {
      nextAddr = &(next->next);
      next = next->next;
    }
    if(prev != NULL)
    {
      prevAddr = &(prev->prev);
      prev = prev->prev;
    }
  }
  updateContiguous();
}

void my_mallopt(int policy)
{
  if(policy == FIRST_FIT)
  {
    currentPolicy = FIRST_FIT;
  } else if(policy == BEST_FIT)
  {
    currentPolicy = BEST_FIT;
  } else
  {
    fprintf(stderr, "Error, not a valid policy");
    return;
  }
}

void my_mallinfo()
{
  fprintf(stdout, "Current number of bytes allocated: %d\n", bytesAlloc);
  fprintf(stdout, "Current amount of free space: %d\n", freeSpace);
  fprintf(stdout, "Current largest contiguous free space: %d\n", largestSpace);
  fprintf(stdout, "Current policy number: %d\n", currentPolicy);
}
