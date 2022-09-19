#include <ctype.h>
#include <stdlib.h>
#include <locale.h>

#define _SP -1

/* /usr/lib/locale/ */
/*
C cz da de de_AT de_CH el en_AU en_CA en_IE en_NZ en_UK en_US en_US.UTF-8 es es_AR es_BO es_CL es_CO
es_CR es_EC es_GT es_MX es_NI es_PA es_PE es_PY es_SV es_UY es_VE et fr fr_BE fr_CA fr_CH hu iso_8859_1 it
lt lv nl nl_BE no pl POSIX pt pt_BR ru su sv tr

{"pl", "pl.ISO8859-2"},
  {"pl_PL", "pl.ISO8859-2"},
  {"pt", "pt.ISO8859-1"},
  {"pt_BR", "pt.ISO8859-1"},
  {"pt_PT", "pt.ISO8859-1"},
  {"ru",    "ru.ISO8859-5"},
  {"ru_SU", "ru.ISO8859-5"},
  {"sh",    "sh.ISO8859-2"},
  {"sh_YU", "sh.ISO8859-2"},
  {"sk",    "sk.ISO8859-2"},
  {"sv", "sv.ISO8859-1"},
  {"sv_SE", "sv.ISO8859-1"},
  {"tchinese", "zh.EUC"},
  {"tr",    "tr.ISO8859-3"},

*/

char *Locales[] = {
  "iso_8859_1",
  "pl_PL",
  "tr",
  "ru",
  NULL
};

static signed char Soundex_code[256];

static int BuildSoundexTable ()
{
  int i = 0; 
  while (i < sizeof(Soundex_code)/sizeof(char))
    {
      char ch2 = toupper((unsigned char)i);
      switch (ch2)
        {
        case ' ': case '\r': case '\n': ch2 = 32; break;
        case 'H': case 'W': ch2 = 0; break;
        case 'B': case 'P': ch2 = 1; break;
        case 'C': case 'G': case 'J': case 'K': case 'Q': case 'X': ch2 = 2; break;
        case 'D': case 'T': ch2 = 3; break;
        case 'L': ch2 = 4; break;
        case 'M': case 'N': ch2 = 5; break;
        case 'R': ch2 = 6; break;
        case 'A': case 'E': case 'I': case 'O': case 'U': case 'Y': ch2 = 7; break;
        case 'F': case 'V': ch2 = 8;
        case 'S': case 'Z': ch2 = 9; break;
        default:
          if (!isalpha((unsigned char)i)) ch2 = -1;
          break;
        }
      Soundex_code[i++] = ch2;
    };
  return i;
}

main(int argc, char **argv)
{
  int i;
  char *ptr = setlocale (LC_CTYPE, argv[1]);
  if (ptr == NULL)
    ptr = "UNKNOWN";
  BuildSoundexTable();

  printf("static const char _soundex_%s[] = {", ptr);
  for (i = 0; i < 256; i++)
   {
     if (i % 8 == 0)
        printf("\n  ");
      if (!isspace(i) && isprint(i))
        printf ("/*%c*/", i);
      else
        printf ("/* */");
      if ((unsigned char)Soundex_code[i] == (unsigned char)32)
	printf("_SP");
      else
	printf( "%3d", Soundex_code[i]);
      if (i != 255)
        printf(", ");
   }
  printf("\n};\n\n");

#if 0
  printf("char _ctype_%s[] = {", ptr);
  for (i = 0; i < 256; i++)
   {
     if (i % 8 == 0)
        printf("\n  ");
      if (!isspace(i) && isprint(i))
        printf ("/*%c*/", i);
      else
        printf ("/* */");
      printf( "'\\%03o'", (__ctype + 1)[i]);
      if (i != 255)
	printf(", ");
   }
  printf("\n};\n\n");

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

  printf("unsigned char _trans_lower_%s[] = {", ptr);
  for (i = 0; i < 256; i++)
   {
     if (i % 8 == 0)
        printf("\n  ");
      if (!isspace(i) && isprint(i))
        printf ("/*%c*/", i);
      else
        printf ("/* */");
      printf( "'\\%03o'", __trans_lower[i]);
      if (i != 255)
        printf(", ");
   }
  printf("\n};\n\n");
#endif
}
