/*@@@
File:		gdt.h
Version:	1.01
Description:	Generic Data Type definitions
Author:		Nassib Nassar, nrn@cnidr.org
         	Edward C. Zimmermann@bsn.com
@@@*/

#ifndef GDT_H
#define GDT_H

#include "platform.h"

#ifdef _WIN32
//# include <winnt.h>
#endif

#include <stdlib.h>
#include <stdio.h>

/* all platforms */
typedef unsigned char BYTE;
typedef BYTE* PBYTE;
typedef int INT;
typedef unsigned int UINT;
typedef UINT* PUINT;
typedef float FLOAT;
typedef double DOUBLE;
typedef short SHORT;
typedef SHORT* PSHORT;

typedef long double NUMBER; 

typedef FLOAT* PFLOAT;
typedef char CHR;
typedef CHR* PCHR;
typedef CHR** PPCHR;
typedef unsigned char UCHR;
typedef UCHR* PUCHR;
typedef UCHR** PPUCHR;
typedef FILE* PFILE;

#include "gdt-sys.h"
#include "bytes.hxx"

#endif /* GDT_H */
