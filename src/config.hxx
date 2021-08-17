/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
#ifndef _CONFIG_HXX
# define _CONFIG_HXX 1

#include "platform.h"
// Use configuration options
//
//
//
#define USE_MDTHASHTABLE 1

/*The RLDCACHE class is used to maintain directory information on
  files retrieved by URL and stored in temporary files.  */
#define NO_RLDCACHE 1 /* Do we have a RLDCACHE in IB or not?  */

#ifdef DOTS_IN_WORDS
# ifdef STANDALONE
#  define  DOTS_IN_WORDS 0
# else
#  define DOTS_IN_WORDS 1
# endif
#endif

#define USE_STD_MAP 0


#define FIND_CONCAT_WORDS 1 /* Find flowers if flow-ers fails */

#endif
