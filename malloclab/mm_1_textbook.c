/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * as same as the textbook
 * 隐式空闲链表+首次适配+原始realloc版
 * Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   99%    5694  0.008783   648
 1       yes   99%    5848  0.006399   914
 2       yes   99%    6648  0.010500   633
 3       yes  100%    5380  0.008353   644
 4       yes   66%   14400  0.000094153191
 5       yes   92%    4800  0.007289   659
 6       yes   92%    4800  0.006480   741
 7       yes   55%   12000  0.146743    82
 8       yes   51%   24000  0.266604    90
 9       yes   27%   14401  0.074746   193
10       yes   34%   14401  0.002447  5885
Total          74%  112372  0.538439   209

Perf index = 44 (util) + 14 (thru) = 58/100
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
		/* Team name */
		"XXXXXXX",
		/* First member's full name */
		"yzf",
		/* First member's email address */
		"@Fd",
		/* Second member's full name (leave blank if none) */
		"",
		/* Second member's email address (leave blank if none) */
		""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8           /*Double word size*/
#define CHUNKSIZE (1<<12) /*the page size in bytes is 4K*/

#define MAX(x,y)    ((x)>(y)?(x):(y))

#define PACK(size,alloc)    ((size) | (alloc))

#define GET(p)  (*(unsigned int *)(p))
#define PUT(p,val)  (*(unsigned int *)(p) = (val))

// lowest three bits are zero, because of 8 bytes aligned
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p)    (GET(p) & 0x1)

#define HDRP(bp)    ((char *)(bp)-WSIZE)
#define FTRP(bp)    ((char *)(bp)+GET_SIZE(HDRP(bp))-DSIZE)

#define NEXT_BLKP(bp)   ((char *)(bp)+GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp)   ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t size);
static void place(void *bp,size_t asize);
static char *heap_listp = 0;
/*
 * mm_init - initialize the malloc package.
 * The return value should be -1 if there was a problem in performing the initialization, 0 otherwise
 */
int mm_init(void)
{
	// -1 means some error

	// heaper_listp 是一个静态全局变量，总是指向序言块
	// mem_sbrk return the start address of the new area
	if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) {
		return -1;
	}
	PUT(heap_listp, 0);	// Alignment padding
	PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));	// 8/1, 序言块头
	PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));	// 8/1, 序言块尾
	PUT(heap_listp + (3*WSIZE), PACK(0, 1));	// 0/1, 结尾块hdr
	heap_listp += (2*WSIZE);
	// extend the empty heap with a free block of CHUNKSIZE bytes
	if(extend_heap(CHUNKSIZE/WSIZE) == NULL){
		return -1;
	}
	return 0;
}


static void *extend_heap(size_t words){
	char *bp;
	size_t size;
	/* 8 bytes aligned, so the words should be even numbers of words */
	size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
	// bp point to the start address of the new extended area
	if((long)(bp = mem_sbrk(size)) == -1)
		return NULL;

	// initialize free block header/footer and the epilogue header
	PUT(HDRP(bp), PACK(size, 0));	// free block header
	PUT(FTRP(bp), PACK(size, 0));	// free block footer
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));	// new epilogue header, 扩展了空间，新的结尾块

	// coalesce if the previous block was free
	return coalesce(bp);
}
/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
	size_t asize;	// save adjusted block size
	size_t extendsize;	// save amount to extend heap if no fit
	char *bp;

	// ignore spurious request
	if(size == 0) return NULL;

	// adjust block size to include overhead and alignment requirements
	if(size <= DSIZE){
		asize = 2*(DSIZE);	// minimum is 16
	} else {
		asize = (DSIZE)*((size + (DSIZE) + (DSIZE-1)) / (DSIZE));
	}
	// search the free list fot a fit
	if((bp = find_fit(asize)) != NULL){
		place(bp,asize);
		return bp;
	}

	// no fit found, get more memory and place the block
	extendsize = MAX(asize,CHUNKSIZE);
	if((bp = extend_heap(extendsize/WSIZE)) == NULL){
		return NULL;
	}
	place(bp,asize);
	return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
/* end mmfree */
	if(bp == 0)
		return;

/* begin mmfree */
	size_t size = GET_SIZE(HDRP(bp));
	// clear the mark bit from 1 to 0, means the block is free
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	// coalesce if prev or succ block is free
	coalesce(bp);
}

static void *coalesce(void *bp){
	size_t  prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t  next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	if(prev_alloc && next_alloc) {
		// case 1
		return bp;
	} else if (prev_alloc && !next_alloc) {
		// case 2
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size,0));
		PUT(FTRP(bp), PACK(size,0));
	} else if (!prev_alloc && next_alloc) {
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size,0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
		bp = PREV_BLKP(bp);
	} else {
		size += GET_SIZE(FTRP(NEXT_BLKP(bp)))+ GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
		bp = PREV_BLKP(bp);
	}
	return bp;
}


static void *find_fit(size_t size){
	void *bp;
	for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
		if(!GET_ALLOC(HDRP(bp)) && (size <= GET_SIZE(HDRP(bp)))){
			// 未分配且满足需要的字节大小
			return bp;
		}
	}
	return NULL;
}



static void place(void *bp,size_t asize){
	size_t csize = GET_SIZE(HDRP(bp));
	// csize-asize 即放置之后剩余的大小
	if((csize - asize) >= (2*DSIZE)){
		PUT(HDRP(bp), PACK(asize,1));
		PUT(FTRP(bp), PACK(asize,1));

		// 分割剩余部分， 将剩余部分设置为空闲block
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize - asize,0));
		PUT(FTRP(bp), PACK(csize - asize,0));
	} else {
		PUT(HDRP(bp), PACK(csize,1));
		PUT(FTRP(bp), PACK(csize,1));
	}
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
	size_t oldsize;
	void *newptr;

	/* If size == 0 then this is just free, and we return NULL. */
	if(size == 0) {
		mm_free(ptr);
		return NULL;
	}

	/* If oldptr is NULL, then this is just malloc. */
	if(ptr == NULL) {
		return mm_malloc(size);
	}

	newptr = mm_malloc(size);

	/* If realloc() fails,  the original block is left untouched  */
	if(!newptr) {
		return 0;
	}

	/* Copy the old data. */
	oldsize = GET_SIZE(HDRP(ptr));
	if(size < oldsize) oldsize = size;
	memcpy(newptr, ptr, oldsize);

	/* Free the old block. */
	mm_free(ptr);

	return newptr;
}





