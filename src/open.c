#include <unistd.h>

main()
{
 const long p = sysconf(_SC_OPEN_MAX);
 printf("OPEN MAX = %ld\n", p);
}
