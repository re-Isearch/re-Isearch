/*@@@
File:		gdt-sys.h
Version:	(system-specific configuration)
Description:	System-specific data type definitions
@@@*/

#ifndef GDT_SYS_H
#define GDT_SYS_H

#include <unistd.h>
#include <memory.h>
#include "platform.h"

#ifdef NO_ALLOCA
# if (NO_ALLOCA == 0)
#  undef NO_ALLOCA
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if (SIZEOF_INT == 2)
	typedef int INT2;
	typedef unsigned int UINT2;
#else
#if (SIZEOF_SHORT_INT == 2)
	typedef short int INT2;
	typedef unsigned short int UINT2;
#endif
#endif

#if (SIZEOF_INT == 4)
	typedef int INT4;
	typedef unsigned int UINT4;
#else
#if (SIZEOF_LONG_INT == 4)
	typedef long int INT4;
	typedef unsigned long int UINT4;
#endif
#endif

/* INT8 */
#ifndef _MSC_VER
#if (SIZEOF_INT == 8)
	typedef int INT8;
	typedef unsigned int UINT8;
#define INT8_FORMAT ""
#define CONST64(c)  c

#elif (SIZEOF_LONG_INT == 8)
	typedef long int INT8;
	typedef unsigned long int UINT8;
#define INT8_FORMAT "l"
#define CONST64(c)  c##L

#elif (SIZEOF_LONG_LONG_INT == 8)
	typedef long long int INT8;
	typedef unsigned long long int UINT8;
#define INT8_FORMAT "ll"
#define CONST64(c)  c##LL

#endif

#else /* Windows */

#if defined(_WIN32)
typedef unsigned __int64 UINT8;
typedef __int64          INT8;
#define INT8_FORMAT "I64"
#define CONST64(c)  c
#else
typedef unsigned long long UINT8;
typedef signed   long long INT8;
#define INT8_FORMAT "ll"
#define CONST64(c)  c##LL
#endif

#endif

typedef INT2*  PINT2;
typedef UINT2* PUINT2;

typedef INT4*  PINT4;
typedef UINT4* PUINT4;

typedef INT8*  PINT8;
typedef UINT8* PUINT8;


#define UINT8_low_part(_x)	((UINT4)(_x))
#define UINT8_high_part(_x)	((UINT4)((UINT8)(_x)>>32))
#define INT8_low_part(_x)	((INT4)(_x))
#define INT8_high_part(_x)	((INT4)((INT8)(_x)>>32))
#define cons_UINT8(hi, lo)	((((UINT8)(hi)) << 32) | (UINT4)(lo))
#define cons_INT8(hi, lo)	((((INT8)(hi)) << 32) | (UINT4)(lo))
 
#define MAX_UINT8  UINT8(-1)

#ifdef __cplusplus
#ifdef _NO_BOOL_TYPE
typedef bool GDT_BOOLEAN;
#else
typedef bool GDT_BOOLEAN;
#endif
#else /* C does not have bool */
typedef int  GDT_BOOLEAN;
#endif

const GDT_BOOLEAN GDT_FALSE = (1==0); 
const GDT_BOOLEAN GDT_TRUE = (1==1);

#ifndef HOST_MACHINE_64
# error "HOST MACHINE NOT DEFINED??"
#endif

#if HOST_MACHINE_64
# ifndef O_BUILD_IB32
#  define O_BUILD_IB64 1
# endif
#endif  

#ifdef O_BUILD_IB64 /* 64 Bit Edition? */
# define MAX_GPTYPE 0xffffffffffff0000ULL
  typedef UINT8   GPTYPE;
  typedef GPTYPE* PGPTYPE;
# define __USE_FILE_OFFSET64
#else
# define MAX_GPTYPE 0xffffff00U
  typedef UINT4   GPTYPE;
  typedef GPTYPE* PGPTYPE;
#endif

#ifdef __cplusplus
}
#endif

#endif /* GDT_SYS_H */

