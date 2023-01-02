/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)mmap.cxx  1.23 02/05/01 00:33:17 BSN"

/*
LCDhandle = CreateFile("file", .. bunch of other parameters);
// Creates a file mapping to cover the entire file (no security, no name);
LCDMapHandle = CreateFileMapping(LCDhandle, 0, PAGE_READWRITE, 0, 0, NULL);
// Get a pointer to mapped memory for the entire file
LCDptr = (int *)MapViewOfFile(LCDMapHandle, PAGE_ALL_ACCESS, 0, 0, 0); 
*/

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
//#include <sys/file.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "defs.hxx"
#include "string.hxx"
#include "mmap.hxx"
#include <errno.h>

#ifdef KERNEL32
# define MMAP_MAX MAX_INT
#else
# define MMAP_MAX SIZE_MAX/4L
#endif

#ifdef WINAPI
#ifndef caddr_t
typedef char* caddr_t;
#endif
#endif

#if defined(LINUX) || defined(BSD386) || defined(WIN32)
#define HAVE_MADVISE 0
#else
#define HAVE_MADVISE 1
#endif

#ifndef _WIN32
# ifndef O_BINARY
#  define O_BINARY 0
# endif 
#endif

#ifdef _WIN32
static UINT winGetGranularity()
{
  static UINT ag = 0;
  if(ag == 0)
    {
      SYSTEM_INFO si;
      GetSystemInfo(&si);
      if ((ag = si.dwAllocationGranularity) == 0)
	ag = 65536L;
    }
  return ag;
}
# define ALLOC_GRAINULARITY winGetGranularity()
# ifndef PAGESIZE
#  define PAGESIZE winGetPageSize()
extern UINT winGetPageSize();
# endif
#endif

#ifndef PAGESIZE
# ifdef _SC_PAGESIZE 
#  define PAGESIZE sysconf(_SC_PAGESIZE)
# else
#  define PAGESIZE 4096
# endif
#endif

#ifndef  ALLOC_GRAINULARITY
# define  ALLOC_GRAINULARITY PAGESIZE
#endif

#ifndef PAGEOFFSET
# define  PAGEOFFSET  (ALLOC_GRAINULARITY - 1)
#endif
#ifndef PAGEMASK
# define  PAGEMASK (~PAGEOFFSET)
#endif



MMAP::MMAP()
{
  addr = NULL;
  ptr = NULL;
  inode = len = size = 0;
  window = 0;
  fhandle = 0;
}


MMAP::MMAP(const STRING& name)
{
  inode  = 0;
  len    = 0;
  window = 0;
  fhandle= 0;
  map((const char *)name, O_RDONLY, 0, 0, MapNormal);
}

MMAP::MMAP(const STRING& name, enum mapping_access flag)
{
  inode =  0;
  len   =  0;
  window = 0;
  fhandle= 0;

  map((const char *)name, O_RDONLY, 0, 0, flag);
}


MMAP::MMAP(const STRING& name, off_t from, off_t to, enum mapping_access flag)
{
  inode  = 0;
  len    = 0;
  window = 0;
  fhandle= 0;

  map((const char *)name, O_RDONLY, from, to, flag);
}

MMAP::MMAP(const STRING& name, int permit, off_t from, off_t to,
     enum mapping_access flag)
{
  inode  = 0;
  len    = 0;
  window = 0;
  fhandle= 0;

  map((const char *)name, permit, from, to, flag);
}


MMAP::MMAP(const char *name)
{
  inode  = 0;
  len    = 0;
  window = 0;
  fhandle= 0;

  map(name, O_RDONLY, 0, 0, MapNormal);
}

MMAP::MMAP(const char *name, off_t from, off_t to, enum mapping_access flag)
{
  inode  = 0;
  len    = 0;
  window = 0;
  fhandle= 0;

  map(name, O_RDONLY, from, to, flag);
}

MMAP::MMAP(const char *name, int permit, off_t from, off_t to,
     enum mapping_access flag)
{
  inode  = 0;
  len    = 0;
  window = 0;
  fhandle= 0;

  map(name, permit, from, to, flag);
}


