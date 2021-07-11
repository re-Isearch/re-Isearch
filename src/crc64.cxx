/*
 * UTIL/CRCTEST.C	- CRC tester, by Matt Dillon
 * 
 * This program is designed to test an N-bit CRC against a word dictionary
 * which you pipe into it.   It is not required for normal diablo operation.
 *
 * Warning: This program will eat a lot of memory with large sets.  If the
 * set is known to be unique, you can use -u to reduce the memory footprint.
 *
 * cat unique-words | CRCTEST [-u] [-v] [-q] [-h#]
 *
 *	-h#	set final hash size, in bits 16-64, Default is 64 bits.
 *	-u	assume unique input, do not store string contents
 *	-v	verbose output, print collisions
 *	-q	quiet output, do not print the count every 100,000 tests
 *
 * The expected number of collisions is (NSAMP * (NSAMP-1) / 2) / 2^CRCBITS.
 *
 * This is calculated through statistics.  If you had 7 samples and an 8 bit
 * CRC (256 slots), the number of collisions is
 *
 *		sample #1	0/256
 *		sample #2	1/256
 *		sample #3	2/256
 *		sample #4	3/256
 *		sample #5	4/256
 *		sample #6	5/256
 *		sample #7	6/256
 *	+	sample #8	7/256
 *	------------------------------
 *		    [ 8*(8-1)/2 ] / 2^CRCBITS
 *
 *      NOTE!! this only works if 2^CRCBITS is substantially larger then NSAMP
 *      because we aren't taking into account the fact that a prior samples
 *      may collide and not increment the chance of collision for later 
 *      samples.
 * 
 * So, for example, a 36 bit CRC with 1M samples should result in around 7
 * collisions.  A 42 bit CRC with 1M samples should result in around 0.1
 * collision.  A 42 bit CRC with 3 million samples should result in around 1 
 * collision.
 */

#include "gdt-sys.h"
#include <string.h>

typedef UINT4 hint_t;	/* we want a 32 bit unsigned integer here */

typedef struct hash_t {
    hint_t	h1;
    hint_t	h2;
} hash_t;

typedef struct Hash {
    struct Hash *ha_Next;
    hash_t	ha_Hv;
} Hash;

#define TESTHSIZE	(4 * 1024 * 1024)
#define TESTHMASK	(TESTHSIZE - 1)

static hash_t testhash(const char *p, size_t Length, size_t Size);

static int   HashLimit = 64;

#ifdef MAIN

static Hash *HashAry[TESTHSIZE];

int	UniqueOpt;
int	VerboseOpt = 0;
int	QuietOpt = 0;

static void * rmalloc(int bytes)
{
    static char *RBuf = NULL;
    static int    RSize = 0;

    bytes = (bytes + 3) & ~3;

    if (bytes > RSize) {
        RBuf = malloc(65536);
        RSize = 65536;
    }
    RBuf += bytes;
    RSize -= bytes;
    return(RBuf - bytes);
}


int main(int ac, char **av)
{
    char buf[256];
    int count = 0;
    int total = 0;
    int skip = 100000;
    int HashLimit = 64;

    int i;

    for (i = 1; i < ac; ++i) {
	char *p = av[i];

	if (*p == '-') {
	    p += 2;
	    switch(p[-1]) {
	    case 'u':
		UniqueOpt = 1;
		break;
	    case 'h':
		/*
		 * We can't go above 64 for obvious reasons.  We can't go
		 * below 16 due to the way I generate the polynomial.
		 */
		HashLimit = strtol(p, NULL, 0);
		if (HashLimit > 64 || HashLimit < 16) {
		    printf("valid values for -h between 16 & 64 inclusive\n");
		    exit(1);
		}
		break;
	    case 'q':
		QuietOpt = 1;
		break;
	    case 'v':
		VerboseOpt = 1;
		break;
	    default:
		fprintf(stderr, "Unknown option: %s\n", p - 2);
		exit(1);
	    }
	}
    }

    while (fgets(buf, sizeof(buf), stdin) != NULL) {
	int i;
	hash_t hv;
	Hash *h;
	char *s;
        char *ctxt; // strtok_r context

	for (s = strtok_r (buf, " ,\t\r\n", &ctxt); s; s = strtok_r (NULL, " ,\t\r\n", &ctxt)) {
	    hv = testhash(s, strlen(s), HashLimit);
	    i = (hv.h1 ^ hv.h2) & TESTHMASK;
	    /* printf("%08x.%08x (%d) %s\n", hv.h1, hv.h2, i, s); */
	    for (h = HashAry[i]; h; h = h->ha_Next) {
		if (h->ha_Hv.h1 == hv.h1 && h->ha_Hv.h2 == hv.h2) {
		    if (UniqueOpt || strcmp(s, (char *)(h + 1)) != 0) {
			if (VerboseOpt) {
			    printf("Collision: %s\t%s\n", 
				s, 
				((UniqueOpt) ? "?" : (char *)(h + 1))
			    );
			}
			++count;
			++total;
		    }
		    break;
		}
	    }
	    if (h == NULL) {
		h = rmalloc(sizeof(Hash) + ((UniqueOpt) ? 0 : strlen(s) + 1));
		h->ha_Next = HashAry[i];
		h->ha_Hv = hv;
		if (UniqueOpt == 0)
		    strcpy((char *)(h + 1), s);
		HashAry[i] = h;
		++total;
	    }
	}
	if (total >= skip) {
	    if (QuietOpt == 0) {
		printf("Count %d/%d\n", count, total);
		fflush(stdout);
	    }
	    skip += 100000;
	}
    }
    printf("Count %d/%d\n", count, total);
    return(0);
}

