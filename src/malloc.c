/*
Jaedyn K. Brown
CSE3320-001

*/
// The MIT License (MIT)
// 
// Copyright (c) 2016, 2017, 2021 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// 7f704d5f-9811-4b91-a918-57c1bb646b70
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
*/

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)      ((b) + 1)
#define BLOCK_HEADER(ptr)   ((struct _block *)(ptr) - 1)


static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;
bool block_found = false;


int counter = 0;
/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct _block 
{
   size_t  size;         /* Size of the allocated _block of memory in bytes */
   struct _block *prev;  /* Pointer to the previous _block of allcated memory   */
   struct _block *next;  /* Pointer to the next _block of allcated memory   */
   bool   free;          /* Is this _block free?                     */
   char   padding[3];
};


struct _block *heapList = NULL; /* Free list to track the _blocks available */
struct _block *lastBlock = NULL; //block to track the last block used to store data

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes 
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size) 
{
   struct _block *curr = heapList;
   int blockDiff;
   int temp;
   int position;
   int diffSave = 0;
   counter = 0;
   struct _block *traversalBlock = heapList;
   struct _block *winner = NULL;

   //num_requested += size;

#if defined FIT && FIT == 0
   /* First fit */
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }
   lastBlock = curr;//used for next fit
   block_found = true;//used for next fit
#endif

#if defined BEST && BEST == 0
   blockDiff = INT_MAX;
   int differ;
   int requestedSize = size;

   while(curr)//curr not NULL, curr block not free, size of block larger than requested
   {
      if(curr->free && (curr->size > size))//block is free, and enough space to store requested size
      {
         differ = curr->size - size;
         if(differ < blockDiff)
         {
            blockDiff = curr->size - requestedSize;
            winner = curr;
         }
   }
      *last = curr;
      curr = curr->next;//traversal
   }

   curr = winner;

#endif

#if defined WORST && WORST == 0
   counter = 0;
   blockDiff = INT_MIN;
   int diff;
   int requestedSize = size;
   while(curr)//curr not NULL, curr block not free, size of block larger than requested
   {
      if(curr->free && (curr->size > size))//block is free, and enough space to store requested size
      {
         diff = curr->size - size;

         if(diff > blockDiff)
         {
            blockDiff = curr->size - requestedSize;
            winner = curr;//set to block with smallest diff in size avail. to size requested
         }

   }
      *last = curr;
      curr = curr->next;//traversal
   }

   curr = winner;//set equal to whichever address stored in last last
#endif

#if defined NEXT && NEXT == 0
   if(block_found == true)//need to find a block after lastBlock
   {
      while(lastBlock)//there is a value there
      {
         if((lastBlock->free == false) && lastBlock->size >= size)
         {
          *last = lastBlock;
         }
         lastBlock = lastBlock->next;
      }
   curr = last;
   }
   else
   {
      while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }
   lastBlock = curr;
   block_found = true;
   }
#endif

    while(traversalBlock)
   {
      if(traversalBlock)
      {
         num_blocks++;//track how many blocks in list
      }
      traversalBlock = traversalBlock->next;
   }
   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically 
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size) 
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1) 
   {
      return NULL;
   }

   /* Update heapList if not set */
   if (heapList == NULL) 
   {
      heapList = curr;
   }

   /* Attach new _block to prev _block */
   if (last) 
   {
      last->next = curr;
      num_grows++;
   }

   /* Update _block metadata */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;
   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the 
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process 
 * or NULL if failed
 */
void *malloc(size_t size) 
{
   num_mallocs++;
   num_requested = num_requested + size;
   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);
   
   
   
   /* Handle 0 size */
   if (size == 0) 
   {
      printf("size was equal to zero\n");
      return NULL;
   }

   /* Look for free _block */
   struct _block *last = heapList;
   struct _block *next = findFreeBlock(&last, size);
   struct _block *traversalBlock = heapList;
   counter = 0;
   if(next != NULL)
   {
      while(traversalBlock != NULL)
      {
         if(counter == 0)//first iteration
         {
            max_heap = traversalBlock->size; //set temp variable to block size
            counter++;//increment counter
         }
         if(traversalBlock->size >= max_heap)//if block size greater than stored size
         {
            max_heap = traversalBlock->size;//setting variable to that size
         }
         traversalBlock = traversalBlock->next;
   }
      //num_requested += size;
      counter = 0;
   }
   /* TODO: Split free _block if possible */

   /* Could not find free _block, so grow heap */
   if (next == NULL) 
   {
      next = growHeap(last, size);
   while(traversalBlock != NULL)
   {
      if(counter == 0)//first iteration
      {
         max_heap = traversalBlock->size; //set temp variable to block size
         counter++;//increment counter
      }
      if(traversalBlock->size >= max_heap)//if block size greater than stored size
      {
         max_heap = traversalBlock->size;//setting variable to that size
      }
      traversalBlock = traversalBlock->next;
   }
      counter = 0;
   }
   else
   {
      num_reuses++;
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL) 
   {
      return NULL;
   }
   
   /* Mark _block as in use */
   next->free = false;
   /* Return data address associated with _block */
   return BLOCK_DATA(next);
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr) 
{
   
   if (ptr == NULL) 
   {
      return;
   }

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;
   num_frees++;
   
   /* TODO: Coalesce free _blocks if needed */
}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
void *realloc(void *ptr, size_t size){
   if(!ptr)
   {
      return malloc(size);
   }
   else
   {
      void *curr = malloc(size);
      if(curr)
      {
         memcpy(curr, ptr, size);
         free(ptr);
         num_frees++;
      }
      return curr;
   }

}

void *calloc(size_t nmemb, size_t size){

   void *ptr = malloc(size * nmemb);
   if(ptr)
   {
      memset( ptr, 0, (size * nmemb));
   }

   return ptr;
}