MMAP::MMAP(int fd, off_t from, off_t to, enum mapping_access flag)
{
  inode  = 0;
  len    = 0;
  window = 0;
  fhandle= fd;

  map(fd, O_RDONLY, from, to, flag);
}

MMAP::MMAP(FILE *fp, off_t from, off_t to, enum mapping_access flag)
{
  inode  = 0;
  len    = 0;
  window = 0;
  fhandle= 0;

  map(fileno(fp), O_RDONLY, from, to, flag);
}

UCHR *MMAP::map(const char *name, int permit, off_t from, off_t to,
     enum mapping_access flag)
{
  UCHR           *p = 0;

  if (inode > 0)
    {
      struct stat     s;
      if (stat(name, &s) == 0)
	{
	  if (inode == s.st_ino)
          {
            return ptr; // Already mapped
	  }
	}
      // if we failed the stat maybe another try to open?
      errno = 0;
    }
  if (permit == 0)
    permit = O_RDONLY;
  if ((fhandle = open(name, permit|O_BINARY)) >= 0) {
      p = map(fhandle, permit, from, to, flag);
      close (fhandle);
  } else
    Unmap();
  return p;
}

UCHR *MMAP::map(int fd, int permit, off_t from, off_t to,
     enum mapping_access flag)
{
  struct stat     s;
  caddr_t         p = 0;
  int             prot;
  size_t          length = 0;
  off_t           offset = 0;

  if (permit == 0)
    permit = O_RDONLY;
#ifdef _WIN32
   setmode(fd, O_BINARY); // Do I need this?
#endif
  if (fstat(fd, &s) == 0) {
#ifdef _WIN32
# define PAD 2
#else
# define PAD 1
#endif
      const off_t    start = from - (from & (PAGEOFFSET));
      const off_t    end = (to == 0 ? s.st_size :
	// if almost at end of file lets map it all
	((s.st_size - to) <= PAD ? s.st_size : to));
#undef PAD

      if (end > start) {
	if (inode == s.st_ino && inode > 0)
	  {
	    errno = 0;
	    return ptr; // Already mapped
	  }
	Unmap(); // Just in case;
	if (permit == O_RDWR)
	  prot = PROT_READ | PROT_WRITE;
	else if (permit == O_WRONLY)
	  prot = PROT_WRITE;
	else
	  prot = PROT_READ;

	errno =0;
	if (
	     ((end-start) > ( MMAP_MAX - PAGESIZE)) ||
	     (end < start) ||
	     (p = (caddr_t)mmap((caddr_t) 0, length = (size_t)(end-start),
		prot, MAP_SHARED, fd, start)) == (void *) -1) {
	  const char msg[] = "VM Page Map failed";
	  switch (errno) {
	    default:
	      message_log (LOG_ERRNO, "handle %d failed to map [%d].", fd, (int)errno);
	    case 0:
	      message_log (errno == 0 ? LOG_PANIC : LOG_DEBUG, "%s: Can't map bytes from %ld to %ld in fd=%d.",
			msg, (long)start, (long)end, (int)fd);
	      break;
	    case EMFILE:
	      message_log (LOG_ERROR, "%s: The number of mapped regions exceeded an OS implementation-dependent limit", msg);
	      break;
	    case EINVAL:
	      message_log (LOG_ERROR, "%s: Bad values (EINVAL) length=%lu offset=%lu.", msg,
		(long)length, (long)start);
	      break;
	    case ENOMEM:
	      message_log (LOG_ERROR, "%s: Requested and previous mmappings exceed RLIMIT_VMEM", msg);
	      break;
#ifdef EOVERFLOW
	    case EOVERFLOW:
	      message_log (LOG_ERROR, "%s: Region exceeds  the  offset  maximum", msg);
	      break;
#endif
	  }
	  p     = 0;
	  inode = 0;
	  length= 0; 
          size  = 0;

	} else {
	  message_log (LOG_DEBUG, "%d mapped %lu-%lu[%lu bytes]", fd, (long)start, (long)(start+length),(long)length);
	  offset = from - start;
	  size = end - from;
	  inode = s.st_ino;
	}
      }
  } else {
    Unmap(); // Err
  }

