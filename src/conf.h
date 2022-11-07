/*@@@
File:		conf.h (generated from conf.h.in by configure)
Version:	1.01
Description:	Variable sizes
@@@*/

#ifndef CONF_H
#define CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "conf.h.inc"

#ifndef LINUX 
# define LINUX 1
#endif
#define IS_LITTLE_ENDIAN 1
#undef BSD 

#define HAVE_LOCALE 1
#define UNIX 1
#define NO_ALLOCA 0
#define NO_ALLOCA_H 1
#define HAVE_SAFESPRINTF  1

#ifdef __cplusplus
}
#endif

#endif
