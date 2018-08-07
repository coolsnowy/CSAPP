/*
 * 分离空闲链表+最佳适配+改进的realloc版本+ realloc—place针对最后两个测试数据改进
 *
 *Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   99%    5694  0.000303 18792
 1       yes   99%    5848  0.000170 34441
 2       yes   99%    6648  0.000207 32054
 3       yes  100%    5380  0.000191 28109
 4       yes   99%   14400  0.000190 75789
 5       yes   95%    4800  0.001374  3494
 6       yes   95%    4800  0.001038  4625
 7       yes   55%   12000  0.000334 35982
 8       yes   51%   24000  0.000937 25605
 9       yes   95%   14401  0.000182 79126
10       yes   76%   14401  0.000138104507
Total          88%  112372  0.005064 22191

Perf index = 53 (util) + 40 (thru) = 93/100
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"
//#define  DEBUG 1
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
static char *heap_listp = NULL;

// add following statment
int mm_check(char *function);
#define PREV_LINKNODE_RP(bp) ((char *)(bp))
#define NEXT_LINKNODE_RP(bp) ((char*)(bp)+WSIZE)

/*inline function*/
void insertToEmptyList(char *p);
void fixLinkeList(char *p);
char *findListRoot(size_t size);

/*aux function for mm_realloc*/
static void *reallocCoalesce(void *p, size_t newSize, int *isNextFree);
static void reallocPlace(void *bp, size_t asize);

static char *blockListStart = NULL;

/*
 * mm_init - initialize the malloc package.
 * The return value should be -1 if there was a problem in performing the initialization, 0 otherwise
 */
int mm_init(void)
{
	// -1 means some error

	// heaper_listp 是一个静态全局变量，总是指向序言块
	// mem_sbrk return the start address of the new area

	// modify
	if((heap_listp = mem_sbrk(12 * WSIZE)) == (void *)-1) {
		return -1;
	}
	/*
	 * segregated free list 汇总大小类分类方法如下，并将该list表放在heap头部，
	 * 通过序言块与数据块个例，在每一个大小类中，空闲块按照size由大到小排序
	 */
	// 针对于测试数据做了修改
	PUT(heap_listp, 0);              /*block size list<=32*/
	PUT(heap_listp + (1 * WSIZE), 0);    /*block size list<=64*/
	PUT(heap_listp + (2 * WSIZE), 0);    /*block size list<=128*/
	PUT(heap_listp + (3 * WSIZE), 0);    /*block size list<=256*/
	PUT(heap_listp + (4 * WSIZE), 0);    /*block size list<=512*/
	PUT(heap_listp + (5 * WSIZE), 0);    /*block size list<=2048*/
	PUT(heap_listp + (6 * WSIZE), 0);    /*block size list<=4096*/
	PUT(heap_listp + (7 * WSIZE), 0);    /*block size list>4096*/
	PUT(heap_listp + (8 * WSIZE), 0);	/* for alignment*/
	PUT(heap_listp + (9 * WSIZE), PACK(DSIZE, 1));	// 8/1, 序言块头
	PUT(heap_listp + (10 * WSIZE), PACK(DSIZE, 1));	// 8/1, 序言块尾
	PUT(heap_listp + (11 * WSIZE), PACK(0, 1));	// 0/1, 结尾块hdr

	blockListStart = heap_listp;

	heap_listp += (10 * WSIZE);

	// extend the empty heap with a free block of CHUNKSIZE bytes
	if(extend_heap(CHUNKSIZE / DSIZE) == NULL){
		return -1;
	}
// for debuf
#ifdef DEBUG
	mm_check(__FUNCTION__);
#endif
	return 0;
}

