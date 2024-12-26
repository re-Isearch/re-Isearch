/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)qsort.cxx  1.4 04/17/01 13:01:28 BSN"

#define REGISTER 

/*-
 * Copyright (c) 1992, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: qsort.cxx,v 1.1 2007/05/15 15:47:23 edz Exp $
 */

#include <stdlib.h>
#include <memory.h>


typedef int              cmp_t(const void *, const void *);
static inline char      *med3(const void *, const void *, const void *, cmp_t *);
static inline void       swapfunc(void *, void *, int, int);

#define min(a, b)       (a) < (b) ? a : b

/*
 * Qsort routine from Bentley & McIlroy's "Engineering a Sort Function".
 */
#define swapcode(TYPE, parmi, parmj, n) {               \
        long i = (n) / sizeof (TYPE);                   \
        REGISTER TYPE *pi = (TYPE *) (parmi);           \
        REGISTER TYPE *pj = (TYPE *) (parmj);           \
        do {                                            \
                REGISTER TYPE   t = *pi;                \
                *pi++ = *pj;                            \
                *pj++ = t;                              \
        } while (--i > 0);                              \
}

#define SWAPINIT(a, es) swaptype = ((char *)a - (char *)0) % sizeof(long) || \
        es % sizeof(long) ? 2 : es == sizeof(long)? 0 : 1;

static inline void swapfunc(void *a, void *b, int n, int swaptype)
{
        if(swaptype <= 1) swapcode(long, a, b, n)
        else              swapcode(char, a, b, n)
}

#define swap(a, b)                                      \
        if (swaptype == 0) {                            \
                long t = *(long *)(a);                  \
                *(long *)(a) = *(long *)(b);            \
                *(long *)(b) = t;                       \
        } else                                          \
                swapfunc(a, b, es, swaptype)

#define vecswap(a, b, n)        if ((n) > 0) swapfunc(a, b, n, swaptype)

static inline char *med3(const void *a, const void *b, const void *c, cmp_t *cmp)
{
        return (char *) ( cmp(a, b) < 0 ?
               (cmp(b, c) < 0 ? b : (cmp(a, c) < 0 ? c : a ))
              :(cmp(b, c) > 0 ? b : (cmp(a, c) < 0 ? a : c )));
}

void BentleyQsort(void *a, size_t n, size_t es, cmp_t *cmp)
{
        char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
        int d, r, swaptype, swap_cnt;

loop:   SWAPINIT(a, es);
        swap_cnt = 0;
        if (n < 7) {
                for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
                        for (pl = pm; pl > (char *)a && cmp(pl - es, pl) > 0;
                             pl -= es)
                                swap(pl, pl - es);
                return;
        }
        pm = (char *)a + (n / 2) * es;
        if (n > 7) {
                pl = (char *)a;
                pn = (char *)a + (n - 1) * es;
                if (n > 40) {
                        d = (n / 8) * es;
                        pl = med3(pl, pl + d, pl + 2 * d, cmp);
                        pm = med3(pm - d, pm, pm + d, cmp);
                        pn = med3(pn - 2 * d, pn - d, pn, cmp);
                }
                pm = med3(pl, pm, pn, cmp);
        }
        swap(a, pm);
        pa = pb = (char *)a + es;

        pc = pd = (char *)a + (n - 1) * es;
        for (;;) {
                while (pb <= pc && (r = cmp(pb, a)) <= 0) {
                        if (r == 0) {
                                swap_cnt = 1;
                                swap(pa, pb);
                                pa += es;
                        }
                        pb += es;
                }
                while (pb <= pc && (r = cmp(pc, a)) >= 0) {
                        if (r == 0) {
                                swap_cnt = 1;
                                swap(pc, pd);
                                pd -= es;
                        }
                        pc -= es;
                }
                if (pb > pc)
                        break;
                swap(pb, pc);
                swap_cnt = 1;
                pb += es;
                pc -= es;
        }
        if (swap_cnt == 0) {  /* Switch to insertion sort */
                for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
                        for (pl = pm; pl > (char *)a && cmp(pl - es, pl) > 0;
                             pl -= es)
                                swap(pl, pl - es);
                return;
        }

        pn = (char *)a + n * es;
        r = min(pa - (char *)a, pb - pa);
        vecswap(a, pb - r, r);
        r = min(pd - pc, pn - (pd + es));
        vecswap(pb, pn - r, r);
        if ((r = pb - pa) > 0 && (size_t)r > es)
                BentleyQsort(a, r / es, es, cmp);
        if ((r = pd - pc) > 0 && (size_t)r > es) {
                /* Iterate rather than recurse to save stack space */
                a = pn - r;
                n = r / es;
                goto loop;
        }
/*              BentleyQsort(pn - r, r / es, es, cmp);*/
}



