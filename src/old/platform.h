/*
#define WINDOWS 1
*/

#if defined(WIN32) || defined(WINDOWS) 
# ifndef _WIN32
#  define _WIN32
# endif
#endif

// #define  O_BUILD_IB32 1

#ifdef _WIN32
# include "conf_win32.h"
#else
# include "conf.h"
#endif

#ifndef IS_BIG_ENDIAN
# ifdef IS_LITTLE_ENDIAN
#   define IS_BIG_ENDIAN !(IS_LITTLE_ENDIAN)
# endif
#endif
#ifndef IS_LITTLE_ENDIAN
# ifdef IS_BIG_ENDIAN
#   define IS_LITTLE_ENDIAN !(IS_BIG_ENDIAN)
# endif
#endif

/*  Set some platform features */
#ifndef PLATFORM_INCLUDED
#if (SIZEOF_LONG_INT == SIZEOF_LONG_LONG_INT)
# define HOST_MACHINE_64 1 /* This is a 64-bit platform so its all native */
#else
# define HOST_MACHINE_64 0 /* Not a 64-bit platform */
#endif
#define PLATFORM_INCLUDED 1
#endif
