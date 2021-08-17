/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
#ifndef _BYTES_HXX
#define _BYTES_HXX 1

#include "platform.h"
#include <iostream>

#ifdef USE_BSWAP
# include "bswap.h"
#endif

#if IS_BIG_ENDIAN == IS_LITTLE_ENDIAN
# error "Configuration is wrong"
#endif

#if IS_LITTLE_ENDIAN
# undef  IS_BIG_ENDIAN
# define IS_BIG_ENDIAN 0
#endif

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

#ifndef _WIN32
#include <netinet/in.h>
#endif

extern void errno_message(const char *);

inline UINT8 UINT8of(const void *ptr, int y = 0)
{
#if IS_BIG_ENDIAN
  register UINT8 Gpp;
  memcpy(&Gpp, (const BYTE *)ptr+y, 8);
  return Gpp;
#else
  const BYTE *x = (const BYTE *)ptr;
  return (((UINT8)x[y])    << 56) + (((UINT8)x[y+1]) << 48) +
         (((UINT8)x[y+2])  << 40) + (((UINT8)x[y+3]) << 32) +
         (((UINT8)x[y+4]) << 24)  + (((UINT8)x[y+5]) << 16) +
         (((UINT8)x[y+6])  << 8)  + x[y+7];
#endif
}


#if IS_BIG_ENDIAN
# define htonll(x)    (x)
# define ntohll(x)    (x)
#else
# ifdef USE_BSWAP
#  define htonll(x)    __bswap_64 (x)
#  define ntohll(x)    __bswap_64 (x)
# else
#  define htonll(x)  ((((UINT64)htonl(x)) << 32) + htonl(x >> 32))
#  define ntohll(x)  ((((UINT64)ntohl(x)) << 32) + ntohl(x >> 32)) 
# endif
#endif


inline UINT2 getINT2(PFILE fp)
{
  int      r = GETC(fp);
  register UINT2 x = 0;
  if (r != EOF)
    {
      x = (UINT2)r;
      if ((r = GETC(fp)) != EOF)
	x = (x << 8) | (UINT2)r;
    }
  return x;
}
inline SHORT getSHORT(PFILE fp) { return getINT2(fp); }


inline SHORT getSHORT(const void *buffer, size_t x)
{
  return (SHORT)(
        (((UINT2)((const char *)buffer)[x]) << 8)  |
        (UINT2)((const char *)buffer)[x+1] );
}

inline UINT4 getINT4 (PFILE fp)
{
  register UINT4 x;
#if IS_BIG_ENDIAN
  fread(&x, 4, 1, fp);
#else
  x  = ((UINT4)GETC(fp)) << 24;
  x |= ((UINT4)GETC(fp)) << 16;
  x |= ((UINT4)GETC(fp)) << 8; 
  x |=        GETC(fp);
#endif
  return x;
}


inline UINT8 getINT8 (PFILE fp)
{
  register UINT8 x;
#if IS_BIG_ENDIAN
  fread(&x, 8, 1, fp);
#else
  x  = ((UINT8)GETC(fp)) << 56;
  x |= ((UINT8)GETC(fp)) << 48;
  x |= ((UINT8)GETC(fp)) << 40;
  x |= ((UINT8)GETC(fp)) << 32;
  x |= ((UINT8)GETC(fp)) << 24;
  x |= ((UINT8)GETC(fp)) << 16;
  x |= ((UINT8)GETC(fp)) << 8;
  x |=         GETC(fp);
#endif
  return x;
}