/* Byte-wise swap two items of size SIZE. */
#define SWAP(a, b, size)                                                      \
    do                                                                          \
      {                                                                         \
        REGISTER size_t __size = (size);                                        \
        REGISTER char *__a = (a), *__b = (b);                                   \
        do                                                                      \
          {                                                                     \
            char __tmp = *__a;                                                  \
            *__a++ = *__b;                                                      \
            *__b++ = __tmp;                                                     \
          } while (--__size > 0);                                               \
      } while (0)

/*
 * Discontinue quicksort algorithm when partition gets below this size. This
 * particular magic number was chosen to work best on a Sun 4/260.
 */
#define MAX_THRESH 4

/* Stack node declarations used to store unfulfilled partition obligations. */
typedef struct {
	char           *lo;
	char           *hi;
}               stack_node;

/* The next 4 #defines implement a very fast in-line stack abstraction. */
#define STACK_SIZE      (8 * sizeof(unsigned long int))
#define PUSH(low, high) ((void) ((top->lo = (low)), (top->hi = (high)), ++top))
#define POP(low, high)  ((void) (--top, (low = top->lo), (high = top->hi)))
#define STACK_NOT_EMPTY (stack < top)


void SedgewickQsort(void *pbase, size_t total_elems, size_t size, cmp_t cmp)
{
	REGISTER char  *base_ptr = (char *) pbase;
	char           *pivot_buffer;
	const size_t    max_thresh = MAX_THRESH * size;


	if (total_elems == 0) return;

	/*
	 * Allocating SIZE bytes for a pivot buffer facilitates a better
	 * algorithm below since we can do comparisons directly on the pivot.
	 */
	try {
	  pivot_buffer = new char [size];
	} catch (...) {
	  return; // Out of memory
	}

	if (total_elems > MAX_THRESH) {
		char           *lo = base_ptr;
		char           *hi = &lo[size * (total_elems - 1)];
		/* Largest size needed for 32-bit int!!! */
		stack_node      stack[STACK_SIZE];
		stack_node     *top = stack + 1;

		while (STACK_NOT_EMPTY) {
			char           *left_ptr;
			char           *right_ptr;

			char           *pivot = pivot_buffer;

			/*
			 * Select median value from among LO, MID, and HI.
			 * Rearrange LO and HI so the three values are
			 * sorted. This lowers the probability of picking a
			 * pathological pivot value and skips a comparison
			 * for both the LEFT_PTR and RIGHT_PTR.
			 */

			char           *mid = lo + size * ((hi - lo) / size >> 1);

			if ((*cmp) ((void *) mid, (void *) lo) < 0)
				SWAP(mid, lo, size);
			if ((*cmp) ((void *) hi, (void *) mid) < 0)
				SWAP(mid, hi, size);
			else
				goto jump_over;
			if ((*cmp) ((void *) mid, (void *) lo) < 0)
				SWAP(mid, lo, size);
			jump_over:;
			memcpy(pivot, mid, size);
			pivot = pivot_buffer;

			left_ptr = lo + size;
			right_ptr = hi - size;

			/*
			 * Here's the famous ``collapse the walls'' section
			 * of quicksort. Gotta like those tight inner loops!
			 * They are the main reason that this algorithm runs
			 * much faster than others.
			 */
			do {
				while ((*cmp) ((void *) left_ptr, (void *) pivot) < 0)
					left_ptr += size;

				while ((*cmp) ((void *) pivot, (void *) right_ptr) < 0)
					right_ptr -= size;

				if (left_ptr < right_ptr) {
					SWAP(left_ptr, right_ptr, size);
					left_ptr += size;
					right_ptr -= size;
				} else if (left_ptr == right_ptr) {
					left_ptr += size;
					right_ptr -= size;
					break;
				}
			}
			while (left_ptr <= right_ptr);

			if ((size_t) (right_ptr - lo) <= max_thresh) {
				if ((size_t) (hi - left_ptr) <= max_thresh)
					/* Ignore both small partitions. */
					POP(lo, hi);
				else
					/* Ignore small left partition. */
					lo = left_ptr;
			} else if ((size_t) (hi - left_ptr) <= max_thresh)
				/* Ignore small right partition. */
				hi = right_ptr;
			else if ((right_ptr - lo) > (hi - left_ptr)) {
				/* Push larger left partition indices. */
				PUSH(lo, right_ptr);
				lo = left_ptr;
			} else {
				/* Push larger right partition indices. */
				PUSH(left_ptr, hi);
				hi = right_ptr;
			}
		}
	} {
		char           *const end_ptr = &base_ptr[size * (total_elems - 1)];
		char           *tmp_ptr = base_ptr;
#define MIN(x, y) ((x) < (y) ? (x) : (y))
		char           *thresh = MIN(end_ptr, base_ptr + max_thresh);
		REGISTER char  *run_ptr;

		/*
		 * Find smallest element in first threshold and place it at
		 * the array's beginning.  This is the smallest array
		 * element, and the operation speeds up insertion sort's
		 * inner loop.
		 */

		for (run_ptr = tmp_ptr + size; run_ptr <= thresh; run_ptr += size)
			if ((*cmp) ((void *) run_ptr, (void *) tmp_ptr) < 0)
				tmp_ptr = run_ptr;

		if (tmp_ptr != base_ptr)
			SWAP(tmp_ptr, base_ptr, size);

		/*
		 * Insertion sort, running from left-hand-side up to
		 * right-hand-side.
		 */

		run_ptr = base_ptr + size;
		while ((run_ptr += size) <= end_ptr) {
			tmp_ptr = run_ptr - size;
			while ((*cmp) ((void *) run_ptr, (void *) tmp_ptr) < 0)
				tmp_ptr -= size;

			tmp_ptr += size;
			if (tmp_ptr != run_ptr) {
				char           *trav;

				trav = run_ptr + size;
				while (--trav >= run_ptr) {
					char            c = *trav;
					char           *hi, *lo;

					for (hi = lo = trav; (lo -= size) >= tmp_ptr; hi = lo)
						*hi = *lo;
					*hi = c;
				}
			}
		}
	}

	delete[] pivot_buffer;
}

