#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include "platform.h"
#include "gdt-sys.h"

main(int argc, char **argv)
{
   char tmp[1024], outfile[1024], largest[256];
   int len;
   int count = 0, max_len = 0, compLength, charset;
   FILE *inp, *otp;

   if (argc == 1) {
     printf("%s - Compress .sis files\nUsage is: %s sisfile\n", argv[0], argv[0]);
     exit(1);
   }

   if ((inp = fopen(argv[1], "rb")) == NULL) {
      perror(argv[1]);
      exit(-2);
   }

   compLength = (unsigned char)fgetc(inp);
   printf("StringCompLength = %d\n", compLength);
   charset = (unsigned char)fgetc(inp);
   printf("Character set id=%d\n", charset);
   while ((len = fgetc(inp)) != EOF) {
     count++;
     fread(tmp, 1, compLength, inp);
     tmp[len] = 0;
     if (len > max_len) {
       max_len = len;
       memcpy(largest, tmp, len);
printf("%s %d\n", tmp, len);
       largest[len] = '\0';
       if (strlen(largest) != len)
	printf("%s truncated (len!=%d)\n", largest, len);
     }
     fread(&len, sizeof(GPTYPE), 1, inp);
   }
   largest[compLength] = 0;
   printf("Total Elements = %d\n", count);
   if (max_len <= compLength)
     printf("Max Term Length = %d\n", max_len);
   printf("Longest term ='%s' (%d)\n", largest, max_len);

   if (max_len >=(compLength - 1)) {
     printf("%s already compressed!\n", argv[1]);
     fclose(inp);
     exit(0);
   }

   sprintf(outfile, "%s.%d", argv[1], getpid());
   if ((otp = fopen(outfile, "wb")) == NULL) {
      perror(outfile);
   }
   rewind(inp);
   fgetc(inp);
   fputc(max_len, otp);

   printf("Compress...");
   while ((len = fgetc(inp)) != EOF) {
     fread(tmp, 1, compLength, inp);
     fputc((char)len, otp);
     fwrite(tmp, 1, max_len, otp);

     fread(&len, sizeof(GPTYPE), 1, inp);
     fwrite(&len, sizeof(GPTYPE), 1, otp);
     count--;
   }
   if (count == 0) {
      sprintf(tmp, "%s.OLD", argv[1]);
      unlink (tmp);
      if (rename(argv[1], tmp) == 0) {
	if (rename(outfile, argv[1]) == 0)
	  printf("%s Compressed\n", argv[1]);
	else
	  printf("Could not rename %s to %s\n", outfile, argv[1]);
      } else {
 	printf("Could not rename %s to %s\n", argv[1], tmp);
      }
   }
   printf ("OK\n");

   fclose(otp);
   fclose(inp);
   return 0;
}
