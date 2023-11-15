/*
 * 책의 implicit list 구현
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "8team",
    /* First member's full name */
    "Sungjae Bae",
    /* First member's email address */
    "hsm0156@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define PACK(size, alloc) ((size) | (alloc))
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

// 주어진 블록 포인터 bp에 대해 헤더와 푸터(바운더리 태그)의 주소를 계산한다
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// 주어진 블록 포인터 bp에 대해 다음 블록, 이전 블록 주소를 계산한다
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

#define CHECKHEAP(lineno, str)                                                 \
  printf("===========[%s(%s)]", (__func__), (str));                            \
  mm_checkheap((lineno));

// 정적 변수
void *heap_listp = NULL;

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void mm_checkheap(int lineno);
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
  if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1) {
    return -1;
  }
  PUT(heap_listp, 0);                            // alignment 패딩
  PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); // 프롤로그 헤더
  PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); // 프롤로그 푸터
  PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     // 에필로그 헤더
  heap_listp += (2 * WSIZE);

  if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
    return -1;
  }
#ifdef DEBUG
  CHECKHEAP(__LINE__, "");
#endif
  return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
  static int malloc_called = 0;
  malloc_called++;
  if (malloc_called == 110) {
    printf("error condition");
  }
  size_t asize;
  size_t extendsize;
  char *bp;

  if (size == 0) {
    return NULL;
  }

  asize = DSIZE * (((size + (DSIZE) + (DSIZE - 1)) / DSIZE));

  // fit 맞는 블록을 찾아서 거기에 할당한다
  if ((bp = find_fit(asize)) != NULL) {
    place(bp, asize);
#ifdef DEBUG
    char s[30] = {
        0,
    };
    sprintf(s, "size: %d, found fit block", size);
    CHECKHEAP(__LINE__, s);
#endif
    return bp;
  }
  // 못찾으면 힙사이즈를 늘린다
  extendsize = MAX(asize, CHUNKSIZE);
  if ((bp = extend_heap(extendsize / WSIZE)) == NULL) {
#ifdef DEBUG
    char s[40] = {
        0,
    };
    sprintf(s, "size: %d, can't extend_heap[error]", size);
    CHECKHEAP(__LINE__, s);
#endif
    return NULL;
  }
  place(bp, asize);
#ifdef DEBUG
  char s[30] = {
      0,
  };
  sprintf(s, "size: %d, extend_heap called", size);
  CHECKHEAP(__LINE__, s);
#endif
  return bp;
}

// splitting 기능이 필요하다. asize는 이미 aligned 되었다
static void place(void *bp, size_t asize) {
  size_t oldsize = GET_SIZE(HDRP(bp));
  PUT(HDRP(bp), PACK(asize, 1));
  PUT(FTRP(bp), PACK(asize, 1));
  if (asize < oldsize) {
    // split 한다
    void *rest_bp = NEXT_BLKP(bp);
    PUT(HDRP(rest_bp), PACK(oldsize - asize, 0));
    PUT(FTRP(rest_bp), PACK(oldsize - asize, 0));
  }
}

static void *extend_heap(size_t words) {
  char *bp;
  size_t size;
  // 홀수면 (word+1)*WSIZE, 짝수면 word*WSIZE
  size = (words % 2) ? (words + 3) * WSIZE : (words + 2) * WSIZE;
  if ((long)(bp = mem_sbrk(size)) == -1) {
    return NULL;
  }

  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

  return coalesce(bp);
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp) {
  size_t size = GET_SIZE(HDRP(bp));
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  coalesce(bp);
#ifdef DEBUG
  CHECKHEAP(__LINE__, "free done");
#endif
}

// size0으로 바꿔줘야 하는 부분 왜 없음??
static void *coalesce(void *bp) {
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
  size_t size = GET_SIZE(HDRP(bp));

  if (prev_alloc && next_alloc) {
    return bp;
  } else if (prev_alloc && !next_alloc) {
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0)); // 헤더 보고 사이즈만큼 알아서 세팅
  } else if (!prev_alloc && next_alloc) {
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  } else {
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    bp = PREV_BLKP(bp);
  }
  return bp;
}

void *mm_realloc(void *bp, size_t size) {
  if (size <= 0) {
    mm_free(bp);
    return NULL;
  }

  if (bp == NULL) {
    return mm_malloc(size);
  }

  void *newp = mm_malloc(size);
  if (newp == NULL) {
    return NULL;
  }

  size_t oldsize = GET_SIZE(HDRP(bp));

  memcpy(newp, bp, oldsize > size ? size : oldsize);
  mm_free(bp);
#ifdef DEBUG
  CHECKHEAP(__LINE__, "realloc done");
#endif
  return newp;
}

// first fit
static void *find_fit(size_t asize) {
  void *cur = heap_listp + 2 * WSIZE;
  unsigned int cur_size = 0;
  unsigned int cur_alloc = 0;
  while (1) {
    cur_alloc = GET_ALLOC(HDRP(cur));
    cur_size = GET_SIZE(HDRP(cur));
    if (cur_alloc == 0u && cur_size >= asize) {
      return cur;
    }
    if (cur_alloc == 1u && cur_size == 0u) {
      break;
    }
    cur = NEXT_BLKP(cur);
  }
  return NULL;
}

static void mm_checkheap(int lineno) {
  printf("heap checking, line %d ===========\n", lineno);
  void *cur = heap_listp;
  unsigned int header_size = 0;
  unsigned int header_alloc = 0;
  unsigned int footer_size = 0;
  unsigned int footer_alloc = 0;
  while (1) {
    int zero_offset =
        -8 + ((unsigned long long)cur - (unsigned long long)heap_listp);
    int header_offset = zero_offset + 4;
    header_size = GET_SIZE((unsigned long long)(heap_listp) + header_offset);
    header_alloc = GET_ALLOC((unsigned long long)(heap_listp) + header_offset);

    if (header_size == 0u && header_alloc == 1u) {
      printf("[header offset]: %10d [size]: %4u [alloc]: %1u END CONDITION\n",
             header_offset + 8, header_size, header_alloc);
      break;
    }

    int footer_offset = header_offset + header_size - 4;
    footer_size = GET_SIZE((unsigned long long)(heap_listp) + footer_offset);
    footer_alloc = GET_ALLOC((unsigned long long)(heap_listp) + footer_offset);

    printf("[header offset]: %10d [size]: %4u [alloc]: %1u [cur offset]:%10d "
           "[payload size]: %4u "
           "[footer offset]: %10d [size]: %4u [alloc]: %1u\n",
           header_offset + 8, header_size, header_alloc, header_offset + 12,
           header_size - 8, footer_offset + 8, footer_size, footer_alloc);

    cur = NEXT_BLKP(cur);
  }
}