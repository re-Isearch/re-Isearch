#include <ctype.h>
#include <stdlib.h>
#include <locale.h>

/* /usr/lib/locale/ */

main(int argc, char **argv)
{
  int i;
  char *ptr = setlocale (LC_CTYPE, argv[1]);
  if (ptr)
    printf("unsigned char _trans_upper_%s[] = {", ptr);
  for (i = 0; i < 256; i++)
   {
     if (i % 8 == 0)
        printf("\n  ");
      if (!isspace(i) && isprint(i))
        printf ("/*%c*/", i);
      else
        printf ("/* */");
      printf( "'\\%03o'", __trans_upper[i]);
      if (i != 255)
	printf(", ");
   }
  printf("\n};\n\n");
}