#ifdef FREDSORT
/*
** This algorithm is called Frederickson's algorithm. 
** 
**     Split the input into n/logn blocks of size logn.  Keep a priority
**     queue P of the smallest elements in each block (P has size n/logn).
**     After deleting an element in P, delete the same element from its
**     block, and use a brute-force search of the block for the next
**     smallest element.
** 
** Here is the abstract of the talk.
** 
**            Department of Computer Science, University of Toronto
**          (SF = Sandford Fleming Building, 10 King's College Road)
** 
**        -------------------------------------------------------------
** 
**                                   THEORY
**                 SF1101, at 11:00 a.m., Friday 12 June 1998
** 
**                                 Theis Rauhe
**                         BRICS, University of Aarhus
** 
**                 "Optimal Time-Space Trade-offs for Sorting"
** 
** This talk addresses the fundamental problem of sorting n elements from an
** ordered universe in a sequential model of computation and in particular
** consider the time-space trade-off (product of time and space) for this
** problem.  Beame has shown a lower bound of $Omega(n^2)$ for this product
** leaving a gap of a logarithmic factor up to the previously best known upper
** bound of $O(n^2log n)$ due to Frederickson.
** 
** In this talk a comparison based sorting algorithm which closes this gap is
** presented.  This algorithm obtain the optimal time-space product $O(n^2)$
** for the full range of space bounds between $log n$ and $n/log n$.
*/

