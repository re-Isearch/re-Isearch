#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/file.h>

#ifndef O_BINARY
# define O_BINARY 0
#endif

static int IsWord(const char *ptr)
{
  while (*ptr++)
    if (strchr("aeiouy", tolower(*ptr)))
      return 1;
  return 0;
}

int main(int argc, char **argv)
{
  int   ch;
  int   i;
  FILE *fp;
  char  word[1024];

  if (argc > 1 && argv[1])
    fp = fopen(argv[1], "rb");
  else {
    fp = stdin;
#ifdef _WIN32
    setmode(fileno(fp), O_BINARY); 
#endif
  }
  i = 0;
  while ((ch = fgetc(fp)) != EOF)
    {
      if (isalnum(ch))
	{
	  if (i >= sizeof(word)) i = 0;
	  word[i++] = ch;
	}
      else
	{
	  if (i > 3 && IsWord(word)) {
	    fputs (word, stdout);
	    fputc(' ', stdout);
	  }
	  i = 0;
	}
    }
}
