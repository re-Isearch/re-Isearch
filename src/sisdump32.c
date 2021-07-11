#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <arpa/inet.h>

#include "platform.h"
#include "gdt-sys.h"

#undef GPTYPE
#define GPTYPE UINT4
#undef O_BUILD_IB64

static GPTYPE GP(const void *ptr, int y)
{
#if BYTE_ORDER == BIG_ENDIAN
  register GPTYPE Gpp;
  memcpy(&Gpp, (const BYTE *)ptr+y, sizeof(GPTYPE));
  return Gpp;
#else
# ifdef O_BUILD_IB64
  const unsigned char *x = (const unsigned char *)ptr;
  return (((GPTYPE)x[y])   << 56) + (((GPTYPE)x[y+1]) << 48) +
         (((GPTYPE)x[y+2]) << 40) + (((GPTYPE)x[y+3]) << 32) +
         (((GPTYPE)x[y+4]) << 24) + (((GPTYPE)x[y+5]) << 16) +
          (((GPTYPE)x[y+6]) << 8) + x[y+7];

# else
  const unsigned char *x = (const unsigned char *)ptr;
  return (((GPTYPE)x[y])   << 24) + (((GPTYPE)x[y+1]) << 16) +
         (((GPTYPE)x[y+2]) << 8)  + x[y+3];
# endif
#endif
}



main(int argc, char **argv)
{
   char tmp[1024], outfile[1024], last[256];
   int    len;
   GPTYPE pos, old_pos = 0;
   GPTYPE p0, p1;
   int count = 0, max_len = 0, compLength, charset;
   FILE *inp, *otp;

   if (argc == 1) {
     printf("%s - Check %d-bit .sis files\nUsage is: %s sisfile\n",
	argv[0], sizeof(GPTYPE)*8, argv[0]);
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
     fread(&pos, sizeof(GPTYPE), 1, inp); /* Position in .inx */
     p0 = GP(&old_pos, 0);
     p1 = GP(&pos, 0); 
     printf("%s%s # %llu-%llu (%llu)\n", tmp, len >= compLength ? "*" : "", 
	(long long)p0, (long long)p1,
	(long long)(p1 - p0 + 1) );
     old_pos = pos;
   }
   fclose(inp);

   printf("# Total Elements = %u\n", count);

   return 0;
}