typedef unsigned long SETTYPE;

typedef struct _setType {
    unsigned int n; /* in bits */
    SETTYPE* array;
} SET;


static SET *fredDone;
static void *fredArray;
static int fredSize, fredWidth;
static cmp_t fredCmp;

/*
** Compare Fred things using the array index rather than the objects
** themselves.  Use fredArray and fredCmp to call the "real" comparison
** function.
*/
#define FredCmpIndex(fi, fj) \
    (cmp = fredCmp(fredArray + fi.i*fredWidth, fredArray + fj.i*fredWidth)) == 0 ? fi.i - fj.i : cmp;

/*
** Use brute force to find the found element in a block, starting at
** "start" and going for n elements.  Return index of biggest, relative
** to the entire array, ie, start <= biggest < start + n.
*/
#define BruteSearch(start, end) { \
    int i, found = -1; \
    for(i=start; i<end; i++) { \
	if(SetIn(fredDone, i)) continue; \
	if(found == -1) { found = i; continue; } \
	if(FredCmpIndex(i, found) < 0) found = i; \
    } \
}

int FredSort(void *a, size_t n, size_t w, cmp_t compare)
{
    int cmp;
    int i, block, numCopies=0;
    int blockSize = Log2(n);	/* floor(log_2(n)) */
    int numBlocks = (n+blockSize-1)/blockSize;
    HEAP *P = HeapAlloc(numBlocks, FredCmpIndex);
    int outputNext = 0;
    char output[n*w];

	int next, nextsBlock, found;
    
    fredArray = a;
    fredSize = n;
    fredWidth = w;
    fredCmp = compare;
    fredDone = SetAlloc(n);

    /* Initialize the Heap */
    for(block=0; block < numBlocks; block++)
    {
	found = BruteSearch(block*blockSize, MIN((block+1)*blockSize, n));
	assert(block*blockSize <= found && found < MIN((block+1)*blockSize, n));
	HeapInsert(P, found);
    }
    assert(HeapSize(P) == numBlocks);
    
    for(i=0; i<n; i++)
    {
	next = HeapNext(P).i;
	assert(0 <= next && next < n);
	nextsBlock = next / blockSize;
	assert(0 <= nextsBlock && nextsBlock < numBlocks);
	memcpy(output + w*outputNext++, a + w*next, w);
	++numCopies;

	assert(!SetIn(fredDone, next));
	SetAdd(fredDone, next);
	found=BruteSearch(nextsBlock*blockSize,
	    MIN((nextsBlock+1)*blockSize, n));
	if(found != -1)
	    HeapInsert(P, found);
    }

    /* sanity checks */
    for(i=0; i<n; i++)
	assert(SetIn(fredDone,i));
    assert(HeapSize(P) == 0);

    /*
    ** Copy the output back into the correct places.
    */
    memcpy(a, output, n*w);
    numCopies += n;
    return numCopies;
}
#endif


#include <stdint.h>
#include <time.h>

/* Quick and dirty PRNG (xorshift) */
static uint32_t utl_rnd()
{
  static uint32_t rnd = 0;
    while (rnd == 0) rnd = (uint32_t)time(0);
    rnd ^= rnd << 13;
    rnd ^= rnd >> 17;
    rnd ^= rnd << 5;
    return rnd;
}

