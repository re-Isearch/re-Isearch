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
#include <ctype.h>

#include "common.hxx"
#include "ctype.hxx"
#include "lang-codes.hxx"


main(int argc, char **argv)
{
   char tmp[1024], outfile[1024], last[256];
   int len, pos;
   int count = 0, max_len = 0, compLength, charset;
   FILE *inp, *otp;

   if (argc == 1) {
     printf("%s - Check .sis files\nUsage is: %s [sisfile|database]\n", argv[0], argv[0]);
     exit(1);
   }

   if ((inp = fopen(argv[1], "rb")) == NULL) {
      perror(argv[1]);
      exit(-2);
   }
   last[0] = 0;
   compLength = (unsigned char)fgetc(inp);
   printf("StringCompLength = %d\n", compLength);
   charset = (unsigned char)fgetc(inp);

   CHARSET Set(charset);
   printf("Character set id=%d (%s)\n", charset, (const char *)Set);

   int (* SisCompare)(const void *, const void *) = Set.SisCompare();
   int errors = 0;
   int res;
   while ((len = fgetc(inp)) != EOF) {
     count++;
     fread(tmp, 1, compLength, inp);
     tmp[len] = 0;
     if ((res = SisCompare((void *)tmp, (void *)last)) <= 0)
	{
	  if (res || len < compLength)
	    {
	      errors++;
	      printf("Error %s followed by %s\n", last, tmp);
	    }
	}
     strcpy(last, tmp);
     fread(&pos, sizeof(GPTYPE), 1, inp); /* Position in .inx */
   }
   fclose(inp);

   printf("Total Elements = %d\n", count);
   if (errors)
     printf("Found %d\n", errors);
   else
     printf("[OK]\n");
   return errors;
}
