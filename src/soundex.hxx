/************************************************************************
************************************************************************/

/*@@@
File:		soundex.hxx
Version:	1.00
Description:	Soundex support for STRING class
Author: 	Edward C. Zimmermann, edz@nonmonotonic.com
@@@*/

#ifndef SOUNDEX_HXX
#define SOUNDEX_HXX

#include "gdt.h"
#include "string.hxx"

//#define HASH_LEN 4            /* Typicaly 4 or 6 */
#define HASH_LEN 6

UINT8 SoundexEncodeName(const STRING& Name, GDT_BOOLEAN FirstThenLast = GDT_TRUE);
UINT8 SoundexEncode(const STRING& EnglishWord, PSTRING StringBuffer = NULL, const int len=HASH_LEN);
// Encodes "EnglishWord" into soundex and puts soundex code-string into "StringBuffer".

GDT_BOOLEAN SoundexMatch (const STRING& Word, const STRING& Hash);
GDT_BOOLEAN SoundexMatch (const CHR *Word, const STRING& Hash);
// Does the word match the Hash?

#undef HASH_LEN

#endif /* SOUNDEX_HXX */
