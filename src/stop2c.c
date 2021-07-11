#include <stdio.h>
#include <string.h>


void clean_print(unsigned char *str)
{
  while (*str)
    {
      if (*str < 127 && *str >= 32)
	putchar(*str);
      else
	printf("\\%03o", *str);
      str++;
    }

}

main(int argc, char **argv)
{
  FILE *fp;
  char *ptr, *tp;
  char tmp[BUFSIZ];
  char *ext;

  if (argv[1] == NULL)
    ext = "default";
  else if ((ext = strrchr(argv[1], '.')) != NULL)
    ext++;
  else if ((ext = strrchr(argv[1], '/')) != NULL)
   ext++;
  else
   ext = argv[1];

  if (argv[1])
    fp = fopen(argv[1], "r");
  else
    fp = stdin;

  if (fp == NULL)
   {
     perror("Can't read input");
     exit(-1);
   }

  printf("const unsigned char stoplist_%s[] = {\n", ext);
  while (fgets(tmp, BUFSIZ, fp))
    {
      ptr = tmp;
      while (isspace(*ptr))
	ptr++;
      for (tp = ptr + strlen(ptr) - 1; tp > tmp; tp--)
	{
	  if (!isspace(*tp))
	    break;
	  *tp = '\0';
	}
      if (*ptr != '#' && *ptr)
	{
	  if (ptr[1])
	    {
	      putchar('"');
	      clean_print((unsigned char *)ptr);
	      putchar('"');
	      putchar(',');
	      putchar('\n');
	    }
	}
      else if (*ptr)
	printf("/* %s */\n", ptr + 1);
    }
  printf("NULL\n};\n");
}