// minimum block is 16 byte
static void *extend_heap(size_t words){
	char *bp;
	size_t size;
	/* 8 bytes aligned, so the words should be even numbers of words */
	size = (words % 2) ? (words + 1) * DSIZE : words * DSIZE;
	// bp point to the start address of the new extended area
	if((long)(bp = mem_sbrk(size)) == -1)
		return NULL;

	// 旧的结尾快四个字节，刚好成为新分配字块的头部，bp指向新分配的首个字节，刚好指向主体中第一个字节
	// initialize free block header/footer and the epilogue header
	PUT(HDRP(bp), PACK(size, 0));	// free block header
	PUT(FTRP(bp), PACK(size, 0));	// free block footer
	// set pointer to NULL， 主体中prev和next指针
	PUT(NEXT_LINKNODE_RP(bp), 0);
	PUT(PREV_LINKNODE_RP(bp), 0);
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
		asize = 2 * (DSIZE);	// minimum is 16
	} else {
		asize = (DSIZE) * ((size + (DSIZE) + (DSIZE-1)) / (DSIZE));
	}
	// search the free list fot a fit
	if((bp = find_fit(asize)) != NULL){
		place(bp, asize);
#ifdef DEBUG
		mm_check(__FUNCTION__);
#endif
		return bp;
	}

	// no fit found, get more memory and place the block
	extendsize = MAX(asize, CHUNKSIZE);
	if((bp = extend_heap(extendsize / DSIZE)) == NULL){
		return NULL;
	}
	place(bp, asize);
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
	PUT(NEXT_LINKNODE_RP(bp), 0);
	PUT(PREV_LINKNODE_RP(bp), 0);

	// coalesce if prev or succ block is free
	coalesce(bp);
#ifdef DEBUG
	mm_check(__FUNCTION__);
#endif
}


/*
 * mm_realloc - Implemented in a new way
 * improvement : if newSize > oldSize, we try to coalesce first, check whether there
 * are free block next to current block, if the sum of block size > new size, we needn't
 * malloc a new bigger area, if not, we malloc a new sapce
 */
/*
 * mm_realloc
 * 1. Given size is equal to 0 ,just free the given ptr
 * 2. Given ptr is equal to NULL ,just new malloc a space which size is given size
 *
 * 3. oldsize is smaller than new size (asize),than call realloc_coalesce to coalesce the previous and next free block,
 *    if realloc_coalesce success ,just move the payload into new address.
 *    if not success, mm_alloc a new space.
 *
 * 4.oldsize is bigger than new size (asize),just call realloc_place to change the size of ptr.
 */
void *mm_realloc(void *ptr, size_t size)
{
	size_t oldsize = GET_SIZE(HDRP(ptr));
	void *newPtr;
	size_t asize;

	/* If size == 0 then this is just free, and we return NULL. */
	if(size == 0) {
		mm_free(ptr);
		return NULL;
	}

	/* If oldptr is NULL, then this is just malloc. */
	if(ptr == NULL) {
		return mm_malloc(size);
	}

	if(size <= DSIZE) {
		asize = 2 * (DSIZE);	// minimum is 16
	} else {
		asize = (DSIZE) * ((size + (DSIZE) + (DSIZE-1)) / (DSIZE));
	}

	if(oldsize == asize) {
		return ptr;
	}

	if(oldsize < asize) {
		// need a larger space, instead of mallocing a new larger spaec
		// we try to coalesce first and find a enough existed space to use
		int isNextFree = 0;
		char *bp = reallocCoalesce(ptr, asize, &isNextFree);
		if(isNextFree == 1) {
			// case 2
			// we find an enough space, which coalesce current block and next free block
			reallocPlace(bp, asize);
			return bp;
		} else if(isNextFree == 0 && bp != ptr) {
			// case 3 or case 4
			// no matter what, we find a enough space
			// copy the content
			memcpy(bp, ptr, size);
			// set mark flags
			reallocPlace(bp, asize);
			return bp;
		} else {
			// case 1, we don't find a fit space
			newPtr = mm_malloc(size);
			memcpy(newPtr, ptr, size);
			/* Free the old block. */
			mm_free(ptr);
			return newPtr;

		}
	} else {
		// old size >= asize,  no need to alloc a new area
		reallocPlace(ptr, asize);
		return ptr;
	}
}

