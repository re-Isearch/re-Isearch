
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main()
{
  FILE *f=fopen("../src/conf.h.inc", "w");
  if (!f) {
	perror ("Can't open output");
	exit(1);
  }

  fprintf(f, "#ifndef CONF_H_INC\n#define CONF_H_INC\n#ifdef __cplusplus\n\n\
extern \"C\" {\n\n#endif\n\n");


  /* signed and unsigned char need to be same size */
  fprintf(f, "#define SIZEOF_CHAR %u\n", (unsigned int)sizeof(signed char));
  fprintf(f, "#define SIZEOF_UCHAR %u\n", (unsigned int)sizeof(unsigned char));

  fprintf(f, "#define SIZEOF_SHORT_INT %u\n", (unsigned int)sizeof(short int));
  fprintf(f, "#define SIZEOF_INT %u\n", (unsigned int)sizeof(int));
  fprintf(f, "#define SIZEOF_LONG_INT %u\n", (unsigned int)sizeof(long int));
  fprintf(f, "#define SIZEOF_LONG_LONG_INT %u\n", (unsigned int)sizeof(long long int));

  fprintf(f, "#define SIZEOF_FLOAT %u\n", (unsigned int)sizeof(float));
  fprintf(f, "#define SIZEOF_DOUBLE %u\n", (unsigned int)sizeof(double));
  fprintf(f, "#define SIZEOF_LONG_DOUBLE %u\n\n", (unsigned int)sizeof(long double));

   if (!*(unsigned char *)&(uint16_t){1})
     fprintf(f, "#define IS_BIG_ENDIAN 1\n");
   else
     fprintf(f, "#define IS_LITTLE_ENDIAN 1\n");

  exit(0);
}
