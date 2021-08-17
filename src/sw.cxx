/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
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
