#ifndef STRBOX_HXX
#define STRBOX_HXX

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "gdt.h"
#include "defs.hxx"

#ifdef NO_STRDUP
CHR* strdup(CHR *S);
#endif

INT  strbox_StringInSet(CHR *s, CHR **stopList);
void strbox_Upper(CHR *s);
void strbox_TheTime(CHR *buf);
INT  strbox_itemCountd(CHR *S, CHR Delim);
CHR *strbox_itemd(CHR *S, INT X, CHR *Item, CHR Delim);
INT  strbox_itemCount(CHR *S);
CHR *strbox_item(CHR *S, INT X, CHR *Item);
void strbox_Cleanup(CHR *s);
INT  strbox_SGetS(CHR *Dest, CHR *Src);
void strbox_EscapeSpaces(CHR *S);

#endif
