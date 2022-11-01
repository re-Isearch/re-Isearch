/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#include <stdlib.h>
#include <memory.h>
#include "common.hxx"
#include "buffer.hxx"

#ifndef PAGESIZE
#ifdef _WIN32
#  define PAGESIZE winGetPageSize()
extern UINT winGetPageSize();
#else
#  define PAGESIZE 4096
#endif
#endif

BUFFER::BUFFER()
{
  Buffer = NULL;
  Buffer_size = 0;
  PageSize = PAGESIZE;
}

BUFFER::BUFFER(size_t want, size_t size)
{
  Buffer = NULL;
  Buffer_size = 0;
  PageSize = PAGESIZE;
  Want(want, size);
}


void BUFFER::SetPageSize(size_t size)
{

  PageSize = (1 + (size/512))*512;

}

bool BUFFER::Ok() const
{
  if (Buffer)
    return Buffer[Buffer_size] == (unsigned  char)(((Buffer_size+1)/1024)&0xFF);
  return Buffer_size == 0;
}

bool BUFFER::Free(const char *Class, const char *What)
{
  bool res = true;
  if (Buffer)
    {
      if (!Ok())
	{
          message_log (LOG_PANIC, "Buffer %s::%s (%u kbytes) got corrupted (%x != %x)!",
		Class ? Class : "", What && *What ? What : "<Unknown>",
		(unsigned)Buffer_size, Buffer[Buffer_size], (unsigned char)(((Buffer_size+1)/1024)&0xFF) );
	  res = false;
	}
      delete[] Buffer;
    }
  Buffer = NULL;
  Buffer_size = 0;
  return res;
}

bool BUFFER::Free(const char *What)
{
  return Free(NULL, What);
}

bool BUFFER::Free()
{
  return Free(NULL, NULL);
}


#define Alloc(want,size) \
	((1 + ((want*size+size)/PageSize))*PageSize - (size - ((size/PageSize)*PageSize)))  /* Fit on pages */

#define SetMagic() if (Buffer) Buffer[Buffer_size] = (unsigned char)(((Buffer_size+1)/1024)&0xFF)


void *BUFFER::Expand (size_t want, size_t size)
{
  if (Buffer == NULL || want >= (Buffer_size/size))
    {
      unsigned char *oldBuffer = Buffer;
      size_t oldsize  = Buffer_size;
      Buffer_size = Alloc(want,size)+size; 
      try {
        Buffer = new unsigned char [Buffer_size+1];
      } catch (...) {
	message_log (LOG_ERRNO|LOG_PANIC, "Could not expand buffer from %u to %u bytes",
	   (unsigned)oldsize, (unsigned)Buffer_size);
        Buffer = NULL;
        Buffer_size = 0;
        oldsize    = 0;
      }
      if (oldBuffer)
	{
	  if (oldsize)
	    memcpy(Buffer, oldBuffer, oldsize);
	  delete[] oldBuffer;
	}
      SetMagic();
    }
  return (void *)Buffer;
}

void *BUFFER::Want (size_t want, size_t size)
{
  if (Buffer == NULL || want >= (Buffer_size/size))
    {
      if (Buffer) delete[] Buffer;
      Buffer_size = Alloc(want,size)+size; 
      try {
	Buffer = new unsigned char [Buffer_size+1];
      } catch (...) {
	message_log (LOG_ERRNO|LOG_PANIC, "Could not create a %u byte buffer", (unsigned)Buffer_size);
	Buffer_size = 0;
      }
      if (Buffer_size)
	memset(Buffer, 0, Buffer_size*sizeof(unsigned char));
      SetMagic();
    }
  if (Buffer_size > size)
   memset(Buffer, 0, size*sizeof(unsigned char));
  return (void *)Buffer;
}

BUFFER::~BUFFER()
{
  if (Buffer) Free();
}