  addr = (void *)p;
  len = length;
  if (p) {
    p += offset;
#if HAVE_MADVISE
# ifdef __APPLE__
#  define MADV_NOSYNC MADV_WILLNEED
# endif
    if (flag == MapSequential)
      madvise(p, len - offset, MADV_SEQUENTIAL|MADV_NOSYNC);
    else if (flag == MapRandom)
      madvise(p, len - offset, MADV_RANDOM|MADV_NOSYNC);
    else
      madvise(p, len - offset, MADV_NORMAL|MADV_NOSYNC);
#endif
  }
  return ptr = (UCHR *) p;
}

bool MMAP::Advise(int flag, size_t from, size_t to)
{
#if HAVE_MADVISE
  int advise = MADV_NORMAL;

  if ((off_t)(from + to) > len)
    return false;

  if (flag == MapSequential)
    advise = MADV_SEQUENTIAL;
  else if (flag == MapRandom)
    advise = MADV_RANDOM;
  size_t length = (to == 0 ? len : to) - from;
  return madvise( (char *)(ptr+from), length, flag == 0 ? MADV_NORMAL : advise) != -1;
#else
  return false;
#endif
}


bool MMAP::Ok() const
{
#ifdef _WIN32
  return len > 0;
#else
  return (len > 0 && inode);
#endif
}


size_t MMAP::Size() const
{
  return size;
}

bool MMAP::Unmap()
{
  int result = 0;
  if (len)
    result = munmap((caddr_t)addr, len);
  addr = NULL;
  ptr = NULL;
  inode = 0;
  size  = 0;
  len = 0;
  return result == 0;
}

bool MMAP::CreateMap(const STRING& fileName)
{
  return map(fileName.c_str(), O_RDONLY, 0, 0, MapNormal) != NULL;
}

bool MMAP::CreateMap(const STRING& fileName, enum mapping_access flag)
{
  return map(fileName.c_str(), O_RDONLY, 0, 0, flag) != NULL;
}

bool MMAP::CreateMap(int fd, enum mapping_access flag)
{
  return map(fd, O_RDONLY, 0, 0, flag) != NULL;
}

bool MMAP::CreateMap(int fd, off_t from, enum mapping_access flag)
{
  return map(fd, O_RDONLY, from, 0, flag) != NULL;
}

bool MMAP::CreateMap(int fd, off_t from, off_t to, enum mapping_access flag)
{
  return map(fd, O_RDONLY, from, to, flag) != NULL;
}


MMAP::~MMAP()
{
  if (len)
    munmap((caddr_t)addr, len);
}


MMAP_TABLE::MMAP_TABLE(size_t Elements)
{
  MaxElements = Elements > 0 ? Elements : 1;
  Table = new MMAP [MaxElements];
}

MMAP *MMAP_TABLE::Map(size_t Element) const
{
  if (Element >= 0 && Element < MaxElements)
    {
      return &Table[Element];
    }
  return NULL;
}


bool MMAP_TABLE::CreateMap(size_t Element, const STRING& FileName)
{
  if (Element >= 0 && Element < MaxElements)
    {
      return Table[Element].CreateMap(FileName);
    }
  return false;
}

bool MMAP_TABLE::Ok(size_t Element) const
{
  if (Element >= 0 && Element < MaxElements)
    {
      return Table[Element].Ok();
    }
  return false;
}

PUCHR MMAP_TABLE::Ptr(size_t Element) const
{
  if (Element >= 0 && Element < MaxElements)
    {
      return Table[Element].Ptr();
    }
  return NULL;
}

size_t MMAP_TABLE::Size(size_t Element) const
{
  if (Element >= 0 && Element < MaxElements)
    {
      return Table[Element].Size();
    }
  return 0;
}

bool MMAP_TABLE::Advise(size_t Element, int flag, size_t from, size_t to)
{
  if (Element >= 0 && Element < MaxElements)
    {
      return Table[Element].Advise(flag, from, to);
    }
  return false;
}


MMAP_TABLE::~MMAP_TABLE()
{
  delete[] Table;
}


