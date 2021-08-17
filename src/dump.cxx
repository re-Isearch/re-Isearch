/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
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


typedef struct {
  char     sis[StringCompLength+1];
  int      offset;
} dict_t;

size_t length;

#define _T(_c) (isalnum(_c) && !isspace(_c))

static int SisKeys(const void *node1, const void *node2)
{
  const char *p1 = (((const dict_t *)node1)->sis)+1;
  const char *p2 = (((const dict_t *)node2)->sis)+1;

  int diff = strncasecmp(p1, p2, (unsigned char)*(((const dict_t *)node1)->sis));

  if (length && diff == 0 && _T( (unsigned char)p2[length] ) ) {
    diff = -(unsigned char)p2[length];
  }
  return diff;
}

extern int findIt(const char *, const char *, GPTYPE *);

main(int argc, char **argv)
{
  if (argc == 1)
    printf("%s: [db_root [term]]\n", argv[0]);
  else if (argc > 2) {
    GPTYPE from = 0;
    int found = findIt(argv[1], argv[2], &from);   
    if (found)
	printf("FOUND %s:::: %lu -> %lu (%ld times)\n",
		argv[2], from+1, from+found, found);
    else
	printf("NOT FOUND\n");
  } else {
   char tmp[256];
   int len;
   int count = 0;

   FILE *inp = (argv[1] ? fopen(argv[1], "r") : stdin );
   while ((len = fgetc(inp)) != EOF) {
   fread(tmp, 1, StringCompLength, inp);
   tmp[len] = 0;
   fread(&len, sizeof(int), 1, inp);
   printf("%d: %s  \tOFFSET=%d\n", count++, tmp, len);
  }
  if (inp != stdin)
   fclose(inp);
 }
}
  

int findIt(const char *Root, const char *Term, GPTYPE *start)
{
  int num_hits = 0;
  char Index[1024];

  strcpy(Index, Root);
  strcat(Index, ".sis");
  int fd = open(Index, O_RDONLY);
  struct stat st_buf;

  if (fd == -1) {
    perror(Index);
    return num_hits;
  }

  if (fstat(fd, &st_buf) == 0) {
    char *Buffer = mmap((caddr_t)0, st_buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    dict_t n;
    strcpy(&(n.sis)[1], Term);
    length = (n.sis)[0] = strlen(Term);
    if ((n.sis)[length] == '*') {
      (n.sis)[length] = '\0';
      length = 0;
      (n.sis)[0]--;
    }

    madvise (Buffer, st_buf.st_size, MADV_RANDOM); // Binary search..

    // Find any match..
    dict_t *t = (dict_t *) bsearch(&n, Buffer,
	st_buf.st_size/(sizeof(int)+StringCompLength+1), sizeof(int)+StringCompLength+1,
	SisKeys);
    if (t) {
	// We now have ANY Match...

        off_t offset = (long)t-(long)Buffer;
	GPTYPE to = 0, from = 0;

	GPTYPE c1 = (unsigned char)(t->sis)[StringCompLength+1];
	GPTYPE c2 = (unsigned char)(t->sis)[StringCompLength+2];
	GPTYPE c3 = (unsigned char)(t->sis)[StringCompLength+3];
	GPTYPE c4 = (unsigned char)(t->sis)[StringCompLength+4];

	to |= c1 << 24;
	to |= c2 << 16;
	to |= c3 << 8; 
	to |= c4;

	if (offset) {
          c1 = (unsigned char)(t->sis)[-4];
          c2 = (unsigned char)(t->sis)[-3];
          c3 = (unsigned char)(t->sis)[-2];
          c4 = (unsigned char)(t->sis)[-1];

          from |= c1 << 24;
          from |= c2 << 16;
          from |= c3 << 8;
          from |= c4;
	} else from = -1;
	if (start)
	  *start = from;
	num_hits = to - from;
    }
    if (-1 == munmap(Buffer, st_buf.st_size))
	perror("Can't unmap memory");
  }
  close(fd);
  return num_hits;
}
