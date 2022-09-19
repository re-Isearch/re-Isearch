/*  WIN32 */
/*@@@
File:		conf.h (generated from conf.h.in by configure)
Version:	1.01
Description:	Variable sizes
@@@*/

#ifndef CONF_H
#define CONF_H

// It is necessary to explicitely set this here instead of accepting
// the standard 0x0400.
//Support Win98+ and WinNT+ - NO Win95(!)
#define WINVER 0x0400
#define _WIN32_WINDOWS 0x0410

//Include windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


#ifdef __cplusplus
extern "C" {
#endif

#undef HAVE_LOCALE
/* NOTE: It would be good to have LOCALE! */ 
/* define 1 if we've installed the locale stuff */
/* #define HAVE_LOCALE 1 */
/* */

/*
#define HAVE_STRPTIME 1
*/

#ifdef __GNUC__
# if (__GNUC__ >= 4)
#  define HOST_COMPILER "GCC/G++ Version 4.2.x"
# else
#  define HOST_COMPILER "GCC/G++ Version 3.4.x"
# endif
#endif

#define HOST_PLATFORM "WIN32"

#define SIZEOF_SHORT_INT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG_INT 4
#define SIZEOF_LONG_LONG_INT 8

#define rlim_t unsigned long
#define caddr_t char *

#define IS_LITTLE_ENDIAN 1

#undef LINUX
#undef UNIX

#define NO_ALLOCA 1
#define NO_ALLOCA_H 1

#if 0
#undef clock()
#define clock() (clock_t)GetTickCount()
#endif


#ifdef __cplusplus
}
#endif

#endif
