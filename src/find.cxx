#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <ctype.h>

extern "C" {
  caddr_t mmap(caddr_t, size_t, int, int, int, off_t);
  int madvise(caddr_t addr, size_t len, int advice);
//  int munmap(caddr_t, int);
}


#include "defs.hxx"


#define IsTermChar(_x)  (isalnum((unsigned char)_x))
#define IsWordSep(_x)   (!IsTermChar(_x))

#define _T(_c) (isalnum(_c) && !isspace(_c))

// Threaded safe..
static int SisKeys(const void *node1, const void *node2)
{
  const unsigned char *p1 = ((const unsigned char *)node1)+2;
  const unsigned char *p2 = ((const unsigned char *)node2)+1;

  int diff = memcmp(p1, p2, *((const unsigned char *)node1+1));

  if (diff == 0) {
    const size_t length = (size_t)*((const unsigned char *)node1);
    if (length && !IsWordSep(p2[length]))
      diff = -p2[length];
  }
  return diff;
}

extern int findIt(GDT_BOOLEAN, const char *, const char *, GPTYPE *);

main(int argc, char **argv)
{
  if (argc == 1)
    printf("%s: [db_root [term]]\n", argv[0]);

  char Index[1024];

  strcpy(Index, argv[1]);
  strcat(Index, ".sis");

  if (argc > 2) {
    GPTYPE from = 0;
    char *term = strdup(argv[2]);
    size_t len = strlen(term);
    if (term[len-1] == '*') {
      term[len-1] = '\0';
      len = 0;
    }
    int found = findIt(len == 0, Index, term, &from);   
    if (found)
	printf("FOUND %s:::: %lu -> %lu (%ld times)\n",
		argv[2], from, from+found-1, found);
    else
	printf("NOT FOUND\n");
    free(term); // clean up
  } else {
   char tmp[256];
   int len;
   int count = 0;

   FILE *inp = fopen(Index, "r");

   int compLength = (unsigned char)fgetc(inp);

   printf("StringCompLength = %d\n", compLength);

   while ((len = fgetc(inp)) != EOF) {
   fread(tmp, 1, compLength, inp);
   tmp[len] = 0;
   fread(&len, sizeof(int), 1, inp);
   printf("%d: %s  \tOFFSET=%d\n", count++, tmp, len);
  }
  if (inp != stdin)
   fclose(inp);
 }
}
  

int findIt(GDT_BOOLEAN Truncate, const char *Index, const char *Term, GPTYPE *start)
{
  int num_hits = 0;

  int fd = open(Index, O_RDONLY);
  struct stat st_buf;

  if (fd == -1) {
    perror(Index);
    return num_hits;
  }

  if (fstat(fd, &st_buf) == 0) {
    char n[sizeof(int)+StringCompLength+3];
    size_t length;

    // Copy lower case into a BCPL-style string...
    // n[0] contains the match length
    // n[1] contains the string length
    // &n[2] contains the lower case term..
    for (length = 0; length < StringCompLength && Term[length]; length++)
      n[length+2] = tolower (Term[length]);
    n[length+2] = '\0'; // ASCIIZ
    
    n[0] = Truncate ? 0 : (char)length;
    n[1] = (char)length;

    char *Map = mmap((caddr_t)0, st_buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if (Map == (char *)-1) {
      perror("MMAP failed");
      return num_hits; // ERROR
    }

    const int  off = (st_buf.st_size % (sizeof(int)+StringCompLength+1) == 0 ? 0 : 1);
    char *Buffer = Map + off;
    const size_t compLength = off ? (unsigned char)Map[0] : StringCompLength;
    const size_t dsiz = compLength + sizeof(int)+1;

    // If compLength < term length than ALWAYS handle as truncated search
    if (compLength < (unsigned char)n[1])
      n[0] = '\0';

    // Advise the memory manager that we want random access for binary search..
    madvise (Buffer, st_buf.st_size, MADV_RANDOM); // Randon access 

    // Find any match..
    char *t = (char *) bsearch(&n, Buffer, st_buf.st_size/dsiz, dsiz, SisKeys);
    if (t) {
	// We now have ANY Match...
	char *High = t; // Last Element in range
	char *Low = t; // First Element in range

	// If Truncated search the hits can span several elements..
	if (n[0] == '\0') {
	  char *tp;
	  // Scan back to low (replace later with bsearches)
	   for(tp = (char *)t; tp >= Buffer; tp -= dsiz) {
	     if (memcmp(&n[2], &tp[1], (unsigned char)n[1]))
	       break;
	     Low = tp;
	   }
	  // Now look forward ... (replace later)
          for(tp = (char *)t; tp < &Buffer[st_buf.st_size]; tp += dsiz) {
	    if (memcmp(&n[2], &tp[1], (unsigned char)n[1]))
	      break;
	    High = tp;
	  }
	}

	const GPTYPE to =
	   ( ((GPTYPE)((unsigned char)High[compLength+1])) << 24 |
	     ((GPTYPE)((unsigned char)High[compLength+2])) << 16 |
	     ((GPTYPE)((unsigned char)High[compLength+3])) << 8 |
	     (GPTYPE)((unsigned char)High[compLength+4]) );

	// Make sure not the first element...
	const GPTYPE from = ((long)Low - (long)Buffer) ?
	   ( ((GPTYPE)((unsigned char)Low[-4])) << 24 |
	     ((GPTYPE)((unsigned char)Low[-3])) << 16 |
	     ((GPTYPE)((unsigned char)Low[-2])) << 8  |
	     ((GPTYPE)((unsigned char)Low[-1])) ) : -1;
	if (start)
	  *start = from + 1;
	num_hits = to - from;
    }
    if (-1 == munmap(Map, st_buf.st_size))
	perror("Can't unmap memory");
  } else
    close(fd);
  return num_hits;
}
