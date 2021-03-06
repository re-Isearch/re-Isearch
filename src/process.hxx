/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#ifndef _IB_PROCESS_HXX
# define _IB_PROCESS_HXX 1

#include <stdio.h>

// Process stuff
int   _IB_system(const char *command, int Async=1);
int   _IB_system(const char * const *argv, int Async=1);

FILE *_IB_popen(const char *command, const char *type = "r");
FILE *_IB_popen(const char * const argv[], const char *type = "r");
int   _IB_pclose(FILE *fp, int Zombie=0);

#endif
