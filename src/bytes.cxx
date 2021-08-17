/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#ifndef _BYTES_HXX
#define _BYTES_HXX 1

#include "defs.hxx"
#include "strlist.hxx"
#include "common.hxx"

#include "bswap.h"

///////////////////////////////////////////////////////////////////
// Network byte order Read/Write
///////////////////////////////////////////////////////////////////
#ifdef putc_unlocked
# define PUTC putc_unlocked
#else
# define PUTC putc
#endif
#ifdef getc_unlocked 
# define GETC getc_unlocked
#else
# define GETC getc
#endif

#include <netinet/in.h>

#ifdef _BIG_ENDIAN
# define htonll(x)    (x)
# define ntohll(x)    (x)
#else
# if 1
#  define htonll(x)  ((((UINT64)htonl(x)) << 32) + htonl(x >> 32))
#  define ntohll(x)  ((((UINT64)ntohl(x)) << 32) + ntohl(x >> 32)) 
# else
# define htonll(x)    __bswap_64 (x)
# define ntohll(x)    __bswap_64 (x)
# endif
#endif


UINT2 getINT2(PFILE fp)
{
  int      r = GETC(fp);
  register UINT2 x = 0;

  if (r != -1)
    {
      x = (UINT2)r;
      if ((r = GETC(fp)) != -1)
	x = (x << 8) | (UINT2)r;
    }
  return x;
}
SHORT getSHORT(PFILE fp) { return getINT2(fp); }


SHORT getSHORT(const void *buffer, size_t x)
{
  return (SHORT)(
        (((UINT2)((const char *)buffer)[x]) << 8)  |
        (UINT2)((const char *)buffer)[x+1] );
}

UINT4 getINT4 (PFILE fp)
{
  register UINT4 x;
#ifdef _BIG_ENDIAN
  fread(&x, 4, 1, fp);
#else
  x  = (UINT4)GETC(fp) << 24;
  x |= (UINT4)GETC(fp) << 16;
  x |= (UINT4)GETC(fp) << 8; 
  x |=        GETC(fp);
#endif
  return x;
}


UINT8 getINT8 (PFILE fp)
{
  register UINT8 x;
#ifdef _BIG_ENDIAN
  fread(&x, 8, 1, fp);
#else

#if 0
  x = getINT4(fp) << 32;
  x |= getINT4(fp);
#else
  x  = (UINT8)GETC(fp) << 56;
  x |= (UINT8)GETC(fp) << 48;
  x |= (UINT8)GETC(fp) << 40;
  x |= (UINT8)GETC(fp) << 32;
  x |= (UINT8)GETC(fp) << 24;
  x |= (UINT8)GETC(fp) << 16;
  x |= (UINT8)GETC(fp) << 8;
  x |=        GETC(fp);
#endif

#endif
  return x;
}


UINT4 getINT4(const void *buffer, size_t x)
{
#ifdef _BIG_ENDIAN
  UINT4 val;
  memcpy(&val, ((const char *)buffer)+x, 4);
  return val;
#else
  return (
	(((UINT4)((const char *)buffer)[x])   << 24) |
	(((UINT4)((const char *)buffer)[x+1]) << 16) |
	(((UINT4)((const char *)buffer)[x+2]) << 8)  |
	(UINT4)((const char *)buffer)[x+3] );
#endif
}


INT2 putINT2 (const INT2 c, PFILE fp)
{
#ifdef _BIG_ENDIAN
  return fwrite(&c, 2, 1, fp) ? c : -1;
#else
  register INT2 x;
  x  = (INT2)PUTC((int)((c >> 8) & 0xff), fp) << 8;
  x |=       PUTC((int)(c        & 0xff), fp);
  return x;
#endif
}

SHORT putSHORT (const SHORT c, PFILE fp) { return putINT2(c, fp); }


UINT4 putINT4 (const INT4 c, PFILE fp)
{
#ifdef _BIG_ENDIAN
  return fwrite(&c, 4, 1, fp) ? c : -1;
#else
  register UINT4 x;
  x  = (UINT4)PUTC((int)((c>>24) & 0xff), fp) << 24;
  x |= (UINT4)PUTC((int)((c>>16) & 0xff), fp) << 16;
  x |= (UINT4)PUTC((int)((c>> 8) & 0xff), fp) << 8;
  x |= (UINT4)PUTC((int)((c    ) & 0xff), fp);
  return x;
#endif
}

UINT8 putINT8 (const INT8 c, PFILE fp)
{
#ifdef _BIG_ENDIAN
  return fwrite(&c, 8, 1, fp) ? c : -1;
#else
  register UINT8 x;

  x  = (UINT8)PUTC((INT8)((c>>56) & 0xff), fp) << 56;
  x |= (UINT8)PUTC((INT8)((c>>48) & 0xff), fp) << 48;
  x |= (UINT8)PUTC((INT8)((c>>40) & 0xff), fp) << 40;
  x |= (UINT8)PUTC((INT8)((c>>32) & 0xff), fp) << 32;
  x |= (UINT8)PUTC((INT8)((c>>24) & 0xff), fp) << 24;
  x |= (UINT8)PUTC((INT8)((c>>16) & 0xff), fp) << 16;
  x |= (UINT8)PUTC((INT8)((c>> 8) & 0xff), fp) << 8;
  x |= (UINT8)PUTC((INT8)((c    ) & 0xff), fp);
  return x;
#endif
}