#endif

/*
 * Poly: 0x00600340.00F0D50A
 *
 */

#define HINIT1	0xFAC432B1UL
#define HINIT2	0x0CD5E44AUL

#define POLY1	0x00600340UL
#define POLY2	0x00F0D50BUL

hash_t CrcXor[256];
hash_t Poly[64+1];

void inithash(int newLimit)
{
    int i;

    if (newLimit == HashLimit && ( CrcXor[0].h1 != CrcXor[0].h2) && (CrcXor[0].h1 != 0))
      return;

    HashLimit = newLimit;

#ifdef MAIN
    printf("inithash()..");
#endif

    /*
     * Polynomials to use for various crc sizes.  Start with the 64 bit
     * polynomial and shift it right to generate the polynomials for fewer
     * bits.  Note that the polynomial for N bits has no bit set above N-8.
     * This allows us to do a simple table-driven CRC.
     */

    Poly[64].h1 = POLY1;
    Poly[64].h2 = POLY2;
    for (i = 63; i >= 16; --i) {
	Poly[i].h1 = Poly[i+1].h1 >> 1;
	Poly[i].h2 = (Poly[i+1].h2 >> 1) | ((Poly[i+1].h1 & 1) << 31) | 1;
    }

    for (i = 0; i < 256; ++i) {
	int j;
	int v = i;
	hash_t hv = { 0, 0 };

	for (j = 0; j < 8; ++j, (v <<= 1)) {
	    hv.h1 <<= 1;
	    if (hv.h2 & 0x80000000UL)
		hv.h1 |= 1;
	    hv.h2 = (hv.h2 << 1);
	    if (v & 0x80) {
		hv.h1 ^= Poly[HashLimit].h1;
		hv.h2 ^= Poly[HashLimit].h2;
	    }
	}
	CrcXor[i] = hv;
    }
#ifdef MAIN
    printf("\n");
#endif
}

/*
 * testhash() - do the CRC.  The complexity is simply due to the programmable
 *		nature of the number of bits.   We extract the top 8 bits to
 *		use as a table lookup to obtain the polynomial XOR 8 bits at
 *		a time rather then 1 bit at a time.
 */
static hash_t testhash(const char *p, size_t Length, size_t HashLimit)
{
    hash_t hv = { HINIT1, HINIT2 };
    size_t pos = 0;

    inithash(HashLimit);

    if (HashLimit <= 32) {
	int s = HashLimit - 8;
	hint_t m = (hint_t)-1 >> (32 - HashLimit);

	hv.h1 = 0;
	hv.h2 &= m;

	while (pos < Length) {
	    int i = (hv.h2 >> s) & 255;
	    hv.h2 = ((hv.h2 << 8) & m) ^ *p++ ^ CrcXor[i].h2;
	    pos++;
	}
    } else if (HashLimit < 32+8) {
	int s2 = 32 + 8 - HashLimit;	/* bits in byte from h2 */
	hint_t m = (hint_t)-1 >> (64 - HashLimit);

	hv.h1 &= m;
	while (pos < Length) {
	    int i = ((hv.h1 << s2) | (hv.h2 >> (32 - s2))) & 255;
	    hv.h1 = (((hv.h1 << 8) ^ (int)(hv.h2 >> 24)) & m) ^ CrcXor[i].h1;
	    hv.h2 = (hv.h2 << 8) ^ *p++ ^ CrcXor[i].h2;
	    pos++;
	}
    } else {
	int s = HashLimit - 40;
	hint_t m = (hint_t)-1 >> (64 - HashLimit);

	hv.h1 &= m;
	while (pos < Length) {
	    int i = (hv.h1 >> s) & 255;
	    hv.h1 = ((hv.h1 << 8) & m) ^ (int)(hv.h2 >> 24) ^ CrcXor[i].h1;
	    hv.h2 = (hv.h2 << 8) ^ *p++ ^ CrcXor[i].h2;
	    pos++;
	}
    }
    /* printf("%08lx.%08lx\n", (long)hv.h1, (long)hv.h2); */
    return(hv);
}


UINT8 CRC64(const char *ptr, size_t length)
{
  hash_t hv = testhash(ptr, length, 64);
  return (((UINT8)(hv.h1)) << 32) | ((UINT8)(hv.h2)) ;
}


UINT8 CRC64(const char *ptr)
{
  return ptr ? CRC64(ptr, strlen(ptr)) : 0;
}