static void reallocPlace(void *bp, size_t asize) {
	// just set the mark flags
	size_t csize = GET_SIZE(HDRP(bp));

	/*improve the utils for realloc-bal.rep and realloc2-bal.rep, so i comment it*/

	/*
	 * // 针对于测试数据做了修改
	if((csize - asize) >= (2 * DSIZE)) {
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));

		// use the surplus heap block
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize - asize, 0));
		PUT(FTRP(bp), PACK(csize - asize, 0));
		PUT(NEXT_LINKNODE_RP(bp), 0);
		PUT(PREV_LINKNODE_RP(bp), 0);
		coalesce(bp);
	} else {
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
	}
	 */
	PUT(HDRP(bp), PACK(csize,1));
	PUT(FTRP(bp), PACK(csize,1));
}

static void *reallocCoalesce(void *bp, size_t newSize, int *isNextFree) {
	size_t  prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t  next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	*isNextFree = 0;
	// case 1, there are no more space, return ptr is equal to bp
	// case 2, ok, and return ptr is equal to bp, but we set isNextFree = 1
	// case 3, ok, and return ptr isn't equal to bp
	// case 4, ok, same as case 3

	if(prev_alloc && next_alloc) {
		// case 1
	} else if (prev_alloc && !next_alloc) {
		// case 2
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		if(size >= newSize) {
			// use the next free block and coalesce it
			// remove the next block from free list
			fixLinkeList(NEXT_BLKP(bp));
			PUT(HDRP(bp), PACK(size, 1));
			PUT(FTRP(bp), PACK(size, 1));
			// next is free
			*isNextFree = 1;
			return bp;
		}
	} else if (!prev_alloc && next_alloc) {

		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		if(size >= newSize) {
			// use this free block and mark it as used
			// remove the previous block from free list
			fixLinkeList(PREV_BLKP(bp));
			PUT(FTRP(bp), PACK(size, 1));
			PUT(HDRP(PREV_BLKP(bp)), PACK(size, 1));
			bp = PREV_BLKP(bp);
			return bp;
		}
	} else {
		size += GET_SIZE(FTRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
		if(size >= newSize) {
			// remove both from free list
			fixLinkeList(PREV_BLKP(bp));
			fixLinkeList(NEXT_BLKP(bp));
			PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 1));
			PUT(HDRP(PREV_BLKP(bp)), PACK(size, 1));
			bp = PREV_BLKP(bp);
		}
	}
	return bp;
}