#define utl_dpqswap(a,b) do { if (a!=b) { \
                                  uint32_t sz = esz;\
                                  uint8_t  tmp8; \
                                  uint32_t tmp32; \
                                  uint8_t *pa = ((uint8_t *)a); \
                                  uint8_t *pb = ((uint8_t *)b); \
                                  while (sz >= 4) { \
                                    tmp32 = *(uint32_t *)pa; \
                                    *(uint32_t *)pa = *(uint32_t *)pb;\
                                    *(uint32_t *)pb = tmp32;\
                                    sz-=4; pa+=4; pb+=4;\
                                  }\
                                  switch (sz) {\
                                    case 3: tmp8=*pa; *pa=*pb; *pb=tmp8; pa--; pb--;\
                                    case 2: tmp8=*pa; *pa=*pb; *pb=tmp8; pa--; pb--;\
                                    case 1: tmp8=*pa; *pa=*pb; *pb=tmp8; pa--; pb--;\
                                  }\
                                }\
                           } while (0)

#define utl_dpqptr(k)    ((uint8_t *)base+(k)*esz)
#define utl_dpqpush(l,r) do {stack[stack_top][0]=(l); stack[stack_top][1]=(r); stack_top++; } while(0)
#define utl_dpqpop(l,r)  do {stack_top--; l=stack[stack_top][0]; r=stack[stack_top][1];} while(0)

/* Dropin replacement for qsort() using double pivot quicksort */

void DualPivotQsort (void *base, size_t nel, size_t esz, cmp_t *cmp)
{
  int32_t left,right;
  uint8_t *leftptr, *rightptr;

  uint32_t L,K,G;

  int32_t stack[128][2]; // Enough for 2^31 max elements in the array
  int16_t stack_top = 0;

  utl_dpqpush(0,nel-1);
  while (stack_top > 0) {
    utl_dpqpop(left, right);
    if (left < right) {
      if ((right - left) <= 16) {  // Use insertion sort
        for (int32_t i = left+1; i<=right; i++) {
          rightptr = utl_dpqptr(i);
          leftptr = rightptr - esz;
          for (int32_t j=i; j>0 && cmp(leftptr, rightptr) > 0; j--) {
            utl_dpqswap(rightptr, leftptr);
            rightptr = leftptr;
            leftptr = rightptr - esz;
          }
        }
      }
      else {
        leftptr = utl_dpqptr(left);
        rightptr = utl_dpqptr(right);

        /* Randomize pivot to avoid worst case (already sorted array) */
        L = left + (utl_rnd() % (right-left));
        G = left + (utl_rnd() % (right-left));
        utl_dpqswap(utl_dpqptr(L),leftptr);
        utl_dpqswap(utl_dpqptr(G),rightptr);

        if (cmp(leftptr, rightptr) > 0) {
          utl_dpqswap(leftptr, rightptr);
        }
        L=left+1; K=L; G=right-1;
        while (K <= G) {
          if (cmp(utl_dpqptr(K), leftptr) < 0) {
            utl_dpqswap(utl_dpqptr(K), utl_dpqptr(L));
            L++;
          }
          else if (cmp(utl_dpqptr(K), rightptr) > 0) {
            while ((cmp(utl_dpqptr(G), rightptr) > 0) && (K<G))
              G--;

            utl_dpqswap(utl_dpqptr(K), utl_dpqptr(G));
            G--;
            if (cmp(utl_dpqptr(K), leftptr) < 0) {
              utl_dpqswap(utl_dpqptr(K), utl_dpqptr(L));
              L++;
            }
          }
          K++;
        }
        L--; G++;
        utl_dpqswap(leftptr, utl_dpqptr(L));
        utl_dpqswap(rightptr, utl_dpqptr(G));

        utl_dpqpush(G+1, right);
        utl_dpqpush(L+1, G-1);
        utl_dpqpush(left, L-1);
      }
    }
  }
}


//
// Choices are: BentleyQsort, SedgewickQsort, DualPivotQsort, qsort, timsort
//

void (*_IB_Qsort)(void *, size_t, size_t, cmp_t) = BentleyQsort; 