inline UINT4 getINT4(const void *buffer, size_t x)
{
#if IS_BIG_ENDIAN
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


inline UINT2 putINT2 (const INT2 c, PFILE fp)
{
#if IS_BIG_ENDIAN
  return fwrite((void *)&c, sizeof(INT2), 1, fp) ? c : EOF;
#else
  register UINT2 x;
  x  = (UINT2)PUTC((int)((c >> 8) & 0xff), fp) << 8;
  x |= (UINT2)PUTC((int)(c        & 0xff), fp);
//if (x != c) cerr << "ERROR!!!!!! " << x << " != " << c << endl;
  return x;
#endif
}
inline SHORT putSHORT (const SHORT c, PFILE fp) { return (SHORT)putINT2(c, fp); }


inline UINT4 putINT4 (const INT4 c, PFILE fp)
{
#if IS_BIG_ENDIAN
  return fwrite((void *)&c, sizeof(INT4), 1, fp) ? c : EOF;
#else
  register UINT4 x;
  x  = (UINT4)PUTC((int)((c>>24) & 0xff), fp) << 24;
  x |= (UINT4)PUTC((int)((c>>16) & 0xff), fp) << 16;
  x |= (UINT4)PUTC((int)((c>> 8) & 0xff), fp) << 8;
  x |= (UINT4)PUTC((int)((c    ) & 0xff), fp);
  return x;
#endif
}

inline UINT8 putINT8 (const INT8 c, PFILE fp)
{
#if IS_BIG_ENDIAN
  return fwrite(&c, sizeof(INT8), 1, fp) ? c : EOF;
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

inline int Write(const CHR c, PFILE Fp)
{
  if (PUTC(c & 0xFF, Fp) == (UCHR)c)
    return 1;
  if (Fp) errno_message("Write ERR(CHR)");
  return 0;
}

inline int Write(const UCHR c, PFILE Fp)
{
  if ((UCHR)PUTC(c & 0xFF, Fp) == c)
    return 1;
  if (Fp) errno_message("Write ERR(UCHR)");
  return 0;
}

inline int Write(const INT2 hostshort, PFILE Fp)
{
  if (putINT2 (hostshort, Fp) == (UINT2)hostshort)
    return 2;
  if (Fp) errno_message("Write ERR(INT2)");
  return 0;

}

inline int Write(const INT4 hostlong, PFILE Fp)
{
  if (putINT4 (hostlong, Fp) == (UINT4)hostlong)
    return 4;
  if (Fp) errno_message("Write ERR(INT4)");
  return 0;
}

inline int Write(const INT8 hostlonglong, PFILE Fp)
{
  if (putINT8 ((INT8)hostlonglong, Fp) == (UINT8)hostlonglong)
    return 8;
  if (Fp) errno_message("Write ERR(INT8)");
  return 0;
}


inline int Write(const UINT2 hostshort, PFILE Fp)
{
  if (putINT2 ((INT2)hostshort, Fp) == hostshort)
    return 2;
  if (Fp) errno_message("Write ERR(UINT2)");
  return 0;
}

inline int Write(const UINT4 hostlong, PFILE Fp)
{
  if (putINT4 ((INT4)hostlong, Fp) == hostlong)
    return 4;
  if (Fp) errno_message("Write ERR(UINT4)");
  return 0;
}

inline int Write(const UINT8 hostlonglong, PFILE Fp)
{
  if (putINT8 ((INT8)hostlonglong, Fp) == hostlonglong)
    return 8;
  if (Fp) errno_message("Write ERR(UINT8)");
  return 0;
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
  *hostshort = (INT2)getINT2 (Fp);
}
 
inline void Read(PINT4 hostlong, PFILE Fp)
{
  *hostlong = (INT4)getINT4 (Fp); 
}


inline void Read(PINT8 hostlonglong, PFILE Fp)
{
  *hostlonglong = (INT8)getINT8 (Fp);
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

inline int Write(const float c, PFILE Fp)
{
  const INT4 x = (INT4)c;
  int   r0, r1;
  r0 = ::Write (x, Fp);
  r1 = ::Write ((INT4)(((float)c - (float)x)*FLOAT_FACTOR), Fp);
  return r0*r1;
}

inline int Read(float *c, PFILE Fp)
{
  INT4 num, div;
  int  r0 =1, r1=1;
  ::Read(&num, Fp);
  ::Read(&div, Fp);
  if (c) *c = (float)((float)num + ((float)div)/FLOAT_FACTOR);
  return r0*r1;
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

inline int Write(const double c, FILE *Fp)
{
  return fwrite(&c, sizeof(double), 1, Fp);
}
 
inline GDT_BOOLEAN Read(double *c, FILE *Fp)
{
  return fread(c, sizeof(double), 1, Fp) != 0;
}


inline int Write(const long double c, FILE *Fp)
{
  return fwrite(&c, sizeof(long double), 1, Fp);
}

inline GDT_BOOLEAN Read(long double *c, FILE *Fp)
{
  return fread(c, sizeof(long double), 1, Fp) != 0;
}



#endif
