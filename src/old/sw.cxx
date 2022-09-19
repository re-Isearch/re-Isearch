#include "sw.hxx"

main()
{
 printf("# Default Stoplist\n");
 for (int i=0; i < sizeof(stoplist)/sizeof(char *); i++)
  {
    printf("%s\n", stoplist[i]);
  }
 printf("# END\n");
}