static void *coalesce(void *bp){
	size_t  prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t  next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	if(prev_alloc && next_alloc) {
		// case 1
	} else if (prev_alloc && !next_alloc) {
		// case 2
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		// remove the next block from free list
		fixLinkeList(NEXT_BLKP(bp));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	} else if (!prev_alloc && next_alloc) {
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		// remove the previous block from free list
		fixLinkeList(PREV_BLKP(bp));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	} else {
		size += GET_SIZE(FTRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
		// remove both from free list
		fixLinkeList(PREV_BLKP(bp));
		fixLinkeList(NEXT_BLKP(bp));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}

	// after freed, insert it to free list
	insertToEmptyList(bp);
	return bp;
}

inline char *findListRoot(size_t size)
{
	int i = 0;
	if(size <= 8)
		i = 0;
	else if(size <= 16)
		i = 0;
	else if(size <= 32)
		i = 0;
	else if(size <= 64)
		i = 1;
	else if(size <= 128)
		i = 2;
	else if(size <= 256)
		i = 3;
	else if(size <= 512)
		i = 4;
	else if(size <= 2048)
		i = 5;
	else if(size <= 4096)
		i = 6;
	else
		i = 7;
	/*find the index of bin which will put this block */
	return blockListStart + (i * WSIZE);
}


/*
*insert_free_block - insert the free point into segragated free list.
*	In each category the free list is ordered by the free size from big to small.
*   When find a fit free block ,just find for begin to end ,the first fit one is the best fit one.
*/
inline void insertToEmptyList(char *p) {
	/*LIFO double linked list*/
	// 实现思路见注释
	/*
	 * 					root     Node1      Node2
	 * prev pointer		Node1	 NUll		Node1
	 * next	pointer		Null	 Node2		Node3
	 */

	// root 是整个链表的第一个元素，对应于root只保存指向链表中首节点的地址，首节点只有next，prev为NULL
	// pNext原本是链表第一个元素
	// 将p插入到链表头 ： 1 p的next是pNext  2 pNext的prev是p 3 root更新为指向首节点p 注意判断pNext是否为空
	char *root = findListRoot(GET_SIZE(HDRP(p)));
	char *pPrev = root;
	char *pNext = GET(root);

	/*
	 * 在每个大小类中，元素是降序排序的
	 */
	while(pNext != NULL) {
		// 找到一个合适的位置，插入在pNext之后，最终形式为pNext -> p -> pNext的next
		if(GET_SIZE(HDRP(pNext)) >= GET_SIZE(HDRP(p))) {
			break;
		}
		pPrev = pNext;
		pNext = GET(NEXT_LINKNODE_RP(pNext));
	}
	// now we just to insert, pPrev->p->pNext
	if(pPrev == root) {
		// 类似于上面图中的Node1
		PUT(root, p);
		PUT(NEXT_LINKNODE_RP(p), pNext);
		PUT(PREV_LINKNODE_RP(p), NULL);
		if(pNext != NULL)
			PUT(PREV_LINKNODE_RP(pNext), p);
	} else {
		PUT(NEXT_LINKNODE_RP(pPrev), p);
		PUT(PREV_LINKNODE_RP(p), pPrev);
		PUT(NEXT_LINKNODE_RP(p), pNext);
		if(pNext != NULL)
			PUT(PREV_LINKNODE_RP(pNext), p);
	}
}


inline void fixLinkeList(char *p) {
	// use to remove p block from free-list
	char *root = findListRoot(GET_SIZE(HDRP(p)));
	char *pPrev = GET(PREV_LINKNODE_RP(p));
	char *pNext = GET(NEXT_LINKNODE_RP(p));
	// 要删除一个节点p,有两部 ： 1 将pPrev的next指向pNext  2 将pNext的prev指向pPrev
	// 程序中需要注意pNext和pPrev是否为空，分别判断
	if(pPrev == NULL) {
		// 代表是链表的首节点，则pPrev = NULL， 删除p，同时要更新root指向pNext
		if(pNext != NULL) {
			// 若p的next不空，p的next的prev要指向p的prev（Null），因为p被删除了
			PUT(PREV_LINKNODE_RP(pNext), 0);
		}
		// 若prev为空，当前又被free了，则新的root就是p的下一个
		PUT(root, pNext);
	} else {
		if(pNext != NULL) {
			// 若p的next不为空，p的next的prev要指向p的prev，因为p被删除了
			PUT(PREV_LINKNODE_RP(pNext), pPrev);
		}
		// 若prev非空，当前节点未free，则prev的next要指向p的next
		PUT(NEXT_LINKNODE_RP(pPrev), pNext);
	}
	// p如今不在free-list中，清除pointer信息
	PUT(NEXT_LINKNODE_RP(p), 0);
	PUT(PREV_LINKNODE_RP(p), 0);
}


static void *find_fit(size_t size) {
	// best fit
	char *root = findListRoot(size);
	char *tmp = GET(root);
	// heap_listp 指向12 ，root最多只能到9, 所以是 != 10
	for(root; root != (heap_listp - (2 * WSIZE)); root += WSIZE) {
		char *tmpP = GET(root);
		while(tmpP != NULL)
		{
			if(GET_SIZE(HDRP(tmpP)) >= size)
				return tmpP;
			tmpP = GET(NEXT_LINKNODE_RP(tmpP));
		}
	}
	return NULL;
}



static void place(void *bp, size_t asize){
	size_t csize = GET_SIZE(HDRP(bp));
	// bp is using now, remove it from out free-list
	fixLinkeList(bp);
	// csize-asize 即放置之后剩余的大小
	if((csize - asize) >= (2 * DSIZE)) {
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));

		// 分割剩余部分， 将剩余部分设置为空闲block
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize - asize, 0));
		PUT(FTRP(bp), PACK(csize - asize, 0));
		// 将剩余部分标记为free，并进行合并
		PUT(NEXT_LINKNODE_RP(bp), 0);
		PUT(PREV_LINKNODE_RP(bp), 0);
		coalesce(bp);
	} else {
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
	}
}


int mm_check(char *function) {

}






