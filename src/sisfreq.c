#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

main(int argc, char **argv)
{
   char tmp[1024], outfile[1024], last[256];
   int len, pos, old_pos = 0;
   int count = 0, max_len = 0, compLength, charset;
   FILE *inp, *otp;

   if (argc == 1) {
     printf("%s - Check .sis files\nUsage is: %s sisfile\n", argv[0], argv[0]);
     exit(1);
   }

   if ((inp = fopen(argv[1], "rb")) == NULL) {
      perror(argv[1]);
      exit(-2);
   }
   last[0] = 0;
   compLength = (unsigned char)fgetc(inp);
   printf("# StringCompLength = %d\n", compLength);
   charset = (unsigned char)fgetc(inp);
   printf("# Character set id=%d\n", charset);
   while ((len = fgetc(inp)) != EOF) {
     count++;
     fread(tmp, 1, compLength, inp);
     tmp[len] = 0;
     fread(&pos, sizeof(int), 1, inp); /* Position in .inx */
     printf("%ld\t%s\n", pos-old_pos, tmp);
     old_pos = pos;
   }
   fclose(inp);

   printf("# Total Elements = %d\n", count);

   return 0;
}
