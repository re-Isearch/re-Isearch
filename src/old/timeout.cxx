#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "common.hxx"
#include "date.hxx"

main()
{
  SRCH_DATE  compile_time( __DATE__ );
  const long w = 60L*60*24*28*5 + (60L*60*24*14);
  time_t t = compile_time.MkTime()  + w;
  struct tm *tm = gmtime (&t);

  if (tm->tm_wday != 1)
    {
      int x = (tm->tm_wday % 7) + 1;
      tm->tm_mday += x; 
      tm->tm_mday += x;
    }
  tm->tm_hour = 9;
  tm->tm_min  = 0;
  tm->tm_sec  = 0;
  t= mktime (tm);
  printf("%ld", t);
  // printf("Expire number = %ld\n", t);
  // printf("Expires %s", ctime(&t));
}