inline void Write(const CHR c, PFILE Fp)
{
  PUTC(c & 0xFF, Fp);
}

inline void Write(const UCHR c, PFILE Fp)
{
  PUTC(c & 0xFF, Fp);
}

inline void Write(const INT2 hostshort, PFILE Fp)
{
  putINT2 (hostshort, Fp);
}

inline void Write(const INT4 hostlong, PFILE Fp)
{
  putINT4 (hostlong, Fp);
}

inline void Write(const INT8 hostlonglong, PFILE Fp)
{
  putINT8 (hostlonglong, Fp);
}


inline void Write(const UINT2 hostshort, PFILE Fp)
{
  putINT2 (hostshort, Fp);
}

inline void Write(const UINT4 hostlong, PFILE Fp)
{
  putINT4 (hostlong, Fp);
}

inline void Write(const UINT8 hostlonglong, PFILE Fp)
{
  putINT8 (hostlonglong, Fp);
}


inline GDT_BOOLEAN Read(PCHR c, PFILE Fp)
{
  int ch = GETC(Fp);
  if (c) *c = (CHR)ch;
  return ch != EOF; 
}

inline GDT_BOOLEAN Read(PUCHR c, PFILE Fp)
{
  int ch = GETC(Fp);
  if (c) *c = (UCHR)ch; 
  return ch != EOF;
}


inline void Read(PINT2 hostshort, PFILE Fp)
{
#if 0
  INT2  x;
  if (fread(&x, sizeof(INT2), 1, Fp) != 1) 
    return GDT_FALSE; 
  *hostshort = htons(x);
  return GDT_TRUE;
#else
  *hostshort = (INT2)getINT2 (Fp);
#endif
}
 
inline void Read(PINT4 hostlong, PFILE Fp)
{
#if 0
  INT4  x;
  if (fread(&x, sizeof(INT4), 1, Fp) != 1)
    return GDT_FALSE;
  *hostlong = htonl(x);
  return GDT_TRUE;
#else
  *hostlong = (INT4)getINT4 (Fp); 
#endif
}


inline void Read(PINT8 hostlonglong, PFILE Fp)
{
#if 0
  INT8  x;
  if (fread(&x, sizeof(INT8), 1, Fp) != 1) 
    return GDT_FALSE; 
  *hostlonglong = htonll(x);
  return GDT_TRUE;
#else
  *hostlonglong = (INT8)getINT8 (Fp);
#endif
}
 
inline void Read(PUINT2 hostshort, PFILE Fp)
{
  Read((PINT2)hostshort, Fp);
}
 
inline void  Read(PUINT4 hostlong, PFILE Fp)
{
  Read((PINT4)hostlong, Fp);
}

inline void Read(PUINT8 hostlonglong, PFILE Fp)
{
  Read((PINT8)hostlonglong, Fp);
}


#define FLOAT_FACTOR 50000.0

inline void Write(const float c, PFILE Fp)
{
  const INT4 x = (INT4)c;
  ::Write (x, Fp);
  ::Write ((INT4)(((double)c - (double)x)*FLOAT_FACTOR), Fp);
}

inline void Read(float *c, PFILE Fp)
{
  INT4 num, div;
  ::Read(&num, Fp);
  ::Read(&div, Fp);
  if (c) *c = (float)((double)num + ((double)div)/FLOAT_FACTOR);
}




// Doubles -- not yet portable
// Use XDR-style double mapping?
//
//
// IEEE (from a web-page with the standard on www.ee.umd.edu): 
//
//    - -------    ------- ------- -------
//   |s| exp   |  |       |       |       |
//    - -------    ------- ------- -------
//    0 1     7    8                    31
//
//   where:
//
//      s  is the sign bit.
//      1-7   is the exponent represented as a power of 2 in
//         "excess 127" notation (to give positive and negative
//         exponential values).
//      8-31  is the base-2 fraction left-shifted to where the
//         first binary "1" is on the left of an imaginary
//         binary-point (with the exponent raised appropriately
//         during the shift) giving a precision of 24 bits
//         stored in 23 bits.
//
//   for a double (64 bits), IEEE uses 11 bits for the exponent and 52 
//   bits for the mantissa.

inline void Write(const double c, FILE *Fp)
{
  fwrite(&c, sizeof(double), 1, Fp);
}
 
inline GDT_BOOLEAN Read(double *c, FILE *Fp)
{
  return fread(c, sizeof(double), 1, Fp) != 0;
}


inline void Write(const long double c, FILE *Fp)
{
  fwrite(&c, sizeof(long double), 1, Fp);
}

inline GDT_BOOLEAN Read(long double *c, FILE *Fp)
{
  return fread(c, sizeof(long double), 1, Fp) != 0;
}



#endif
