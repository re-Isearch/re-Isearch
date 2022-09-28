#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main(int argc, char *argv[])
{
   const char *memblock;
   int fd;
   struct stat sb;
   long size;

   printf("sizeof(size_t) = %ld\n", sizeof(size_t));

   fd = open(argv[1], O_RDONLY);
   fstat(fd, &sb);
   size = (uint64_t)sb.st_size / 1000000L;
   printf("Size: %lu MB\n", size);

   memblock = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
   if (memblock == MAP_FAILED) handle_error("mmap");

   for(uint64_t i = 0; i < 10; i++)
   {
     printf("[%lu]=%X ", i, memblock[i]);
   }
   printf("\n");
   return 0;
}

