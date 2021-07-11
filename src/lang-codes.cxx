#pragma ident  "@(#)lang-codes.cxx  1.61 02/05/01 00:35:30 BSN"

#ifndef HAVE_LOCALE
#define HAVE_LOCALE 1
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_LOCALE
#include <locale.h>
#endif
#include "defs.hxx"
#include "lang-codes.hxx"

#define OCTET char

extern "C" {
  extern const OCTET _ib_ctype_8859_1[256];
  extern const OCTET _ib_ctype_8859_2[256];
  extern const OCTET _ib_ctype_8859_3[256];
  extern const OCTET _ib_ctype_8859_5[256];
  extern const OCTET _ib_ctype_8859_7[256];
  extern const OCTET _ib_ctype_ASCII[256];
}

static unsigned char* _utf_StrToLower(unsigned char* pString, bool clean=false);
static unsigned char* _utf_StrToUpper(unsigned char* pString, bool clean=false);


// Threaded safe..
// Explicitly JUST for 8-bit character sets
static inline int compareFunc(const void *s1, const void *s2, const OCTET *tab,
	size_t MaxLength = StringCompLength)
{
  int diff;
  const UCHR *p1 = (const UCHR *)s1;
  const UCHR *p2 = (const UCHR *)s2;
  for (size_t x = 0; ((diff = ( tab[*p1] -  tab[*p2])) == 0 && tab[*p1]); )
    {
      if (x++ > MaxLength) break;
      p1++, p2++;
    }
  return diff;
}

/// TODO: 
//
// Need to "fix" so that they can compare two words with '.' in them.
// as in nonmonotonic.net
//

// Threaded safe..
static int SIScompareFunc(const void *node1, const void *node2, const OCTET *tab)
{
  // node1[0] contains the match length
  // node1[1] contains the string length
  // &node[2] contains the lower case term..
  const size_t         stringLen = *((const unsigned char *)node1+1);
  const unsigned char *p1        = ((const unsigned char *)node1)+2;
  // node2[0] contains the length
  // &node2[1] contains the term
//const size_t         termLen   = *((const unsigned char *)node2);
  const unsigned char *p2        = ((const unsigned char *)node2)+1;

  int diff = memcmp(p1, p2, stringLen);
  if (diff == 0) {
    const size_t         matchLen  = *((const unsigned char *)node1);
    if (matchLen && IsTermChr(p2+matchLen))
      diff = -p2[matchLen];
  }
  return diff;
}


/***** UTF ***/
// Covert a UTF string to lowercase
// clean zaps non term characters
static unsigned char *_utf_StrToLower(unsigned char *pString, bool clean)
{
    if (pString && *pString) {
        unsigned char *p = pString;
        unsigned char *pExtChar = 0;
        while (*p) {
            if ((*p >= 0x41) && (*p <= 0x5a)) // US ASCII
                (*p) += 0x20;
            else if (*p > 0xc0) {
                pExtChar = p;
                p++;
                switch (*pExtChar) {
                case 0xc3: // Latin 1
                    if ((*p >= 0x80)
                        && (*p <= 0x9e)
                        && (*p != 0x97))
                        (*p) += 0x20; // US ASCII shift
                    break;
                case 0xc4: // Latin Exteneded
                    if ((*p >= 0x80)
                        && (*p <= 0xb7)
                        && (!(*p % 2))) // Even
                        (*p)++; // Next char is lwr
                    else if ((*p >= 0xb9)
                        && (*p <= 0xbe)
                        && (*p % 2)) // Odd
                        (*p)++; // Next char is lwr
                    else if (*p == 0xbf) {
                        *pExtChar = 0xc5;
                        (*p) = 0x80;
                    }
                    break;
                case 0xc5: // Latin Exteneded
                    if ((*p >= 0x80)
                        && (*p <= 0x88)
                        && (*p % 2)) // Odd
                        (*p)++; // Next char is lwr
                    else if ((*p >= 0x8a)
                        && (*p <= 0xb7)
                        && (!(*p % 2))) // Even
                        (*p)++; // Next char is lwr
                    else if ((*p >= 0xb9)
                        && (*p <= 0xbe)
                        && (*p % 2)) // Odd
                        (*p)++; // Next char is lwr
                    break;
                case 0xc6: // Latin Exteneded
                    switch (*p) {
                    case 0x82:
                    case 0x84:
                    case 0x87:
                    case 0x8b:
                    case 0x91:
                    case 0x98:
                    case 0xa0:
                    case 0xa2:
                    case 0xa4:
                    case 0xa7:
                    case 0xac:
                    case 0xaf:
                    case 0xb3:
                    case 0xb5:
                    case 0xb8:
                    case 0xbc:
                        (*p)++; // Next char is lwr
                        break;
                    default:
                        break;
                    }
                    break;
                case 0xc7: // Latin Exteneded
                    if (*p == 0x84)
                        (*p) = 0x86;
                    else if (*p == 0x85)
                        (*p)++; // Next char is lwr
                    else if (*p == 0x87)
                        (*p) = 0x89;
                    else if (*p == 0x88)
                        (*p)++; // Next char is lwr
                    else if (*p == 0x8a)
                        (*p) = 0x8c;
                    else if (*p == 0x8b)
                        (*p)++; // Next char is lwr
                    else if ((*p >= 0x8d)
                        && (*p <= 0x9c)
                        && (*p % 2)) // Odd
                        (*p)++; // Next char is lwr
                    else if ((*p >= 0x9e)
                        && (*p <= 0xaf)
                        && (!(*p % 2))) // Even
                        (*p)++; // Next char is lwr
                    else if (*p == 0xb1)
                        (*p) = 0xb3;
                    else if (*p == 0xb2)
                        (*p)++; // Next char is lwr
                    else if (*p == 0xb4)
                        (*p)++; // Next char is lwr
                    else if (*p == 0xb8)
                        (*p)++; // Next char is lwr
                    else if (*p == 0xba)
                        (*p)++; // Next char is lwr
                    else if (*p == 0xbc)
                        (*p)++; // Next char is lwr
                    else if (*p == 0xbe)
                        (*p)++; // Next char is lwr
                    break;
                case 0xc8: // Latin Exteneded
                    if ((*p >= 0x80)
                        && (*p <= 0x9f)
                        && (!(*p % 2))) // Even
                        (*p)++; // Next char is lwr
                    else if ((*p >= 0xa2)
                        && (*p <= 0xb3)
                        && (!(*p % 2))) // Even
                        (*p)++; // Next char is lwr
                    else if (*p == 0xbb)
                        (*p)++; // Next char is lwr
                    break;
                case 0xcd: // Greek & Coptic
                    switch (*p) {
                    case 0xb0:
                    case 0xb2:
                    case 0xb6:
                        (*p)++; // Next char is lwr
                        break;
                    default:
                        if (*p == 0xbf) {
                            *pExtChar = 0xcf;
                            (*p) = 0xb3;
                        }
                        break;
                    }
                    break;
                case 0xce: // Greek & Coptic
                    if (*p == 0x86)
                        (*p) = 0xac;
                    else if (*p == 0x88)
                        (*p) = 0xad;
                    else if (*p == 0x89)
                        (*p) = 0xae;
                    else if (*p == 0x8a)
                        (*p) = 0xaf;
                    else if (*p == 0x8c) {
                        *pExtChar = 0xcf;
                        (*p) = 0x8c;
                    }
                    else if (*p == 0x8e) {
                        *pExtChar = 0xcf;
                        (*p) = 0x8d;
                    }
                    else if (*p == 0x8f) {
                        *pExtChar = 0xcf;
                        (*p) = 0x8e;
                    }
                    else if ((*p >= 0x91)
                        && (*p <= 0x9f))
                        (*p) += 0x20; // US ASCII shift
                    else if ((*p >= 0xa0)
                        && (*p <= 0xab)
                        && (*p != 0xa2)) {
                        *pExtChar = 0xcf;
                        (*p) -= 0x20;
                    }
                    break;
                case 0xcf: // Greek & Coptic
                    if (*p == 0x8f)
                        (*p) = 0xb4;
                    else if (*p == 0x91)
                        (*p)++; // Next char is lwr
                    else if ((*p >= 0x98)
                        && (*p <= 0xaf)
                        && (!(*p % 2))) // Even
                        (*p)++; // Next char is lwr
                    else if (*p == 0xb4)
                        (*p) = 0x91;
                    else if (*p == 0xb7)
                        (*p)++; // Next char is lwr
                    else if (*p == 0xb9)
                        (*p) = 0xb2;
                    else if (*p == 0xbb)
                        (*p)++; // Next char is lwr
                    else if (*p == 0xbd) {
                        *pExtChar = 0xcd;
                        (*p) = 0xbb;
                    }
                    else if (*p == 0xbe) {
                        *pExtChar = 0xcd;
                        (*p) = 0xbc;
                    }
                    else if (*p == 0xbf) {
                        *pExtChar = 0xcd;
                        (*p) = 0xbd;
                    }

                    break;
                case 0xd0: // Cyrillic
                    if ((*p >= 0x80)
                        && (*p <= 0x8f)) {
                        *pExtChar = 0xd1;
                        (*p) += 0x10;
                    }
                    else if ((*p >= 0x90)
                        && (*p <= 0x9f))
                        (*p) += 0x20; // US ASCII shift
                    else if ((*p >= 0xa0)
                        && (*p <= 0xaf)) {
                        *pExtChar = 0xd1;
                        (*p) -= 0x20;
                    }
                    break;
                case 0xd1: // Cyrillic supplement
                    if ((*p >= 0xa0)
                        && (*p <= 0xbf)
                        && (!(*p % 2))) // Even
                        (*p)++; // Next char is lwr
                    break;
                case 0xd2: // Cyrillic supplement
                    if (*p == 0x80)
                        (*p)++; // Next char is lwr
                    else if ((*p >= 0x8a)
                        && (*p <= 0xbf)
                        && (!(*p % 2))) // Even
                        (*p)++; // Next char is lwr
                    break;
                case 0xd3: // Cyrillic supplement
                    if ((*p >= 0x81)
                        && (*p <= 0x8e)
                        && (*p % 2)) // Odd
                        (*p)++; // Next char is lwr
                    else if ((*p >= 0x90)
                        && (*p <= 0xbf)
                        && (!(*p % 2))) // Even
                        (*p)++; // Next char is lwr
                    break;
                case 0xd4: // Cyrillic supplement & Armenian
                    if ((*p >= 0x80)
                        && (*p <= 0xaf)
                        && (!(*p % 2))) // Even
                        (*p)++; // Next char is lwr
                    else if ((*p >= 0xb1)
                        && (*p <= 0xbf)) {
                        *pExtChar = 0xd5;
                        (*p) -= 0x10;
                    }
                    break;
                case 0xd5: // Armenian
                    if ((*p >= 0x80)
                        && (*p <= 0x96)
                        && (!(*p % 2))) // Even
                        (*p)++; // Next char is lwr
                    break;
                case 0xe1: // Three byte code
                    pExtChar = p;
                    p++;
                    switch (*pExtChar) {
                    case 0x82: // Georgian
                        if ((*p >= 0xa0)
                            && (*p <= 0xbf)) {
                            *pExtChar = 0x83;
                            (*p) -= 0x10;
                        }
                        break;
                    case 0x83: // Georgian
                        if ((*p >= 0x80)
                            && ((*p <= 0x85)
                                || (*p == 0x87))
                            || (*p == 0x8d))
                            (*p) += 0x30;
                        break;
                    case 0xb8: // Latin extened
                        if ((*p >= 0x80)
                            && (*p <= 0xbf)
                            && (!(*p % 2))) // Even
                            (*p)++; // Next char is lwr
                        break;
                    case 0xb9: // Latin extened
                        if ((*p >= 0x80)
                            && (*p <= 0xbf)
                            && (!(*p % 2))) // Even
                            (*p)++; // Next char is lwr
                        break;
                    case 0xba: // Latin extened
                        if ((*p >= 0x80)
                            && (*p <= 0x94)
                            && (!(*p % 2))) // Even
                            (*p)++; // Next char is lwr
                        else if ((*p >= 0x9e)
                            && (*p <= 0xbf)
                            && (!(*p % 2))) // Even
                            (*p)++; // Next char is lwr
                        break;
                    case 0xbb: // Latin extened
                        if ((*p >= 0x80)
                            && (*p <= 0xbf)
                            && (!(*p % 2))) // Even
                            (*p)++; // Next char is lwr
                        break;
                    case 0xbc: // Greek extened
                        if ((*p >= 0x88)
                            && (*p <= 0x8f))
                            (*p) -= 0x08;
                        else if ((*p >= 0x98)
                            && (*p <= 0x9f))
                            (*p) -= 0x08;
                        else if ((*p >= 0xa8)
                            && (*p <= 0xaf))
                            (*p) -= 0x08;
                        else if ((*p >= 0xb8)
                            && (*p <= 0x8f))
                            (*p) -= 0x08;
                        break;
                    case 0xbd: // Greek extened
                        if ((*p >= 0x88)
                            && (*p <= 0x8d))
                            (*p) -= 0x08;
                        else if ((*p >= 0x98)
                            && (*p <= 0x9f))
                            (*p) -= 0x08;
                        else if ((*p >= 0xa8)
                            && (*p <= 0xaf))
                            (*p) -= 0x08;
                        else if ((*p >= 0xb8)
                            && (*p <= 0x8f))
                            (*p) -= 0x08;
                        break;
                    case 0xbe: // Greek extened
                        if ((*p >= 0x88)
                            && (*p <= 0x8f))
                            (*p) -= 0x08;
                        else if ((*p >= 0x98)
                            && (*p <= 0x9f))
                            (*p) -= 0x08;
                        else if ((*p >= 0xa8)
                            && (*p <= 0xaf))
                            (*p) -= 0x08;
                        else if ((*p >= 0xb8)
                            && (*p <= 0xb9))
                            (*p) -= 0x08;
                        break;
                    case 0xbf: // Greek extened
                        if ((*p >= 0x88)
                            && (*p <= 0x8c))
                            (*p) -= 0x08;
                        else if ((*p >= 0x98)
                            && (*p <= 0x9b))
                            (*p) -= 0x08;
                        else if ((*p >= 0xa8)
                            && (*p <= 0xac))
                            (*p) -= 0x08;
                        break;
                    default:
                        break;
                    }
                    break;
                case 0xf0: // Four byte code
                    pExtChar = p;
                    p++;
                    switch (*pExtChar) {
                    case 0x90:
                        pExtChar = p;
                        p++;
                        switch (*pExtChar) {
                        case 0x92: // Osage 
                            if ((*p >= 0xb0)
                                && (*p <= 0xbf)) {
                                *pExtChar = 0x93;
                                (*p) -= 0x18;
                            }
                            break;
                        case 0x93: // Osage 
                            if ((*p >= 0x80)
                                && (*p <= 0x93))
                                (*p) += 0x18;
                            break;
                        default:
                            break;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                    case 0x9E:
                        pExtChar = p;
                        p++;
                        switch (*pExtChar) {
                        case 0xA4: // Adlam
                            if ((*p >= 0x80)
                                && (*p <= 0xA1))
                                (*p) += 0x22;
                            break;
                        default:
                            break;
                        }
                    break;
                default:
                    break;
                }
                pExtChar = 0;
            }
            p++;
        }
    }
    return pString;
}

static unsigned char* utf_StrToUpper(unsigned char* pString, bool clean)
{
    if (pString && *pString) {
        unsigned char* p = pString;
        unsigned char* pExtChar = 0;
        while (*p) {
            if ((*p >= 0x61) && (*p <= 0x7a)) // US ASCII
                (*p) -= 0x20;
            else if (*p > 0xc0) {
                pExtChar = p;
                p++;
                switch (*pExtChar) {
                case 0xc3: // Latin 1
                    if ((*p >= 0xa0)
                        && (*p <= 0xbe)
                        && (*p != 0xb7))
                        (*p) -= 0x20; // US ASCII shift
                    break;
                case 0xc4: // Latin Exteneded
                    if ((*p >= 0x80)
                        && (*p <= 0xb7)
                        && (*p % 2)) // Odd
                        (*p)--; // Prev char is upr
                    else if ((*p >= 0xb9)
                        && (*p <= 0xbe)
                        && (!(*p % 2))) // Even
                        (*p)--; // Prev char is upr
                    else if (*p == 0xbf) {
                        *pExtChar = 0xc5;
                        (*p) = 0x80;
                    }
                    break;
                case 0xc5: // Latin Exteneded
                    if ((*p >= 0x80)
                        && (*p <= 0x88)
                        && (!(*p % 2))) // Even
                        (*p)--; // Prev char is upr
                    else if ((*p >= 0x8a)
                        && (*p <= 0xb7)
                        && (*p % 2)) // Odd
                        (*p)--; // Prev char is upr
                    else if ((*p >= 0xb9)
                        && (*p <= 0xbe)
                        && (!(*p % 2))) // Even
                        (*p)--; // Prev char is upr
                    break;
                case 0xc6: // Latin Exteneded
                    switch (*p) {
                    case 0x83:
                    case 0x85:
                    case 0x88:
                    case 0x8c:
                    case 0x92:
                    case 0x99:
                    case 0xa1:
                    case 0xa3:
                    case 0xa5:
                    case 0xa8:
                    case 0xad:
                    case 0xb0:
                    case 0xb4:
                    case 0xb6:
                    case 0xb9:
                    case 0xbd:
                        (*p)--; // Prev char is upr
                        break;
                    default:
                        break;
                    }
                    break;
                case 0xc7: // Latin Exteneded
                    if (*p == 0x86)
                        (*p) = 0x84;
                    else if (*p == 0x85)
                        (*p)--; // Prev char is upr
                    else if (*p == 0x89)
                        (*p) = 0x87;
                    else if (*p == 0x88)
                        (*p)--; // Prev char is upr
                    else if (*p == 0x8c)
                        (*p) = 0x8a;
                    else if (*p == 0x8b)
                        (*p)--; // Prev char is upr
                    else if ((*p >= 0x8d)
                        && (*p <= 0x9c)
                        && (!(*p % 2))) // Even
                        (*p)--; // Prev char is upr
                    else if ((*p >= 0x9e)
                        && (*p <= 0xaf)
                        && (*p % 2)) // Odd
                        (*p)--; // Prev char is upr
                    else if (*p == 0xb3)
                        (*p) = 0xb1;
                    else if (*p == 0xb2)
                        (*p)--; // Prev char is upr
                    else if (*p == 0xb4)
                        (*p)--; // Prev char is upr
                    else if (*p == 0xb8)
                        (*p)--; // Prev char is upr
                    else if (*p == 0xba)
                        (*p)--; // Prev char is upr
                    else if (*p == 0xbc)
                        (*p)--; // Prev char is upr
                    else if (*p == 0xbe)
                        (*p)--; // Prev char is upr
                    break;
                case 0xc8: // Latin Exteneded
                    if ((*p >= 0x80)
                        && (*p <= 0x9f)
                        && (*p % 2)) // Odd
                        (*p)--; // Prev char is upr
                    else if ((*p >= 0xa2)
                        && (*p <= 0xb3)
                        && (*p % 2)) // Odd
                        (*p)--; // Prev char is upr
                    else if (*p == 0xbc)
                        (*p)--; // Prev char is upr
                    break;
                case 0xcd: // Greek & Coptic
                    switch (*p) {
                    case 0xb1:
                    case 0xb3:
                    case 0xb7:
                        (*p)--; // Prev char is upr
                        break;
                    default:
                        if (*p == 0xbb) {
                            *pExtChar = 0xcf;
                            (*p) = 0xbd;
                        }
                        else if (*p == 0xbc) {
                            *pExtChar = 0xcf;
                            (*p) = 0xbe;
                        }
                        else if (*p == 0xbd) {
                            *pExtChar = 0xcf;
                            (*p) = 0xbf;
                        }
                        break;
                    }
                    break;
                case 0xce: // Greek & Coptic
                    if (*p == 0xac)
                        (*p) = 0x86;
                    else if (*p == 0xad)
                        (*p) = 0x88;
                    else if (*p == 0xae)
                        (*p) = 0x89;
                    else if (*p == 0xaf)
                        (*p) = 0x8a;
                    else if ((*p >= 0xb1)
                        && (*p <= 0xbf))
                        (*p) -= 0x20; // US ASCII shift
                    break;
                case 0xcf: // Greek & Coptic
                    if (*p == 0xb4)
                        (*p) = 0x8f;
                    else if (*p == 0x92)
                        (*p)--; // Prev char is upr
                    else if ((*p >= 0x98)
                        && (*p <= 0xaf)
                        && (*p % 2)) // Odd
                        (*p)--; // Prev char is upr
                    else if (*p == 0x91)
                        (*p) = 0xb4;
                    else if (*p == 0xb8)
                        (*p)--; // Prev char is upr
                    else if (*p == 0xb2)
                        (*p) = 0xb9;
                    else if (*p == 0xbc)
                        (*p)--; // Prev char is upr
                    else if (*p == 0x8c) {
                        *pExtChar = 0xce;
                        (*p) = 0x8c;
                    }
                    else if (*p == 0x8d) {
                        *pExtChar = 0xce;
                        (*p) = 0x8e;
                    }
                    else if (*p == 0x8e) {
                        *pExtChar = 0xce;
                        (*p) = 0x8f;
                    }
                    else if ((*p >= 0x80)
                        && (*p <= 0x8b)
                        && (*p != 0x82)) {
                        *pExtChar = 0xce;
                        (*p) += 0x20;
                    }
                    else if (*p == 0xb3) {
                        *pExtChar = 0xcd;
                        (*p) = 0xbf;
                    }
                    break;
                case 0xd0: // Cyrillic
                    if ((*p >= 0xb0)
                        && (*p <= 0xbf))
                        (*p) -= 0x20; // US ASCII shift
                    break;
                case 0xd1: // Cyrillic supplement
                    if ((*p >= 0x90)
                        && (*p <= 0x9f)) {
                        *pExtChar = 0xd0;
                        (*p) -= 0x10;
                    }
                    else if ((*p >= 0x80)
                        && (*p <= 0x8f)) {
                        *pExtChar = 0xd0;
                        (*p) += 0x20;
                    }
                    else if ((*p >= 0xa0)
                        && (*p <= 0xbf)
                        && (*p % 2)) // Odd
                        (*p)--; // Prev char is upr
                    break;
                case 0xd2: // Cyrillic supplement
                    if (*p == 0x80)
                        (*p)++; // Prev char is upr
                    else if ((*p >= 0x8a)
                        && (*p <= 0xbf)
                        && (*p % 2)) // Odd
                        (*p)--; // Prev char is upr
                    break;
                case 0xd3: // Cyrillic supplement
                    if ((*p >= 0x81)
                        && (*p <= 0x8e)
                        && (!(*p % 2))) // Even
                        (*p)--; // Prev char is upr
                    else if ((*p >= 0x90)
                        && (*p <= 0xbf)
                        && (*p % 2)) // Odd
                        (*p)--; // Prev char is upr
                    break;
                case 0xd4: // Cyrillic supplement & Armenian
                    if ((*p >= 0x80)
                        && (*p <= 0xaf)
                        && (*p % 2)) // Odd
                        (*p)--; // Prev char is upr
                    break;
                case 0xd5: // Armenian
                    if ((*p >= 0x80)
                        && (*p <= 0x96)
                        && (*p % 2)) // Odd
                        (*p)--; // Prev char is upr
                    else if ((*p >= 0xa1)
                        && (*p <= 0xaf)) {
                        *pExtChar = 0xd4;
                        (*p) += 0x10;
                    }
                    break;
                case 0xe1: // Three byte code
                    pExtChar = p;
                    p++;
                    switch (*pExtChar) {
                    case 0x82: // Georgian
                        break;
                    case 0x83: // Georgian
                        if ((*p >= 0x90)
                            && (*p <= 0xaf)) {
                            *pExtChar = 0x82;
                            (*p) += 0x10;
                        }
                        else if ((*p >= 0xb0)
                            && ((*p <= 0xb5)
                                || (*p == 0xb7))
                            || (*p == 0xbd))
                            (*p) -= 0x30;
                        break;
                    case 0xb8: // Latin extened
                        if ((*p >= 0x80)
                            && (*p <= 0xbf)
                            && (*p % 2)) // Odd
                            (*p)--; // Prev char is upr
                        break;
                    case 0xb9: // Latin extened
                        if ((*p >= 0x80)
                            && (*p <= 0xbf)
                            && (*p % 2)) // Odd
                            (*p)--; // Prev char is upr
                        break;
                    case 0xba: // Latin extened
                        if ((*p >= 0x80)
                            && (*p <= 0x94)
                            && (*p % 2)) // Odd
                            (*p)--; // Prev char is upr
                        else if ((*p >= 0x9e)
                            && (*p <= 0xbf)
                            && (*p % 2)) // Odd
                            (*p)--; // Prev char is upr
                        break;
                    case 0xbb: // Latin extened
                        if ((*p >= 0x80)
                            && (*p <= 0xbf)
                            && (*p % 2)) // Odd
                            (*p)--; // Prev char is upr
                        break;
                    case 0xbc: // Greek extened
                        if ((*p >= 0x80)
                            && (*p <= 0x87))
                            (*p) += 0x08;
                        else if ((*p >= 0x90)
                            && (*p <= 0x97))
                            (*p) += 0x08;
                        else if ((*p >= 0xa0)
                            && (*p <= 0xa7))
                            (*p) += 0x08;
                        else if ((*p >= 0xb0)
                            && (*p <= 0x87))
                            (*p) += 0x08;
                        break;
                    case 0xbd: // Greek extened
                        if ((*p >= 0x80)
                            && (*p <= 0x87))
                            (*p) += 0x08;
                        else if ((*p >= 0x90)
                            && (*p <= 0x97))
                            (*p) += 0x08;
                        else if ((*p >= 0xa0)
                            && (*p <= 0xa7))
                            (*p) += 0x08;
                        else if ((*p >= 0xb0)
                            && (*p <= 0x87))
                            (*p) += 0x08;
                        break;
                    case 0xbe: // Greek extened
                        if ((*p >= 0x80)
                            && (*p <= 0x87))
                            (*p) += 0x08;
                        else if ((*p >= 0x90)
                            && (*p <= 0x97))
                            (*p) += 0x08;
                        else if ((*p >= 0xa0)
                            && (*p <= 0xa7))
                            (*p) += 0x08;
                        else if ((*p >= 0xb0)
                            && (*p <= 0xb1))
                            (*p) += 0x08;
                        break;
                    case 0xbf: // Greek extened
                        if ((*p >= 0x80)
                            && (*p <= 0x84))
                            (*p) += 0x08;
                        else if ((*p >= 0x90)
                            && (*p <= 0x93))
                            (*p) += 0x08;
                        else if ((*p >= 0xa0)
                            && (*p <= 0xa4))
                            (*p) += 0x08;
                        break;
                    default:
                        break;
                    }
                    break;
                case 0xf0: // Four byte code
                    pExtChar = p;
                    p++;
                    switch (*pExtChar) {
                    case 0x90:
                        pExtChar = p;
                        p++;
                        switch (*pExtChar) {
                        case 0x92: // Osage 
                            break;
                        case 0x93: // Osage 
                            if ((*p >= 0x80)
                                && (*p <= 0x93))
                                (*p) += 0x18;
                            else if ((*p >= 0x98)
                                && (*p <= 0xa7)) {
                                *pExtChar = 0x92;
                                (*p) += 0x18;
                            }
                            break;
                        default:
                            break;
                        }
                        break;
                    case 0x9E:
                        pExtChar = p;
                        p++;
                        switch (*pExtChar) {
                        case 0xA4: // Adlam     
                            if ((*p >= 0xa2)
                                && (*p <= 0xc3))
                                (*p) -= 0x22;
                            break;
                        default:
                            break;
                        }
                        break;
                    default:
                        break;
                    }
                    break;
                default:
                    break;
                }
                pExtChar = 0;
            }
            p++;
        }
    }
    return pString;
}
/*************/

/* Unix Aliases */
static const struct _aliases {
  const char *locale;
  const char *code;
} LocaleAliases [] = {
  {"C",         "en.ISO8859-1"},
  {"chinese",	"zh.EUC"},
  {"cs",        "cs.ISO8859-2"},
  {"czech",     "cs.ISO8859-2"},
  {"da",	"da.ISO8859-1"},
  {"da_DK",	"da.ISO8859-1"},
  {"danish",    "da.ISO8859-1"},
  {"de",	"de.ISO8859-1"},
  {"de_AT",	"de-AT.ISO8859-1"},
  {"de_CH",	"de-CH.ISO8859-1"},
  {"de_DE",	"de-DE.ISO8859-1"},
  {"deutsch",   "de.ISO8859-2"},
  {"dutch",     "nl.ISO8859-1"},
  {"el",	"el.ISO8859-7"},
  {"el_GR",	"el.ISO8859-7"},
  {"en",	"en.ISO8859-1"},
  {"en_AU",	"en-AU.ISO8859-1"},
  {"en_CA",	"en-CA.ISO8859-1"},
  {"en_GB",	"en-GB.ISO8859-1"},
  {"en_IE",	"en-IE.ISO8859-1"},
  {"en_NZ",	"en-NZ.ISO8859-1"},
  {"en_UK",	"en-UK.ISO8859-1"},
  {"en_US",	"en-US.ISO8859-1"},
  {"es",	"es.ISO8859-1"},
  {"es_AR",	"es-AR.ISO8859-1"},
  {"es_BO",	"es-BO.ISO8859-1"},
  {"es_CL",	"es-CL.ISO8859-1"},
  {"es_CO",	"es-CO.ISO8859-1"},
  {"es_CR",	"es-CR.ISO8859-1"},
  {"es_EC",	"es-EC.ISO8859-1"},
  {"es_ES",	"es-ES.ISO8859-1"},
  {"es_GT",	"es-GT.ISO8859-1"},
  {"es_MX",	"es-MX.ISO8859-1"},
  {"es_NI",	"es-NI.ISO8859-1"},
  {"es_PA",	"es-PA.ISO8859-1"},
  {"es_PE",	"es-PE.ISO8859-1"},
  {"es_PY",	"es-PY.ISO8859-1"},
  {"es_SV",	"es-SV.ISO8859-1"},
  {"es_UY",	"es-UY.ISO8859-1"},
  {"es_VE",	"es-VE.ISO8859-1"},
  {"fi",	"fi.ISO8859-1"},
  {"fi_FI",	"fi-FI.ISO8859-1"},
  {"finnish",   "fi.ISO8859-1"},
  {"fr",	"fr.ISO8859-1"},
  {"fr_BE",	"fr-BE.ISO8859-1"},
  {"fr_CA",	"fr-CA.ISO8859-1"},
  {"fr_CH",	"fr-CH.ISO8859-1"},
  {"fr_FR",	"fr-FR.ISO8859-1"},
  {"french",    "fr.ISO8859-1"},
  {"german",    "de.ISO8859-1"},
  {"greek",     "el.ISO8859-7"},
  {"hebrew",    "iw.ISO8859-8"},
  {"hu",        "hu.ISO8859-2"},
  {"hungarian", "hu.ISO8859-2"},
  {"icelandic", "is.ISO8859-1"},
  {"is",	"is.ISO8859-1"},
  {"is_IS",	"is-IS.ISO8859-1"},
/* Unspecified Languages */
  {"iso-8859-1", "zzz.ISO8859-1"},
  {"iso-8859-2", "zzz.ISO8859-2"},
  {"iso-8859-3", "zzz.ISO8859-3"},
  {"iso-8859-4", "zzz.ISO8859-4"},
  {"iso-8859-5", "zzz.ISO8859-5"},
  {"iso-8859-6", "zzz.ISO8859-6"},
  {"iso-8859-7", "zzz.ISO8859-7"},
  {"iso-8859-8", "zzz.ISO8859-8"},
  {"iso-8859-9", "zzz.ISO8859-9"},
  {"iso-8859-10","zzz.ISO8859-10"},
  {"iso_8859_1", "zzz.ISO8859-1"},
  {"iso_8859_2", "zzz.ISO8859-2"},
  {"iso_8859_3", "zzz.ISO8859-3"},
  {"iso_8859_4", "zzz.ISO8859-4"},
  {"iso_8859_5", "zzz.ISO8859-5"},
  {"iso_8859_6", "zzz.ISO8859-6"},
  {"iso_8859_7", "zzz.ISO8859-7"},
  {"iso_8859_8", "zzz.ISO8859-8"},
  {"iso_8859_9", "zzz.ISO8859-9"},
  {"iso_8859_10","zzz.ISO8859-10"},
/* Back to list */
  {"it",	"it.ISO8859-1"},
  {"it_CH",	"it-CH.ISO8859-1"},
  {"it_IT",	"it-IT.ISO8859-1"},
  {"italian",   "it.ISO8859-1"},
  {"italiano",  "it.ISO8859-1"},
  {"iw",        "he.ISO8859-8"},
  {"ja",	"ja.EUC"},
  {"ja_JP", 	"ja.EUC"},
  {"ja_JP.MSK", "ja.MSK"},
  {"ja_JP.MSK",	"ja.SJIS"},
  {"ja_JP.SJIS","ja.MSK"},
  {"ja_JP.SJIS","ja.SJIS"},
  {"japan",	"ja.EUC"},
  {"japanese",	"ja.EUC"},
  {"ko",	"ko.EUC"},
  {"ko_KR",	"ko.EUC"},
  {"korean",	"ko.EUC"},
  {"nl",	"nl.ISO8859-1"},
  {"nl_BE",	"nl-BE.ISO8859-1"},
  {"nl_NL",	"nl-NL.ISO8859-1"},
  {"no",	"no.ISO8859-1"},
  {"no_NO",	"no.ISO8859-1"},
  {"pl",	"pl.ISO8859-2"},
  {"pl_PL",	"pl.ISO8859-2"},
  {"polish",    "pl.ISO8859-2"},
  {"pt",	"pt.ISO8859-1"},
  {"pt_BR",	"pt-BR.ISO8859-1"},
  {"pt_PT",	"pt-PT.ISO8859-1"},
  {"ro",        "ro.ISO8859-2"},
  {"romanian",  "ro.ISO8859-2"},
  {"ru", 	"ru.ISO8859-5"},
  {"ru_SU",	"ru.ISO8859-5"},
  {"russian",   "ru.ISO8859-5"},
  {"sh", 	"sh.ISO8859-2"},
  {"sh_YU",	"sh.ISO8859-2"},
  {"sk", 	"sk.ISO8859-2"},
  {"sl",        "sl.ISO8859-2"},
  {"slovak",    "sk.ISO8859-2"},
  {"slovene",   "sl.ISO8859-2"},
  {"spanish",   "es.ISO8859-1"},
  {"sv",	"sv.ISO8859-1"},
  {"sv_SE",	"sv.ISO8859-1"},
  {"svenska",   "sv.ISO8859-1"},
  {"swedish",   "sv.ISO8859-1"},
  {"tchinese",	"zh.EUC"},
  {"tr", 	"tr.ISO8859-3"},
  {"turkish",   "tr.ISO8859-3"},
  {"UTF8",      "zzz.UTF8"}, // NOT SUPPORTED YET
  {"UTF-8",     "zzz.UTF8"}, // NOT SUPPORTED YET
  {"zh", 	"zh.EUC"},
  {"zh_TW",	"zh.EUC"}
};

size_t GetLocaleCodes(char ***codes, char ***values)
{
  static char **Codes = NULL;
  static char **Values = NULL;
  static size_t i = 0;
  if (i == 0)
    {
      Codes = new char *[ sizeof(LocaleAliases)/sizeof(LocaleAliases[0]) + 1 ];
      Values = new char *[ sizeof(LocaleAliases)/sizeof(LocaleAliases[0]) + 1 ];
      for (; i < sizeof(LocaleAliases)/sizeof(LocaleAliases[0]); i++)
        {
          Codes[i]  = (char *)LocaleAliases[i].locale;
          Values[i] = (char *)LocaleAliases[i].code;
        }
      Codes[i] = Values[i] = NULL;
    }
  *codes = Codes;
  *values = Values;
  return i;
}


/*
TODO:
  Pack language in the first 10 bits
  followed by country

  eg.  Const = (1 << 10);

  Language = LangCode & (Const - 1);
  Country  = LangCode / Const;

  LangCode = Language + Country *Const;

*/


/* Countries */
static const struct _country2 {
  const char *code;
  short       num;
} Country2[] = {
  {"AD",  20}, {"AE", 784}, {"AF",   4}, {"AG",  28}, {"AI", 660},
  {"AL",   8}, {"AM",  51}, {"AN", 530}, {"AO",  24}, {"AQ",  10},
  {"AR",  32}, {"AS",  16}, {"AT",  40}, {"AU",  36}, {"AW", 533},
  {"AZ",  31}, {"BA",  70}, {"BB",  52}, {"BD",  50}, {"BE",  56},
  {"BF", 854}, {"BG", 100}, {"BH",  48}, {"BI", 108}, {"BJ", 204},
  {"BM",  60}, {"BN",  96}, {"BO",  68}, {"BR",  76}, {"BS",  44},
  {"BT",  64}, {"BV",  74}, {"BW",  72}, {"BY", 112}, {"BZ",  84},
  {"CA", 124}, {"CC", 166}, {"CF", 140}, {"CG", 178}, {"CH", 756},
  {"CI", 384}, {"CK", 184}, {"CL", 152}, {"CM", 120}, {"CN", 156},
  {"CO", 170}, {"CR", 188}, {"CU", 192}, {"CV", 132}, {"CX", 162},
  {"CY", 196}, {"CZ", 203}, {"DE", 276}, {"DJ", 262}, {"DK", 208},
  {"DM", 212}, {"DO", 214}, {"DZ",  12}, {"EC", 218}, {"EE", 233},
  {"EG", 818}, {"EH", 732}, {"ER", 232}, {"ES", 724}, {"ET", 231},
  {"FI", 246}, {"FJ", 242}, {"FK", 238}, {"FM", 583}, {"FO", 234},
  {"FR", 250}, {"FX", 249}, {"GA", 266}, {"GB", 826}, {"GD", 308},
  {"GE", 268}, {"GF", 254}, {"GH", 288}, {"GI", 292}, {"GL", 304},
  {"GM", 270}, {"GN", 324}, {"GP", 312}, {"GQ", 226}, {"GR", 300},
  {"GS", 239}, {"GT", 320}, {"GU", 316}, {"GW", 624}, {"GY", 328},
  {"HK", 344}, {"HM", 334}, {"HN", 340}, {"HR", 191}, {"HT", 332},
  {"HU", 348}, {"ID", 360}, {"IE", 372}, {"IL", 376}, {"IN", 356},
  {"IO",  86}, {"IQ", 368}, {"IR", 364}, {"IS", 352}, {"IT", 380},
  {"JM", 388}, {"JO", 400}, {"JP", 392}, {"KE", 404}, {"KG", 417},
  {"KH", 116}, {"KI", 296}, {"KM", 174}, {"KN", 659}, {"KP", 408},
  {"KR", 410}, {"KW", 414}, {"KY", 136}, {"KZ", 398}, {"LA", 418},
  {"LB", 422}, {"LC", 662}, {"LI", 438}, {"LK", 144}, {"LR", 430},
  {"LS", 426}, {"LT", 440}, {"LU", 442}, {"LV", 428}, {"LY", 434},
  {"MA", 504}, {"MC", 492}, {"MD", 498}, {"MG", 450}, {"MH", 584},
  {"MK", 807}, {"ML", 466}, {"MM", 104}, {"MN", 496}, {"MO", 446},
  {"MP", 580}, {"MQ", 474}, {"MR", 478}, {"MS", 500}, {"MT", 470},
  {"MU", 480}, {"MV", 462}, {"MW", 454}, {"MX", 484}, {"MY", 458},
  {"MZ", 508}, {"NA", 516}, {"NC", 540}, {"NE", 562}, {"NF", 574},
  {"NG", 566}, {"NI", 558}, {"NL", 528}, {"NO", 578}, {"NP", 524},
  {"NR", 520}, {"NU", 570}, {"NZ", 554}, {"OM", 512}, {"PA", 591},
  {"PE", 604}, {"PF", 258}, {"PG", 598}, {"PH", 608}, {"PK", 586},
  {"PL", 616}, {"PM", 666}, {"PN", 612}, {"PR", 630}, {"PT", 620},
  {"PW", 585}, {"PY", 600}, {"QA", 634}, {"RE", 638}, {"RO", 642},
  {"RU", 643}, {"RW", 646}, {"SA", 682}, {"SB",  90}, {"SC", 690},
  {"SD", 736}, {"SE", 752}, {"SG", 702}, {"SH", 654}, {"SI", 705},
  {"SJ", 744}, {"SK", 703}, {"SL", 694}, {"SM", 674}, {"SN", 686},
  {"SO", 706}, {"SR", 740}, {"ST", 678}, {"SV", 222}, {"SY", 760},
  {"SZ", 748}, {"TC", 796}, {"TD", 148}, {"TF", 260}, {"TG", 768},
  {"TH", 764}, {"TJ", 762}, {"TK", 772}, {"TM", 795}, {"TN", 788},
  {"TO", 776}, {"TP", 626}, {"TR", 792}, {"TT", 780}, {"TV", 798},
  {"TW", 158}, {"TZ", 834}, {"UA", 804}, {"UG", 800}, {"UM", 581},
  {"US", 840}, {"UY", 858}, {"UZ", 860}, {"VA", 336}, {"VC", 670},
  {"VE", 862}, {"VG",  92}, {"VI", 850}, {"VN", 704}, {"VU", 548},
  {"WF", 876}, {"WS", 882}, {"YE", 887}, {"YT", 175}, {"YU", 891},
  {"ZA", 710}, {"ZM", 894}, {"ZR", 180}, {"ZW", 716}, {"ZZ", 1000}
#define _NOWHERE 0xef
};


// Valid domain?
/*
    {"ARPA", "Arpanet"},
    {"COM", "Commercial Entity"},
    {"EDU", "Educational Institution"},
    {"GOV", "Government Office"},
    {"INT", "International Organization"},
    {"MIL", "US Military"},
    {"NET", "Network Service Provider"},
    {"ORG", "Organizations"},
    {"BITNET", "Educational BitNet"},
    {"UUCP",  "Unix-to-Unix Copy"},
*/


static const struct _country3 {
  const char   *code;
  short         num;
  unsigned char idx; // The position in the 2Country code
} Country3[] = {
  {"ABW", 533, 0x0e}, {"AFG",   4, 0x02}, {"AGO",  24, 0x08}, {"AIA", 660, 0x04},
  {"ALB",   8, 0x05}, {"AND",  20, 0x00}, {"ANT", 530, 0x07}, {"ARE", 784, 0x01},
  {"ARG",  32, 0x0a}, {"ARM",  51, 0x06}, {"ASM",  16, 0x0b}, {"ATA",  10, 0x09},
  {"ATF", 260, 0xcb}, {"ATG",  28, 0x03}, {"AUS",  36, 0x0d}, {"AUT",  40, 0x0c},
  {"AZE",  31, 0x0f}, {"BDI", 108, 0x17}, {"BEL",  56, 0x13}, {"BEN", 204, 0x18},
  {"BFA", 854, 0x14}, {"BGD",  50, 0x12}, {"BGR", 100, 0x15}, {"BHR",  48, 0x16},
  {"BHS",  44, 0x1d}, {"BIH",  70, 0x10}, {"BLR", 112, 0x21}, {"BLZ",  84, 0x22},
  {"BMU",  60, 0x19}, {"BOL",  68, 0x1b}, {"BRA",  76, 0x1c}, {"BRB",  52, 0x11},
  {"BRN",  96, 0x1a}, {"BTN",  64, 0x1e}, {"BVT",  74, 0x1f}, {"BWA",  72, 0x20},
  {"CAF", 140, 0x25}, {"CAN", 124, 0x23}, {"CCK", 166, 0x24}, {"CHE", 756, 0x27},
  {"CHL", 152, 0x2a}, {"CHN", 156, 0x2c}, {"CIV", 384, 0x28}, {"CMR", 120, 0x2b},
  {"COG", 178, 0x26}, {"COK", 184, 0x29}, {"COL", 170, 0x2d}, {"COM", 174, 0x70},
  {"CPV", 132, 0x30}, {"CRI", 188, 0x2e}, {"CUB", 192, 0x2f}, {"CXR", 162, 0x31},
  {"CYM", 136, 0x75}, {"CYP", 196, 0x32}, {"CZE", 203, 0x33}, {"DEU", 276, 0x34},
  {"DJI", 262, 0x35}, {"DMA", 212, 0x37}, {"DNK", 208, 0x36}, {"DOM", 214, 0x38},
  {"DZA",  12, 0x39}, {"ECU", 218, 0x3a}, {"EGY", 818, 0x3c}, {"ERI", 232, 0x3e},
  {"ESH", 732, 0x3d}, {"ESP", 724, 0x3f}, {"EST", 233, 0x3b}, {"ETH", 231, 0x40},
  {"FIN", 246, 0x41}, {"FJI", 242, 0x42}, {"FLK", 238, 0x43}, {"FRA", 250, 0x46},
  {"FRO", 234, 0x45}, {"FSM", 583, 0x44}, {"FXX", 249, 0x47}, {"GAB", 266, 0x48},
  {"GBR", 826, 0x49}, {"GEO", 268, 0x4b}, {"GHA", 288, 0x4d}, {"GIB", 292, 0x4e},
  {"GIN", 324, 0x51}, {"GLP", 312, 0x52}, {"GMB", 270, 0x50}, {"GNB", 624, 0x58},
  {"GNQ", 226, 0x53}, {"GRC", 300, 0x54}, {"GRD", 308, 0x4a}, {"GRL", 304, 0x4f},
  {"GTM", 320, 0x56}, {"GUF", 254, 0x4c}, {"GUM", 316, 0x57}, {"GUY", 328, 0x59},
  {"HKG", 344, 0x5a}, {"HMD", 334, 0x5b}, {"HND", 340, 0x5c}, {"HRV", 191, 0x5d},
  {"HTI", 332, 0x5e}, {"HUN", 348, 0x5f}, {"IDN", 360, 0x60}, {"IND", 356, 0x63},
  {"IOT",  86, 0x64}, {"IRL", 372, 0x61}, {"IRN", 364, 0x66}, {"IRQ", 368, 0x65},
  {"ISL", 352, 0x67}, {"ISR", 376, 0x62}, {"ITA", 380, 0x68}, {"JAM", 388, 0x69},
  {"JOR", 400, 0x6a}, {"JPN", 392, 0x6b}, {"KAZ", 398, 0x76}, {"KEN", 404, 0x6c},
  {"KGZ", 417, 0x6d}, {"KHM", 116, 0x6e}, {"KIR", 296, 0x6f}, {"KNA", 659, 0x71},
  {"KOR", 410, 0x73}, {"KWT", 414, 0x74}, {"LAO", 418, 0x77}, {"LBN", 422, 0x78},
  {"LBR", 430, 0x7c}, {"LBY", 434, 0x81}, {"LCA", 662, 0x79}, {"LIE", 438, 0x7a},
  {"LKA", 144, 0x7b}, {"LSO", 426, 0x7d}, {"LTU", 440, 0x7e}, {"LUX", 442, 0x7f},
  {"LVA", 428, 0x80}, {"MAC", 446, 0x8b}, {"MAR", 504, 0x82}, {"MCO", 492, 0x83},
  {"MDA", 498, 0x84}, {"MDG", 450, 0x85}, {"MDV", 462, 0x92}, {"MEX", 484, 0x94},
  {"MHL", 584, 0x86}, {"MKD", 807, 0x87}, {"MLI", 466, 0x88}, {"MLT", 470, 0x90},
  {"MMR", 104, 0x89}, {"MNG", 496, 0x8a}, {"MNP", 580, 0x8c}, {"MOZ", 508, 0x96},
  {"MRT", 478, 0x8e}, {"MSR", 500, 0x8f}, {"MTQ", 474, 0x8d}, {"MUS", 480, 0x91},
  {"MWI", 454, 0x93}, {"MYS", 458, 0x95}, {"MYT", 175, 0xe9}, {"NAM", 516, 0x97},
  {"NCL", 540, 0x98}, {"NER", 562, 0x99}, {"NFK", 574, 0x9a}, {"NGA", 566, 0x9b},
  {"NIC", 558, 0x9c}, {"NIU", 570, 0xa1}, {"NLD", 528, 0x9d}, {"NOR", 578, 0x9e},
  {"NPL", 524, 0x9f}, {"NRU", 520, 0xa0}, {"NZL", 554, 0xa2}, {"OMN", 512, 0xa3},
  {"PAK", 586, 0xa9}, {"PAN", 591, 0xa4}, {"PCN", 612, 0xac}, {"PER", 604, 0xa5},
  {"PHL", 608, 0xa8}, {"PLW", 585, 0xaf}, {"PNG", 598, 0xa7}, {"POL", 616, 0xaa},
  {"PRI", 630, 0xad}, {"PRK", 408, 0x72}, {"PRT", 620, 0xae}, {"PRY", 600, 0xb0},
  {"PYF", 258, 0xa6}, {"QAT", 634, 0xb1}, {"REU", 638, 0xb2}, {"ROM", 642, 0xb3},
  {"RUS", 643, 0xb4}, {"RWA", 646, 0xb5}, {"SAU", 682, 0xb6}, {"SDN", 736, 0xb9},
  {"SEN", 686, 0xc2}, {"SGP", 702, 0xbb}, {"SGS", 239, 0x55}, {"SHN", 654, 0xbc},
  {"SJM", 744, 0xbe}, {"SLB",  90, 0xb7}, {"SLE", 694, 0xc0}, {"SLV", 222, 0xc6},
  {"SMR", 674, 0xc1}, {"SOM", 706, 0xc3}, {"SPM", 666, 0xab}, {"STP", 678, 0xc5},
  {"SUR", 740, 0xc4}, {"SVK", 703, 0xbf}, {"SVN", 705, 0xbd}, {"SWE", 752, 0xba},
  {"SWZ", 748, 0xc8}, {"SYC", 690, 0xb8}, {"SYR", 760, 0xc7}, {"TCA", 796, 0xc9},
  {"TCD", 148, 0xca}, {"TGO", 768, 0xcc}, {"THA", 764, 0xcd}, {"TJK", 762, 0xce},
  {"TKL", 772, 0xcf}, {"TKM", 795, 0xd0}, {"TMP", 626, 0xd3}, {"TON", 776, 0xd2},
  {"TTO", 780, 0xd5}, {"TUN", 788, 0xd1}, {"TUR", 792, 0xd4}, {"TUV", 798, 0xd6},
  {"TWN", 158, 0xd7}, {"TZA", 834, 0xd8}, {"UGA", 800, 0xda}, {"UKR", 804, 0xd9},
  {"UMI", 581, 0xdb}, {"URY", 858, 0xdd}, {"USA", 840, 0xdc}, {"UZB", 860, 0xde},
  {"VAT", 336, 0xdf}, {"VCT", 670, 0xe0}, {"VEN", 862, 0xe1}, {"VGB",  92, 0xe2},
  {"VIR", 850, 0xe3}, {"VNM", 704, 0xe4}, {"VUT", 548, 0xe5}, {"WLF", 876, 0xe6},
  {"WSM", 882, 0xe7}, {"YEM", 887, 0xe8}, {"YUG", 891, 0xea}, {"ZAF", 710, 0xeb},
  {"ZAR", 180, 0xed}, {"ZMB", 894, 0xec}, {"ZWE", 716, 0xee}, {"ZZZ", 1000, 0xef}
};

// TO ADD
// AALAND ISLANDS                                  AX      ALA     248

static const struct _countryN {
  short         num;
  unsigned char idx;
  const char   *country;
} CountryN[] = {
  {  4, 0x02, "AFGHANISTAN"},
  {  8, 0x05, "ALBANIA"},
  { 10, 0x09, "ANTARCTICA"},
  { 12, 0x39, "ALGERIA"},
  { 16, 0x0b, "AMERICAN SAMOA"},
  { 20, 0x00, "ANDORRA"},
  { 24, 0x08, "ANGOLA"},
  { 28, 0x03, "ANTIGUA AND BARBUDA"},
  { 31, 0x0f, "AZERBAIJAN"},
  { 32, 0x0a, "ARGENTINA"},
  { 36, 0x0d, "AUSTRALIA"},
  { 40, 0x0c, "AUSTRIA"},
  { 44, 0x1d, "BAHAMAS"},
  { 48, 0x16, "BAHRAIN"},
  { 50, 0x12, "BANGLADESH"},
  { 51, 0x06, "ARMENIA"},
  { 52, 0x11, "BARBADOS"},
  { 56, 0x13, "BELGIUM"},
  { 60, 0x19, "BERMUDA"},
  { 64, 0x1e, "BHUTAN"},
  { 68, 0x1b, "BOLIVIA"},
  { 70, 0x10, "BOSNIA AND HERZEGOWINA"},
  { 72, 0x20, "BOTSWANA"},
  { 74, 0x1f, "BOUVET ISLAND"},
  { 76, 0x1c, "BRAZIL"},
  { 84, 0x22, "BELIZE"},
  { 86, 0x64, "BRITISH INDIAN OCEAN TERRITORY"},
  { 90, 0xb7, "SOLOMON ISLANDS"},
  { 92, 0xe2, "VIRGIN ISLANDS (BRITISH)"},
  { 96, 0x1a, "BRUNEI DARUSSALAM"},
  {100, 0x15, "BULGARIA"},
  {104, 0x89, "MYANMAR"},
  {108, 0x17, "BURUNDI"},
  {112, 0x21, "BELARUS"},
  {116, 0x6e, "CAMBODIA"},
  {120, 0x2b, "CAMEROON"},
  {124, 0x23, "CANADA"},
  {132, 0x30, "CAPE VERDE"},
  {136, 0x75, "CAYMAN ISLANDS"},
  {140, 0x25, "CENTRAL AFRICAN REPUBLIC"},
  {144, 0x7b, "SRI LANKA"},
  {148, 0xca, "CHAD"},
  {152, 0x2a, "CHILE"},
  {156, 0x2c, "CHINA"},
  {158, 0xd7, "TAIWAN, PROVINCE OF CHINA"},
  {162, 0x31, "CHRISTMAS ISLAND"},
  {166, 0x24, "COCOS (KEELING) ISLANDS"},
  {170, 0x2d, "COLOMBIA"},
  {174, 0x70, "COMOROS"},
  {175, 0xe9, "MAYOTTE"},
  {178, 0x26, "CONGO"},
  {180, 0xed, "ZAIRE"},
  {184, 0x29, "COOK ISLANDS"},
  {188, 0x2e, "COSTA RICA"},
  {191, 0x5d, "CROATIA"},
  {192, 0x2f, "CUBA"},
  {196, 0x32, "CYPRUS"},
  {203, 0x33, "CZECH REPUBLIC"},
  {204, 0x18, "BENIN"},
  {208, 0x36, "DENMARK"},
  {212, 0x37, "DOMINICA"},
  {214, 0x38, "DOMINICAN REPUBLIC"},
  {218, 0x3a, "ECUADOR"},
  {222, 0xc6, "EL SALVADOR"},
  {226, 0x53, "EQUATORIAL GUINEA"},
  {231, 0x40, "ETHIOPIA"},
  {232, 0x3e, "ERITREA"},
  {233, 0x3b, "ESTONIA"},
  {234, 0x45, "FAROE ISLANDS"},
  {238, 0x43, "FALKLAND ISLANDS (MALVINAS)"},
  {239, 0x55, "SOUTH GEORGIA AND THE SOUTH SANDWICH ISLANDS"},
  {242, 0x42, "FIJI"},
  {246, 0x41, "FINLAND"},
  {249, 0x47, "FRANCE, METROPOLITAN"},
  {250, 0x46, "FRANCE"},
  {254, 0x4c, "FRENCH GUIANA"},
  {258, 0xa6, "FRENCH POLYNESIA"},
  {260, 0xcb, "FRENCH SOUTHERN TERRITORIES"},
  {262, 0x35, "DJIBOUTI"},
  {266, 0x48, "GABON"},
  {268, 0x4b, "GEORGIA"},
  {270, 0x50, "GAMBIA"},
  {276, 0x34, "GERMANY"},
  {288, 0x4d, "GHANA"},
  {292, 0x4e, "GIBRALTAR"},
  {296, 0x6f, "KIRIBATI"},
  {300, 0x54, "GREECE"},
  {304, 0x4f, "GREENLAND"},
  {308, 0x4a, "GRENADA"},
  {312, 0x52, "GUADELOUPE"},
  {316, 0x57, "GUAM"},
  {320, 0x56, "GUATEMALA"},
  {324, 0x51, "GUINEA"},
  {328, 0x59, "GUYANA"},
  {332, 0x5e, "HAITI"},
  {334, 0x5b, "HEARD AND MC DONALD ISLANDS"},
  {336, 0xdf, "VATICAN CITY STATE (HOLY SEE)"},
  {340, 0x5c, "HONDURAS"},
  {344, 0x5a, "HONG KONG"},
  {348, 0x5f, "HUNGARY"},
  {352, 0x67, "ICELAND"},
  {356, 0x63, "INDIA"},
  {360, 0x60, "INDONESIA"},
  {364, 0x66, "IRAN (ISLAMIC REPUBLIC OF)"},
  {368, 0x65, "IRAQ"},
  {372, 0x61, "IRELAND"},
  {376, 0x62, "ISRAEL"},
  {380, 0x68, "ITALY"},
  {384, 0x28, "COTE D'IVOIRE"},
  {388, 0x69, "JAMAICA"},
  {392, 0x6b, "JAPAN"},
  {398, 0x76, "KAZAKHSTAN"},
  {400, 0x6a, "JORDAN"},
  {404, 0x6c, "KENYA"},
  {408, 0x72, "KOREA, DEMOCRATIC PEOPLE'S REPUBLIC OF"},
  {410, 0x73, "KOREA, REPUBLIC OF"},
  {414, 0x74, "KUWAIT"},
  {417, 0x6d, "KYRGYZSTAN"},
  {418, 0x77, "LAO PEOPLE'S DEMOCRATIC REPUBLIC"},
  {422, 0x78, "LEBANON"},
  {426, 0x7d, "LESOTHO"},
  {428, 0x80, "LATVIA"},
  {430, 0x7c, "LIBERIA"},
  {434, 0x81, "LIBYAN ARAB JAMAHIRIYA"},
  {438, 0x7a, "LIECHTENSTEIN"},
  {440, 0x7e, "LITHUANIA"},
  {442, 0x7f, "LUXEMBOURG"},
  {446, 0x8b, "MACAU"},
  {450, 0x85, "MADAGASCAR"},
  {454, 0x93, "MALAWI"},
  {458, 0x95, "MALAYSIA"},
  {462, 0x92, "MALDIVES"},
  {466, 0x88, "MALI"},
  {470, 0x90, "MALTA"},
  {474, 0x8d, "MARTINIQUE"},
  {478, 0x8e, "MAURITANIA"},
  {480, 0x91, "MAURITIUS"},
  {484, 0x94, "MEXICO"},
  {492, 0x83, "MONACO"},
  {496, 0x8a, "MONGOLIA"},
  {498, 0x84, "MOLDOVA, REPUBLIC OF"},
  {500, 0x8f, "MONTSERRAT"},
  {504, 0x82, "MOROCCO"},
  {508, 0x96, "MOZAMBIQUE"},
  {512, 0xa3, "OMAN"},
  {516, 0x97, "NAMIBIA"},
  {520, 0xa0, "NAURU"},
  {524, 0x9f, "NEPAL"},
  {528, 0x9d, "NETHERLANDS"},
  {530, 0x07, "NETHERLANDS ANTILLES"},
  {533, 0x0e, "ARUBA"},
  {540, 0x98, "NEW CALEDONIA"},
  {548, 0xe5, "VANUATU"},
  {554, 0xa2, "NEW ZEALAND"},
  {558, 0x9c, "NICARAGUA"},
  {562, 0x99, "NIGER"},
  {566, 0x9b, "NIGERIA"},
  {570, 0xa1, "NIUE"},
  {574, 0x9a, "NORFOLK ISLAND"},
  {578, 0x9e, "NORWAY"},
  {580, 0x8c, "NORTHERN MARIANA ISLANDS"},
  {581, 0xdb, "UNITED STATES MINOR OUTLYING ISLANDS"},
  {583, 0x44, "MICRONESIA (FEDERATED STATES OF)"},
  {584, 0x86, "MARSHALL ISLANDS"},
  {585, 0xaf, "PALAU"},
  {586, 0xa9, "PAKISTAN"},
  {591, 0xa4, "PANAMA"},
  {598, 0xa7, "PAPUA NEW GUINEA"},
  {600, 0xb0, "PARAGUAY"},
  {604, 0xa5, "PERU"},
  {608, 0xa8, "PHILIPPINES"},
  {612, 0xac, "PITCAIRN"},
  {616, 0xaa, "POLAND"},
  {620, 0xae, "PORTUGAL"},
  {624, 0x58, "GUINEA-BISSAU"},
  {626, 0xd3, "EAST TIMOR"},
  {630, 0xad, "PUERTO RICO"},
  {634, 0xb1, "QATAR"},
  {638, 0xb2, "REUNION"},
  {642, 0xb3, "ROMANIA"},
  {643, 0xb4, "RUSSIAN FEDERATION"},
  {646, 0xb5, "RWANDA"},
  {654, 0xbc, "ST. HELENA"},
  {659, 0x71, "SAINT KITTS AND NEVIS"},
  {660, 0x04, "ANGUILLA"},
  {662, 0x79, "SAINT LUCIA"},
  {666, 0xab, "ST. PIERRE AND MIQUELON"},
  {670, 0xe0, "SAINT VINCENT AND THE GRENADINES"},
  {674, 0xc1, "SAN MARINO"},
  {678, 0xc5, "SAO TOME AND PRINCIPE"},
  {682, 0xb6, "SAUDI ARABIA"},
  {686, 0xc2, "SENEGAL"},
  {690, 0xb8, "SEYCHELLES"},
  {694, 0xc0, "SIERRA LEONE"},
  {702, 0xbb, "SINGAPORE"},
  {703, 0xbf, "SLOVAKIA"},
  {704, 0xe4, "VIET NAM"},
  {705, 0xbd, "SLOVENIA"},
  {706, 0xc3, "SOMALIA"},
  {710, 0xeb, "SOUTH AFRICA"},
  {716, 0xee, "ZIMBABWE"},
  {724, 0x3f, "SPAIN"},
  {732, 0x3d, "WESTERN SAHARA"},
  {736, 0xb9, "SUDAN"},
  {740, 0xc4, "SURINAME"},
  {744, 0xbe, "SVALBARD AND JAN MAYEN ISLANDS"},
  {748, 0xc8, "SWAZILAND"},
  {752, 0xba, "SWEDEN"},
  {756, 0x27, "SWITZERLAND"},
  {760, 0xc7, "SYRIAN ARAB REPUBLIC"},
  {762, 0xce, "TAJIKISTAN"},
  {764, 0xcd, "THAILAND"},
  {768, 0xcc, "TOGO"},
  {772, 0xcf, "TOKELAU"},
  {776, 0xd2, "TONGA"},
  {780, 0xd5, "TRINIDAD AND TOBAGO"},
  {784, 0x01, "UNITED ARAB EMIRATES"},
  {788, 0xd1, "TUNISIA"},
  {792, 0xd4, "TURKEY"},
  {795, 0xd0, "TURKMENISTAN"},
  {796, 0xc9, "TURKS AND CAICOS ISLANDS"},
  {798, 0xd6, "TUVALU"},
  {800, 0xda, "UGANDA"},
  {804, 0xd9, "UKRAINE"},
  {807, 0x87, "MACEDONIA, THE FORMER YUGOSLAV REPUBLIC OF"},
  {818, 0x3c, "EGYPT"},
  {826, 0x49, "UNITED KINGDOM"},
  {834, 0xd8, "TANZANIA, UNITED REPUBLIC OF"},
  {840, 0xdc, "UNITED STATES"},
  {850, 0xe3, "VIRGIN ISLANDS (U.S.)"},
  {854, 0x14, "BURKINA FASO"},
  {858, 0xdd, "URUGUAY"},
  {860, 0xde, "UZBEKISTAN"},
  {862, 0xe1, "VENEZUELA"},
  {876, 0xe6, "WALLIS AND FUTUNA ISLANDS"},
  {882, 0xe7, "SAMOA"},
  {887, 0xe8, "YEMEN"},
  {891, 0xea, "YUGOSLAVIA"},
  {894, 0xec, "ZAMBIA"},
  {1000, 0xef, "NOWHERE"},
};

#if 0
/////////// Unused at this time /////

static int Country2Compare(const void *node1, const void *node2)
{
  return (strcasecmp(((const struct _country2 *)node1)->code,
	((const struct _country2 *)node2)->code));
}

static int Country3Compare(const void *node1, const void *node2)
{
  return (strcasecmp(((const struct _country3 *)node1)->code,
        ((const struct _country3 *)node2)->code));
}

static int CountryNCompare(const void *node1, const void *node2)
{
  return ((const struct _countryN *)node2)->num - ((const struct _countryN *)node1)->num;
}


static const char *countryId2iso3166(int num)
{
  struct _countryN n;
  struct _countryN *t;
  n.num = num;
  t = (struct _countryN *) bsearch(&n, CountryN, sizeof(CountryN)/sizeof(CountryN[0]),
	sizeof(CountryN[0]), CountryNCompare);
  return t ? Country2[t->idx].code : NULL;
}

static const char *countryId2Country(int num)
{
  struct _countryN n;
  struct _countryN *t;
  n.num = num;
  t = (struct _countryN *) bsearch(&n, CountryN, sizeof(CountryN)/sizeof(CountryN[0]),
	sizeof(CountryN[0]), CountryNCompare);
  return t ? t->country : NULL; 
}

static SHORT iso3166Code2Id (const char *Code)
{
  if (Code == NULL || *Code == '\0')
    return 0;

  const size_t len = strlen(Code);
  if (len == 2)
    {
      struct _country2 n;
      struct _country2 *t;
      n.code = Code;
      t = (struct _country2 *) bsearch(&n, Country2, sizeof(Country2)/sizeof(Country2[0]),
	sizeof(Country2[0]), Country2Compare);
      if (t)
        return t->num;
    }
  else if (len == 3)
    {
      struct _country3 n;
      struct _country3 *t;
      n.code = Code;
      t = (struct _country3 *) bsearch(&n, Country3, sizeof(Country3)/sizeof(Country3[0]),
        sizeof(Country3[0]), Country3Compare);
      if (t)
        return t->num;
    }
  return atoi(Code);
}
///////////////////////////////////////////////////
#endif

/*
Code--language      54     Code-language
                           language of the item.
                           The codes are defined by the
                           target.
*/
/*
 languages        [101]  IMPLICIT SEQUENCE OF InternationalString OPTIONAL,
                  -- Languages supported for message strings.  Each is a three-character
                  -- language code from Z39.53-1994.
*/
static const struct _lang {
  const char *code;
  const int   num;
  const char *lang;
} ISO_Languages[] = {
/* NOTE: code be sorted! */
  {"aa",  	1,	"Afar"},
  {"aar",  	2,	"Afar"},
  {"ab",  	3,	"Abkhazian"},
  {"abk",  	4,	"Abkhazian"},
  {"ace",  	5,	"Achinese"},
  {"ach",  	6,	"Acoli"},
  {"ada",  	7,	"Adangme"},
  {"af",  	8,	"Afrikaans"},
  {"afa",  	9,	"Afro-Asiatic"},
  {"afh",  	10,	"Afrihili"},
  {"afr",  	11,	"Africaans"},
  {"ajm",  	12,	"Aljamia"},
  {"aka",  	13,	"Akan"},
  {"akk",  	14,	"Akkadian"},
  {"alb",  	15,	"Albanian"},
  {"ale",  	16,	"Aleut"},
  {"alg",  	17,	"Algonquian languages"},
  {"am",  	18,	"Amharic"},
  {"amh",  	19,	"Amharic"},
  {"ang",  	20,	"Old-English"},
  {"apa",  	21,	"Apache languages"},
  {"ar",  	22,	"Arabic"},
  {"ara",  	23,	"Arabic"},
  {"arc",  	24,	"Aramaic"},
  {"arm",  	25,	"Armenian"},
  {"arn",  	26,	"Araucanian"},
  {"arp",  	27,	"Arapaho"},
  {"art",  	28,	"Artificial"},
  {"arw",  	29,	"Arawak"},
  {"as",  	30,	"Assamese"},
  {"asm",  	31,	"Assamese"},
  {"ath",  	32,	"Athapascan languages"},
  {"ava",  	33,	"Avaric"},
  {"ave",  	34,	"Avestan"},
  {"awa",  	35,	"Awandhi"},
  {"ay",  	36,	"Aymara"},
  {"aym",  	37,	"Aymara"},
  {"az",  	38,	"Azerbaijani"},
  {"aze",  	39,	"Azerbaijani"},
  {"ba",  	40,	"Bashkir"},
  {"bad",  	41,	"Banda"},
  {"bai",  	42,	"Bamileke languages"},
  {"bak",  	43,	"Bashkir"},
  {"bal",  	44,	"Baluchi"},
  {"bam",  	45,	"Bambara"},
  {"ban",  	46,	"Balinese"},
  {"baq",  	47,	"Basque"},
  {"bas",  	48,	"Basa"},
  {"bat",  	49,	"Baltic"},
  {"be",  	50,	"Byelorussian"},
  {"bej",  	51,	"Beja"},
  {"bel",  	52,	"Byelorussian"},
  {"bem",  	53,	"Bemba"},
  {"ben",  	54,	"Bengali"},
  {"ber",  	55,	"Berber languages"},
  {"bg",  	56,	"Bulgarian"},
  {"bh",  	57,	"Bihari"},
  {"bho",  	58,	"Bhojpuri"},
  {"bi",  	59,	"Bislama"},
  {"bih",  	60,	"Bihari"},
  {"bik",  	61,	"Bikol"},
  {"bin",  	62,	"Bini"},
  {"bis",  	63,	"Bislama"},
  {"bla",  	64,	"Siksika"},
  {"bn",  	65,	"Bengali; Bangla"},
  {"bo",  	66,	"Tibetan"},
  {"bod",  	67,	"Tibetan"},
  {"br",  	68,	"Breton"},
  {"bra",  	69,	"Braj"},
  {"bre",  	70,	"Breton"},
  {"bug",  	71,	"Buginese"},
  {"bul",  	72,	"Bulgarian"},
  {"bur",  	73,	"Burmese"},
  {"ca",  	74,	"Catalan"},
  {"cad",  	75,	"Caddo"},
  {"cai",  	76,	"Central American Indian"},
  {"car",  	77,	"Carib"},
  {"cat",  	78,	"Catalan"},
  {"cau",  	79,	"Caucasian"},
  {"ceb",  	80,	"Cebuano"},
  {"cel",  	81,	"Celtic"},
  {"ces",  	82,	"Czeck"},
  {"cha",  	83,	"Chamorro"},
  {"chb",  	84,	"Chibcha"},
  {"che",  	85,	"Chechen"},
  {"chg",  	86,	"Chagatai"},
  {"chi",  	87,	"Chinese"},
  {"chn",  	88,	"Chinook jargon"},
  {"cho",  	89,	"Choctaw"},
  {"chr",  	90,	"Cherokee"},
  {"chu",  	91,	"Church Slavic"},
  {"chv",  	92,	"Chuvash"},
  {"chy",  	93,	"Cheyenne"},
  {"co",  	94,	"Corsican"},
  {"cop",  	95,	"Coptic"},
  {"cor",  	96,	"Cornish"},
  {"cos",  	97,	"Corsican"},
  {"cpe",  	98,	"English-Creoles and pidgins"},
  {"cpf",  	99,	"French-Creoles and pidgins"},
  {"cpp",  	100,	"Portuguese-Creoles and pidgins"},
  {"cre",  	101,	"Cree"},
  {"crp",  	102,	"Misc-Creoles and pidgins"},
  {"cs",  	103,	"Czech"},
  {"cus",  	104,	"Cushitic"},
  {"cy",  	105,	"Welsh"},
  {"cym",  	106,	"Welsh"},
  {"cze",  	107,	"Czech"},
  {"da",  	108,	"Dansk"},
  {"dak",  	109,	"Dakota"},
  {"dan",  	110,	"Dansk"},
  {"de",  	111,	"Deutsch"},
  {"de-AT",  	112,	"Austrian German"},
  {"de-CH",  	113,	"Swiss-German"},
  {"de-DE",  	114,	"German (Germany)"},
  {"del",  	115,	"Delaware"},
  {"deu",  	116,	"Deutsch"},
  {"din",  	117,	"Dinka"},
  {"doi",  	118,	"Dogri"},
  {"dra",  	119,	"Dravidian"},
  {"dua",  	120,	"Duala"},
  {"dum",  	121,	"Middle-Dutch"},
  {"dut",  	122,	"Dutch"},
  {"dyu",  	123,	"Dyula"},
  {"dz",  	124,	"Bhutani"},
  {"dzo",  	125,	"Dzongkha"},
  {"efi",  	126,	"Efik"},
  {"egy",  	127,	"Egyptian"},
  {"eka",  	128,	"Ekajuk"},
  {"el",  	129,	"Greek"},
  {"ell",  	130,	"Modern-Greek"},
  {"elx",  	131,	"Elamite"},
  {"en",  	132,	"English"},
  {"en-AU",  	133,	"English (Australia)"},
  {"en-CA",  	134,	"Canadian-English"},
  {"en-GB",  	135,	"British-English"},
  {"en-IE",  	136,	"Irish-English"},
  {"en-NZ",  	137,	"English (New Zealand)"},
  {"en-UK",  	138,	"UK English"},
  {"en-US",  	139,	"US English"},
  {"en-cokney", 140,	"Cokney-English"},
  {"eng",  	141,	"English"},
  {"enm",  	142,	"Middle-English"},
  {"eo",  	143,	"Esperanto"},
  {"epo",  	144,	"Esperanto"},
  {"es",  	145,	"Español"},
  {"es-AR",  	146,	"Spanish (Argentina)"},
  {"es-BO",  	147,	"Spanish (Bolivia)"},
  {"es-CL",  	148,	"Spanish (Chile)"},
  {"es-CO",  	149,	"Spanish (Colombia)"},
  {"es-CR",  	150,	"Spanish (Costa Rica)"},
  {"es-EC",  	151,	"Spanish (Ecuador)"},
  {"es-ES",  	152,	"Spanish (Spain)"},
  {"es-GT",  	153,	"Spanish (Guatemala)"},
  {"es-MX",  	154,	"Spanish (Mexico)"},
  {"es-NI",  	155,	"Spanish (Nicaragua)"},
  {"es-PA",  	156,	"Spanish (Panama)"},
  {"es-PE",  	157,	"Spanish (Peru)"},
  {"es-PY",  	158,	"Spanish (Paraguay)"},
  {"es-SV",  	159,	"Spanish (El Salvador)"},
  {"es-UY",  	160,	"Spanish (Uruguay)"},
  {"es-VE",  	161,	"Spanish (Venezuela)"},
  {"esk",  	162,	"Eskimo"},
  {"esl",  	163,	"Español"},
  {"est",  	164,	"Estonian"},
  {"et",  	165,	"Estonian"},
  {"eth",  	166,	"Ethiopic"},
  {"eu",  	167,	"Basque"},
  {"eus",  	168,	"Basque"},
  {"ewe",  	169,	"Ewe"},
  {"ewo",  	170,	"Ewondo"},
  {"fa",  	171,	"Persian"},
  {"fan",  	172,	"Fang"},
  {"fao",  	173,	"Faroese"},
  {"fas",  	174,	"Persian"},
  {"fat",  	175,	"Fanti"},
  {"fi",  	176,	"Finnish"},
  {"fij",  	177,	"Fijian"},
  {"fin",  	178,	"Finnish"},
  {"fiu",  	179,	"Finno-Ugrian"},
  {"fj",  	180,	"Fiji"},
  {"fo",  	181,	"Faeroese"},
  {"fon",  	182,	"Fon"},
  {"fr",  	183,	"Francais"},
  {"fr-BE",  	184,	"French (Belgium)"},
  {"fr-CA",  	185,	"French (Canadian)"},
  {"fr-CH",  	186,	"French (Swiss)"},
  {"fr-FR",  	187,	"French (France)"},
  {"fra",  	188,	"Francais"},
  {"fre",  	189,	"Francais"},
  {"frm",  	190,	"Middle-French"},
  {"fro",  	191,	"Old-French"},
  {"fry",  	192,	"Friesian"},
  {"ful",  	193,	"Fulah"},
  {"fy",  	194,	"Frisian"},
  {"ga",  	195,	"Irish"},
  {"gaa",  	196,	"Ga"},
  {"gae",  	197,	"Gaelic"},
  {"gai",  	198,	"Irish"},
  {"gay",  	199,	"Gayo"},
  {"gd",  	200,	"Gaelic"},
  {"gdh",  	201,	"Gaelic"},
  {"gem",  	202,	"Germanic"},
  {"geo",  	203,	"Georgian"},
  {"ger",  	204,	"Deutsch"},
  {"gil",  	205,	"Gilbertese"},
  {"gl",  	206,	"Galician"},
  {"glg",  	207,	"Gallegan"},
  {"gmh",  	208,	"Middle-High-German"},
  {"gn",  	209,	"Guamni"},
  {"goh",  	210,	"Old-High-German"},
  {"gon",  	211,	"Gondi"},
  {"got",  	212,	"Gothic"},
  {"grb",  	213,	"Grebo"},
  {"grc",  	214,	"Ancient-Greek"},
  {"gre",  	215,	"Modern-Greek"},
  {"grn",  	216,	"Guarani"},
  {"gu",  	217,	"Gujarati"},
  {"guj",  	218,	"Gujarati"},
  {"ha",  	219,	"Hausa"},
  {"hai",  	220,	"Haida"},
  {"hau",  	221,	"Hausa"},
  {"haw",  	222,	"Hawaiian"},
  {"he",  	223,	"Hebrew"},
  {"heb",  	224,	"Hebrew"},
  {"her",  	225,	"Herero"},
  {"hi",  	226,	"Hindi"},
  {"hil",  	227,	"Hiligaynon"},
  {"him",  	228,	"Himachali"},
  {"hin",  	229,	"Hindi"},
  {"hmo",  	230,	"Hiri Motu"},
  {"hr",  	231,	"Croatian"},
  {"hu",  	232,	"Hungarian"},
  {"hun",  	233,	"Hungarian"},
  {"hup",  	234,	"Hupa"},
  {"hy",  	235,	"Armenian"},
  {"hye",  	236,	"Armenian"},
  {"i-sami-no", 237,	"North Sami (Norway)"},
  {"ia",  	238,	"Interlingua"},
  {"iba",  	239,	"Iban"},
  {"ibo",  	240,	"Igbo"},
  {"ice",  	241,	"Icelandic"},
  {"id",  	242,	"Indonesian"},
  {"ie",  	243,	"lnteriingue"},
  {"ijo",  	244,	"Ijo"},
  {"ik",  	245,	"Knupiak"},
  {"iku",  	246,	"Inuktitut"},
  {"ile",  	247,	"Interlingue"},
  {"ilo",  	248,	"Iloko"},
  {"in",  	249,	"Indonesian"},
  {"ina",  	250,	"Interlingua (International Auxilary Language Association)"},
  {"inc",  	251,	"Indic"},
  {"ind",  	252,	"Indonesian"},
  {"ine",  	253,	"Indo-European"},
  {"ipk",  	254,	"Inupiak"},
  {"ira",  	255,	"Iranian"},
  {"iri",  	256,	"Irish"},
  {"iro",  	257,	"Iroquoian languages"},
  {"is",  	258,	"Icelandic"},
  {"isl",  	259,	"Icelandic"},
  {"it",  	260,	"Italiano"},
  {"it-CH",  	261,	"Italian (Swiss)"},
  {"it-IT",  	262,	"Italian (Italy)"},
  {"ita",  	263,	"Italiano"},
  {"iu",  	264,	"Inuktitut"},
  {"iw",  	265,	"Hebrew"},
  {"ja",  	266,	"Japanese"},
  {"jav",  	267,	"Javanese"},
  {"jaw",  	268,	"Javanese"},
  {"ji",  	269,	"Yiddish"},
  {"jpn",  	270,	"Japanese"},
  {"jpr",  	271,	"Judeo-Persian"},
  {"jrb",  	272,	"Judeo-Arabic"},
  {"jw",  	273,	"Javanese"},
  {"ka",  	274,	"Georgian"},
  {"kaa",  	275,	"Kara-Kalpak"},
  {"kab",  	276,	"Kabyle"},
  {"kac",  	277,	"Kachin"},
  {"kal",  	278,	"Greenlandic"},
  {"kam",  	279,	"Kamba"},
  {"kan",  	280,	"Kannada"},
  {"kar",  	281,	"Karen"},
  {"kas",  	282,	"Kashmiri"},
  {"kat",  	283,	"Georgian"},
  {"kau",  	284,	"Kanuri"},
  {"kaw",  	285,	"Kawi"},
  {"kaz",  	286,	"Kazakh"},
  {"kha",  	287,	"Khasi"},
  {"khi",  	288,	"Khoisan"},
  {"khm",  	289,	"Khmer"},
  {"kho",  	290,	"Khotanese"},
  {"kik",  	291,	"Kikuyu"},
  {"kin",  	292,	"Kinyarwanda"},
  {"kir",  	293,	"Kirghiz"},
  {"kk",  	294,	"Kazakh"},
  {"kl",  	295,	"Greaenlandic"},
  {"km",  	296,	"Cambodian"},
  {"kn",  	297,	"Kannada"},
  {"ko",  	298,	"Korean"},
  {"kok",  	299,	"Konkani"},
  {"kon",  	300,	"Kongo"},
  {"kor",  	301,	"Korean"},
  {"kpe",  	302,	"Kpelle"},
  {"kro",  	303,	"Kru"},
  {"kru",  	304,	"Kurukh"},
  {"ks",  	305,	"Kashmiri"},
  {"ku",  	306,	"Kurdish"},
  {"kua",  	307,	"Kuanyama"},
  {"kur",  	308,	"Kurdish"},
  {"kus",  	309,	"Kusaie"},
  {"kut",  	310,	"Kutenai"},
  {"ky",  	311,	"Kirghiz"},
  {"la",  	312,	"Latin"},
  {"lad",  	313,	"Ladino"},
  {"lah",  	314,	"Lahnda"},
  {"lam",  	315,	"Lamba"},
  {"lao",  	316,	"Lao"},
  {"lap",  	317,	"Lapp languages"},
  {"lat",  	318,	"Latin"},
  {"lav",  	319,	"Latvian"},
  {"lin",  	320,	"Lingala"},
  {"lit",  	321,	"Lithuanian"},
  {"ln",  	322,	"Lingala"},
  {"lo",  	323,	"Laothian"},
  {"lol",  	324,	"Mongo"},
  {"loz",  	325,	"Lozi"},
  {"lt",  	326,	"Lithuainnian"},
  {"lub",  	327,	"Luba-Katanga"},
  {"lug",  	328,	"Ganda"},
  {"lui",  	329,	"Luiseno"},
  {"lun",  	330,	"Lunda"},
  {"luo",  	331,	"Luo"},
  {"lv",  	332,	"Lettish"},
  {"mac",  	333,	"Macedonian"},
  {"mad",  	334,	"Madurese"},
  {"mag",  	335,	"Magahi"},
  {"mah",  	336,	"Marshall"},
  {"mai",  	337,	"Maithili"},
  {"mak",  	338,	"Makasar"},
  {"mal",  	339,	"Malayalam"},
  {"man",  	340,	"Mandingo"},
  {"mao",  	341,	"Maori"},
  {"map",  	342,	"Austronesian"},
  {"mar",  	343,	"Marathi"},
  {"mas",  	344,	"Masai"},
  {"max",  	345,	"Manx"},
  {"may",  	346,	"Malay"},
  {"men",  	347,	"Mende"},
  {"mg",  	348,	"Malagasy"},
  {"mi",  	349,	"Maori"},
  {"mic",  	350,	"Micmac"},
  {"min",  	351,	"Minangkabau"},
  {"mis",  	352,	"Miscellaneous"},
  {"mk",  	353,	"Macedonian"},
  {"mke",  	354,	"Macedonian"},
  {"mkh",  	355,	"Mon-Khmer"},
  {"ml",  	356,	"Malayalam"},
  {"mlg",  	357,	"Malagasy"},
  {"mlt",  	358,	"Maltese"},
  {"mn",  	359,	"Mongolian"},
  {"mni",  	360,	"Manipuri"},
  {"mno",  	361,	"Manobo languages"},
  {"mo",  	362,	"Moldavian"},
  {"moh",  	363,	"Mohawk"},
  {"mol",  	364,	"Moldavian"},
  {"mon",  	365,	"Mongolian"},
  {"mos",  	366,	"Mossi"},
  {"mr",  	367,	"Marathi"},
  {"mri",  	368,	"Maori"},
  {"ms",  	369,	"Malay"},
  {"msa",  	370,	"Malay"},
  {"mt",  	371,	"Maltese"},
  {"mul",  	372,	"Multiple languages"},
  {"mun",  	373,	"Munda"},
  {"mus",  	374,	"Creek"},
  {"mwr",  	375,	"Marwari"},
  {"my",  	376,	"Burmese"},
  {"mya",  	377,	"Burmese"},
  {"myn",  	378,	"Mayan languages"},
  {"na",  	379,	"Nauru"},
  {"nah",  	380,	"Aztec"},
  {"nai",  	381,	"North American Indian"},
  {"nau",  	382,	"Nauru"},
  {"nav",  	383,	"Navajo"},
  {"nde",  	384,	"Ndebele"},
  {"ndo",  	385,	"Ndonga"},
  {"ne",  	386,	"Nepali"},
  {"nep",  	387,	"Nepali"},
  {"new",  	388,	"Newari"},
  {"nic",  	389,	"Niger-Kordofanian"},
  {"niu",  	390,	"Niuean"},
  {"nl",  	391,	"Dutch"},
  {"nl-BE",  	392,	"Dutch (Belgium)"},
  {"nl-NL",  	393,	"Dutch (Holland)"},
  {"nld",  	394,	"Dutch"},
  {"no",  	395,	"Norwegian"},
  {"no-bok",  	396,	"Norwegian Book Language"},
  {"no-nyn",  	397,	"Norwegian New Norwegian"},
  {"non",  	398,	"Old-Norse"},
  {"nor",  	399,	"Norwegian"},
  {"nso",  	400,	"Northern Sohto"},
  {"nub",  	401,	"Nubian languages"},
  {"nya",  	402,	"Nyanja"},
  {"nym",  	403,	"Nyamwezi"},
  {"nyn",  	404,	"Nyankole"},
  {"nyo",  	405,	"Nyoro"},
  {"nzi",  	406,	"Nzima"},
  {"oc",  	407,	"Occitan"},
  {"oci",  	408,	"Langue d'oc"},
  {"oji",  	409,	"Ojibwa"},
  {"om",  	410,	"Afan Oromo"},
  {"or",  	411,	"Oriya"},
  {"ori",  	412,	"Oriya"},
  {"orm",  	413,	"Oromo"},
  {"osa",  	414,	"Osage"},
  {"oss",  	415,	"Ossetic"},
  {"ota",  	416,	"Ottoman-Turkish"},
  {"oto",  	417,	"Otomian languages"},
  {"pa",  	418,	"Punjabi"},
  {"paa",  	419,	"Papuan-Australian"},
  {"pag",  	420,	"Pangasinan"},
  {"pal",  	421,	"Pahlavi"},
  {"pam",  	422,	"Pampanga"},
  {"pan",  	423,	"Panjabi"},
  {"pap",  	424,	"Papiamento"},
  {"pau",  	425,	"Palauan"},
  {"peo",  	426,	"Old-Persian"},
  {"per",  	427,	"Persian"},
  {"pl",  	428,	"Polish"},
  {"pli",  	429,	"Pali"},
  {"pol",  	430,	"Polish"},
  {"pon",  	431,	"Ponape"},
  {"por",  	432,	"Portuguese"},
  {"pra",  	433,	"Prakrit languages"},
  {"pro",  	434,	"Old-Provencal"},
  {"ps",  	435,	"Pushto"},
  {"pt",  	436,	"Portuguese"},
  {"pt-BR",  	437,	"Portuguese (Brazil)"},
  {"pt-PT",  	438,	"Portuguese (Portugal)"},
  {"pus",  	439,	"Pushto"},
  {"qu",  	440,	"Ouechua"},
  {"que",  	441,	"Quechua"},
  {"raj",  	442,	"Rajasthani"},
  {"rar",  	443,	"Rarotongan"},
  {"rm",  	444,	"Rhaeto-Romance"},
  {"rn",  	445,	"Kirundi"},
  {"ro",  	446,	"Romanian"},
  {"roa",  	447,	"Romance"},
  {"roh",  	448,	"Raeto-Romance"},
  {"rom",  	449,	"Romany"},
  {"ron",  	450,	"Romanian"},
  {"ru",  	451,	"Russian"},
  {"rum",  	452,	"Romanian"},
  {"run",  	453,	"Rundi"},
  {"rus",  	454,	"Russian"},
  {"rw",  	455,	"Kinya"},
  {"sa",  	456,	"Sanskrit"},
  {"sad",  	457,	"Sandawe"},
  {"sag",  	458,	"Sango"},
  {"sai",  	459,	"South American Indian"},
  {"sal",  	460,	"Salishan"},
  {"sam",  	461,	"Samaritan Aramaic"},
  {"san",  	462,	"Sanskrit"},
  {"sco",  	463,	"Scots"},
  {"scr",  	464,	"Serbo-Croatian"},
  {"sd",  	465,	"Sindhi"},
  {"sel",  	466,	"Selkup"},
  {"sem",  	467,	"Semitic"},
  {"sg",  	468,	"Sangro"},
  {"sh",  	469,	"Serbo-Croatian"},
  {"shn",  	470,	"Shan"},
  {"si",  	471,	"Singhalese"},
  {"sid",  	472,	"Sidamo"},
  {"sin",  	473,	"Sinhalese"},
  {"sio",  	474,	"Siouan languages"},
  {"sit",  	475,	"Sino-Tibetan"},
  {"sk",  	476,	"Slovak"},
  {"sl",  	477,	"Slovenian"},
  {"sla",  	478,	"Slavic"},
  {"slk",  	479,	"Slovak"},
  {"slo",  	480,	"Slovak"},
  {"slv",  	481,	"Slovenian"},
  {"sm",  	482,	"Samoan"},
  {"smo",  	483,	"Samoan"},
  {"sn",  	484,	"Shona"},
  {"sna",  	485,	"Shona"},
  {"snd",  	486,	"Sindhi"},
  {"so",  	487,	"Somali"},
  {"sog",  	488,	"Sogdian"},
  {"som",  	489,	"Somali"},
  {"son",  	490,	"Songhai"},
  {"sot",  	491,	"Sotho"},
  {"spa",  	492,	"Español"},
  {"sq",  	493,	"Albanian"},
  {"sqi",  	494,	"Albanian"},
  {"sr",  	495,	"Serbian"},
  {"srr",  	496,	"Serer"},
  {"ss",  	497,	"Siswati"},
  {"ssa",  	498,	"Nilo-Saharan"},
  {"ssw",  	499,	"Swazi"},
  {"st",  	500,	"Sesotho"},
  {"su",  	501,	"Sundanese"},
  {"suk",  	502,	"Sukuma"},
  {"sun",  	503,	"Sundanese"},
  {"sus",  	504,	"Susu"},
  {"sux",  	505,	"Sumerian"},
  {"sv",  	506,	"Svenska"},
  {"sve",  	507,	"Svenska"},
  {"sw",  	508,	"Swahili"},
  {"swa",  	509,	"Swahili"},
  {"swe",  	510,	"Svenska"},
  {"syr",  	511,	"Syriac"},
  {"ta",  	512,	"Tamil"},
  {"tah",  	513,	"Tahitian"},
  {"tam",  	514,	"Tamil"},
  {"tat",  	515,	"Tatar"},
  {"te",  	516,	"Tegulu"},
  {"tel",  	517,	"Telugu"},
  {"tem",  	518,	"Timne"},
  {"ter",  	519,	"Tereno"},
  {"tg",  	520,	"Tajik"},
  {"tgk",  	521,	"Tajik"},
  {"tgl",  	522,	"Tagalog"},
  {"th",  	523,	"Thai"},
  {"tha",  	524,	"Thai"},
  {"ti",  	525,	"Tigrinya"},
  {"tib",  	526,	"Tibetan"},
  {"tig",  	527,	"Tigre"},
  {"tir",  	528,	"Tigrinya"},
  {"tiv",  	529,	"Tivi"},
  {"tk",  	530,	"Turkmen"},
  {"tl",  	531,	"Tagalog"},
  {"tli",  	532,	"Tlingit"},
  {"tn",  	533,	"Setswana"},
  {"to",  	534,	"Tonga"},
  {"tog",  	535,	"Nyasa-Tonga"},
  {"ton",  	536,	"Tonga"},
  {"tr",  	537,	"Turkish"},
  {"tru",  	538,	"Truk"},
  {"ts",  	539,	"Tsonga"},
  {"tsi",  	540,	"Tsimshian"},
  {"tsn",  	541,	"Tswana"},
  {"tso",  	542,	"Tsonga"},
  {"tt",  	543,	"Tatar"},
  {"tuk",  	544,	"Turkmen"},
  {"tum",  	545,	"Tumbuka"},
  {"tur",  	546,	"Turkish"},
  {"tut",  	547,	"Altaic"},
  {"tw",  	548,	"Twi"},
  {"twi",  	549,	"Twi"},
  {"ug",  	550,	"Uigur"},
  {"uga",  	551,	"Ugaritic"},
  {"uig",  	552,	"Uighur"},
  {"uk",  	553,	"Ukrainian"},
  {"ukr",  	554,	"Ukrainian"},
  {"umb",  	555,	"Umbundu"},
  {"und",  	556,	"Undetermined"},
  {"ur",  	557,	"Urdu"},
  {"urd",  	558,	"Urdu"},
  {"us",  	559,	"American"},
  {"uz",  	560,	"Uzbek"},
  {"uzb",  	561,	"Uzbek"},
  {"vai",  	562,	"Vai"},
  {"ven",  	563,	"Venda"},
  {"vi",  	564,	"Vietnamese"},
  {"vie",  	565,	"Vietnamese"},
  {"vo",  	566,	"Volapuek"},
  {"vol",  	567,	"Volapuk"},
  {"vot",  	568,	"Votic"},
  {"wak",  	569,	"Wakashan languages"},
  {"wal",  	570,	"Walamo"},
  {"war",  	571,	"Waray"},
  {"was",  	572,	"Washo"},
  {"wel",  	573,	"Welsh"},
  {"wen",  	574,	"Sorbian languages"},
  {"wo",  	575,	"Wolof"},
  {"wol",  	576,	"Wolof"},
  {"x-klingon", 577,	"Klingon (TV series Star Trek)"},
  {"xh",  	578,	"Xhosa"},
  {"xho",  	579,	"Xhosa"},
  {"yao",  	580,	"Yao"},
  {"yap",  	581,	"Yap"},
  {"yi",  	582,	"Yiddish"},
  {"yid",  	583,	"Yiddish"},
  {"yo",  	584,	"Yoruba"},
  {"yo",  	585,	"Yoruba"},
  {"yor",  	586,	"Yoruba"},
  {"za",  	587,	"Zhuang"},
  {"zap",  	588,	"Zapotec"},
  {"zen",  	589,	"Zenaga"},
  {"zh",  	590,	"Chinese"},
  {"zha",  	591,	"Zhuang"},
  {"zho",  	592,	"Chinese"},
  {"zu",  	593,	"Zulu"},
  {"zul",  	594,	"Zulu"},
  {"zun",  	595,	"Zuni"},
  {"zz",  	596,  	"Misc"},
  {"zzz",  	597,	"Unknown"}
#define DEFAULT_LANGUAGE	597 /* None */
#define MAX_LANGUAGE            597
};


size_t GetLangCodes(char ***codes, char ***values)
{
  static char **Codes = NULL;
  static char **Values = NULL;
  static size_t i = 0;
  if (i == 0)
    {
      Codes = new char *[ sizeof(ISO_Languages)/sizeof(ISO_Languages[0]) + 1 ];
      Values = new char *[ sizeof(ISO_Languages)/sizeof(ISO_Languages[0]) + 1 ];
      for (; i < sizeof(ISO_Languages)/sizeof(ISO_Languages[0]); i++)
	{
	  Codes[i]  = (char *)ISO_Languages[i].code;
          Values[i] = (char *)ISO_Languages[i].lang;
	}
      Codes[i] = Values[i] = NULL;
    }
  *codes = Codes;
  *values = Values;
  return i;
}


static int LangCompareKeys(const void *node1, const void *node2)
{
  return (strcmp(((const struct _lang *)node1)->code,
	((const struct _lang *)node2)->code));
}

#if 0 /* NOT USED */
static int LanguageCompareKeys(const void *node1, const void *node2)
{
  return (strcasecmp(((const struct _lang *)node1)->lang,
        ((const struct _lang *)node2)->lang));
}
#endif

static int AliasesCompareKeys(const void *node1, const void *node2)
{
  return (strcmp(((const struct _aliases *)node1)->locale,
        ((const struct _aliases *)node2)->locale));
}

static const char *Locale2Cannon(const char *name)
{
  if (name && *name)
    {
      struct _aliases n;
      struct _aliases *t;
      n.locale = name;
      t = (struct _aliases *) bsearch(&n, LocaleAliases,
        sizeof(LocaleAliases)/sizeof(LocaleAliases[0]), sizeof(LocaleAliases[0]),
        AliasesCompareKeys);
      if (t)
        return t->code;
    }
  return name;
}

const char *Locale2Lang(const char *name)
{
  const char *res = Locale2Cannon(name);
  return Id2Lang (  Lang2Id ( res ) );
}

SHORT Locale2Id(const char *name)
{
  return Lang2Id (Locale2Lang (name) );
}

SHORT Lang2Id (const STRING& name)
{
  return Lang2Id ((const char *)name);
}


SHORT Lang2Id (const char *name)
{
  static  int    default_id = -1;
  struct _lang  *t;
  struct _lang   node = {"", 0, ""};
  char           tmp[30];
  int            id = 0;

  if (name == NULL || *name == '\0')
    {
      if (default_id == -1)
	{
	  char *tcp = getenv("LANG");
	  if (tcp && strcmp("C", tcp) != 0)
	    {
	      if ((tcp = (char *)Locale2Lang(tcp)) != NULL)
		{
		  strcpy (tmp, tcp);
		  if ((tcp = strchr(tmp, '.')) != NULL)
		    *tcp = '\0';
		  node.code = tmp;
		}
	      else
		node.code = "und";
	    }
	  else
	    node.code = "en";
	}
      else
	return default_id; // Have a global
    }
  else
    {
      strncpy(tmp, name, sizeof(tmp));
      tmp[sizeof(tmp)-1] = '\0';
      for (char *tcp = tmp; *tcp; tcp++)
	{
	  if (*tcp == '.' || *tcp == ' ' || *tcp == '\t' || *tcp == '\n')
	    {
	      *tcp = '\0';
	      break;
	    }
	}
      node.code = tmp;
    }

  if ((t = (struct _lang *) bsearch(&node, ISO_Languages,
	sizeof(ISO_Languages)/sizeof(ISO_Languages[0]), sizeof(ISO_Languages[0]),
	LangCompareKeys)) != NULL)
    {
      id = t->num;
      if (default_id == -1)
	default_id = id;
    }
  return id;
}

SHORT Language2Id (const char *name)
{
  if (name == NULL || *name == '\0')
    return Lang2Id(name);

  // ISO 2 or 3 code specified or xx-yy format?
  if (strlen(name) <= 3 || name[2] == '-')
    return Lang2Id(name);
  for (size_t i=0; i< sizeof(ISO_Languages)/sizeof(ISO_Languages[0]); i++)
    {
      if (strcasecmp(name, ISO_Languages[i].lang) == 0)
	return i + 1;
    }
  return 0; // NOT FOUND
}


SHORT LangX2Id (const char *Language)
{
  SHORT id;

  if ((id = Lang2Id(Language)) == 0)
    if ((id = Language2Id(Language)) == 0)
      id = DEFAULT_LANGUAGE;
  return id;
}

SHORT LangX2Id (const STRING& Language)
{
  return LangX2Id((const char *)Language);
}

const char *Id2Lang (const SHORT Code)
{
  if (Code > 0 && (size_t)Code <= sizeof(ISO_Languages)/sizeof(ISO_Languages[0]))
    return ISO_Languages[Code-1].code;
  return ISO_Languages[MAX_LANGUAGE-1].code; 
}

const char *Id2Language (const SHORT Code)
{
  if (Code > 0 && (size_t)Code <= sizeof(ISO_Languages)/sizeof(ISO_Languages[0]))
    return ISO_Languages[Code-1].lang;
  return "Unknown Language"; 
}

const char *Lang2Language (const STRING& name)
{
  return Id2Language ( Lang2Id(name) );
}


const char *Lang2Language (const char *name)
{
  return Id2Language ( Lang2Id(name) );
}

 // First is official, third is Registry, rest are aliases
static const char *iso_8859_1[] = {"iso-8859-1",  "8859-1", "ISO_8859-1:1987",
	"ISO8859-1",  "ISOLatin1",       "latin-1",  "iso_8859_1",  NULL};
static const char *iso_8859_2[] = {"iso-8859-2",  "8859-2", "ISO_8859-2:1987",
	"ISO8859-2",  "ISOLatin2",       "latin-2",  "iso_8859_2",  NULL};
static const char *iso_8859_3[] = {"iso-8859-3",  "8859-3", "ISO_8859-3:1987",
	"ISO8859-3",  "ISOLatin3",       "latin-3",  "iso_8859_3",  NULL};
static const char *iso_8859_4[] = {"iso-8859-4",  "8859-4", "ISO_8859-4:1987",
	"ISO8859-4",  "ISOLatin4",       "latin-4",  "iso_8859_4",  NULL};
static const char *iso_8859_5[] = {"iso-8859-5",  "8859-5", "ISO_8859-5:1987",
	"ISO8859-5",  "ISOLatinCryllic", "cryllic",  "iso_8859_5",  NULL};
static const char *iso_8859_6[] = {"iso-8859-6",  "8859-6", "ISO_8859-6:1987",
	"ISO8859-6",  "ISOLatinArabic",  "arabic",   "iso_8859_6",  NULL};
static const char *iso_8859_7[] = {"iso-8859-7",  "8859-7", "ISO_8859-7:1987",
	"ISO8859-7",  "ISOLatinGreek",   "greek",    "iso_8859_7",  NULL};
static const char *iso_8859_8[] = {"iso-8859-8",  "8859-8", "ISO_8859-8:1987",
	"ISO8859-8",  "ISOLatinHebrew",  "hebrew",   "iso_8859_8",  NULL};
static const char *iso_8859_9[] = {"iso-8859-9",  "8859-9", "ISO_8859-9:1987",
	"ISO8859-9",  "ISOLatin5",       "latin-5",  "iso_8859_9",  NULL};
static const char *iso_8859_10[]= {"iso-8859-10", "8859-10", "ISO_8859-10:1987",
	"ISO8859-10", "ISOLatin6",       "latin-6",  "iso_8859_10", NULL};
static const char *iso_8859_11[]= {"iso-8859-11", "8859-11", "ISO_8859-11:1987",
        "ISO8859-11", "ISOThai",       "thai",  "iso_8859_11", NULL};
static const char *iso_8859_12[]= {"iso-8859-12", "8859-12", "ISO_8859-12:1987",
        "ISO8859-12", "iso_8859_12", NULL};
static const char *iso_8859_13[]= {"iso-8859-13", "8859-13", "ISO_8859-13:1987",
        "ISO8859-13", "ISOLatin7",       "latin-7",  "iso_8859_13", NULL};
static const char *iso_8859_14[]= {"iso-8859-14", "8859-14", "ISO_8859-14:1987",
        "ISO8859-14", "ISOLatin8",       "latin-8",  "iso_8859_14", NULL};
static const char *iso_8859_15[]= {"iso-8859-15", "8859-16", "ISO_8859-15:1987",
        "ISO8859-15", "ISOLatin9",       "latin-9",  "iso_8859_15", NULL};
static const char *iso_8859_16[]= {"iso-8859-16", "8859-16", "ISO_8859-16:1987",
        "ISO8859-16", "ISOLatin10",     "latin-10",  "iso_8859_16", NULL};
static const char *usascii[]    = {"us-ascii",     "ascii",   "ANSI_X3.4-1968",
	"ISO646-US", "usascii",      "C",           NULL};

// SPECIAL
static const char *unicode_UTF[]    = {"utf-8", "utf8", "ISO_10646-1/AnnexD:2000", "ISO_10646-UTF", "utf", NULL};
static const BYTE UTF8 = 128;


  // Map locale to cannonical
static const struct {
  BYTE Id;
  const char **names;
} charsets[] = {
  {0,  usascii},
  {1,  iso_8859_1},
  {2,  iso_8859_2},
  {3,  iso_8859_3},
  {4,  iso_8859_4},
  {5,  iso_8859_5},
  {6,  iso_8859_6},
  {7,  iso_8859_7},
  {8,  iso_8859_8},
  {9,  iso_8859_9},
  {10, iso_8859_10},
  {11, iso_8859_11},
  {12, iso_8859_12},
  {13, iso_8859_13},
  {14, iso_8859_14},
  {15, iso_8859_15},
  {16, iso_8859_16},
  {UTF8, unicode_UTF}
};

static const INT MAX_CHARSET_ID=16;

BYTE Charset2Id (const char *name)
{
  BYTE set = 1; // ISO 8859-1

  const char *lc_ctype = name ? name : getenv("LANG");

  if (lc_ctype == NULL)
    {
      if ((lc_ctype = getenv("LC_CTYPE")) == NULL)
#ifdef HAVE_LOCALE
	lc_ctype = setlocale(LC_CTYPE,NULL);
#else
	return set;
#endif
    }
  if ((set = Charset2Id ((STRING)lc_ctype)) != 0xFF)
    {
      return set;
    }
  // Not one of these...
  if ((lc_ctype = Locale2Cannon(lc_ctype)) != NULL)
    {
      const char *t = (const char *)strchr(lc_ctype, '.');
      if (t != NULL)
	{
	  return Charset2Id(t+1);
	}
    }
  return set; // NOT FOUND
}

BYTE Charset2Id (const STRING& Name)
{
  for (int i=0; i <= MAX_CHARSET_ID+1; i++)
    {
      const char **tp = charsets[i].names;
      while (*tp && Name.CaseCompare(*tp))
        tp++;
      if (*tp)
	{
	  return charsets[i].Id;
	}
    }
//  logf (LOG_WARN, "Charset '%s' not supported", (const char *)Name);
  return 0xFF; // NOT FOUND
}

const CHR *Id2Charset(BYTE CharsetId)
{
  if (CharsetId == UTF8)
    return *(charsets[MAX_CHARSET_ID+1].names);
  if (CharsetId <= MAX_CHARSET_ID)
    return *(charsets[CharsetId].names); // Return Official Name
  return "Unknown";
}

static const CHR *Id2CharsetRegistry(BYTE CharsetId)
{
  if (CharsetId == UTF8)
    return (charsets[MAX_CHARSET_ID+1].names)[2]; // Return Registry Name
  if (CharsetId <= MAX_CHARSET_ID)
    return (charsets[CharsetId].names)[2]; // Return Registry Name
  return "Unknown";
}


#define UDF (UINT2)-1

// Tables mapping ASCII, ISO 8859-1 through ISO 8859-16 to ISO 10646.
static const UINT2 ISO8859[17][256]= {
// ASCII (Really same as 8859-1 
{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
 0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
 0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
 0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
 0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
 0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
 0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff},
// ISO 8859-1
{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
 0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
 0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
 0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
 0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
 0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
 0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff},
// ISO 8859-2
{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
 0x00a0,0x0104,0x02d8,0x0141,0x00a4,0x013d,0x015a,0x00a7,
 0x00a8,0x0160,0x015e,0x0164,0x0179,0x00ad,0x017d,0x017b,
 0x00b0,0x0105,0x02db,0x0142,0x00b4,0x013e,0x015b,0x02c7,
 0x00b8,0x0161,0x015f,0x0165,0x017a,0x02dd,0x017e,0x017c,
 0x0154,0x00c1,0x00c2,0x0102,0x00c4,0x0139,0x0106,0x00c7,
 0x010c,0x00c9,0x0118,0x00cb,0x011a,0x00cd,0x00ce,0x010e,
 0x0110,0x0143,0x0147,0x00d3,0x00d4,0x0150,0x00d6,0x00d7,
 0x0158,0x016e,0x00da,0x0170,0x00dc,0x00dd,0x0162,0x00df,
 0x0155,0x00e1,0x00e2,0x0103,0x00e4,0x013a,0x0107,0x00e7,
 0x010d,0x00e9,0x0119,0x00eb,0x011b,0x00ed,0x00ee,0x010f,
 0x0111,0x0144,0x0148,0x00f3,0x00f4,0x0151,0x00f6,0x00f7,
 0x0159,0x016f,0x00fa,0x0171,0x00fc,0x00fd,0x0163,0x02d9},
// ISO 8859-3
{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
 0x00a0,0x0126,0x02d8,0x00a3,0x00a4,    UDF,0x0124,0x00a7,
 0x00a8,0x0130,0x015e,0x011e,0x0134,0x00ad,    UDF,0x017b,
 0x00b0,0x0127,0x00b2,0x00b3,0x00b4,0x00b5,0x0125,0x00b7,
 0x00b8,0x0131,0x015f,0x011f,0x0135,0x00bd,    UDF,0x017c,
 0x00c0,0x00c1,0x00c2,    UDF,0x00c4,0x010a,0x0108,0x00c7,
 0x00c8,0x00c9,0x00ca,0x00cb,0x00cc,0x00cd,0x00ce,0x00cf,
     UDF,0x00d1,0x00d2,0x00d3,0x00d4,0x0120,0x00d6,0x00d7,
 0x011c,0x00d9,0x00da,0x00db,0x00dc,0x016c,0x015c,0x00df,
 0x00e0,0x00e1,0x00e2,    UDF,0x00e4,0x010b,0x0109,0x00e7,
 0x00e8,0x00e9,0x00ea,0x00eb,0x00ec,0x00ed,0x00ee,0x00ef,
     UDF,0x00f1,0x00f2,0x00f3,0x00f4,0x0121,0x00f6,0x00f7,
 0x011d,0x00f9,0x00fa,0x00fb,0x00fc,0x016d,0x015d,0x02d9},
// ISO 8859-4
{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
 0x00a0,0x0104,0x0138,0x0156,0x00a4,0x0128,0x013b,0x00a7,
 0x00a8,0x0160,0x0112,0x0122,0x0166,0x00ad,0x017d,0x00af,
 0x00b0,0x0105,0x02db,0x0157,0x00b4,0x0129,0x013c,0x02c7,
 0x00b8,0x0161,0x0113,0x0123,0x0167,0x014a,0x017e,0x014b,
 0x0100,0x00c1,0x00c2,0x00c3,0x00c4,0x00c5,0x00c6,0x012e,
 0x010c,0x00c9,0x0118,0x00cb,0x0116,0x00cd,0x00ce,0x012a,
 0x0110,0x0145,0x014c,0x0136,0x00d4,0x00d5,0x00d6,0x00d7,
 0x00d8,0x0172,0x00da,0x00db,0x00dc,0x0168,0x016a,0x00df,
 0x0101,0x00e1,0x00e2,0x00e3,0x00e4,0x00e5,0x00e6,0x012f,
 0x010d,0x00e9,0x0119,0x00eb,0x0117,0x00ed,0x00ee,0x012b,
 0x0111,0x0146,0x014d,0x0137,0x00f4,0x00f5,0x00f6,0x00f7,
 0x00f8,0x0173,0x00fa,0x00fb,0x00fc,0x0169,0x016b,0x02d9},
// ISO 8859-5
{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
 0x00a0,0x0401,0x0402,0x0403,0x0404,0x0405,0x0406,0x0407,
 0x0408,0x0409,0x040a,0x040b,0x040c,0x00ad,0x040e,0x040f,
 0x0410,0x0411,0x0412,0x0413,0x0414,0x0415,0x0416,0x0417,
 0x0418,0x0419,0x041a,0x041b,0x041c,0x041d,0x041e,0x041f,
 0x0420,0x0421,0x0422,0x0423,0x0424,0x0425,0x0426,0x0427,
 0x0428,0x0429,0x042a,0x042b,0x042c,0x042d,0x042e,0x042f,
 0x0430,0x0431,0x0432,0x0433,0x0434,0x0435,0x0436,0x0437,
 0x0438,0x0439,0x043a,0x043b,0x043c,0x043d,0x043e,0x043f,
 0x0440,0x0441,0x0442,0x0443,0x0444,0x0445,0x0446,0x0447,
 0x0448,0x0449,0x044a,0x044b,0x044c,0x044d,0x044e,0x044f,
 0x2116,0x0451,0x0452,0x0453,0x0454,0x0455,0x0456,0x0457,
 0x0458,0x0459,0x045a,0x045b,0x045c,0x00a7,0x045e,0x045f},
// ISO 8859-6
{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
 0x00a0,    UDF,    UDF,    UDF,0x00a4,    UDF,    UDF,    UDF,
     UDF,    UDF,    UDF,    UDF,0x060c,0x00ad,    UDF,    UDF,
     UDF,    UDF,    UDF,    UDF,    UDF,    UDF,    UDF,    UDF,
     UDF,    UDF,    UDF,0x061b,    UDF,    UDF,    UDF,0x061f,
     UDF,0x0621,0x0622,0x0623,0x0624,0x0625,0x0626,0x0627,
 0x0628,0x0629,0x062a,0x062b,0x062c,0x062d,0x062e,0x062f,
 0x0630,0x0631,0x0632,0x0633,0x0634,0x0635,0x0636,0x0637,
 0x0638,0x0639,0x063a,    UDF,    UDF,    UDF,    UDF,    UDF,
 0x0640,0x0641,0x0642,0x0643,0x0644,0x0645,0x0646,0x0647,
 0x0648,0x0649,0x064a,0x064b,0x064c,0x064d,0x064e,0x064f,
 0x0650,0x0651,0x0652,    UDF,    UDF,    UDF,    UDF,    UDF,
     UDF,    UDF,    UDF,    UDF,    UDF,    UDF,    UDF,    UDF},
// ISO 8859-7
{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
 0x00a0,0x2018,0x2019,0x00a3,    UDF,    UDF,0x00a6,0x00a7,
 0x00a8,0x00a9,    UDF,0x00ab,0x00ac,0x00ad,    UDF,0x2015,
 0x00b0,0x00b1,0x00b2,0x00b3,0x0384,0x0385,0x0386,0x00b7,
 0x0388,0x0389,0x038a,0x00bb,0x038c,0x00bd,0x038e,0x038f,
 0x0390,0x0391,0x0392,0x0393,0x0394,0x0395,0x0396,0x0397,
 0x0398,0x0399,0x039a,0x039b,0x039c,0x039d,0x039e,0x039f,
 0x03a0,0x03a1,    UDF,0x03a3,0x03a4,0x03a5,0x03a6,0x03a7,
 0x03a8,0x03a9,0x03aa,0x03ab,0x03ac,0x03ad,0x03ae,0x03af,
 0x03b0,0x03b1,0x03b2,0x03b3,0x03b4,0x03b5,0x03b6,0x03b7,
 0x03b8,0x03b9,0x03ba,0x03bb,0x03bc,0x03bd,0x03be,0x03bf,
 0x03c0,0x03c1,0x03c2,0x03c3,0x03c4,0x03c5,0x03c6,0x03c7,
 0x03c8,0x03c9,0x03ca,0x03cb,0x03cc,0x03cd,0x03ce,    UDF},
// ISO 8859-8
{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
 0x00a0,    UDF,0x00a2,0x00a3,0x00a4,0x00a5,0x00a6,0x00a7,
 0x00a8,0x00a9,0x00d7,0x00ab,0x00ac,0x00ad,0x00ae,0x203e,
 0x00b0,0x00b1,0x00b2,0x00b3,0x00b4,0x00b5,0x00b6,0x00b7,
 0x00b8,0x00b9,0x00f7,0x00bb,0x00bc,0x00bd,0x00be,    UDF,
     UDF,    UDF,    UDF,    UDF,    UDF,    UDF,    UDF,    UDF,
     UDF,    UDF,    UDF,    UDF,    UDF,    UDF,    UDF,    UDF,
     UDF,    UDF,    UDF,    UDF,    UDF,    UDF,    UDF,    UDF,
     UDF,    UDF,    UDF,    UDF,    UDF,    UDF,    UDF,0x2017,
 0x05d0,0x05d1,0x05d2,0x05d3,0x05d4,0x05d5,0x05d6,0x05d7,
 0x05d8,0x05d9,0x05da,0x05db,0x05dc,0x05dd,0x05de,0x05df,
 0x05e0,0x05e1,0x05e2,0x05e3,0x05e4,0x05e5,0x05e6,0x05e7,
 0x05e8,0x05e9,0x05ea,    UDF,    UDF,    UDF,    UDF,    UDF},
// ISO 8859-9
{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
 0x00a0,0x00a1,0x00a2,0x00a3,0x00a4,0x00a5,0x00a6,0x00a7,
 0x00a8,0x00a9,0x00aa,0x00ab,0x00ac,0x00ad,0x00ae,0x00af,
 0x00b0,0x00b1,0x00b2,0x00b3,0x00b4,0x00b5,0x00b6,0x00b7,
 0x00b8,0x00b9,0x00ba,0x00bb,0x00bc,0x00bd,0x00be,0x00bf,
 0x00c0,0x00c1,0x00c2,0x00c3,0x00c4,0x00c5,0x00c6,0x00c7,
 0x00c8,0x00c9,0x00ca,0x00cb,0x00cc,0x00cd,0x00ce,0x00cf,
 0x011e,0x00d1,0x00d2,0x00d3,0x00d4,0x00d5,0x00d6,0x00d7,
 0x00d8,0x00d9,0x00da,0x00db,0x00dc,0x0130,0x015e,0x00df,
 0x00e0,0x00e1,0x00e2,0x00e3,0x00e4,0x00e5,0x00e6,0x00e7,
 0x00e8,0x00e9,0x00ea,0x00eb,0x00ec,0x00ed,0x00ee,0x00ef,
 0x011f,0x00f1,0x00f2,0x00f3,0x00f4,0x00f5,0x00f6,0x00f7,
 0x00f8,0x00f9,0x00fa,0x00fb,0x00fc,0x0131,0x015f,0x00ff},
// ISO 8859-10
{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
 0x00a0,0x0104,0x0112,0x0122,0x012a,0x0128,0x0136,0x00a7,
 0x013b,0x0110,0x0160,0x0166,0x017d,0x00ad,0x016a,0x014a,
 0x00b0,0x0105,0x0113,0x0123,0x012b,0x0129,0x0137,0x00b7,
 0x013c,0x0110,0x0161,0x0167,0x017e,0x2014,0x016b,0x014b,
 0x0100,0x00c1,0x00c2,0x00c3,0x00c4,0x00c5,0x00c6,0x012e,
 0x010c,0x00c9,0x0118,0x00cb,0x0116,0x00cd,0x00ce,0x00cf,
 0x00d0,0x0145,0x014c,0x00d3,0x00d4,0x00d5,0x00d6,0x0168,
 0x00d8,0x0172,0x00da,0x00db,0x00dc,0x00dd,0x00de,0x00df,
 0x0101,0x00e1,0x00e2,0x00e3,0x00e4,0x00e5,0x00e6,0x012f,
 0x010d,0x00e9,0x0119,0x00eb,0x0117,0x00ed,0x00ee,0x00ef,
 0x00f0,0x0146,0x014d,0x00f3,0x00f4,0x00f5,0x00f6,0x0169,
 0x00f8,0x0173,0x00fa,0x00fb,0x00fc,0x00fd,0x00fe,0x0138},
/* ISO 8859-11 Thai */
{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
 0xa0, 0xe01, 0xe02, 0xe03, 0xe04, 0xe05, 0xe06, 0xe07, 0xe08, 
 0xe09, 0xe0a, 0xe0b, 0xe0c, 0xe0d, 0xe0e, 0xe0f, 0xe10, 
 0xe11, 0xe12, 0xe13, 0xe14, 0xe15, 0xe16, 0xe17, 0xe18, 
 0xe19, 0xe1a, 0xe1b, 0xe1c, 0xe1d, 0xe1e, 0xe1f, 0xe20, 
 0xe21, 0xe22, 0xe23, 0xe24, 0xe25, 0xe26, 0xe27, 0xe28, 
 0xe29, 0xe2a, 0xe2b, 0xe2c, 0xe2d, 0xe2e, 0xe2f, 0xe30, 
 0xe31, 0xe32, 0xe33, 0xe34, 0xe35, 0xe36, 0xe37, 0xe38, 
 0xe39, 0xe3a, 0xe3b, 0xe3c, 0xe3d, 0xe3e, 0xe3f, 0xe40, 
 0xe41, 0xe42, 0xe43, 0xe44, 0xe45, 0xe46, 0xe47, 0xe48, 
 0xe49, 0xe4a, 0xe4b, 0xe4c, 0xe4d, 0xe4e, 0xe4f, 0xe50, 
 0xe51, 0xe52, 0xe53, 0xe54, 0xe55, 0xe56, 0xe57, 0xe58, 
 0xe59, 0xe5a, 0xe5b,  0xfc, 0xfd,  0xfe,  0xff},
/* ISO 8859-12 rejected Celtic (Bogus) */
{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
 /* drafts have been rejected so this one is bogus */
  /* 0xa0 */
  0x00a0, 0x1e02, 0x1e03, 0x00a3, 0x010a, 0x010b, 0x1e0a, 0x00a7,
  0x1e80, 0x00a9, 0x1e82, 0x1e0b, 0x1ef2, 0x00ad, 0x00ae, 0x0178,
  /* 0xb0 */
  0x1e1e, 0x1e1f, 0x0120, 0x0121, 0x1e40, 0x1e41, 0x00b6, 0x1e56,
  0x1e81, 0x1e57, 0x1e83, 0x1e60, 0x1ef3, 0x1e84, 0x1e85, 0x1e61,
  /* 0xc0 */
  0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
  0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
  /* 0xd0 */
  0x0174, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x1e6a,
  0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x0176, 0x00df,
  /* 0xe0 */
  0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
  0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
  /* 0xf0 */
  0x0175, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x1e6b,
  0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x0177, 0x00ff},

/* ISO 8859-13 */
{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
  /* 0xa0 */
  0x00a0, 0x201d, 0x00a2, 0x00a3, 0x00a4, 0x201e, 0x00a6, 0x00a7,
  0x00d8, 0x00a9, 0x0156, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00c6,
  /* 0xb0 */
  0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x201c, 0x00b5, 0x00b6, 0x00b7,
  0x00f8, 0x00b9, 0x0157, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00e6,
  /* 0xc0 */
  0x0104, 0x012e, 0x0100, 0x0106, 0x00c4, 0x00c5, 0x0118, 0x0112,
  0x010c, 0x00c9, 0x0179, 0x0116, 0x0122, 0x0136, 0x012a, 0x013b,
  /* 0xd0 */
  0x0160, 0x0143, 0x0145, 0x00d3, 0x014c, 0x00d5, 0x00d6, 0x00d7,
  0x0172, 0x0141, 0x015a, 0x016a, 0x00dc, 0x017b, 0x017d, 0x00df,
  /* 0xe0 */
  0x0105, 0x012f, 0x0101, 0x0107, 0x00e4, 0x00e5, 0x0119, 0x0113,
  0x010d, 0x00e9, 0x017a, 0x0117, 0x0123, 0x0137, 0x012b, 0x013c,
  /* 0xf0 */
  0x0161, 0x0144, 0x0146, 0x00f3, 0x014d, 0x00f5, 0x00f6, 0x00f7,
  0x0173, 0x0142, 0x015b, 0x016b, 0x00fc, 0x017c, 0x017e, 0x2019},

/* ISO 8859-14 (Latin-8 or Celtic) */
{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
  /* 0xa0 */
  0x00a0, 0x1e02, 0x1e03, 0x00a3, 0x010a, 0x010b, 0x1e0a, 0x00a7,
  0x1e80, 0x00a9, 0x1e82, 0x1e0b, 0x1ef2, 0x00ad, 0x00ae, 0x0178,
  /* 0xb0 */
  0x1e1e, 0x1e1f, 0x0120, 0x0121, 0x1e40, 0x1e41, 0x00b6, 0x1e56,
  0x1e81, 0x1e57, 0x1e83, 0x1e60, 0x1ef3, 0x1e84, 0x1e85, 0x1e61,
  /* 0xc0 */
  0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
  0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
  /* 0xd0 */
  0x0174, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x1e6a,
  0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x0176, 0x00df,
  /* 0xe0 */
  0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
  0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
  /* 0xf0 */
  0x0175, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x1e6b,
  0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x0177, 0x00ff},

/* ISO 8859-15 (latin-9, German stuff plus EURO) */
{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
  /* 0xa0 */
  0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x20ac, 0x00a5, 0x0160, 0x00a7,
  0x0161, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
  /* 0xb0 */
  0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x017d, 0x00b5, 0x00b6, 0x00b7,
  0x017e, 0x00b9, 0x00ba, 0x00bb, 0x0152, 0x0153, 0x0178, 0x00bf,
  /* rest */
 0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
 0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
 0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
 0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff},

/* ISO 8859-16 */
{0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
 0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
 0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
 0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
 0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,
 0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,
 0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
  /* 0xa0 */
  0x00a0, 0x0104, 0x0105, 0x0141, 0x20ac, 0x00ab, 0x0160, 0x00a7,
  0x0161, 0x00a9, 0x0218, 0x201e, 0x0179, 0x00ad, 0x017a, 0x017b,
  /* 0xb0 */
  0x00b0, 0x00b1, 0x010c, 0x0142, 0x017d, 0x201d, 0x00b6, 0x00b7,
  0x017e, 0x010d, 0x0219, 0x00bb, 0x0152, 0x0153, 0x0178, 0x017c,
  /* 0xc0 */
  0x00c0, 0x00c1, 0x00c2, 0x0102, 0x00c4, 0x0106, 0x00c6, 0x00c7,
  0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
  /* 0xd0 */
  0x0110, 0x0143, 0x00d2, 0x00d3, 0x00d4, 0x0150, 0x00d6, 0x015a,
  0x0170, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x0118, 0x021a, 0x00df,
  /* 0xe0 */
  0x00e0, 0x00e1, 0x00e2, 0x0103, 0x00e4, 0x0107, 0x00e6, 0x00e7,
  0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
  /* 0xf0 */
  0x0111, 0x0144, 0x00f2, 0x00f3, 0x00f4, 0x0151, 0x00f6, 0x015b,
  0x0171, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x0119, 0x021b, 0x00ff }
};

#define NO_CHR  0xabcd /* No Unicode available */

// Tables mapping WINDOWS to ISO 10646.
static const UINT2 WINDOWS_CODE_PAGE[2][256]= {
/* Windows 1251  */
{ 0x0402, 0x0403, 0x201a, 0x0453, 0x201e, 0x2026, 0x2020, 0x2021, /* 0x80 */
  0x20ac, 0x2030, 0x0409, 0x2039, 0x040a, 0x040c, 0x040b, 0x040f, /* 0x88 */
  0x0452, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, /* 0x90 */
  NO_CHR,  0x2122, 0x0459, 0x203a, 0x045a, 0x045c, 0x045b, 0x045f, /* 0x98 */
  0x00a0, 0x040e, 0x045e, 0x0408, 0x00a4, 0x0490, 0x00a6, 0x00a7, /* 0xa0 */
  0x0401, 0x00a9, 0x0404, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x0407, /* 0xa8 */
  0x00b0, 0x00b1, 0x0406, 0x0456, 0x0491, 0x00b5, 0x00b6, 0x00b7, /* 0xb0 */
  0x0451, 0x2116, 0x0454, 0x00bb, 0x0458, 0x0405, 0x0455, 0x0457, /* 0xb8 */
  0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, /* 0xc0 */
  0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, 0x041f, /* 0xc8 */
  0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, /* 0xd0 */
  0x0428, 0x0429, 0x042a, 0x042b, 0x042c, 0x042d, 0x042e, 0x042f, /* 0xd8 */
  0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, /* 0xe0 */
  0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, 0x043f, /* 0xe8 */
  0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, /* 0xf0 */
  0x0448, 0x0449, 0x044a, 0x044b, 0x044c, 0x044d, 0x044e, 0x044f }, /* 0xf8 */
/* Windows 1257 (Windows Baltic)  */ // Nearly ISO 8859-13.
{ 0x20ac, NO_CHR, 0x201a, NO_CHR, 0x201e, 0x2026, 0x2020, 0x2021, /* 0x80 */
  NO_CHR, 0x2030, NO_CHR, 0x2039, NO_CHR, NO_CHR, NO_CHR, NO_CHR, /* 0x88 */
  NO_CHR, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, /* 0x90 */
  NO_CHR, 0x2122, NO_CHR, 0x203a, NO_CHR, NO_CHR, NO_CHR, NO_CHR, /* 0x98 */
  0x00a0, NO_CHR, 0x00a2, 0x00a3, 0x00a4, NO_CHR, 0x00a6, 0x00a7, /* 0xa0 */
  0x00d8, 0x00a9, 0x0156, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00c6, /* 0xa8 */
  0x00b0, 0x00b1, 0x00b2, 0x00b3, NO_CHR, 0x00b5, 0x00b6, 0x00b7, /* 0xb0 */
  0x00f8, 0x00b9, 0x0157, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00e6, /* 0xb8 */
  0x0104, 0x012e, 0x0100, 0x0106, 0x00c4, 0x00c5, 0x0118, 0x0112, /* 0xc0 */
  0x010c, 0x00c9, 0x0179, 0x0116, 0x0122, 0x0136, 0x012a, 0x013b, /* 0xc8 */
  0x0160, 0x0143, 0x0145, 0x00d3, 0x014c, 0x00d5, 0x00d6, 0x00d7, /* 0xd0 */
  0x0172, 0x0141, 0x015a, 0x016a, 0x00dc, 0x017b, 0x017d, 0x00df, /* 0xd8 */
  0x0105, 0x012f, 0x0101, 0x0107, 0x00e4, 0x00e5, 0x0119, 0x0113, /* 0xe0 */
  0x010d, 0x00e9, 0x017a, 0x0117, 0x0123, 0x0137, 0x012b, 0x013c, /* 0xe8 */
  0x0161, 0x0144, 0x0146, 0x00f3, 0x014d, 0x00f5, 0x00f6, 0x00f7, /* 0xf0 */
  0x0173, 0x0142, 0x015b, 0x016b, 0x00fc, 0x017c, 0x017e, NO_CHR} /* 0xf8 */
};


CHARSET::CHARSET()
{
  Which = 0xFF;
  SetSet( );
}

CHARSET::CHARSET(const CHARSET& OtherSet)
{
  Which = 0xFF;
  SetSet(OtherSet);
}


CHARSET::CHARSET(const char *Name)
{
  Which = 0xFF;
  SetSet( Name ? Charset2Id (Name) : GetGlobalCharset() );
}

CHARSET::CHARSET(const STRING& Name)
{
  Which = 0xFF;
  SetSet( Charset2Id ((const char *)Name) );
}

CHARSET::CHARSET(BYTE Id)
{
  Which = 0xFF;
  SetSet (Id);
}

BYTE CHARSET::SetSet ()
{
  return SetSet( GetGlobalCharset() );
}


BYTE CHARSET::SetSet (const char *Name)
{
  if (Name == NULL)
    return SetSet (GetGlobalCharset());
  return SetSet( Charset2Id (Name) );
}


BYTE CHARSET::SetSet (const STRING& Name)
{
  return SetSet( Charset2Id ((const char *)Name) );
}

BYTE CHARSET::SetSet (const CHARSET& Other)
{
  ctype  = Other.ctype;
  upper  = Other.upper;
  lower  = Other.lower;
  ascii  = Other.ascii;
  cclass = Other.cclass;
  CharTab= Other.CharTab;
  return Which  = Other.Which;
}

BYTE CHARSET::GetSet (STRING *Name) const
{
  if (Name)
    *Name = Id2Charset(Which);
  return Which;
}


STRING  CHARSET::ToLower(const STRING& String) const
{
#if 0
  if (UTF_encoded)
    {
        STRING newString = String.Dup();
        _utf_StrToLower(newString.stealData());
        return newString;
    } 
#endif
  const size_t len = String.GetLength();
  STRING NewString ('\0', len);
  unsigned char *p = (unsigned char *)String.c_str();

  for (size_t i=0; i < len; i++)
    NewString.SetChar(i, (char)(ib_tolower(p[i])));
  return NewString;
}

STRING  CHARSET::ToUpper(const STRING& String) const
{
#if 0
  if (UTF_encoded)
    {
        STRING newString = String.Dup();
        _utf_StrToLower(newString.stealData());
    } else
#endif
  const size_t len = String.GetLength();
  STRING NewString ('\0', len);
  unsigned char *p = (unsigned char *)String.c_str();

  for (size_t i=0; i < len; i++)
    NewString.SetChar(i, (char)(ib_toupper(p[i])));
  return NewString;
}


GDT_BOOLEAN CHARSET::Read(FILE *Fp)
{
  BYTE  Cset;

  ::Read(&Cset, Fp);
  return SetSet ( Cset ) != 0xFF;
}


void CHARSET::Write(FILE *Fp) const
{
  ::Write((BYTE)Which, Fp);
}


CHARSET::~CHARSET()
{
}


/*
 * The following is based upon some code that was provided by Ken Thompson
 * of AT&T Bell Laboratories, <ken@research.att.com> to the X/Open Joint
 * Internationalization Group.
 */
struct Tab {
  const int     cmask;
  const int     cval;
  const int     shift;
  const long    lmask;
  const long    lval;
}  bytetab[] = {
  {0x80,  0x00,   0*6,    0x7F,           0},              /* 1 byte sequence */
  {0xE0,  0xC0,   1*6,    0x7FF,          0x80},           /* 2 byte sequence */
  {0xF0,  0xE0,   2*6,    0xFFFF,         0x800},          /* 3 byte sequence */
  {0xF8,  0xF0,   3*6,    0x1FFFFF,       0x10000},        /* 4 byte sequence */
  {0xFC,  0xF8,   4*6,    0x3FFFFFF,      0x200000},       /* 5 byte sequence */
  {0xFE,  0xFC,   5*6,    0x7FFFFFFF,     0x4000000},      /* 6 byte sequence */
  {0,0,0,0,0}                                              /* end of table    */
};


/* Rename wctomb since we son't know if the clib function works
 * and don't want any conflicts with incorrect wchar_t typedefs */
static int Wctomb( char *s, long wc )
{
  long l;
  int c, nc;
  const struct Tab *t;
  
  if (s == NULL)
    return 0;
  
  l = wc;
  nc = 0;
  for ( t=bytetab; t->cmask; t++ ) {
    nc++;
    if ( l <= t->lmask ) {
      c = t->shift;
      *s = t->cval | ( l >> c );
      while ( c > 0 ) {
	c -= 6;
	s++;
	*s = 0x80 | ( ( l >> c ) & 0x3F );
      }
      return nc;
    }
  }
  return -1;
}



#if 0

GDT_BOOLEAN IsLatin1(UINT2 *src, size_t len)
{
  for (size_t i=0; i<len; i++)
    if (src[i] > 255) return GDT_FALSE;
  return GDT_TRUE;
}

GDT_BOOLEAN UCSToLatin1(const char *dest, UINT2 *src, size_t len)
{
  unsigned ch;
  for (size_t i=0; i<len; i++)
    dest[i] = ((ch = src[i]) < 256) ? (char)ch : '?';
}

#endif


int UTFTo(UINT2 *dest, const char *buf, size_t len)
{
  long            wc;
  int             c0, c;
  struct Tab     *t;
  size_t          count = 0;
  const char     *src = buf;

  if (src == NULL || len == 0) return 0;

  do {
    wc = c0 = *src++ & 0xff;
    for (t = bytetab; t->cmask; t++) {
      if ((c0 & t->cmask) == t->cval) {
	wc &= t->lmask;
	/* Undefined characters map to ? */
	dest[count++] = wc > 0xFFFF ? (UINT2)'?' : (UINT2) wc;
	break;
      }
      c = (*src++ ^ 0x80) & 0xFF;
      if (c & 0xC0)
	break;
      wc = (wc << 6) | c;
    }		/* for */
  } while (c0 != 0 && count < len);
  return src - buf;
}


char *CHARSET::ToUTF(char *buffer, const char *From) const
{
  UINT2 value;
  char  *ptr = buffer;

  while (From && *From)
    {
/* Generate a mask of LENGTH one-bits, right justified in a word.  */
#define MASK(Length) ((unsigned) ~(~0 << (Length)))
      value  = Which > 1 ? CharTab[(unsigned char)*From++] : *From++;
      if (value & ~MASK (7))
        if (value & ~MASK (11))
          {
            /* 3 bytes - more than 11 bits, but not more than 16.  */
            *ptr++ = ((char)((MASK (3) << 5) | (MASK (6) & value >> 12)));
            *ptr++ = ((char)((1 << 7) | (MASK (6) & value >> 6)));
            *ptr++ = ((char)(((1 << 7) | (MASK (6) & value))));
          }
        else
          {
            /* 2 bytes - more than 7 bits, but not more than 11.  */
            *ptr++ = ((char)( ((MASK (2) << 6) | (MASK (6) & value >> 6))));
            *ptr++ = ((char)(((1 << 7) | (MASK (6) & value))));
          }
        else
          {
            /* 1 byte - not more than 7 bits (that is, ASCII).  */
            *ptr++ = ((char)value);
          }
    }
  if (ptr) *ptr = '\0';
#undef MASK
  return buffer;
}


STRING CHARSET::ToUTF(const STRING& From) const
{
  STRING utf;
  ToUTF(&utf, From);
  return utf;
}

void CHARSET::ToUTF(STRING *Buffer, const STRING& From) const
{
  const size_t Len = From.GetLength();
  UINT2 value;

  Buffer->Clear();
  for (size_t i=1; i<=Len; i++)
    {
/* Generate a mask of LENGTH one-bits, right justified in a word.  */
#define MASK(Length) ((unsigned) ~(~0 << (Length)))
      value  = CharTab[(unsigned char)(From.GetChr(i))];
      if (value & ~MASK (7))
	if (value & ~MASK (11))
	  {
	    /* 3 bytes - more than 11 bits, but not more than 16.  */
	    Buffer->Cat ((char)((MASK (3) << 5) | (MASK (6) & value >> 12)));
	    Buffer->Cat ((char)((1 << 7) | (MASK (6) & value >> 6)));
	    Buffer->Cat ((char)(((1 << 7) | (MASK (6) & value))));
	  }
	else
	  {
	    /* 2 bytes - more than 7 bits, but not more than 11.  */
	    Buffer->Cat ((char)( ((MASK (2) << 6) | (MASK (6) & value >> 6))));
	    Buffer->Cat ((char)(((1 << 7) | (MASK (6) & value))));
	  }
	else
	  {
	    /* 1 byte - not more than 7 bits (that is, ASCII).  */
	    Buffer->Cat ((char)value);
	  }
    }
#undef MASK
}

// Load UCS-2 buffer
size_t CHARSET::ReadUCS (UINT2 *buf, size_t DataFileSize, PFILE Fp) const
{
  size_t total = 0;
  if (Fp)
    {
      int ch;
      while (total < DataFileSize)
	{
	  if ((ch = getc(Fp)) == EOF)
	    break;
	  buf[total++] = CharTab[(unsigned char)ch]; 
	}
    }
  return total;
}

// Ctypes
#if 0
static const unsigned char _ib_ctype_8859_1[] = {
  /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', 
  /* */'\040', /* */'\050', /* */'\050', /* */'\050', /* */'\050', /* */'\050', /* */'\040', /* */'\040', 
  /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', 
  /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', 
  /* */'\110', /*!*/'\020', /*"*/'\020', /*#*/'\020', /*$*/'\020', /*%*/'\020', /*&*/'\020', /*'*/'\020', 
  /*(*/'\020', /*)*/'\020', /***/'\020', /*+*/'\020', /*,*/'\020', /*-*/'\020', /*.*/'\020', /* / */'\020', 
  /*0*/'\204', /*1*/'\204', /*2*/'\204', /*3*/'\204', /*4*/'\204', /*5*/'\204', /*6*/'\204', /*7*/'\204', 
  /*8*/'\204', /*9*/'\204', /*:*/'\020', /*;*/'\020', /*<*/'\020', /*=*/'\020', /*>*/'\020', /*?*/'\020', 
  /*@*/'\020', /*A*/'\201', /*B*/'\201', /*C*/'\201', /*D*/'\201', /*E*/'\201', /*F*/'\201', /*G*/'\001', 
  /*H*/'\001', /*I*/'\001', /*J*/'\001', /*K*/'\001', /*L*/'\001', /*M*/'\001', /*N*/'\001', /*O*/'\001', 
  /*P*/'\001', /*Q*/'\001', /*R*/'\001', /*S*/'\001', /*T*/'\001', /*U*/'\001', /*V*/'\001', /*W*/'\001', 
  /*X*/'\001', /*Y*/'\001', /*Z*/'\001', /*[*/'\020', /*\*/'\020', /*]*/'\020', /*^*/'\020', /*_*/'\020', 
  /*`*/'\020', /*a*/'\202', /*b*/'\202', /*c*/'\202', /*d*/'\202', /*e*/'\202', /*f*/'\202', /*g*/'\002', 
  /*h*/'\002', /*i*/'\002', /*j*/'\002', /*k*/'\002', /*l*/'\002', /*m*/'\002', /*n*/'\002', /*o*/'\002', 
  /*p*/'\002', /*q*/'\002', /*r*/'\002', /*s*/'\002', /*t*/'\002', /*u*/'\002', /*v*/'\002', /*w*/'\002', 
  /*x*/'\002', /*y*/'\002', /*z*/'\002', /*{*/'\020', /*|*/'\020', /*}*/'\020', /*~*/'\020', /* */'\040', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\110', /*¡*/'\020', /*¢*/'\020', /*£*/'\020', /*¤*/'\020', /*¥*/'\020', /*¦*/'\020', /*§*/'\020', 
  /*¨*/'\020', /*©*/'\020', /*ª*/'\020', /*«*/'\020', /*¬*/'\020', /*­*/'\020', /*®*/'\020', /*¯*/'\020', 
  /*°*/'\020', /*±*/'\020', /*²*/'\020', /*³*/'\020', /*´*/'\020', /*µ*/'\020', /*¶*/'\020', /*·*/'\020', 
  /*¸*/'\020', /*¹*/'\020', /*º*/'\020', /*»*/'\020', /*¼*/'\020', /*½*/'\020', /*¾*/'\020', /*¿*/'\020', 
  /*À*/'\001', /*Á*/'\001', /*Â*/'\001', /*Ã*/'\001', /*Ä*/'\001', /*Å*/'\001', /*Æ*/'\001', /*Ç*/'\001', 
  /*È*/'\001', /*É*/'\001', /*Ê*/'\001', /*Ë*/'\001', /*Ì*/'\001', /*Í*/'\001', /*Î*/'\001', /*Ï*/'\001', 
  /*Ð*/'\001', /*Ñ*/'\001', /*Ò*/'\001', /*Ó*/'\001', /*Ô*/'\001', /*Õ*/'\001', /*Ö*/'\001', /*×*/'\020', 
  /*Ø*/'\001', /*Ù*/'\001', /*Ú*/'\001', /*Û*/'\001', /*Ü*/'\001', /*Ý*/'\001', /*Þ*/'\001', /*ß*/'\002', 
  /*à*/'\002', /*á*/'\002', /*â*/'\002', /*ã*/'\002', /*ä*/'\002', /*å*/'\002', /*æ*/'\002', /*ç*/'\002', 
  /*è*/'\002', /*é*/'\002', /*ê*/'\002', /*ë*/'\002', /*ì*/'\002', /*í*/'\002', /*î*/'\002', /*ï*/'\002', 
  /*ð*/'\002', /*ñ*/'\002', /*ò*/'\002', /*ó*/'\002', /*ô*/'\002', /*õ*/'\002', /*ö*/'\002', /*÷*/'\020', 
  /*ø*/'\002', /*ù*/'\002', /*ú*/'\002', /*û*/'\002', /*ü*/'\002', /*ý*/'\002', /*þ*/'\002', /*ÿ*/'\002'
};
#endif

// Collapse Latin1
static const OCTET _trans_ascii_8859_1[] = {
'\000',	'\001',	'\002',	'\003',	'\004',	'\005',	'\006',	'\007',	
'\010',	'\011',	'\012',	'\013',	'\014',	'\015',	'\016',	'\017',	
'\020',	'\021',	'\022',	'\023',	'\024',	'\025',	'\026',	'\027',	
'\030',	'\031',	'\032',	'\033',	'\034',	'\035',	'\036',	'\037',	
'\040',	'\041',	'\042',	'\043',	'\044',	'\045',	'\046',	'\047',	
'\050',	'\051',	'\052',	'\053',	'\054',	'\055',	'\056',	'\057',	
'\060',	'\061',	'\062',	'\063',	'\064',	'\065',	'\066',	'\067',	
'\070',	'\071',	'\072',	'\073',	'\074',	'\075',	'\076',	'\077',	
'\100',	  'a',	  'b',	  'c',	  'd',	  'e',	  'f',	  'g',	
  'h',	  'i',	  'j',	  'k',	  'l',	  'm',	  'n',	  'o',	
  'p',	  'q',	  'r',	  's',	  't',	  'u',	  'v',	  'w',	
  'x',	  'y',	  'z',	'\133',	'\134',	'\135',	'\136',	'\137',	
'\140',	  'a',	  'b',	  'c',	  'd',	  'e',	  'f',	  'g',	
  'h',	  'i',	  'j',	  'k',	  'l',	  'm',	  'n',	  'o',	
  'p',	  'q',	  'r',	  's',	  't',	  'u',	  'v',	  'w',	
  'x',	  'y',	  'z',	'\173',	'\174',	'\175',	'\176',	'\177',	
'\200',	'\201',	'\202',	'\203',	'\204',	'\205',	'\206',	'\207',	
'\210',	'\211',	'\212',	'\213',	'\214',	'\215',	'\216',	'\217',	
'\220',	'\221',	'\222',	'\223',	'\224',	'\225',	'\226',	'\227',	
'\230',	'\231',	'\232',	'\233',	'\234',	'\235',	'\236',	'\237',	
'\240',	'\241',	'\242',	'\243',	'\244',	'\245',	'\246',	'\247',	
'\250',	'\250',	'\252',	'\253',	'\254',	'\255',	'\256',	'\257',	
'\260',	'\261',	'\262',	'\263',	'\264',	'\265',	'\266',	'\267',	
'\270',	'\271',	'\272',	'\273',	'\274',	'\275',	'\276',	'\277',	
  'a',	  'a',	  'a',	  'a',	  'a',	  'a',	  'a',	  'c',	
  'e',	  'e',	  'e',	  'e',	  'i',	  'i',	  'i',	  'i',	
'\320',	  'n',	  'o',	  'o',	  'o',	  'o',	  'o',	  'o',	
  'o',	  'u',	  'u',	  'u',	  'u',	  'y',	'\336',	'\337',	
  'a',	  'a',	  'a',	  'a',	  'a',	  'a',	  'a',	  'c',	
  'e',	  'e',	  'e',	  'e',	  'i',	  'i',	  'i',	  'i',	
'\360',	  'n',	  'o',	  'o',	  'o',	  'o',	  'o',	  'o',	
  'o',	  'u',	  'u',	  'u',	  'u',	  'y',	'\376',	'\377',	
};





static const OCTET _trans_upper_8859_1[] = {
  /* */'\000', /* */'\001', /* */'\002', /* */'\003', /* */'\004', /* */'\005', /* */'\006', /* */'\007', 
  /* */'\010', /* */'\011', /* */'\012', /* */'\013', /* */'\014', /* */'\015', /* */'\016', /* */'\017', 
  /* */'\020', /* */'\021', /* */'\022', /* */'\023', /* */'\024', /* */'\025', /* */'\026', /* */'\027', 
  /* */'\030', /* */'\031', /* */'\032', /* */'\033', /* */'\034', /* */'\035', /* */'\036', /* */'\037', 
  /* */'\040', /*!*/'\041', /*"*/'\042', /*#*/'\043', /*$*/'\044', /*%*/'\045', /*&*/'\046', /*'*/'\047', 
  /*(*/'\050', /*)*/'\051', /***/'\052', /*+*/'\053', /*,*/'\054', /*-*/'\055', /*.*/'\056', /* / */'\057', 
  /*0*/'\060', /*1*/'\061', /*2*/'\062', /*3*/'\063', /*4*/'\064', /*5*/'\065', /*6*/'\066', /*7*/'\067', 
  /*8*/'\070', /*9*/'\071', /*:*/'\072', /*;*/'\073', /*<*/'\074', /*=*/'\075', /*>*/'\076', /*?*/'\077', 
  /*@*/'\100', /*A*/'\101', /*B*/'\102', /*C*/'\103', /*D*/'\104', /*E*/'\105', /*F*/'\106', /*G*/'\107', 
  /*H*/'\110', /*I*/'\111', /*J*/'\112', /*K*/'\113', /*L*/'\114', /*M*/'\115', /*N*/'\116', /*O*/'\117', 
  /*P*/'\120', /*Q*/'\121', /*R*/'\122', /*S*/'\123', /*T*/'\124', /*U*/'\125', /*V*/'\126', /*W*/'\127', 
  /*X*/'\130', /*Y*/'\131', /*Z*/'\132', /*[*/'\133', /*\*/'\134', /*]*/'\135', /*^*/'\136', /*_*/'\137', 
  /*`*/'\140', /*a*/'\101', /*b*/'\102', /*c*/'\103', /*d*/'\104', /*e*/'\105', /*f*/'\106', /*g*/'\107', 
  /*h*/'\110', /*i*/'\111', /*j*/'\112', /*k*/'\113', /*l*/'\114', /*m*/'\115', /*n*/'\116', /*o*/'\117', 
  /*p*/'\120', /*q*/'\121', /*r*/'\122', /*s*/'\123', /*t*/'\124', /*u*/'\125', /*v*/'\126', /*w*/'\127', 
  /*x*/'\130', /*y*/'\131', /*z*/'\132', /*{*/'\173', /*|*/'\174', /*}*/'\175', /*~*/'\176', /* */'\177', 
  /* */'\200', /* */'\201', /* */'\202', /* */'\203', /* */'\204', /* */'\205', /* */'\206', /* */'\207', 
  /* */'\210', /* */'\211', /* */'\212', /* */'\213', /* */'\214', /* */'\215', /* */'\216', /* */'\217', 
  /* */'\220', /* */'\221', /* */'\222', /* */'\223', /* */'\224', /* */'\225', /* */'\226', /* */'\227', 
  /* */'\230', /* */'\231', /* */'\232', /* */'\233', /* */'\234', /* */'\235', /* */'\236', /* */'\237', 
  /* */'\240', /*¡*/'\241', /*¢*/'\242', /*£*/'\243', /*¤*/'\244', /*¥*/'\245', /*¦*/'\246', /*§*/'\247', 
  /*¨*/'\250', /*©*/'\251', /*ª*/'\252', /*«*/'\253', /*¬*/'\254', /*­*/'\255', /*®*/'\256', /*¯*/'\257', 
  /*°*/'\260', /*±*/'\261', /*²*/'\262', /*³*/'\263', /*´*/'\264', /*µ*/'\265', /*¶*/'\266', /*·*/'\267', 
  /*¸*/'\270', /*¹*/'\271', /*º*/'\272', /*»*/'\273', /*¼*/'\274', /*½*/'\275', /*¾*/'\276', /*¿*/'\277', 
  /*À*/'\300', /*Á*/'\301', /*Â*/'\302', /*Ã*/'\303', /*Ä*/'\304', /*Å*/'\305', /*Æ*/'\306', /*Ç*/'\307', 
  /*È*/'\310', /*É*/'\311', /*Ê*/'\312', /*Ë*/'\313', /*Ì*/'\314', /*Í*/'\315', /*Î*/'\316', /*Ï*/'\317', 
  /*Ð*/'\320', /*Ñ*/'\321', /*Ò*/'\322', /*Ó*/'\323', /*Ô*/'\324', /*Õ*/'\325', /*Ö*/'\326', /*×*/'\327', 
  /*Ø*/'\330', /*Ù*/'\331', /*Ú*/'\332', /*Û*/'\333', /*Ü*/'\334', /*Ý*/'\335', /*Þ*/'\336', /*ß*/'\337', 
  /*à*/'\300', /*á*/'\301', /*â*/'\302', /*ã*/'\303', /*ä*/'\304', /*å*/'\305', /*æ*/'\306', /*ç*/'\307', 
  /*è*/'\310', /*é*/'\311', /*ê*/'\312', /*ë*/'\313', /*ì*/'\314', /*í*/'\315', /*î*/'\316', /*ï*/'\317', 
  /*ð*/'\320', /*ñ*/'\321', /*ò*/'\322', /*ó*/'\323', /*ô*/'\324', /*õ*/'\325', /*ö*/'\326', /*÷*/'\367', 
  /*ø*/'\330', /*ù*/'\331', /*ú*/'\332', /*û*/'\333', /*ü*/'\334', /*ý*/'\335', /*þ*/'\336', /*ÿ*/'\377'
};

static const OCTET _trans_lower_8859_1[] = {
  /* */'\000', /* */'\001', /* */'\002', /* */'\003', /* */'\004', /* */'\005', /* */'\006', /* */'\007', 
  /* */'\010', /* */'\011', /* */'\012', /* */'\013', /* */'\014', /* */'\015', /* */'\016', /* */'\017', 
  /* */'\020', /* */'\021', /* */'\022', /* */'\023', /* */'\024', /* */'\025', /* */'\026', /* */'\027', 
  /* */'\030', /* */'\031', /* */'\032', /* */'\033', /* */'\034', /* */'\035', /* */'\036', /* */'\037', 
  /* */'\040', /*!*/'\041', /*"*/'\042', /*#*/'\043', /*$*/'\044', /*%*/'\045', /*&*/'\046', /*'*/'\047', 
  /*(*/'\050', /*)*/'\051', /***/'\052', /*+*/'\053', /*,*/'\054', /*-*/'\055', /*.*/'\056', /* / */'\057', 
  /*0*/'\060', /*1*/'\061', /*2*/'\062', /*3*/'\063', /*4*/'\064', /*5*/'\065', /*6*/'\066', /*7*/'\067', 
  /*8*/'\070', /*9*/'\071', /*:*/'\072', /*;*/'\073', /*<*/'\074', /*=*/'\075', /*>*/'\076', /*?*/'\077', 
  /*@*/'\100', /*A*/'\141', /*B*/'\142', /*C*/'\143', /*D*/'\144', /*E*/'\145', /*F*/'\146', /*G*/'\147', 
  /*H*/'\150', /*I*/'\151', /*J*/'\152', /*K*/'\153', /*L*/'\154', /*M*/'\155', /*N*/'\156', /*O*/'\157', 
  /*P*/'\160', /*Q*/'\161', /*R*/'\162', /*S*/'\163', /*T*/'\164', /*U*/'\165', /*V*/'\166', /*W*/'\167', 
  /*X*/'\170', /*Y*/'\171', /*Z*/'\172', /*[*/'\133', /*\*/'\134', /*]*/'\135', /*^*/'\136', /*_*/'\137', 
  /*`*/'\140', /*a*/'\141', /*b*/'\142', /*c*/'\143', /*d*/'\144', /*e*/'\145', /*f*/'\146', /*g*/'\147', 
  /*h*/'\150', /*i*/'\151', /*j*/'\152', /*k*/'\153', /*l*/'\154', /*m*/'\155', /*n*/'\156', /*o*/'\157', 
  /*p*/'\160', /*q*/'\161', /*r*/'\162', /*s*/'\163', /*t*/'\164', /*u*/'\165', /*v*/'\166', /*w*/'\167', 
  /*x*/'\170', /*y*/'\171', /*z*/'\172', /*{*/'\173', /*|*/'\174', /*}*/'\175', /*~*/'\176', /* */'\177', 
  /* */'\200', /* */'\201', /* */'\202', /* */'\203', /* */'\204', /* */'\205', /* */'\206', /* */'\207', 
  /* */'\210', /* */'\211', /* */'\212', /* */'\213', /* */'\214', /* */'\215', /* */'\216', /* */'\217', 
  /* */'\220', /* */'\221', /* */'\222', /* */'\223', /* */'\224', /* */'\225', /* */'\226', /* */'\227', 
  /* */'\230', /* */'\231', /* */'\232', /* */'\233', /* */'\234', /* */'\235', /* */'\236', /* */'\237', 
  /* */'\240', /*¡*/'\241', /*¢*/'\242', /*£*/'\243', /*¤*/'\244', /*¥*/'\245', /*¦*/'\246', /*§*/'\247', 
  /*¨*/'\250', /*©*/'\251', /*ª*/'\252', /*«*/'\253', /*¬*/'\254', /*­*/'\255', /*®*/'\256', /*¯*/'\257', 
  /*°*/'\260', /*±*/'\261', /*²*/'\262', /*³*/'\263', /*´*/'\264', /*µ*/'\265', /*¶*/'\266', /*·*/'\267', 
  /*¸*/'\270', /*¹*/'\271', /*º*/'\272', /*»*/'\273', /*¼*/'\274', /*½*/'\275', /*¾*/'\276', /*¿*/'\277', 
  /*À*/'\340', /*Á*/'\341', /*Â*/'\342', /*Ã*/'\343', /*Ä*/'\344', /*Å*/'\345', /*Æ*/'\346', /*Ç*/'\347', 
  /*È*/'\350', /*É*/'\351', /*Ê*/'\352', /*Ë*/'\353', /*Ì*/'\354', /*Í*/'\355', /*Î*/'\356', /*Ï*/'\357', 
  /*Ð*/'\360', /*Ñ*/'\361', /*Ò*/'\362', /*Ó*/'\363', /*Ô*/'\364', /*Õ*/'\365', /*Ö*/'\366', /*×*/'\327', 
  /*Ø*/'\370', /*Ù*/'\371', /*Ú*/'\372', /*Û*/'\373', /*Ü*/'\374', /*Ý*/'\375', /*Þ*/'\376', /*ß*/'\337', 
  /*à*/'\340', /*á*/'\341', /*â*/'\342', /*ã*/'\343', /*ä*/'\344', /*å*/'\345', /*æ*/'\346', /*ç*/'\347', 
  /*è*/'\350', /*é*/'\351', /*ê*/'\352', /*ë*/'\353', /*ì*/'\354', /*í*/'\355', /*î*/'\356', /*ï*/'\357', 
  /*ð*/'\360', /*ñ*/'\361', /*ò*/'\362', /*ó*/'\363', /*ô*/'\364', /*õ*/'\365', /*ö*/'\366', /*÷*/'\367', 
  /*ø*/'\370', /*ù*/'\371', /*ú*/'\372', /*û*/'\373', /*ü*/'\374', /*ý*/'\375', /*þ*/'\376', /*ÿ*/'\377'
};


// ISO 8859-1 (Latin-1) Soundex Character Codes
// with support for accented characters and ligatures.

// Soundex Code tables for other ISO 8859-x character
// sets can be designed. The utility of the soundex
// algorithm for non Anglo-Saxon languages is questionable.
// [even for English its utility is very specific]
#define _SP -1
static const signed char _soundex_8859_1[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, _SP, -1, -1, _SP, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, _SP, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1,
    7 /* A */, 1 /* B */, 2 /* C */, 3 /* D */,
    7 /* E */, 8 /* F */, 2 /* G */, 0 /* H */,
    7 /* I */, 2 /* J */, 2 /* K */, 4 /* L */,
    5 /* M */, 5 /* N */, 7 /* O */, 1 /* P */,
    2 /* Q */, 6 /* R */, 9 /* S */, 3 /* T */,
    7 /* U */, 8 /* V */, 0 /* W */, 2 /* X */,
    7 /* Y */, 9 /* Z */,
    -1, -1, -1, -1, -1, -1,
    7 /* a */, 1 /* b */, 2 /* c */, 3 /* d */,
    7 /* e */, 8 /* f */, 2 /* g */, 0 /* h */,
    7 /* i */, 2 /* j */, 2 /* k */, 4 /* l */,
    5 /* m */, 5 /* n */, 7 /* o */, 1 /* p */,
    2 /* q */, 6 /* r */, 9 /* s */, 3 /* t */,
    7 /* u */, 8 /* v */, 0 /* w */, 2 /* x */,
    7 /* y */, 9 /* z */ ,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1,
    7 /* À */, 7 /* Á */, 7 /* Â */, 7 /* Ã */,
    7 /* Ä */, 7 /* Å */, 7 /* Æ */, 2 /* Ç */,
    7 /* È */, 7 /* É */, 7 /* Ê */, 7 /* Ë */,
    7 /* Ì */, 7 /* Í */, 7 /* Î */, 7 /* Ï */,
    3 /* Ð */, 5 /* Ñ */, 7 /* Ò */, 7 /* Ó */,
    7 /* Ô */, 7 /* Õ */, 7 /* Ö */, -1,
    7 /* Ø */, 7 /* Ù */, 7 /* Ú */, 7 /* Û */,
    7 /* Ü */, 0 /* Ý */, 3 /* Þ */, 2 /* ß */,
    7 /* à */, 7 /* á */, 7 /* â */, 7 /* ã */,
    7 /* ä */, 7 /* å */, 7 /* æ */, 2 /* ç */,
    7 /* è */, 7 /* é */, 7 /* ê */, 7 /* ë */,
    7 /* ì */, 7 /* í */, 7 /* î */, 7 /* ï */,
    3 /* ð */, 5 /* ñ */, 7 /* ò */, 7 /* ó */,
    7 /* ô */, 7 /* õ */, 7 /* ö */, -1,
    7 /* ø */, 7 /* ù */, 7 /* ú */, 7 /* û */,
    7 /* ü */, 0 /* ý */, 3 /* þ */, 0 /* ÿ */
};

static const OCTET _cclass_8859_1[] = {
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', // 0 - 7
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', // 8 - 15
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', // 15 - 23
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000', // 24 - 31
 '\000', '\041', '\042', '\043', '\044', '\045', '\046', '\047', // 32 - 39
 '\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057', // 40 - 47
 '\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
 '\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
 '\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
 '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
 '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
 '\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
 '\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
 '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
 '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
 '\170', '\171', '\172', '\173', '\174', '\175', '\176', '\000',
 '\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
 '\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
 '\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
 '\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
 '\000', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
 '\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
 '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
 '\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
 '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
 '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
 '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\327',
 '\370', '\371', '\372', '\373', '\374', '\375', '\376', '\337',
 '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
 '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
 '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
 '\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
};

static int func8859_1(const void *s1, const void *s2)
{
  return compareFunc(s1, s2, _cclass_8859_1); 
}

static int Compare8859_1(const void *s1, const void *s2, size_t len)
{
  return compareFunc(s1, s2, _cclass_8859_1, len);
}


static int SIS8859_1(const void *s1, const void *s2)
{
  return SIScompareFunc(s1, s2, _ib_ctype_8859_1 /* _cclass_8859_1 */);
}


#if 0
static const unsigned char _ib_ctype_8859_2[] = {
  /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', 
  /* */'\040', /* */'\050', /* */'\050', /* */'\050', /* */'\050', /* */'\050', /* */'\040', /* */'\040', 
  /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', 
  /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', 
  /* */'\110', /*!*/'\020', /*"*/'\020', /*#*/'\020', /*$*/'\020', /*%*/'\020', /*&*/'\020', /*'*/'\020', 
  /*(*/'\020', /*)*/'\020', /***/'\020', /*+*/'\020', /*,*/'\020', /*-*/'\020', /*.*/'\020', /* / */'\020', 
  /*0*/'\204', /*1*/'\204', /*2*/'\204', /*3*/'\204', /*4*/'\204', /*5*/'\204', /*6*/'\204', /*7*/'\204', 
  /*8*/'\204', /*9*/'\204', /*:*/'\020', /*;*/'\020', /*<*/'\020', /*=*/'\020', /*>*/'\020', /*?*/'\020', 
  /*@*/'\020', /*A*/'\201', /*B*/'\201', /*C*/'\201', /*D*/'\201', /*E*/'\201', /*F*/'\201', /*G*/'\001', 
  /*H*/'\001', /*I*/'\001', /*J*/'\001', /*K*/'\001', /*L*/'\001', /*M*/'\001', /*N*/'\001', /*O*/'\001', 
  /*P*/'\001', /*Q*/'\001', /*R*/'\001', /*S*/'\001', /*T*/'\001', /*U*/'\001', /*V*/'\001', /*W*/'\001', 
  /*X*/'\001', /*Y*/'\001', /*Z*/'\001', /*[*/'\020', /*\*/'\020', /*]*/'\020', /*^*/'\020', /*_*/'\020', 
  /*`*/'\020', /*a*/'\202', /*b*/'\202', /*c*/'\202', /*d*/'\202', /*e*/'\202', /*f*/'\202', /*g*/'\002', 
  /*h*/'\002', /*i*/'\002', /*j*/'\002', /*k*/'\002', /*l*/'\002', /*m*/'\002', /*n*/'\002', /*o*/'\002', 
  /*p*/'\002', /*q*/'\002', /*r*/'\002', /*s*/'\002', /*t*/'\002', /*u*/'\002', /*v*/'\002', /*w*/'\002', 
  /*x*/'\002', /*y*/'\002', /*z*/'\002', /*{*/'\020', /*|*/'\020', /*}*/'\020', /*~*/'\020', /* */'\040', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\110', /*¡*/'\001', /* */'\000', /*£*/'\001', /* */'\000', /*¥*/'\001', /*¦*/'\001', /* */'\000', 
  /* */'\000', /*©*/'\001', /*ª*/'\001', /*«*/'\001', /*¬*/'\001', /* */'\000', /*®*/'\001', /*¯*/'\001', 
  /* */'\000', /*±*/'\002', /* */'\000', /*³*/'\002', /* */'\000', /*µ*/'\002', /*¶*/'\002', /* */'\000', 
  /* */'\000', /*¹*/'\002', /*º*/'\002', /*»*/'\002', /*¼*/'\002', /* */'\000', /*¾*/'\002', /*¿*/'\002', 
  /*À*/'\001', /*Á*/'\001', /*Â*/'\001', /*Ã*/'\001', /*Ä*/'\001', /*Å*/'\001', /*Æ*/'\001', /*Ç*/'\001', 
  /*È*/'\001', /*É*/'\001', /*Ê*/'\001', /*Ë*/'\001', /*Ì*/'\001', /*Í*/'\001', /*Î*/'\001', /*Ï*/'\001', 
  /*Ð*/'\001', /*Ñ*/'\001', /*Ò*/'\001', /*Ó*/'\001', /*Ô*/'\001', /*Õ*/'\001', /*Ö*/'\001', /*×*/'\020', 
  /*Ø*/'\001', /*Ù*/'\001', /*Ú*/'\001', /*Û*/'\001', /*Ü*/'\001', /*Ý*/'\001', /*Þ*/'\001', /*ß*/'\002', 
  /*à*/'\002', /*á*/'\002', /*â*/'\002', /*ã*/'\002', /*ä*/'\002', /*å*/'\002', /*æ*/'\002', /*ç*/'\002', 
  /*è*/'\002', /*é*/'\002', /*ê*/'\002', /*ë*/'\002', /*ì*/'\002', /*í*/'\002', /*î*/'\002', /*ï*/'\002', 
  /*ð*/'\002', /*ñ*/'\002', /*ò*/'\002', /*ó*/'\002', /*ô*/'\002', /*õ*/'\002', /*ö*/'\002', /*÷*/'\020', 
  /*ø*/'\002', /*ù*/'\002', /*ú*/'\002', /*û*/'\002', /*ü*/'\002', /*ý*/'\002', /*þ*/'\002', /* */'\000'
};
#endif

static const OCTET _trans_upper_8859_2[] = {
  /* */'\000', /* */'\001', /* */'\002', /* */'\003', /* */'\004', /* */'\005', /* */'\006', /* */'\007', 
  /* */'\010', /* */'\011', /* */'\012', /* */'\013', /* */'\014', /* */'\015', /* */'\016', /* */'\017', 
  /* */'\020', /* */'\021', /* */'\022', /* */'\023', /* */'\024', /* */'\025', /* */'\026', /* */'\027', 
  /* */'\030', /* */'\031', /* */'\032', /* */'\033', /* */'\034', /* */'\035', /* */'\036', /* */'\037', 
  /* */'\040', /*!*/'\041', /*"*/'\042', /*#*/'\043', /*$*/'\044', /*%*/'\045', /*&*/'\046', /*'*/'\047', 
  /*(*/'\050', /*)*/'\051', /***/'\052', /*+*/'\053', /*,*/'\054', /*-*/'\055', /*.*/'\056', /* / */'\057', 
  /*0*/'\060', /*1*/'\061', /*2*/'\062', /*3*/'\063', /*4*/'\064', /*5*/'\065', /*6*/'\066', /*7*/'\067', 
  /*8*/'\070', /*9*/'\071', /*:*/'\072', /*;*/'\073', /*<*/'\074', /*=*/'\075', /*>*/'\076', /*?*/'\077', 
  /*@*/'\100', /*A*/'\101', /*B*/'\102', /*C*/'\103', /*D*/'\104', /*E*/'\105', /*F*/'\106', /*G*/'\107', 
  /*H*/'\110', /*I*/'\111', /*J*/'\112', /*K*/'\113', /*L*/'\114', /*M*/'\115', /*N*/'\116', /*O*/'\117', 
  /*P*/'\120', /*Q*/'\121', /*R*/'\122', /*S*/'\123', /*T*/'\124', /*U*/'\125', /*V*/'\126', /*W*/'\127', 
  /*X*/'\130', /*Y*/'\131', /*Z*/'\132', /*[*/'\133', /*\*/'\134', /*]*/'\135', /*^*/'\136', /*_*/'\137', 
  /*`*/'\140', /*a*/'\101', /*b*/'\102', /*c*/'\103', /*d*/'\104', /*e*/'\105', /*f*/'\106', /*g*/'\107', 
  /*h*/'\110', /*i*/'\111', /*j*/'\112', /*k*/'\113', /*l*/'\114', /*m*/'\115', /*n*/'\116', /*o*/'\117', 
  /*p*/'\120', /*q*/'\121', /*r*/'\122', /*s*/'\123', /*t*/'\124', /*u*/'\125', /*v*/'\126', /*w*/'\127', 
  /*x*/'\130', /*y*/'\131', /*z*/'\132', /*{*/'\173', /*|*/'\174', /*}*/'\175', /*~*/'\176', /* */'\177', 
  /* */'\200', /* */'\201', /* */'\202', /* */'\203', /* */'\204', /* */'\205', /* */'\206', /* */'\207', 
  /* */'\210', /* */'\211', /* */'\212', /* */'\213', /* */'\214', /* */'\215', /* */'\216', /* */'\217', 
  /* */'\220', /* */'\221', /* */'\222', /* */'\223', /* */'\224', /* */'\225', /* */'\226', /* */'\227', 
  /* */'\230', /* */'\231', /* */'\232', /* */'\233', /* */'\234', /* */'\235', /* */'\236', /* */'\237', 
  /* */'\240', /*¡*/'\241', /* */'\242', /*£*/'\243', /* */'\244', /*¥*/'\245', /*¦*/'\246', /* */'\247', 
  /* */'\250', /*©*/'\251', /*ª*/'\252', /*«*/'\253', /*¬*/'\254', /* */'\255', /*®*/'\256', /*¯*/'\257', 
  /* */'\260', /*±*/'\241', /* */'\262', /*³*/'\243', /* */'\264', /*µ*/'\245', /*¶*/'\246', /* */'\267', 
  /* */'\270', /*¹*/'\251', /*º*/'\252', /*»*/'\253', /*¼*/'\254', /* */'\275', /*¾*/'\256', /*¿*/'\257', 
  /*À*/'\300', /*Á*/'\301', /*Â*/'\302', /*Ã*/'\303', /*Ä*/'\304', /*Å*/'\305', /*Æ*/'\306', /*Ç*/'\307', 
  /*È*/'\310', /*É*/'\311', /*Ê*/'\312', /*Ë*/'\313', /*Ì*/'\314', /*Í*/'\315', /*Î*/'\316', /*Ï*/'\317', 
  /*Ð*/'\320', /*Ñ*/'\321', /*Ò*/'\322', /*Ó*/'\323', /*Ô*/'\324', /*Õ*/'\325', /*Ö*/'\326', /*×*/'\327', 
  /*Ø*/'\330', /*Ù*/'\331', /*Ú*/'\332', /*Û*/'\333', /*Ü*/'\334', /*Ý*/'\335', /*Þ*/'\336', /*ß*/'\337', 
  /*à*/'\300', /*á*/'\301', /*â*/'\302', /*ã*/'\303', /*ä*/'\304', /*å*/'\305', /*æ*/'\306', /*ç*/'\307', 
  /*è*/'\310', /*é*/'\311', /*ê*/'\312', /*ë*/'\313', /*ì*/'\314', /*í*/'\315', /*î*/'\316', /*ï*/'\317', 
  /*ð*/'\320', /*ñ*/'\321', /*ò*/'\322', /*ó*/'\323', /*ô*/'\324', /*õ*/'\325', /*ö*/'\326', /*÷*/'\367', 
  /*ø*/'\330', /*ù*/'\331', /*ú*/'\332', /*û*/'\333', /*ü*/'\334', /*ý*/'\335', /*þ*/'\336', /* */'\377'
};

static const OCTET _trans_lower_8859_2[] = {
  /* */'\000', /* */'\001', /* */'\002', /* */'\003', /* */'\004', /* */'\005', /* */'\006', /* */'\007', 
  /* */'\010', /* */'\011', /* */'\012', /* */'\013', /* */'\014', /* */'\015', /* */'\016', /* */'\017', 
  /* */'\020', /* */'\021', /* */'\022', /* */'\023', /* */'\024', /* */'\025', /* */'\026', /* */'\027', 
  /* */'\030', /* */'\031', /* */'\032', /* */'\033', /* */'\034', /* */'\035', /* */'\036', /* */'\037', 
  /* */'\040', /*!*/'\041', /*"*/'\042', /*#*/'\043', /*$*/'\044', /*%*/'\045', /*&*/'\046', /*'*/'\047', 
  /*(*/'\050', /*)*/'\051', /***/'\052', /*+*/'\053', /*,*/'\054', /*-*/'\055', /*.*/'\056', /* / */'\057', 
  /*0*/'\060', /*1*/'\061', /*2*/'\062', /*3*/'\063', /*4*/'\064', /*5*/'\065', /*6*/'\066', /*7*/'\067', 
  /*8*/'\070', /*9*/'\071', /*:*/'\072', /*;*/'\073', /*<*/'\074', /*=*/'\075', /*>*/'\076', /*?*/'\077', 
  /*@*/'\100', /*A*/'\141', /*B*/'\142', /*C*/'\143', /*D*/'\144', /*E*/'\145', /*F*/'\146', /*G*/'\147', 
  /*H*/'\150', /*I*/'\151', /*J*/'\152', /*K*/'\153', /*L*/'\154', /*M*/'\155', /*N*/'\156', /*O*/'\157', 
  /*P*/'\160', /*Q*/'\161', /*R*/'\162', /*S*/'\163', /*T*/'\164', /*U*/'\165', /*V*/'\166', /*W*/'\167', 
  /*X*/'\170', /*Y*/'\171', /*Z*/'\172', /*[*/'\133', /*\*/'\134', /*]*/'\135', /*^*/'\136', /*_*/'\137', 
  /*`*/'\140', /*a*/'\141', /*b*/'\142', /*c*/'\143', /*d*/'\144', /*e*/'\145', /*f*/'\146', /*g*/'\147', 
  /*h*/'\150', /*i*/'\151', /*j*/'\152', /*k*/'\153', /*l*/'\154', /*m*/'\155', /*n*/'\156', /*o*/'\157', 
  /*p*/'\160', /*q*/'\161', /*r*/'\162', /*s*/'\163', /*t*/'\164', /*u*/'\165', /*v*/'\166', /*w*/'\167', 
  /*x*/'\170', /*y*/'\171', /*z*/'\172', /*{*/'\173', /*|*/'\174', /*}*/'\175', /*~*/'\176', /* */'\177', 
  /* */'\200', /* */'\201', /* */'\202', /* */'\203', /* */'\204', /* */'\205', /* */'\206', /* */'\207', 
  /* */'\210', /* */'\211', /* */'\212', /* */'\213', /* */'\214', /* */'\215', /* */'\216', /* */'\217', 
  /* */'\220', /* */'\221', /* */'\222', /* */'\223', /* */'\224', /* */'\225', /* */'\226', /* */'\227', 
  /* */'\230', /* */'\231', /* */'\232', /* */'\233', /* */'\234', /* */'\235', /* */'\236', /* */'\237', 
  /* */'\240', /*¡*/'\261', /* */'\242', /*£*/'\263', /* */'\244', /*¥*/'\265', /*¦*/'\266', /* */'\247', 
  /* */'\250', /*©*/'\271', /*ª*/'\272', /*«*/'\273', /*¬*/'\274', /* */'\255', /*®*/'\276', /*¯*/'\277', 
  /* */'\260', /*±*/'\261', /* */'\262', /*³*/'\263', /* */'\264', /*µ*/'\265', /*¶*/'\266', /* */'\267', 
  /* */'\270', /*¹*/'\271', /*º*/'\272', /*»*/'\273', /*¼*/'\274', /* */'\275', /*¾*/'\276', /*¿*/'\277', 
  /*À*/'\340', /*Á*/'\341', /*Â*/'\342', /*Ã*/'\343', /*Ä*/'\344', /*Å*/'\345', /*Æ*/'\346', /*Ç*/'\347', 
  /*È*/'\350', /*É*/'\351', /*Ê*/'\352', /*Ë*/'\353', /*Ì*/'\354', /*Í*/'\355', /*Î*/'\356', /*Ï*/'\357', 
  /*Ð*/'\360', /*Ñ*/'\361', /*Ò*/'\362', /*Ó*/'\363', /*Ô*/'\364', /*Õ*/'\365', /*Ö*/'\366', /*×*/'\327', 
  /*Ø*/'\370', /*Ù*/'\371', /*Ú*/'\372', /*Û*/'\373', /*Ü*/'\374', /*Ý*/'\375', /*Þ*/'\376', /*ß*/'\337', 
  /*à*/'\340', /*á*/'\341', /*â*/'\342', /*ã*/'\343', /*ä*/'\344', /*å*/'\345', /*æ*/'\346', /*ç*/'\347', 
  /*è*/'\350', /*é*/'\351', /*ê*/'\352', /*ë*/'\353', /*ì*/'\354', /*í*/'\355', /*î*/'\356', /*ï*/'\357', 
  /*ð*/'\360', /*ñ*/'\361', /*ò*/'\362', /*ó*/'\363', /*ô*/'\364', /*õ*/'\365', /*ö*/'\366', /*÷*/'\367', 
  /*ø*/'\370', /*ù*/'\371', /*ú*/'\372', /*û*/'\373', /*ü*/'\374', /*ý*/'\375', /*þ*/'\376', /* */'\377'
};

/*
'0080 = 80   :0080 "HIGH NULL"
'0081 = 81   :0081 "HIGH CONTROL-A"
'0082 = 82   :0082 BREAK PERMITTED HERE (BPH)
'0083 = 83   :0083 NO BREAK HERE (NBH)
'0084 = 84   :0084 INDEX (IND) (deprecated)
'0085 = 85   :0085 NEXT LINE (NEL)
'0086 = 86   :0086 START OF SELECTED AREA (SSA)
'0087 = 87   :0087 END OF SELECTED AREA (ESA)
'0088 = 88   :0088 CHARACTER TABULATION SET (HTS)
'0089 = 89   :0089 CHARACTER TABULATION WITH JUSTIFICATION (HTJ)
'008A = 8A   :008A LINE TABULATION SET (VTS)
'008B = 8B   :008B PARTIAL LINE FORWARD (PLD)
'008C = 8C   :008C PARTIAL LINE BACKWARD (PLU)
'008D = 8D   :008D REVERSE LINE FEED (RI)
'008E = 8E   :008E SINGLE-SHIFT TWO (SS2)
'008F = 8F   :008F SINGLE-SHIFT THREE (SS3)
'0090 = 90   :0090 DEVICE CONTROL STRING (DCS)
'0091 = 91   :0091 PRIVATE USE ONE (PU1)
'0092 = 92   :0092 PRIVATE USE TWO (PU2)
'0093 = 93   :0093 SET TRANSMIT STATE (STS)
'0094 = 94   :0094 CANCEL CHARACTER (CCH)
'0095 = 95   :0095 MESSAGE WAITING (MW)
'0096 = 96   :0096 START OF GUARDED AREA (SPA)
'0097 = 97   :0097 END OF GUARDED AREA (EPA)
'0098 = 98   :0098 START OF STRING (SOS)
'0099 = 99   :0099 "HIGH CONTROL-Y"
'009A = 9A   :009A SINGLE CHARACTER INTRODUCER (SCI)
'009B = 9B   :009B CONTROL SEQUENCE INTRODUCER (CSI)
'009C = 9C   :009C STRING TERMINATOR (ST)
'009D = 9D   :009D OPERATING SYSTEM COMMAND (OSC)
'009E = 9E   :009E PRIVACY MESSAGE (PM)
'009F = 9F   :009F APPLICATION PROGRAM COMMAND (APC)

'00A0 = A0   :00A0 NO-BREAK SPACE
'0104 = A1   :0104 LATIN CAPITAL LETTER A WITH OGONEK
'02D8 = A2   :02D8 BREVE
'0141 = A3   :0141 LATIN CAPITAL LETTER L WITH STROKE
'00A4 = A4   :00A4 CURRENCY SIGN
'013D = A5   :013D LATIN CAPITAL LETTER L WITH CARON
'015A = A6   :015A LATIN CAPITAL LETTER S WITH ACUTE
'00A7 = A7   :00A7 SECTION SIGN
'00A8 = A8   :00A8 DIAERESIS
'0160 = A9   :0160 LATIN CAPITAL LETTER S WITH CARON
'015E = AA   :015E LATIN CAPITAL LETTER S WITH CEDILLA
'0164 = AB   :0164 LATIN CAPITAL LETTER T WITH CARON
'0179 = AC   :0179 LATIN CAPITAL LETTER Z WITH ACUTE
'00AD = AD   :00AD SOFT HYPHEN
'017D = AE   :017D LATIN CAPITAL LETTER Z WITH CARON
'017B = AF   :017B LATIN CAPITAL LETTER Z WITH DOT ABOVE

'00B0 = B0   :00B0 DEGREE SIGN
'0105 = B1   :0105 LATIN SMALL LETTER A WITH OGONEK
'02DB = B2   :02DB OGONEK
'0142 = B3   :0142 LATIN SMALL LETTER L WITH STROKE
'00B4 = B4   :00B4 ACUTE ACCENT
'013E = B5   :013E LATIN SMALL LETTER L WITH CARON
'015B = B6   :015B LATIN SMALL LETTER S WITH ACUTE
'02C7 = B7   :02C7 CARON (Mandarin Chinese third tone)
'00B8 = B8   :00B8 CEDILLA
'0161 = B9   :0161 LATIN SMALL LETTER S WITH CARON
'015F = BA   :015F LATIN SMALL LETTER S WITH CEDILLA
'0165 = BB   :0165 LATIN SMALL LETTER T WITH CARON
'017A = BC   :017A LATIN SMALL LETTER Z WITH ACUTE
'02DD = BD   :02DD DOUBLE ACUTE ACCENT
'017E = BE   :017E LATIN SMALL LETTER Z WITH CARON
'017C = BF   :017C LATIN SMALL LETTER Z WITH DOT ABOVE

'0154 = C0   :0154 LATIN CAPITAL LETTER R WITH ACUTE
'00C1 = C1   :00C1 LATIN CAPITAL LETTER A WITH ACUTE
'00C2 = C2   :00C2 LATIN CAPITAL LETTER A WITH CIRCUMFLEX
'0102 = C3   :0102 LATIN CAPITAL LETTER A WITH BREVE
'00C4 = C4   :00C4 LATIN CAPITAL LETTER A WITH DIAERESIS
'0139 = C5   :0139 LATIN CAPITAL LETTER L WITH ACUTE
'0106 = C6   :0106 LATIN CAPITAL LETTER C WITH ACUTE
'00C7 = C7   :00C7 LATIN CAPITAL LETTER C WITH CEDILLA
'010C = C8   :010C LATIN CAPITAL LETTER C WITH CARON
'00C9 = C9   :00C9 LATIN CAPITAL LETTER E WITH ACUTE
'0118 = CA   :0118 LATIN CAPITAL LETTER E WITH OGONEK
'00CB = CB   :00CB LATIN CAPITAL LETTER E WITH DIAERESIS
'011A = CC   :011A LATIN CAPITAL LETTER E WITH CARON
'00CD = CD   :00CD LATIN CAPITAL LETTER I WITH ACUTE
'00CE = CE   :00CE LATIN CAPITAL LETTER I WITH CIRCUMFLEX
'010E = CF   :010E LATIN CAPITAL LETTER D WITH CARON

'0110 = D0   :0110 LATIN CAPITAL LETTER D WITH STROKE
'0143 = D1   :0143 LATIN CAPITAL LETTER N WITH ACUTE
'0147 = D2   :0147 LATIN CAPITAL LETTER N WITH CARON
'00D3 = D3   :00D3 LATIN CAPITAL LETTER O WITH ACUTE
'00D4 = D4   :00D4 LATIN CAPITAL LETTER O WITH CIRCUMFLEX
'0150 = D5   :0150 LATIN CAPITAL LETTER O WITH DOUBLE ACUTE
'00D6 = D6   :00D6 LATIN CAPITAL LETTER O WITH DIAERESIS
'00D7 = D7   :00D7 MULTIPLICATION SIGN
'0158 = D8   :0158 LATIN CAPITAL LETTER R WITH CARON
'016E = D9   :016E LATIN CAPITAL LETTER U WITH RING ABOVE
'00DA = DA   :00DA LATIN CAPITAL LETTER U WITH ACUTE
'0170 = DB   :0170 LATIN CAPITAL LETTER U WITH DOUBLE ACUTE
'00DC = DC   :00DC LATIN CAPITAL LETTER U WITH DIAERESIS
'00DD = DD   :00DD LATIN CAPITAL LETTER Y WITH ACUTE
'0162 = DE   :0162 LATIN CAPITAL LETTER T WITH CEDILLA
'00DF = DF   :00DF LATIN SMALL LETTER SHARP S (German)

'0155 = E0   :0155 LATIN SMALL LETTER R WITH ACUTE
'00E1 = E1   :00E1 LATIN SMALL LETTER A WITH ACUTE
'00E2 = E2   :00E2 LATIN SMALL LETTER A WITH CIRCUMFLEX
'0103 = E3   :0103 LATIN SMALL LETTER A WITH BREVE
'00E4 = E4   :00E4 LATIN SMALL LETTER A WITH DIAERESIS
'013A = E5   :013A LATIN SMALL LETTER L WITH ACUTE
'0107 = E6   :0107 LATIN SMALL LETTER C WITH ACUTE
'00E7 = E7   :00E7 LATIN SMALL LETTER C WITH CEDILLA
'010D = E8   :010D LATIN SMALL LETTER C WITH CARON
'00E9 = E9   :00E9 LATIN SMALL LETTER E WITH ACUTE
'0119 = EA   :0119 LATIN SMALL LETTER E WITH OGONEK
'00EB = EB   :00EB LATIN SMALL LETTER E WITH DIAERESIS
'011B = EC   :011B LATIN SMALL LETTER E WITH CARON
'00ED = ED   :00ED LATIN SMALL LETTER I WITH ACUTE
'00EE = EE   :00EE LATIN SMALL LETTER I WITH CIRCUMFLEX
'010F = EF   :010F LATIN SMALL LETTER D WITH CARON

'0111 = F0   :0111 LATIN SMALL LETTER D WITH STROKE
'0144 = F1   :0144 LATIN SMALL LETTER N WITH ACUTE
'0148 = F2   :0148 LATIN SMALL LETTER N WITH CARON
'00F3 = F3   :00F3 LATIN SMALL LETTER O WITH ACUTE
'00F4 = F4   :00F4 LATIN SMALL LETTER O WITH CIRCUMFLEX
'0151 = F5   :0151 LATIN SMALL LETTER O WITH DOUBLE ACUTE
'00F6 = F6   :00F6 LATIN SMALL LETTER O WITH DIAERESIS
'00F7 = F7   :00F7 DIVISION SIGN
'0159 = F8   :0159 LATIN SMALL LETTER R WITH CARON
'016F = F9   :016F LATIN SMALL LETTER U WITH RING ABOVE
'00FA = FA   :00FA LATIN SMALL LETTER U WITH ACUTE
'0171 = FB   :0171 LATIN SMALL LETTER U WITH DOUBLE ACUTE
'00FC = FC   :00FC LATIN SMALL LETTER U WITH DIAERESIS
'00FD = FD   :00FD LATIN SMALL LETTER Y WITH ACUTE
'0163 = FE   :0163 LATIN SMALL LETTER T WITH CEDILLA
'02D9 = FF   :02D9 DOT ABOVE (Mandarin Chinese light tone)
*/


static const char _soundex_8859_2[] = {
  /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, 
  /* */ -1, /* */ -1, /* */_SP, /* */ -1, /* */ -1, /* */_SP, /* */ -1, /* */ -1, 
  /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, 
  /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, 
  /* */_SP, /*!*/ -1, /*"*/ -1, /*#*/ -1, /*$*/ -1, /*%*/ -1, /*&*/ -1, /*'*/ -1, 
  /*(*/ -1, /*)*/ -1, /***/ -1, /*+*/ -1, /*,*/ -1, /*-*/ -1, /*.*/ -1, /* / */ -1, 
  /*0*/ -1, /*1*/ -1, /*2*/ -1, /*3*/ -1, /*4*/ -1, /*5*/ -1, /*6*/ -1, /*7*/ -1, 
  /*8*/ -1, /*9*/ -1, /*:*/ -1, /*;*/ -1, /*<*/ -1, /*=*/ -1, /*>*/ -1, /*?*/ -1, 
  /*@*/ -1, /*A*/  7, /*B*/  1, /*C*/  2, /*D*/  3, /*E*/  7, /*F*/  9, /*G*/  2, 
  /*H*/  0, /*I*/  7, /*J*/  2, /*K*/  2, /*L*/  4, /*M*/  5, /*N*/  5, /*O*/  7, 
  /*P*/  1, /*Q*/  2, /*R*/  6, /*S*/  9, /*T*/  3, /*U*/  7, /*V*/  9, /*W*/  0, 
  /*X*/  2, /*Y*/  7, /*Z*/  9, /*[*/ -1, /*\*/ -1, /*]*/ -1, /*^*/ -1, /*_*/ -1, 
  /*`*/ -1, /*a*/  7, /*b*/  1, /*c*/  2, /*d*/  3, /*e*/  7, /*f*/  9, /*g*/  2, 
  /*h*/  0, /*i*/  7, /*j*/  2, /*k*/  2, /*l*/  4, /*m*/  5, /*n*/  5, /*o*/  7, 
  /*p*/  1, /*q*/  2, /*r*/  6, /*s*/  9, /*t*/  3, /*u*/  7, /*v*/  9, /*w*/  0, 
  /*x*/  2, /*y*/  7, /*z*/  9, /*{*/ -1, /*|*/ -1, /*}*/ -1, /*~*/ -1, /* */ -1, 
  /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, 
  /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, 
  /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, 
  /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, /* */ -1, 
  /* */ -1, /*¡*/-95, /* */ -1, /*£*/-93, /* */ -1, /*¥*/-91, /*¦*/-90, /* */ -1, 
  /* */ -1, /*©*/-87, /*ª*/-86, /*«*/-85, /*¬*/-84, /* */ -1, /*®*/-82, /*¯*/-81, 
  /* */ -1, /*±*/-95, /* */ -1, /*³*/-93, /* */ -1, /*µ*/-91, /*¶*/-90, /* */ -1, 
  /* */ -1, /*¹*/-87, /*º*/-86, /*»*/-85, /*¼*/-84, /* */ -1, /*¾*/-82, /*¿*/-81, 
  /*À*/-64, /*Á*/-63, /*Â*/-62, /*Ã*/-61, /*Ä*/-60, /*Å*/-59, /*Æ*/-58, /*Ç*/-57, 
  /*È*/-56, /*É*/-55, /*Ê*/-54, /*Ë*/-53, /*Ì*/-52, /*Í*/-51, /*Î*/-50, /*Ï*/-49, 
  /*Ð*/-48, /*Ñ*/-47, /*Ò*/-46, /*Ó*/-45, /*Ô*/-44, /*Õ*/-43, /*Ö*/-42, /*×*/ -1, 
  /*Ø*/-40, /*Ù*/-39, /*Ú*/-38, /*Û*/-37, /*Ü*/-36, /*Ý*/-35, /*Þ*/-34, /*ß*/-33, 
  /*à*/-64, /*á*/-63, /*â*/-62, /*ã*/-61, /*ä*/-60, /*å*/-59, /*æ*/-58, /*ç*/-57, 
  /*è*/-56, /*é*/-55, /*ê*/-54, /*ë*/-53, /*ì*/-52, /*í*/-51, /*î*/-50, /*ï*/-49, 
  /*ð*/-48, /*ñ*/-47, /*ò*/-46, /*ó*/-45, /*ô*/-44, /*õ*/-43, /*ö*/-42, /*÷*/ -1, 
  /*ø*/-40, /*ù*/-39, /*ú*/-38, /*û*/-37, /*ü*/-36, /*ý*/-35, /*þ*/-34, /* */ -1
};

static const OCTET _cclass_8859_2[] = {
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
 '\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
 '\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
 '\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
 '\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
 '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
 '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
 '\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
 '\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
 '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
 '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
 '\170', '\171', '\172', '\173', '\174', '\175', '\176', '\000',
 '\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
 '\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
 '\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
 '\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
 '\000', '\261', '\242', '\263', '\244', '\265', '\266', '\247',
 '\250', '\271', '\272', '\273', '\274', '\255', '\276', '\277',
 '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
 '\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
 '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
 '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
 '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\327',
 '\370', '\371', '\372', '\373', '\374', '\375', '\376', '\337',
 '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
 '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
 '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
 '\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377'
};

static int func8859_2(const void *s1, const void *s2)
{
  return compareFunc(s1, s2, _cclass_8859_2);
}

static int Compare8859_2(const void *s1, const void *s2, size_t len)
{
  return compareFunc(s1, s2, _cclass_8859_2, len);
}


static int SIS8859_2(const void *s1, const void *s2)
{
  return SIScompareFunc(s1, s2, _ib_ctype_8859_2 /* _cclass_8859_2 */);
}


#if 0
static const unsigned char _ib_ctype_8859_3[] = {
  /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', 
  /* */'\040', /* */'\050', /* */'\050', /* */'\050', /* */'\050', /* */'\050', /* */'\040', /* */'\040', 
  /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', 
  /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', 
  /* */'\110', /*!*/'\020', /*"*/'\020', /*#*/'\020', /*$*/'\020', /*%*/'\020', /*&*/'\020', /*'*/'\020', 
  /*(*/'\020', /*)*/'\020', /***/'\020', /*+*/'\020', /*,*/'\020', /*-*/'\020', /*.*/'\020', /* / */'\020', 
  /*0*/'\204', /*1*/'\204', /*2*/'\204', /*3*/'\204', /*4*/'\204', /*5*/'\204', /*6*/'\204', /*7*/'\204', 
  /*8*/'\204', /*9*/'\204', /*:*/'\020', /*;*/'\020', /*<*/'\020', /*=*/'\020', /*>*/'\020', /*?*/'\020', 
  /*@*/'\020', /*A*/'\201', /*B*/'\201', /*C*/'\201', /*D*/'\201', /*E*/'\201', /*F*/'\201', /*G*/'\001', 
  /*H*/'\001', /*I*/'\001', /*J*/'\001', /*K*/'\001', /*L*/'\001', /*M*/'\001', /*N*/'\001', /*O*/'\001', 
  /*P*/'\001', /*Q*/'\001', /*R*/'\001', /*S*/'\001', /*T*/'\001', /*U*/'\001', /*V*/'\001', /*W*/'\001', 
  /*X*/'\001', /*Y*/'\001', /*Z*/'\001', /*[*/'\020', /*\*/'\020', /*]*/'\020', /*^*/'\020', /*_*/'\020', 
  /*`*/'\020', /*a*/'\202', /*b*/'\202', /*c*/'\202', /*d*/'\202', /*e*/'\202', /*f*/'\202', /*g*/'\002', 
  /*h*/'\002', /*i*/'\002', /*j*/'\002', /*k*/'\002', /*l*/'\002', /*m*/'\002', /*n*/'\002', /*o*/'\002', 
  /*p*/'\002', /*q*/'\002', /*r*/'\002', /*s*/'\002', /*t*/'\002', /*u*/'\002', /*v*/'\002', /*w*/'\002', 
  /*x*/'\002', /*y*/'\002', /*z*/'\002', /*{*/'\020', /*|*/'\020', /*}*/'\020', /*~*/'\020', /* */'\040', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\110', /*¡*/'\020', /*¢*/'\020', /*£*/'\020', /*¤*/'\020', /*¥*/'\020', /*¦*/'\020', /*§*/'\020', 
  /*¨*/'\020', /*©*/'\020', /*ª*/'\020', /*«*/'\020', /*¬*/'\020', /*­*/'\020', /*®*/'\020', /*¯*/'\020', 
  /*°*/'\020', /*±*/'\020', /*²*/'\020', /*³*/'\020', /*´*/'\020', /*µ*/'\020', /*¶*/'\020', /*·*/'\020', 
  /*¸*/'\020', /*¹*/'\020', /*º*/'\020', /*»*/'\020', /*¼*/'\020', /*½*/'\020', /*¾*/'\020', /*¿*/'\020', 
  /*À*/'\001', /*Á*/'\001', /*Â*/'\001', /*Ã*/'\001', /*Ä*/'\001', /*Å*/'\001', /*Æ*/'\001', /*Ç*/'\001', 
  /*È*/'\001', /*É*/'\001', /*Ê*/'\001', /*Ë*/'\001', /*Ì*/'\001', /*Í*/'\001', /*Î*/'\001', /*Ï*/'\001', 
  /*Ð*/'\001', /*Ñ*/'\001', /*Ò*/'\001', /*Ó*/'\001', /*Ô*/'\001', /*Õ*/'\001', /*Ö*/'\001', /*×*/'\020', 
  /*Ø*/'\001', /*Ù*/'\001', /*Ú*/'\001', /*Û*/'\001', /*Ü*/'\001', /*Ý*/'\001', /*Þ*/'\001', /*ß*/'\002', 
  /*à*/'\002', /*á*/'\002', /*â*/'\002', /*ã*/'\002', /*ä*/'\002', /*å*/'\002', /*æ*/'\002', /*ç*/'\002', 
  /*è*/'\002', /*é*/'\002', /*ê*/'\002', /*ë*/'\002', /*ì*/'\002', /*í*/'\002', /*î*/'\002', /*ï*/'\002', 
  /*ð*/'\002', /*ñ*/'\002', /*ò*/'\002', /*ó*/'\002', /*ô*/'\002', /*õ*/'\002', /*ö*/'\002', /*÷*/'\020', 
  /*ø*/'\002', /*ù*/'\002', /*ú*/'\002', /*û*/'\002', /*ü*/'\002', /*ý*/'\002', /*þ*/'\002', /*ÿ*/'\002'
};
#endif

static const OCTET _trans_upper_8859_3[] = {
  /* */'\000', /* */'\001', /* */'\002', /* */'\003', /* */'\004', /* */'\005', /* */'\006', /* */'\007', 
  /* */'\010', /* */'\011', /* */'\012', /* */'\013', /* */'\014', /* */'\015', /* */'\016', /* */'\017', 
  /* */'\020', /* */'\021', /* */'\022', /* */'\023', /* */'\024', /* */'\025', /* */'\026', /* */'\027', 
  /* */'\030', /* */'\031', /* */'\032', /* */'\033', /* */'\034', /* */'\035', /* */'\036', /* */'\037', 
  /* */'\040', /*!*/'\041', /*"*/'\042', /*#*/'\043', /*$*/'\044', /*%*/'\045', /*&*/'\046', /*'*/'\047', 
  /*(*/'\050', /*)*/'\051', /***/'\052', /*+*/'\053', /*,*/'\054', /*-*/'\055', /*.*/'\056', /* / */'\057', 
  /*0*/'\060', /*1*/'\061', /*2*/'\062', /*3*/'\063', /*4*/'\064', /*5*/'\065', /*6*/'\066', /*7*/'\067', 
  /*8*/'\070', /*9*/'\071', /*:*/'\072', /*;*/'\073', /*<*/'\074', /*=*/'\075', /*>*/'\076', /*?*/'\077', 
  /*@*/'\100', /*A*/'\101', /*B*/'\102', /*C*/'\103', /*D*/'\104', /*E*/'\105', /*F*/'\106', /*G*/'\107', 
  /*H*/'\110', /*I*/'\111', /*J*/'\112', /*K*/'\113', /*L*/'\114', /*M*/'\115', /*N*/'\116', /*O*/'\117', 
  /*P*/'\120', /*Q*/'\121', /*R*/'\122', /*S*/'\123', /*T*/'\124', /*U*/'\125', /*V*/'\126', /*W*/'\127', 
  /*X*/'\130', /*Y*/'\131', /*Z*/'\132', /*[*/'\133', /*\*/'\134', /*]*/'\135', /*^*/'\136', /*_*/'\137', 
  /*`*/'\140', /*a*/'\101', /*b*/'\102', /*c*/'\103', /*d*/'\104', /*e*/'\105', /*f*/'\106', /*g*/'\107', 
  /*h*/'\110', /*i*/'\111', /*j*/'\112', /*k*/'\113', /*l*/'\114', /*m*/'\115', /*n*/'\116', /*o*/'\117', 
  /*p*/'\120', /*q*/'\121', /*r*/'\122', /*s*/'\123', /*t*/'\124', /*u*/'\125', /*v*/'\126', /*w*/'\127', 
  /*x*/'\130', /*y*/'\131', /*z*/'\132', /*{*/'\173', /*|*/'\174', /*}*/'\175', /*~*/'\176', /* */'\177', 
  /* */'\200', /* */'\201', /* */'\202', /* */'\203', /* */'\204', /* */'\205', /* */'\206', /* */'\207', 
  /* */'\210', /* */'\211', /* */'\212', /* */'\213', /* */'\214', /* */'\215', /* */'\216', /* */'\217', 
  /* */'\220', /* */'\221', /* */'\222', /* */'\223', /* */'\224', /* */'\225', /* */'\226', /* */'\227', 
  /* */'\230', /* */'\231', /* */'\232', /* */'\233', /* */'\234', /* */'\235', /* */'\236', /* */'\237', 
  /* */'\240', /*¡*/'\241', /*¢*/'\242', /*£*/'\243', /*¤*/'\244', /*¥*/'\245', /*¦*/'\246', /*§*/'\247', 
  /*¨*/'\250', /*©*/'\251', /*ª*/'\252', /*«*/'\253', /*¬*/'\254', /*­*/'\255', /*®*/'\256', /*¯*/'\257', 
  /*°*/'\260', /*±*/'\261', /*²*/'\262', /*³*/'\263', /*´*/'\264', /*µ*/'\265', /*¶*/'\266', /*·*/'\267', 
  /*¸*/'\270', /*¹*/'\271', /*º*/'\272', /*»*/'\273', /*¼*/'\274', /*½*/'\275', /*¾*/'\276', /*¿*/'\277', 
  /*À*/'\300', /*Á*/'\301', /*Â*/'\302', /*Ã*/'\303', /*Ä*/'\304', /*Å*/'\305', /*Æ*/'\306', /*Ç*/'\307', 
  /*È*/'\310', /*É*/'\311', /*Ê*/'\312', /*Ë*/'\313', /*Ì*/'\314', /*Í*/'\315', /*Î*/'\316', /*Ï*/'\317', 
  /*Ð*/'\320', /*Ñ*/'\321', /*Ò*/'\322', /*Ó*/'\323', /*Ô*/'\324', /*Õ*/'\325', /*Ö*/'\326', /*×*/'\327', 
  /*Ø*/'\330', /*Ù*/'\331', /*Ú*/'\332', /*Û*/'\333', /*Ü*/'\334', /*Ý*/'\335', /*Þ*/'\336', /*ß*/'\337', 
  /*à*/'\300', /*á*/'\301', /*â*/'\302', /*ã*/'\303', /*ä*/'\304', /*å*/'\305', /*æ*/'\306', /*ç*/'\307', 
  /*è*/'\310', /*é*/'\311', /*ê*/'\312', /*ë*/'\313', /*ì*/'\314', /*í*/'\315', /*î*/'\316', /*ï*/'\317', 
  /*ð*/'\320', /*ñ*/'\321', /*ò*/'\322', /*ó*/'\323', /*ô*/'\324', /*õ*/'\325', /*ö*/'\326', /*÷*/'\367', 
  /*ø*/'\330', /*ù*/'\331', /*ú*/'\332', /*û*/'\333', /*ü*/'\334', /*ý*/'\335', /*þ*/'\336', /*ÿ*/'\377'
};

static const OCTET _trans_lower_8859_3[] = {
  /* */'\000', /* */'\001', /* */'\002', /* */'\003', /* */'\004', /* */'\005', /* */'\006', /* */'\007', 
  /* */'\010', /* */'\011', /* */'\012', /* */'\013', /* */'\014', /* */'\015', /* */'\016', /* */'\017', 
  /* */'\020', /* */'\021', /* */'\022', /* */'\023', /* */'\024', /* */'\025', /* */'\026', /* */'\027', 
  /* */'\030', /* */'\031', /* */'\032', /* */'\033', /* */'\034', /* */'\035', /* */'\036', /* */'\037', 
  /* */'\040', /*!*/'\041', /*"*/'\042', /*#*/'\043', /*$*/'\044', /*%*/'\045', /*&*/'\046', /*'*/'\047', 
  /*(*/'\050', /*)*/'\051', /***/'\052', /*+*/'\053', /*,*/'\054', /*-*/'\055', /*.*/'\056', /* / */'\057', 
  /*0*/'\060', /*1*/'\061', /*2*/'\062', /*3*/'\063', /*4*/'\064', /*5*/'\065', /*6*/'\066', /*7*/'\067', 
  /*8*/'\070', /*9*/'\071', /*:*/'\072', /*;*/'\073', /*<*/'\074', /*=*/'\075', /*>*/'\076', /*?*/'\077', 
  /*@*/'\100', /*A*/'\141', /*B*/'\142', /*C*/'\143', /*D*/'\144', /*E*/'\145', /*F*/'\146', /*G*/'\147', 
  /*H*/'\150', /*I*/'\151', /*J*/'\152', /*K*/'\153', /*L*/'\154', /*M*/'\155', /*N*/'\156', /*O*/'\157', 
  /*P*/'\160', /*Q*/'\161', /*R*/'\162', /*S*/'\163', /*T*/'\164', /*U*/'\165', /*V*/'\166', /*W*/'\167', 
  /*X*/'\170', /*Y*/'\171', /*Z*/'\172', /*[*/'\133', /*\*/'\134', /*]*/'\135', /*^*/'\136', /*_*/'\137', 
  /*`*/'\140', /*a*/'\141', /*b*/'\142', /*c*/'\143', /*d*/'\144', /*e*/'\145', /*f*/'\146', /*g*/'\147', 
  /*h*/'\150', /*i*/'\151', /*j*/'\152', /*k*/'\153', /*l*/'\154', /*m*/'\155', /*n*/'\156', /*o*/'\157', 
  /*p*/'\160', /*q*/'\161', /*r*/'\162', /*s*/'\163', /*t*/'\164', /*u*/'\165', /*v*/'\166', /*w*/'\167', 
  /*x*/'\170', /*y*/'\171', /*z*/'\172', /*{*/'\173', /*|*/'\174', /*}*/'\175', /*~*/'\176', /* */'\177', 
  /* */'\200', /* */'\201', /* */'\202', /* */'\203', /* */'\204', /* */'\205', /* */'\206', /* */'\207', 
  /* */'\210', /* */'\211', /* */'\212', /* */'\213', /* */'\214', /* */'\215', /* */'\216', /* */'\217', 
  /* */'\220', /* */'\221', /* */'\222', /* */'\223', /* */'\224', /* */'\225', /* */'\226', /* */'\227', 
  /* */'\230', /* */'\231', /* */'\232', /* */'\233', /* */'\234', /* */'\235', /* */'\236', /* */'\237', 
  /* */'\240', /*¡*/'\241', /*¢*/'\242', /*£*/'\243', /*¤*/'\244', /*¥*/'\245', /*¦*/'\246', /*§*/'\247', 
  /*¨*/'\250', /*©*/'\251', /*ª*/'\252', /*«*/'\253', /*¬*/'\254', /*­*/'\255', /*®*/'\256', /*¯*/'\257', 
  /*°*/'\260', /*±*/'\261', /*²*/'\262', /*³*/'\263', /*´*/'\264', /*µ*/'\265', /*¶*/'\266', /*·*/'\267', 
  /*¸*/'\270', /*¹*/'\271', /*º*/'\272', /*»*/'\273', /*¼*/'\274', /*½*/'\275', /*¾*/'\276', /*¿*/'\277', 
  /*À*/'\340', /*Á*/'\341', /*Â*/'\342', /*Ã*/'\343', /*Ä*/'\344', /*Å*/'\345', /*Æ*/'\346', /*Ç*/'\347', 
  /*È*/'\350', /*É*/'\351', /*Ê*/'\352', /*Ë*/'\353', /*Ì*/'\354', /*Í*/'\355', /*Î*/'\356', /*Ï*/'\357', 
  /*Ð*/'\360', /*Ñ*/'\361', /*Ò*/'\362', /*Ó*/'\363', /*Ô*/'\364', /*Õ*/'\365', /*Ö*/'\366', /*×*/'\327', 
  /*Ø*/'\370', /*Ù*/'\371', /*Ú*/'\372', /*Û*/'\373', /*Ü*/'\374', /*Ý*/'\375', /*Þ*/'\376', /*ß*/'\337', 
  /*à*/'\340', /*á*/'\341', /*â*/'\342', /*ã*/'\343', /*ä*/'\344', /*å*/'\345', /*æ*/'\346', /*ç*/'\347', 
  /*è*/'\350', /*é*/'\351', /*ê*/'\352', /*ë*/'\353', /*ì*/'\354', /*í*/'\355', /*î*/'\356', /*ï*/'\357', 
  /*ð*/'\360', /*ñ*/'\361', /*ò*/'\362', /*ó*/'\363', /*ô*/'\364', /*õ*/'\365', /*ö*/'\366', /*÷*/'\367', 
  /*ø*/'\370', /*ù*/'\371', /*ú*/'\372', /*û*/'\373', /*ü*/'\374', /*ý*/'\375', /*þ*/'\376', /*ÿ*/'\377'
};

static const OCTET _cclass_8859_3[] = {
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
 '\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
 '\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
 '\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
 '\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
 '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
 '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
 '\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
 '\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
 '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
 '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
 '\170', '\171', '\172', '\173', '\174', '\175', '\176', '\000',
 '\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
 '\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
 '\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
 '\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
 '\000', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
 '\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
 '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
 '\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
 '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
 '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
 '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\327',
 '\370', '\371', '\372', '\373', '\374', '\375', '\376', '\337',
 '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
 '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
 '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
 '\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377'
};

static int func8859_3(const void *s1, const void *s2)
{
  return compareFunc(s1, s2, _cclass_8859_3);
}

static int Compare8859_3(const void *s1, const void *s2, size_t len)
{
  return compareFunc(s1, s2, _cclass_8859_3, len);
}


static int SIS8859_3(const void *s1, const void *s2)
{
  return SIScompareFunc(s1, s2, _ib_ctype_8859_3 /* _cclass_8859_3 */);
}


#if 0
static const unsigned char _ib_ctype_8859_5[] = {
  /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', 
  /* */'\040', /* */'\050', /* */'\050', /* */'\050', /* */'\050', /* */'\050', /* */'\040', /* */'\040', 
  /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', 
  /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', 
  /* */'\110', /*!*/'\020', /*"*/'\020', /*#*/'\020', /*$*/'\020', /*%*/'\020', /*&*/'\020', /*'*/'\020', 
  /*(*/'\020', /*)*/'\020', /***/'\020', /*+*/'\020', /*,*/'\020', /*-*/'\020', /*.*/'\020', /* / */'\020', 
  /*0*/'\204', /*1*/'\204', /*2*/'\204', /*3*/'\204', /*4*/'\204', /*5*/'\204', /*6*/'\204', /*7*/'\204', 
  /*8*/'\204', /*9*/'\204', /*:*/'\020', /*;*/'\020', /*<*/'\020', /*=*/'\020', /*>*/'\020', /*?*/'\020', 
  /*@*/'\020', /*A*/'\201', /*B*/'\201', /*C*/'\201', /*D*/'\201', /*E*/'\201', /*F*/'\201', /*G*/'\001', 
  /*H*/'\001', /*I*/'\001', /*J*/'\001', /*K*/'\001', /*L*/'\001', /*M*/'\001', /*N*/'\001', /*O*/'\001', 
  /*P*/'\001', /*Q*/'\001', /*R*/'\001', /*S*/'\001', /*T*/'\001', /*U*/'\001', /*V*/'\001', /*W*/'\001', 
  /*X*/'\001', /*Y*/'\001', /*Z*/'\001', /*[*/'\020', /*\*/'\020', /*]*/'\020', /*^*/'\020', /*_*/'\020', 
  /*`*/'\020', /*a*/'\202', /*b*/'\202', /*c*/'\202', /*d*/'\202', /*e*/'\202', /*f*/'\202', /*g*/'\002', 
  /*h*/'\002', /*i*/'\002', /*j*/'\002', /*k*/'\002', /*l*/'\002', /*m*/'\002', /*n*/'\002', /*o*/'\002', 
  /*p*/'\002', /*q*/'\002', /*r*/'\002', /*s*/'\002', /*t*/'\002', /*u*/'\002', /*v*/'\002', /*w*/'\002', 
  /*x*/'\002', /*y*/'\002', /*z*/'\002', /*{*/'\020', /*|*/'\020', /*}*/'\020', /*~*/'\020', /* */'\040', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\110', /*¡*/'\001', /*¢*/'\001', /*£*/'\001', /*¤*/'\001', /*¥*/'\001', /*¦*/'\001', /*§*/'\001', 
  /*¨*/'\001', /*©*/'\001', /*ª*/'\001', /*«*/'\001', /*¬*/'\001', /*­*/'\020', /*®*/'\001', /*¯*/'\001', 
  /*°*/'\001', /*±*/'\001', /*²*/'\001', /*³*/'\001', /*´*/'\001', /*µ*/'\001', /*¶*/'\001', /*·*/'\001', 
  /*¸*/'\001', /*¹*/'\001', /*º*/'\001', /*»*/'\001', /*¼*/'\001', /*½*/'\001', /*¾*/'\001', /*¿*/'\001', 
  /*À*/'\001', /*Á*/'\001', /*Â*/'\001', /*Ã*/'\001', /*Ä*/'\001', /*Å*/'\001', /*Æ*/'\001', /*Ç*/'\001', 
  /*È*/'\001', /*É*/'\001', /*Ê*/'\001', /*Ë*/'\001', /*Ì*/'\001', /*Í*/'\001', /*Î*/'\001', /*Ï*/'\001', 
  /*Ð*/'\002', /*Ñ*/'\002', /*Ò*/'\002', /*Ó*/'\002', /*Ô*/'\002', /*Õ*/'\002', /*Ö*/'\002', /*×*/'\002', 
  /*Ø*/'\002', /*Ù*/'\002', /*Ú*/'\002', /*Û*/'\002', /*Ü*/'\002', /*Ý*/'\002', /*Þ*/'\002', /*ß*/'\002', 
  /*à*/'\002', /*á*/'\002', /*â*/'\002', /*ã*/'\002', /*ä*/'\002', /*å*/'\002', /*æ*/'\002', /*ç*/'\002', 
  /*è*/'\002', /*é*/'\002', /*ê*/'\002', /*ë*/'\002', /*ì*/'\002', /*í*/'\002', /*î*/'\002', /*ï*/'\002', 
  /*ð*/'\020', /*ñ*/'\002', /*ò*/'\002', /*ó*/'\002', /*ô*/'\002', /*õ*/'\002', /*ö*/'\002', /*÷*/'\002', 
  /*ø*/'\002', /*ù*/'\002', /*ú*/'\002', /*û*/'\002', /*ü*/'\002', /* */'\000', /*þ*/'\002', /*ÿ*/'\002'
};
#endif

static const OCTET _trans_upper_8859_5[] = {
  /* */'\000', /* */'\001', /* */'\002', /* */'\003', /* */'\004', /* */'\005', /* */'\006', /* */'\007', 
  /* */'\010', /* */'\011', /* */'\012', /* */'\013', /* */'\014', /* */'\015', /* */'\016', /* */'\017', 
  /* */'\020', /* */'\021', /* */'\022', /* */'\023', /* */'\024', /* */'\025', /* */'\026', /* */'\027', 
  /* */'\030', /* */'\031', /* */'\032', /* */'\033', /* */'\034', /* */'\035', /* */'\036', /* */'\037', 
  /* */'\040', /*!*/'\041', /*"*/'\042', /*#*/'\043', /*$*/'\044', /*%*/'\045', /*&*/'\046', /*'*/'\047', 
  /*(*/'\050', /*)*/'\051', /***/'\052', /*+*/'\053', /*,*/'\054', /*-*/'\055', /*.*/'\056', /* / */'\057', 
  /*0*/'\060', /*1*/'\061', /*2*/'\062', /*3*/'\063', /*4*/'\064', /*5*/'\065', /*6*/'\066', /*7*/'\067', 
  /*8*/'\070', /*9*/'\071', /*:*/'\072', /*;*/'\073', /*<*/'\074', /*=*/'\075', /*>*/'\076', /*?*/'\077', 
  /*@*/'\100', /*A*/'\101', /*B*/'\102', /*C*/'\103', /*D*/'\104', /*E*/'\105', /*F*/'\106', /*G*/'\107', 
  /*H*/'\110', /*I*/'\111', /*J*/'\112', /*K*/'\113', /*L*/'\114', /*M*/'\115', /*N*/'\116', /*O*/'\117', 
  /*P*/'\120', /*Q*/'\121', /*R*/'\122', /*S*/'\123', /*T*/'\124', /*U*/'\125', /*V*/'\126', /*W*/'\127', 
  /*X*/'\130', /*Y*/'\131', /*Z*/'\132', /*[*/'\133', /*\*/'\134', /*]*/'\135', /*^*/'\136', /*_*/'\137', 
  /*`*/'\140', /*a*/'\101', /*b*/'\102', /*c*/'\103', /*d*/'\104', /*e*/'\105', /*f*/'\106', /*g*/'\107', 
  /*h*/'\110', /*i*/'\111', /*j*/'\112', /*k*/'\113', /*l*/'\114', /*m*/'\115', /*n*/'\116', /*o*/'\117', 
  /*p*/'\120', /*q*/'\121', /*r*/'\122', /*s*/'\123', /*t*/'\124', /*u*/'\125', /*v*/'\126', /*w*/'\127', 
  /*x*/'\130', /*y*/'\131', /*z*/'\132', /*{*/'\173', /*|*/'\174', /*}*/'\175', /*~*/'\176', /* */'\177', 
  /* */'\200', /* */'\201', /* */'\202', /* */'\203', /* */'\204', /* */'\205', /* */'\206', /* */'\207', 
  /* */'\210', /* */'\211', /* */'\212', /* */'\213', /* */'\214', /* */'\215', /* */'\216', /* */'\217', 
  /* */'\220', /* */'\221', /* */'\222', /* */'\223', /* */'\224', /* */'\225', /* */'\226', /* */'\227', 
  /* */'\230', /* */'\231', /* */'\232', /* */'\233', /* */'\234', /* */'\235', /* */'\236', /* */'\237', 
  /* */'\240', /*¡*/'\241', /*¢*/'\242', /*£*/'\243', /*¤*/'\244', /*¥*/'\245', /*¦*/'\246', /*§*/'\247', 
  /*¨*/'\250', /*©*/'\251', /*ª*/'\252', /*«*/'\253', /*¬*/'\254', /*­*/'\255', /*®*/'\256', /*¯*/'\257', 
  /*°*/'\260', /*±*/'\261', /*²*/'\262', /*³*/'\263', /*´*/'\264', /*µ*/'\265', /*¶*/'\266', /*·*/'\267', 
  /*¸*/'\270', /*¹*/'\271', /*º*/'\272', /*»*/'\273', /*¼*/'\274', /*½*/'\275', /*¾*/'\276', /*¿*/'\277', 
  /*À*/'\300', /*Á*/'\301', /*Â*/'\302', /*Ã*/'\303', /*Ä*/'\304', /*Å*/'\305', /*Æ*/'\306', /*Ç*/'\307', 
  /*È*/'\310', /*É*/'\311', /*Ê*/'\312', /*Ë*/'\313', /*Ì*/'\314', /*Í*/'\315', /*Î*/'\316', /*Ï*/'\317', 
  /*Ð*/'\260', /*Ñ*/'\261', /*Ò*/'\262', /*Ó*/'\263', /*Ô*/'\264', /*Õ*/'\265', /*Ö*/'\266', /*×*/'\267', 
  /*Ø*/'\270', /*Ù*/'\271', /*Ú*/'\272', /*Û*/'\273', /*Ü*/'\274', /*Ý*/'\275', /*Þ*/'\276', /*ß*/'\277', 
  /*à*/'\300', /*á*/'\301', /*â*/'\302', /*ã*/'\303', /*ä*/'\304', /*å*/'\305', /*æ*/'\306', /*ç*/'\307', 
  /*è*/'\310', /*é*/'\311', /*ê*/'\312', /*ë*/'\313', /*ì*/'\314', /*í*/'\315', /*î*/'\316', /*ï*/'\317', 
  /*ð*/'\360', /*ñ*/'\241', /*ò*/'\242', /*ó*/'\243', /*ô*/'\244', /*õ*/'\245', /*ö*/'\246', /*÷*/'\247', 
  /*ø*/'\250', /*ù*/'\251', /*ú*/'\252', /*û*/'\253', /*ü*/'\254', /* */'\375', /*þ*/'\256', /*ÿ*/'\257'
};

static const OCTET _trans_lower_8859_5[] = {
  /* */'\000', /* */'\001', /* */'\002', /* */'\003', /* */'\004', /* */'\005', /* */'\006', /* */'\007', 
  /* */'\010', /* */'\011', /* */'\012', /* */'\013', /* */'\014', /* */'\015', /* */'\016', /* */'\017', 
  /* */'\020', /* */'\021', /* */'\022', /* */'\023', /* */'\024', /* */'\025', /* */'\026', /* */'\027', 
  /* */'\030', /* */'\031', /* */'\032', /* */'\033', /* */'\034', /* */'\035', /* */'\036', /* */'\037', 
  /* */'\040', /*!*/'\041', /*"*/'\042', /*#*/'\043', /*$*/'\044', /*%*/'\045', /*&*/'\046', /*'*/'\047', 
  /*(*/'\050', /*)*/'\051', /***/'\052', /*+*/'\053', /*,*/'\054', /*-*/'\055', /*.*/'\056', /* / */'\057', 
  /*0*/'\060', /*1*/'\061', /*2*/'\062', /*3*/'\063', /*4*/'\064', /*5*/'\065', /*6*/'\066', /*7*/'\067', 
  /*8*/'\070', /*9*/'\071', /*:*/'\072', /*;*/'\073', /*<*/'\074', /*=*/'\075', /*>*/'\076', /*?*/'\077', 
  /*@*/'\100', /*A*/'\141', /*B*/'\142', /*C*/'\143', /*D*/'\144', /*E*/'\145', /*F*/'\146', /*G*/'\147', 
  /*H*/'\150', /*I*/'\151', /*J*/'\152', /*K*/'\153', /*L*/'\154', /*M*/'\155', /*N*/'\156', /*O*/'\157', 
  /*P*/'\160', /*Q*/'\161', /*R*/'\162', /*S*/'\163', /*T*/'\164', /*U*/'\165', /*V*/'\166', /*W*/'\167', 
  /*X*/'\170', /*Y*/'\171', /*Z*/'\172', /*[*/'\133', /*\*/'\134', /*]*/'\135', /*^*/'\136', /*_*/'\137', 
  /*`*/'\140', /*a*/'\141', /*b*/'\142', /*c*/'\143', /*d*/'\144', /*e*/'\145', /*f*/'\146', /*g*/'\147', 
  /*h*/'\150', /*i*/'\151', /*j*/'\152', /*k*/'\153', /*l*/'\154', /*m*/'\155', /*n*/'\156', /*o*/'\157', 
  /*p*/'\160', /*q*/'\161', /*r*/'\162', /*s*/'\163', /*t*/'\164', /*u*/'\165', /*v*/'\166', /*w*/'\167', 
  /*x*/'\170', /*y*/'\171', /*z*/'\172', /*{*/'\173', /*|*/'\174', /*}*/'\175', /*~*/'\176', /* */'\177', 
  /* */'\200', /* */'\201', /* */'\202', /* */'\203', /* */'\204', /* */'\205', /* */'\206', /* */'\207', 
  /* */'\210', /* */'\211', /* */'\212', /* */'\213', /* */'\214', /* */'\215', /* */'\216', /* */'\217', 
  /* */'\220', /* */'\221', /* */'\222', /* */'\223', /* */'\224', /* */'\225', /* */'\226', /* */'\227', 
  /* */'\230', /* */'\231', /* */'\232', /* */'\233', /* */'\234', /* */'\235', /* */'\236', /* */'\237', 
  /* */'\240', /*¡*/'\361', /*¢*/'\362', /*£*/'\363', /*¤*/'\364', /*¥*/'\365', /*¦*/'\366', /*§*/'\367', 
  /*¨*/'\370', /*©*/'\371', /*ª*/'\372', /*«*/'\373', /*¬*/'\374', /*­*/'\255', /*®*/'\376', /*¯*/'\377', 
  /*°*/'\320', /*±*/'\321', /*²*/'\322', /*³*/'\323', /*´*/'\324', /*µ*/'\325', /*¶*/'\326', /*·*/'\327', 
  /*¸*/'\330', /*¹*/'\331', /*º*/'\332', /*»*/'\333', /*¼*/'\334', /*½*/'\335', /*¾*/'\336', /*¿*/'\337', 
  /*À*/'\340', /*Á*/'\341', /*Â*/'\342', /*Ã*/'\343', /*Ä*/'\344', /*Å*/'\345', /*Æ*/'\346', /*Ç*/'\347', 
  /*È*/'\350', /*É*/'\351', /*Ê*/'\352', /*Ë*/'\353', /*Ì*/'\354', /*Í*/'\355', /*Î*/'\356', /*Ï*/'\357', 
  /*Ð*/'\320', /*Ñ*/'\321', /*Ò*/'\322', /*Ó*/'\323', /*Ô*/'\324', /*Õ*/'\325', /*Ö*/'\326', /*×*/'\327', 
  /*Ø*/'\330', /*Ù*/'\331', /*Ú*/'\332', /*Û*/'\333', /*Ü*/'\334', /*Ý*/'\335', /*Þ*/'\336', /*ß*/'\337', 
  /*à*/'\340', /*á*/'\341', /*â*/'\342', /*ã*/'\343', /*ä*/'\344', /*å*/'\345', /*æ*/'\346', /*ç*/'\347', 
  /*è*/'\350', /*é*/'\351', /*ê*/'\352', /*ë*/'\353', /*ì*/'\354', /*í*/'\355', /*î*/'\356', /*ï*/'\357', 
  /*ð*/'\360', /*ñ*/'\361', /*ò*/'\362', /*ó*/'\363', /*ô*/'\364', /*õ*/'\365', /*ö*/'\366', /*÷*/'\367', 
  /*ø*/'\370', /*ù*/'\371', /*ú*/'\372', /*û*/'\373', /*ü*/'\374', /* */'\375', /*þ*/'\376', /*ÿ*/'\377'
};

static const OCTET _cclass_8859_5[] = {
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
 '\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
 '\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
 '\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
 '\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
 '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
 '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
 '\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
 '\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
 '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
 '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
 '\170', '\171', '\172', '\173', '\174', '\175', '\176', '\000',
 '\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
 '\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
 '\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
 '\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
 '\000', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
 '\370', '\371', '\372', '\373', '\374', '\255', '\376', '\377',
 '\320', '\321', '\322', '\323', '\324', '\325', '\326', '\327',
 '\330', '\331', '\332', '\333', '\334', '\335', '\336', '\337',
 '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
 '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
 '\320', '\321', '\322', '\323', '\324', '\325', '\326', '\327',
 '\330', '\331', '\332', '\333', '\334', '\335', '\336', '\337',
 '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
 '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
 '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
 '\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377'
};

static int func8859_5(const void *s1, const void *s2)
{
  return compareFunc(s1, s2, _cclass_8859_5);
}

static int Compare8859_5(const void *s1, const void *s2, size_t len)
{
  return compareFunc(s1, s2, _cclass_8859_5, len);
}


static int SIS8859_5(const void *s1, const void *s2)
{
  return SIScompareFunc(s1, s2, _ib_ctype_8859_5 /* _cclass_8859_5 */);
}

#if 0
static const unsigned char _ib_ctype_8859_7[] = {
  /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', 
  /* */'\040', /* */'\050', /* */'\050', /* */'\050', /* */'\050', /* */'\050', /* */'\040', /* */'\040', 
  /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', 
  /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', 
  /* */'\110', /*!*/'\020', /*"*/'\020', /*#*/'\020', /*$*/'\020', /*%*/'\020', /*&*/'\020', /*'*/'\020', 
  /*(*/'\020', /*)*/'\020', /***/'\020', /*+*/'\020', /*,*/'\020', /*-*/'\020', /*.*/'\020', /* / */'\020', 
  /*0*/'\204', /*1*/'\204', /*2*/'\204', /*3*/'\204', /*4*/'\204', /*5*/'\204', /*6*/'\204', /*7*/'\204', 
  /*8*/'\204', /*9*/'\204', /*:*/'\020', /*;*/'\020', /*<*/'\020', /*=*/'\020', /*>*/'\020', /*?*/'\020', 
  /*@*/'\020', /*A*/'\201', /*B*/'\201', /*C*/'\201', /*D*/'\201', /*E*/'\201', /*F*/'\201', /*G*/'\001', 
  /*H*/'\001', /*I*/'\001', /*J*/'\001', /*K*/'\001', /*L*/'\001', /*M*/'\001', /*N*/'\001', /*O*/'\001', 
  /*P*/'\001', /*Q*/'\001', /*R*/'\001', /*S*/'\001', /*T*/'\001', /*U*/'\001', /*V*/'\001', /*W*/'\001', 
  /*X*/'\001', /*Y*/'\001', /*Z*/'\001', /*[*/'\020', /*\*/'\020', /*]*/'\020', /*^*/'\020', /*_*/'\020', 
  /*`*/'\020', /*a*/'\202', /*b*/'\202', /*c*/'\202', /*d*/'\202', /*e*/'\202', /*f*/'\202', /*g*/'\002', 
  /*h*/'\002', /*i*/'\002', /*j*/'\002', /*k*/'\002', /*l*/'\002', /*m*/'\002', /*n*/'\002', /*o*/'\002', 
  /*p*/'\002', /*q*/'\002', /*r*/'\002', /*s*/'\002', /*t*/'\002', /*u*/'\002', /*v*/'\002', /*w*/'\002', 
  /*x*/'\002', /*y*/'\002', /*z*/'\002', /*{*/'\020', /*|*/'\020', /*}*/'\020', /*~*/'\020', /* */'\040', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\110', /*¡*/'\020', /*¢*/'\020', /*£*/'\020', /* */'\000', /* */'\000', /*¦*/'\020', /*§*/'\020', 
  /*¨*/'\020', /*©*/'\020', /* */'\000', /*«*/'\020', /*¬*/'\020', /*­*/'\020', /* */'\000', /*¯*/'\020', 
  /*°*/'\020', /*±*/'\020', /*²*/'\020', /*³*/'\020', /* */'\000', /* */'\000', /*¶*/'\001', /*·*/'\020', 
  /*¸*/'\001', /*¹*/'\001', /*º*/'\001', /*»*/'\020', /*¼*/'\001', /*½*/'\020', /*¾*/'\001', /*¿*/'\001', 
  /*À*/'\001', /*Á*/'\001', /*Â*/'\001', /*Ã*/'\001', /*Ä*/'\001', /*Å*/'\001', /*Æ*/'\001', /*Ç*/'\001', 
  /*È*/'\001', /*É*/'\001', /*Ê*/'\001', /*Ë*/'\001', /*Ì*/'\001', /*Í*/'\001', /*Î*/'\001', /*Ï*/'\001', 
  /*Ð*/'\001', /*Ñ*/'\001', /* */'\000', /*Ó*/'\001', /*Ô*/'\001', /*Õ*/'\001', /*Ö*/'\001', /*×*/'\001', 
  /*Ø*/'\001', /*Ù*/'\001', /*Ú*/'\001', /*Û*/'\001', /*Ü*/'\002', /*Ý*/'\002', /*Þ*/'\002', /*ß*/'\002', 
  /*à*/'\002', /*á*/'\002', /*â*/'\002', /*ã*/'\002', /*ä*/'\002', /*å*/'\002', /*æ*/'\002', /*ç*/'\002', 
  /*è*/'\002', /*é*/'\002', /*ê*/'\002', /*ë*/'\002', /*ì*/'\002', /*í*/'\002', /*î*/'\002', /*ï*/'\002', 
  /*ð*/'\002', /*ñ*/'\002', /*ò*/'\002', /*ó*/'\002', /*ô*/'\002', /*õ*/'\002', /*ö*/'\002', /*÷*/'\002', 
  /*ø*/'\002', /*ù*/'\002', /*ú*/'\002', /*û*/'\002', /*ü*/'\002', /*ý*/'\002', /*þ*/'\002', /* */'\000'
};
#endif

static const OCTET _trans_upper_8859_7[] = {
  /* */'\000', /* */'\001', /* */'\002', /* */'\003', /* */'\004', /* */'\005', /* */'\006', /* */'\007', 
  /* */'\010', /* */'\011', /* */'\012', /* */'\013', /* */'\014', /* */'\015', /* */'\016', /* */'\017', 
  /* */'\020', /* */'\021', /* */'\022', /* */'\023', /* */'\024', /* */'\025', /* */'\026', /* */'\027', 
  /* */'\030', /* */'\031', /* */'\032', /* */'\033', /* */'\034', /* */'\035', /* */'\036', /* */'\037', 
  /* */'\040', /*!*/'\041', /*"*/'\042', /*#*/'\043', /*$*/'\044', /*%*/'\045', /*&*/'\046', /*'*/'\047', 
  /*(*/'\050', /*)*/'\051', /***/'\052', /*+*/'\053', /*,*/'\054', /*-*/'\055', /*.*/'\056', /* / */'\057', 
  /*0*/'\060', /*1*/'\061', /*2*/'\062', /*3*/'\063', /*4*/'\064', /*5*/'\065', /*6*/'\066', /*7*/'\067', 
  /*8*/'\070', /*9*/'\071', /*:*/'\072', /*;*/'\073', /*<*/'\074', /*=*/'\075', /*>*/'\076', /*?*/'\077', 
  /*@*/'\100', /*A*/'\101', /*B*/'\102', /*C*/'\103', /*D*/'\104', /*E*/'\105', /*F*/'\106', /*G*/'\107', 
  /*H*/'\110', /*I*/'\111', /*J*/'\112', /*K*/'\113', /*L*/'\114', /*M*/'\115', /*N*/'\116', /*O*/'\117', 
  /*P*/'\120', /*Q*/'\121', /*R*/'\122', /*S*/'\123', /*T*/'\124', /*U*/'\125', /*V*/'\126', /*W*/'\127', 
  /*X*/'\130', /*Y*/'\131', /*Z*/'\132', /*[*/'\133', /*\*/'\134', /*]*/'\135', /*^*/'\136', /*_*/'\137', 
  /*`*/'\140', /*a*/'\101', /*b*/'\102', /*c*/'\103', /*d*/'\104', /*e*/'\105', /*f*/'\106', /*g*/'\107', 
  /*h*/'\110', /*i*/'\111', /*j*/'\112', /*k*/'\113', /*l*/'\114', /*m*/'\115', /*n*/'\116', /*o*/'\117', 
  /*p*/'\120', /*q*/'\121', /*r*/'\122', /*s*/'\123', /*t*/'\124', /*u*/'\125', /*v*/'\126', /*w*/'\127', 
  /*x*/'\130', /*y*/'\131', /*z*/'\132', /*{*/'\173', /*|*/'\174', /*}*/'\175', /*~*/'\176', /* */'\177', 
  /* */'\200', /* */'\201', /* */'\202', /* */'\203', /* */'\204', /* */'\205', /* */'\206', /* */'\207', 
  /* */'\210', /* */'\211', /* */'\212', /* */'\213', /* */'\214', /* */'\215', /* */'\216', /* */'\217', 
  /* */'\220', /* */'\221', /* */'\222', /* */'\223', /* */'\224', /* */'\225', /* */'\226', /* */'\227', 
  /* */'\230', /* */'\231', /* */'\232', /* */'\233', /* */'\234', /* */'\235', /* */'\236', /* */'\237', 
  /* */'\240', /*¡*/'\241', /*¢*/'\242', /*£*/'\243', /* */'\244', /* */'\245', /*¦*/'\246', /*§*/'\247', 
  /*¨*/'\250', /*©*/'\251', /* */'\252', /*«*/'\253', /*¬*/'\254', /*­*/'\255', /* */'\256', /*¯*/'\257', 
  /*°*/'\260', /*±*/'\261', /*²*/'\262', /*³*/'\263', /* */'\264', /* */'\265', /*¶*/'\266', /*·*/'\267', 
  /*¸*/'\270', /*¹*/'\271', /*º*/'\272', /*»*/'\273', /*¼*/'\274', /*½*/'\275', /*¾*/'\276', /*¿*/'\277', 
  /*À*/'\300', /*Á*/'\301', /*Â*/'\302', /*Ã*/'\303', /*Ä*/'\304', /*Å*/'\305', /*Æ*/'\306', /*Ç*/'\307', 
  /*È*/'\310', /*É*/'\311', /*Ê*/'\312', /*Ë*/'\313', /*Ì*/'\314', /*Í*/'\315', /*Î*/'\316', /*Ï*/'\317', 
  /*Ð*/'\320', /*Ñ*/'\321', /* */'\322', /*Ó*/'\323', /*Ô*/'\324', /*Õ*/'\325', /*Ö*/'\326', /*×*/'\327', 
  /*Ø*/'\330', /*Ù*/'\331', /*Ú*/'\332', /*Û*/'\333', /*Ü*/'\266', /*Ý*/'\270', /*Þ*/'\271', /*ß*/'\272', 
  /*à*/'\340', /*á*/'\301', /*â*/'\302', /*ã*/'\303', /*ä*/'\304', /*å*/'\305', /*æ*/'\306', /*ç*/'\307', 
  /*è*/'\310', /*é*/'\311', /*ê*/'\312', /*ë*/'\313', /*ì*/'\314', /*í*/'\315', /*î*/'\316', /*ï*/'\317', 
  /*ð*/'\320', /*ñ*/'\321', /*ò*/'\323', /*ó*/'\363', /*ô*/'\324', /*õ*/'\325', /*ö*/'\326', /*÷*/'\327', 
  /*ø*/'\330', /*ù*/'\331', /*ú*/'\332', /*û*/'\333', /*ü*/'\274', /*ý*/'\276', /*þ*/'\277', /* */'\377'
};

static const OCTET _trans_lower_8859_7[] = {
  /* */'\000', /* */'\001', /* */'\002', /* */'\003', /* */'\004', /* */'\005', /* */'\006', /* */'\007', 
  /* */'\010', /* */'\011', /* */'\012', /* */'\013', /* */'\014', /* */'\015', /* */'\016', /* */'\017', 
  /* */'\020', /* */'\021', /* */'\022', /* */'\023', /* */'\024', /* */'\025', /* */'\026', /* */'\027', 
  /* */'\030', /* */'\031', /* */'\032', /* */'\033', /* */'\034', /* */'\035', /* */'\036', /* */'\037', 
  /* */'\040', /*!*/'\041', /*"*/'\042', /*#*/'\043', /*$*/'\044', /*%*/'\045', /*&*/'\046', /*'*/'\047', 
  /*(*/'\050', /*)*/'\051', /***/'\052', /*+*/'\053', /*,*/'\054', /*-*/'\055', /*.*/'\056', /* / */'\057', 
  /*0*/'\060', /*1*/'\061', /*2*/'\062', /*3*/'\063', /*4*/'\064', /*5*/'\065', /*6*/'\066', /*7*/'\067', 
  /*8*/'\070', /*9*/'\071', /*:*/'\072', /*;*/'\073', /*<*/'\074', /*=*/'\075', /*>*/'\076', /*?*/'\077', 
  /*@*/'\100', /*A*/'\141', /*B*/'\142', /*C*/'\143', /*D*/'\144', /*E*/'\145', /*F*/'\146', /*G*/'\147', 
  /*H*/'\150', /*I*/'\151', /*J*/'\152', /*K*/'\153', /*L*/'\154', /*M*/'\155', /*N*/'\156', /*O*/'\157', 
  /*P*/'\160', /*Q*/'\161', /*R*/'\162', /*S*/'\163', /*T*/'\164', /*U*/'\165', /*V*/'\166', /*W*/'\167', 
  /*X*/'\170', /*Y*/'\171', /*Z*/'\172', /*[*/'\133', /*\*/'\134', /*]*/'\135', /*^*/'\136', /*_*/'\137', 
  /*`*/'\140', /*a*/'\141', /*b*/'\142', /*c*/'\143', /*d*/'\144', /*e*/'\145', /*f*/'\146', /*g*/'\147', 
  /*h*/'\150', /*i*/'\151', /*j*/'\152', /*k*/'\153', /*l*/'\154', /*m*/'\155', /*n*/'\156', /*o*/'\157', 
  /*p*/'\160', /*q*/'\161', /*r*/'\162', /*s*/'\163', /*t*/'\164', /*u*/'\165', /*v*/'\166', /*w*/'\167', 
  /*x*/'\170', /*y*/'\171', /*z*/'\172', /*{*/'\173', /*|*/'\174', /*}*/'\175', /*~*/'\176', /* */'\177', 
  /* */'\200', /* */'\201', /* */'\202', /* */'\203', /* */'\204', /* */'\205', /* */'\206', /* */'\207', 
  /* */'\210', /* */'\211', /* */'\212', /* */'\213', /* */'\214', /* */'\215', /* */'\216', /* */'\217', 
  /* */'\220', /* */'\221', /* */'\222', /* */'\223', /* */'\224', /* */'\225', /* */'\226', /* */'\227', 
  /* */'\230', /* */'\231', /* */'\232', /* */'\233', /* */'\234', /* */'\235', /* */'\236', /* */'\237', 
  /* */'\240', /*¡*/'\241', /*¢*/'\242', /*£*/'\243', /* */'\244', /* */'\245', /*¦*/'\246', /*§*/'\247', 
  /*¨*/'\250', /*©*/'\251', /* */'\252', /*«*/'\253', /*¬*/'\254', /*­*/'\255', /* */'\256', /*¯*/'\257', 
  /*°*/'\260', /*±*/'\261', /*²*/'\262', /*³*/'\263', /* */'\264', /* */'\265', /*¶*/'\334', /*·*/'\267', 
  /*¸*/'\335', /*¹*/'\336', /*º*/'\337', /*»*/'\273', /*¼*/'\374', /*½*/'\275', /*¾*/'\375', /*¿*/'\376', 
  /*À*/'\300', /*Á*/'\341', /*Â*/'\342', /*Ã*/'\343', /*Ä*/'\344', /*Å*/'\345', /*Æ*/'\346', /*Ç*/'\347', 
  /*È*/'\350', /*É*/'\351', /*Ê*/'\352', /*Ë*/'\353', /*Ì*/'\354', /*Í*/'\355', /*Î*/'\356', /*Ï*/'\357', 
  /*Ð*/'\360', /*Ñ*/'\361', /* */'\322', /*Ó*/'\362', /*Ô*/'\364', /*Õ*/'\365', /*Ö*/'\366', /*×*/'\367', 
  /*Ø*/'\370', /*Ù*/'\371', /*Ú*/'\372', /*Û*/'\373', /*Ü*/'\334', /*Ý*/'\335', /*Þ*/'\336', /*ß*/'\337', 
  /*à*/'\340', /*á*/'\341', /*â*/'\342', /*ã*/'\343', /*ä*/'\344', /*å*/'\345', /*æ*/'\346', /*ç*/'\347', 
  /*è*/'\350', /*é*/'\351', /*ê*/'\352', /*ë*/'\353', /*ì*/'\354', /*í*/'\355', /*î*/'\356', /*ï*/'\357', 
  /*ð*/'\360', /*ñ*/'\361', /*ò*/'\362', /*ó*/'\363', /*ô*/'\364', /*õ*/'\365', /*ö*/'\366', /*÷*/'\367', 
  /*ø*/'\370', /*ù*/'\371', /*ú*/'\372', /*û*/'\373', /*ü*/'\374', /*ý*/'\375', /*þ*/'\376', /* */'\377'
};

static const OCTET _cclass_8859_7[] = {
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
 '\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
 '\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
 '\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
 '\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
 '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
 '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
 '\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
 '\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
 '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
 '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
 '\170', '\171', '\172', '\173', '\174', '\175', '\176', '\000',
 '\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
 '\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
 '\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
 '\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
 '\000', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
 '\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
 '\260', '\261', '\262', '\263', '\264', '\265', '\334', '\267',
 '\335', '\336', '\337', '\273', '\374', '\275', '\375', '\376',
 '\300', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
 '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
 '\360', '\361', '\322', '\362', '\364', '\365', '\366', '\367',
 '\370', '\371', '\372', '\373', '\334', '\335', '\336', '\337',
 '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
 '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
 '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
 '\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377'
};

static int func8859_7(const void *s1, const void *s2)
{
  return compareFunc(s1, s2, _cclass_8859_7);
}

static int Compare8859_7(const void *s1, const void *s2, size_t len)
{
  return compareFunc(s1, s2, _cclass_8859_7, len);
}


static int SIS8859_7(const void *s1, const void *s2)
{
  return SIScompareFunc(s1, s2, _ib_ctype_8859_7 /* _cclass_8859_7 */);
}

#if 0

static const OCTET _trans_lower_8859_9[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
  0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,

  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x9a, 0x8b, 0x9c, 0x8d, 0x8e, 0x8f,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0xff,
  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
  0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
  0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xd7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xdf,
  0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};



#endif


#define HAVE_ASCII
#if 0
static unsigned char _ib_ctype_ASCII[] = {
  /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', 
  /* */'\040', /* */'\050', /* */'\050', /* */'\050', /* */'\050', /* */'\050', /* */'\040', /* */'\040', 
  /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', 
  /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', /* */'\040', 
  /* */'\110', /*!*/'\020', /*"*/'\020', /*#*/'\020', /*$*/'\020', /*%*/'\020', /*&*/'\020', /*'*/'\020', 
  /*(*/'\020', /*)*/'\020', /***/'\020', /*+*/'\020', /*,*/'\020', /*-*/'\020', /*.*/'\020', /* / */'\020', 
  /*0*/'\204', /*1*/'\204', /*2*/'\204', /*3*/'\204', /*4*/'\204', /*5*/'\204', /*6*/'\204', /*7*/'\204', 
  /*8*/'\204', /*9*/'\204', /*:*/'\020', /*;*/'\020', /*<*/'\020', /*=*/'\020', /*>*/'\020', /*?*/'\020', 
  /*@*/'\020', /*A*/'\201', /*B*/'\201', /*C*/'\201', /*D*/'\201', /*E*/'\201', /*F*/'\201', /*G*/'\001', 
  /*H*/'\001', /*I*/'\001', /*J*/'\001', /*K*/'\001', /*L*/'\001', /*M*/'\001', /*N*/'\001', /*O*/'\001', 
  /*P*/'\001', /*Q*/'\001', /*R*/'\001', /*S*/'\001', /*T*/'\001', /*U*/'\001', /*V*/'\001', /*W*/'\001', 
  /*X*/'\001', /*Y*/'\001', /*Z*/'\001', /*[*/'\020', /*\*/'\020', /*]*/'\020', /*^*/'\020', /*_*/'\020', 
  /*`*/'\020', /*a*/'\202', /*b*/'\202', /*c*/'\202', /*d*/'\202', /*e*/'\202', /*f*/'\202', /*g*/'\002', 
  /*h*/'\002', /*i*/'\002', /*j*/'\002', /*k*/'\002', /*l*/'\002', /*m*/'\002', /*n*/'\002', /*o*/'\002', 
  /*p*/'\002', /*q*/'\002', /*r*/'\002', /*s*/'\002', /*t*/'\002', /*u*/'\002', /*v*/'\002', /*w*/'\002', 
  /*x*/'\002', /*y*/'\002', /*z*/'\002', /*{*/'\020', /*|*/'\020', /*}*/'\020', /*~*/'\020', /* */'\040', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', 
  /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000', /* */'\000'
};
#endif

static const OCTET _trans_upper_ASCII[] = {
  /* */'\000', /* */'\001', /* */'\002', /* */'\003', /* */'\004', /* */'\005', /* */'\006', /* */'\007', 
  /* */'\010', /* */'\011', /* */'\012', /* */'\013', /* */'\014', /* */'\015', /* */'\016', /* */'\017', 
  /* */'\020', /* */'\021', /* */'\022', /* */'\023', /* */'\024', /* */'\025', /* */'\026', /* */'\027', 
  /* */'\030', /* */'\031', /* */'\032', /* */'\033', /* */'\034', /* */'\035', /* */'\036', /* */'\037', 
  /* */'\040', /*!*/'\041', /*"*/'\042', /*#*/'\043', /*$*/'\044', /*%*/'\045', /*&*/'\046', /*'*/'\047', 
  /*(*/'\050', /*)*/'\051', /***/'\052', /*+*/'\053', /*,*/'\054', /*-*/'\055', /*.*/'\056', /* / */'\057', 
  /*0*/'\060', /*1*/'\061', /*2*/'\062', /*3*/'\063', /*4*/'\064', /*5*/'\065', /*6*/'\066', /*7*/'\067', 
  /*8*/'\070', /*9*/'\071', /*:*/'\072', /*;*/'\073', /*<*/'\074', /*=*/'\075', /*>*/'\076', /*?*/'\077', 
  /*@*/'\100', /*A*/'\101', /*B*/'\102', /*C*/'\103', /*D*/'\104', /*E*/'\105', /*F*/'\106', /*G*/'\107', 
  /*H*/'\110', /*I*/'\111', /*J*/'\112', /*K*/'\113', /*L*/'\114', /*M*/'\115', /*N*/'\116', /*O*/'\117', 
  /*P*/'\120', /*Q*/'\121', /*R*/'\122', /*S*/'\123', /*T*/'\124', /*U*/'\125', /*V*/'\126', /*W*/'\127', 
  /*X*/'\130', /*Y*/'\131', /*Z*/'\132', /*[*/'\133', /*\*/'\134', /*]*/'\135', /*^*/'\136', /*_*/'\137', 
  /*`*/'\140', /*a*/'\101', /*b*/'\102', /*c*/'\103', /*d*/'\104', /*e*/'\105', /*f*/'\106', /*g*/'\107', 
  /*h*/'\110', /*i*/'\111', /*j*/'\112', /*k*/'\113', /*l*/'\114', /*m*/'\115', /*n*/'\116', /*o*/'\117', 
  /*p*/'\120', /*q*/'\121', /*r*/'\122', /*s*/'\123', /*t*/'\124', /*u*/'\125', /*v*/'\126', /*w*/'\127', 
  /*x*/'\130', /*y*/'\131', /*z*/'\132', /*{*/'\173', /*|*/'\174', /*}*/'\175', /*~*/'\176', /* */'\177', 
  /* */'\200', /* */'\201', /* */'\202', /* */'\203', /* */'\204', /* */'\205', /* */'\206', /* */'\207', 
  /* */'\210', /* */'\211', /* */'\212', /* */'\213', /* */'\214', /* */'\215', /* */'\216', /* */'\217', 
  /* */'\220', /* */'\221', /* */'\222', /* */'\223', /* */'\224', /* */'\225', /* */'\226', /* */'\227', 
  /* */'\230', /* */'\231', /* */'\232', /* */'\233', /* */'\234', /* */'\235', /* */'\236', /* */'\237', 
  /* */'\240', /* */'\241', /* */'\242', /* */'\243', /* */'\244', /* */'\245', /* */'\246', /* */'\247', 
  /* */'\250', /* */'\251', /* */'\252', /* */'\253', /* */'\254', /* */'\255', /* */'\256', /* */'\257', 
  /* */'\260', /* */'\261', /* */'\262', /* */'\263', /* */'\264', /* */'\265', /* */'\266', /* */'\267', 
  /* */'\270', /* */'\271', /* */'\272', /* */'\273', /* */'\274', /* */'\275', /* */'\276', /* */'\277', 
  /* */'\300', /* */'\301', /* */'\302', /* */'\303', /* */'\304', /* */'\305', /* */'\306', /* */'\307', 
  /* */'\310', /* */'\311', /* */'\312', /* */'\313', /* */'\314', /* */'\315', /* */'\316', /* */'\317', 
  /* */'\320', /* */'\321', /* */'\322', /* */'\323', /* */'\324', /* */'\325', /* */'\326', /* */'\327', 
  /* */'\330', /* */'\331', /* */'\332', /* */'\333', /* */'\334', /* */'\335', /* */'\336', /* */'\337', 
  /* */'\340', /* */'\341', /* */'\342', /* */'\343', /* */'\344', /* */'\345', /* */'\346', /* */'\347', 
  /* */'\350', /* */'\351', /* */'\352', /* */'\353', /* */'\354', /* */'\355', /* */'\356', /* */'\357', 
  /* */'\360', /* */'\361', /* */'\362', /* */'\363', /* */'\364', /* */'\365', /* */'\366', /* */'\367', 
  /* */'\370', /* */'\371', /* */'\372', /* */'\373', /* */'\374', /* */'\375', /* */'\376', /* */'\377'
};

static const OCTET _trans_lower_ASCII[] = {
  /* */'\000', /* */'\001', /* */'\002', /* */'\003', /* */'\004', /* */'\005', /* */'\006', /* */'\007', 
  /* */'\010', /* */'\011', /* */'\012', /* */'\013', /* */'\014', /* */'\015', /* */'\016', /* */'\017', 
  /* */'\020', /* */'\021', /* */'\022', /* */'\023', /* */'\024', /* */'\025', /* */'\026', /* */'\027', 
  /* */'\030', /* */'\031', /* */'\032', /* */'\033', /* */'\034', /* */'\035', /* */'\036', /* */'\037', 
  /* */'\040', /*!*/'\041', /*"*/'\042', /*#*/'\043', /*$*/'\044', /*%*/'\045', /*&*/'\046', /*'*/'\047', 
  /*(*/'\050', /*)*/'\051', /***/'\052', /*+*/'\053', /*,*/'\054', /*-*/'\055', /*.*/'\056', /* / */'\057', 
  /*0*/'\060', /*1*/'\061', /*2*/'\062', /*3*/'\063', /*4*/'\064', /*5*/'\065', /*6*/'\066', /*7*/'\067', 
  /*8*/'\070', /*9*/'\071', /*:*/'\072', /*;*/'\073', /*<*/'\074', /*=*/'\075', /*>*/'\076', /*?*/'\077', 
  /*@*/'\100', /*A*/'\141', /*B*/'\142', /*C*/'\143', /*D*/'\144', /*E*/'\145', /*F*/'\146', /*G*/'\147', 
  /*H*/'\150', /*I*/'\151', /*J*/'\152', /*K*/'\153', /*L*/'\154', /*M*/'\155', /*N*/'\156', /*O*/'\157', 
  /*P*/'\160', /*Q*/'\161', /*R*/'\162', /*S*/'\163', /*T*/'\164', /*U*/'\165', /*V*/'\166', /*W*/'\167', 
  /*X*/'\170', /*Y*/'\171', /*Z*/'\172', /*[*/'\133', /*\*/'\134', /*]*/'\135', /*^*/'\136', /*_*/'\137', 
  /*`*/'\140', /*a*/'\141', /*b*/'\142', /*c*/'\143', /*d*/'\144', /*e*/'\145', /*f*/'\146', /*g*/'\147', 
  /*h*/'\150', /*i*/'\151', /*j*/'\152', /*k*/'\153', /*l*/'\154', /*m*/'\155', /*n*/'\156', /*o*/'\157', 
  /*p*/'\160', /*q*/'\161', /*r*/'\162', /*s*/'\163', /*t*/'\164', /*u*/'\165', /*v*/'\166', /*w*/'\167', 
  /*x*/'\170', /*y*/'\171', /*z*/'\172', /*{*/'\173', /*|*/'\174', /*}*/'\175', /*~*/'\176', /* */'\177', 
  /* */'\200', /* */'\201', /* */'\202', /* */'\203', /* */'\204', /* */'\205', /* */'\206', /* */'\207', 
  /* */'\210', /* */'\211', /* */'\212', /* */'\213', /* */'\214', /* */'\215', /* */'\216', /* */'\217', 
  /* */'\220', /* */'\221', /* */'\222', /* */'\223', /* */'\224', /* */'\225', /* */'\226', /* */'\227', 
  /* */'\230', /* */'\231', /* */'\232', /* */'\233', /* */'\234', /* */'\235', /* */'\236', /* */'\237', 
  /* */'\240', /* */'\241', /* */'\242', /* */'\243', /* */'\244', /* */'\245', /* */'\246', /* */'\247', 
  /* */'\250', /* */'\251', /* */'\252', /* */'\253', /* */'\254', /* */'\255', /* */'\256', /* */'\257', 
  /* */'\260', /* */'\261', /* */'\262', /* */'\263', /* */'\264', /* */'\265', /* */'\266', /* */'\267', 
  /* */'\270', /* */'\271', /* */'\272', /* */'\273', /* */'\274', /* */'\275', /* */'\276', /* */'\277', 
  /* */'\300', /* */'\301', /* */'\302', /* */'\303', /* */'\304', /* */'\305', /* */'\306', /* */'\307', 
  /* */'\310', /* */'\311', /* */'\312', /* */'\313', /* */'\314', /* */'\315', /* */'\316', /* */'\317', 
  /* */'\320', /* */'\321', /* */'\322', /* */'\323', /* */'\324', /* */'\325', /* */'\326', /* */'\327', 
  /* */'\330', /* */'\331', /* */'\332', /* */'\333', /* */'\334', /* */'\335', /* */'\336', /* */'\337', 
  /* */'\340', /* */'\341', /* */'\342', /* */'\343', /* */'\344', /* */'\345', /* */'\346', /* */'\347', 
  /* */'\350', /* */'\351', /* */'\352', /* */'\353', /* */'\354', /* */'\355', /* */'\356', /* */'\357', 
  /* */'\360', /* */'\361', /* */'\362', /* */'\363', /* */'\364', /* */'\365', /* */'\366', /* */'\367', 
  /* */'\370', /* */'\371', /* */'\372', /* */'\373', /* */'\374', /* */'\375', /* */'\376', /* */'\377'
};

static const OCTET _cclass_ASCII[] = {
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\000', '\000', '\000', '\000', '\000', '\000', '\000',
 '\000', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
 '\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
 '\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
 '\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
 '\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
 '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
 '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
 '\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
 '\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
 '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
 '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
 '\170', '\171', '\172', '\173', '\174', '\175', '\176', '\000',
 '\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
 '\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
 '\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
 '\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
 '\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
 '\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
 '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
 '\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
 '\300', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
 '\310', '\311', '\312', '\313', '\314', '\315', '\316', '\317',
 '\320', '\321', '\322', '\323', '\324', '\325', '\326', '\327',
 '\330', '\331', '\332', '\333', '\334', '\335', '\336', '\337',
 '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
 '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
 '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
 '\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377'
};

static int funcASCII(const void *s1, const void *s2)
{
  return compareFunc(s1, s2, _cclass_ASCII);
}

static int CompareASCII(const void *s1, const void *s2, size_t len)
{
  return compareFunc(s1, s2, _cclass_ASCII, len);
}


static int SISASCII(const void *s1, const void *s2)
{
  return SIScompareFunc(s1, s2, _ib_ctype_ASCII /* _cclass_ASCII */);
}


/***** UTF8 ******/
// In progress

static int SISUTF8(const void *s1, const void *s2)
{
  return SIScompareFunc(s1, s2, NULL);
}

/****************/


static OCTET _cclass_Local[256];

static int funcLocal(const void *s1, const void *s2)
{
  return compareFunc(s1, s2, _cclass_Local);
}


static GDT_BOOLEAN _setcharset(BYTE, const OCTET **, const OCTET **,
const OCTET **, const OCTET **);

BYTE CHARSET::SetSet (BYTE Id)
{
  if (Id == Which && (Id <= MAX_CHARSET_ID || Id == UTF8))
    {
      return Which; // Already done
    }

  if (Id <= MAX_CHARSET_ID)
    CharTab = ISO8859[Which = Id];
  else
    Which = 0xFF; // Bogus set

  _setcharset(Which, &ctype, &upper, &lower, &cclass);
  switch (Which)
    {
      case 0:
        cclassComp = funcASCII;
        sisComp    = SISASCII;
	nComp      = CompareASCII;
        break;
      case 1:
      default:
        cclassComp = func8859_1;
        sisComp    = SIS8859_1;
	nComp      = Compare8859_1;
        break;
      case 2:
        cclassComp = func8859_2;
        sisComp    = SIS8859_2;
	nComp      = Compare8859_2;
        break;
      case 3:
        cclassComp = func8859_3;
        sisComp    = SIS8859_3;
	nComp      = Compare8859_3;
        break;
      case 5:
        cclassComp = func8859_5;
        sisComp    = SIS8859_5;
	nComp      = Compare8859_5;
        break;
      case 7:
        cclassComp = func8859_7;
        sisComp    = SIS8859_7;
	nComp     = Compare8859_7;
        break;
      // Use ASCII tables
      case 4:
      case 6:
      case 8:
	cclassComp = funcASCII;
	sisComp    = SISASCII;
	nComp      = CompareASCII;
	break;

      // Local charsets...
      case 0xF0:
      case 0xF1:
      case 0xF2:
      case 0xF3:
      case 0xF4:
      case 0xF5:
      case 0xF6:
      case 0xF7:
      case 0xF8:
      case 0xF9:
      case 0xFA:
      case 0xFB:
      case 0xFC:
      case 0xFD:
      case 0xFE:
	break;
    }
  return Which;
}



/* Default ISO 8858-1 */
const OCTET *__IB_ctype       = _ib_ctype_8859_1;
const OCTET *__IB_trans_upper = _trans_upper_8859_1;
const OCTET *__IB_trans_lower = _trans_lower_8859_1;
const OCTET *__IB_trans_ascii = _trans_ascii_8859_1;
const OCTET *__IB_cclass      = _cclass_8859_1;


static BYTE CharsetId = 1;

// Support 8859-x
GDT_BOOLEAN SetGlobalCharset (const STRING& Name)
{
  BYTE set = Charset2Id (Name);
  return SetGlobalCharset ( set );
}

static GDT_BOOLEAN _setcharset(BYTE Charset,
	const OCTET     **ctype,
	const OCTET     **trans_upper,
	const OCTET     **trans_lower,
	const OCTET     **cclass)
{
  const char msgfmt[] = "Set character set to %s";

  BYTE oldCharsetId = CharsetId;
  CharsetId = Charset; 

  switch (CharsetId)
    {
      case  0:
	*ctype = _ib_ctype_ASCII;
	*trans_upper = _trans_upper_ASCII;
	*trans_lower = _trans_lower_ASCII;
	*cclass      = _cclass_ASCII;
	logf (LOG_DEBUG, msgfmt, "ASCII");
	break;
      case  1:
	*ctype = _ib_ctype_8859_1;
        *trans_upper = _trans_upper_8859_1;
        *trans_lower = _trans_lower_8859_1;
	*cclass      = _cclass_8859_1;
	logf (LOG_DEBUG, msgfmt, "ISO 8859-1");
	break;
      case  2:
	*ctype = _ib_ctype_8859_2;
	*trans_upper = _trans_upper_8859_2;
	*trans_lower = _trans_lower_8859_2;
	*cclass      = _cclass_8859_2;
	logf (LOG_DEBUG, msgfmt, "ISO 8859-2");
	break;
      case  3:
	*ctype = _ib_ctype_8859_3;
	*trans_upper = _trans_upper_8859_3;
	*trans_lower = _trans_lower_8859_3;
	*cclass      = _cclass_8859_3;
	logf (LOG_DEBUG, msgfmt, "ISO 8859-3");
	break;
      case 4:
#ifdef HAVE_8859_4
	*ctype = _ib_ctype_8859_4;
	*trans_upper = _trans_upper_8859_4;
	*trans_lower = _trans_lower_8859_4;
	*cclass      = _cclass_8859_4;
	logf (LOG_DEBUG, msgfmt, "ISO 8859-4");
#else
	_setcharset(0, ctype, trans_upper, trans_lower, cclass);
#endif
	break;
      case  5:
	*ctype = _ib_ctype_8859_5;
	*trans_upper = _trans_upper_8859_5;
	*trans_lower = _trans_lower_8859_5;
	*cclass      = _cclass_8859_5;
	logf (LOG_DEBUG, msgfmt, "ISO 8859-5");
	break;
      case 6:
#ifdef HAVE_8859_6
	*ctype = _ib_ctype_8859_6;
	*trans_upper = _trans_upper_8859_6;
	*trans_lower = _trans_lower_8859_6;
	*cclass      = _cclass_8859_6;
	logf (LOG_DEBUG, msgfmt, "ISO 8859-6");
#else
	_setcharset(0, ctype, trans_upper, trans_lower, cclass);
#endif
	break;
      case  7:
	*ctype = _ib_ctype_8859_7;
	*trans_upper = _trans_upper_8859_7;
	*trans_lower = _trans_lower_8859_7;
	*cclass      = _cclass_8859_7;
	logf (LOG_DEBUG, msgfmt, "ISO 8859-7");
	break;
      case  8:
#ifdef HAVE_8859_8
	*ctype = _ib_ctype_8859_8;
	*trans_upper = _trans_upper_8859_8;
	*trans_lower = _trans_lower_8859_8;
	*cclass      = _cclass_8859_8;
	logf (LOG_DEBUG, msgfmt, "ISO 8859-8");
#else
	_setcharset(0, ctype, trans_upper, trans_lower, cclass);
#endif
	break;
#ifdef HAVE_8859_9
      case  9:
	*ctype = _ib_ctype_8859_9;
	*trans_upper = _trans_upper_8859_9;
	*trans_lower = _trans_lower_8859_9;
	*cclass      = _cclass_8859_9;
	logf (LOG_DEBUG, msgfmt, "ISO 8859-9");
	break;
#endif
#ifdef HAVE_8859_10
      case 10:
	*ctype = _ib_ctype_8859_10;
	*trans_upper = _trans_upper_8859_10;
	*trans_lower = _trans_lower_8859_10;
	*cclass      = _cclass_8859_10;
	logf (LOG_DEBUG, msgfmt, "ISO 8859-10");
	break;
#endif
      case 0xFF: 
	{
	  BYTE id = Charset2Id (NULL); // Use locale
	  if (id != 0xFF)
	    return _setcharset(id, ctype, trans_upper, trans_lower, cclass);
	}
	// fall into...
      default: /* Not available */
	logf (LOG_DEBUG, "Using old character set %d", oldCharsetId);
	CharsetId = oldCharsetId;
	return GDT_FALSE;
    }
  return GDT_TRUE;
}


GDT_BOOLEAN SetGlobalCharset (BYTE Charset)
{
  return _setcharset(Charset,
	&__IB_ctype, &__IB_trans_upper, &__IB_trans_lower, &__IB_cclass);
}

BYTE GetGlobalCharset(PSTRING StringBuffer)
{
  if (StringBuffer)
    *StringBuffer = Id2Charset(CharsetId);
  return CharsetId;
}


#undef  _ib_isalpha
#undef  _ib_isupper
#undef  _ib_islower
#undef  _ib_isdigit
#undef  _ib_isxdigit
#undef  _ib_isalnum
#undef  _ib_isspace
#undef  _ib_ispunct
#undef  _ib_isprint
#undef  _ib_isgraph
#undef  _ib_iscntrl
#undef  _ib_toupper
#undef  _ib_tolower
#undef  _ib_isascii
#undef  _ib_toascii

/* function versions */
int  _ib_isalpha(int c)  { return ((__IB_ctype)[(UCHR)(c)] & (_IB_U | _IB_L));}
int  _ib_isupper(int c)  { return ((__IB_ctype)[(UCHR)(c)] & _IB_U);}
int  _ib_islower(int c)  { return ((__IB_ctype)[(UCHR)(c)] & _IB_L);}
int  _ib_isdigit(int c)  { return ((__IB_ctype)[(UCHR)(c)] & _IB_N);}
int  _ib_isxdigit(int c) { return ((__IB_ctype)[(UCHR)(c)] & _IB_X);}
int  _ib_isalnum(int c)  { return ((__IB_ctype)[(UCHR)(c)] & (_IB_U | _IB_L | _IB_N));}
int  _ib_isspace(int c)  { return ((__IB_ctype)[(UCHR)(c)] & _IB_S);}
int  _ib_ispunct(int c)  { return ((__IB_ctype)[(UCHR)(c)] & _IB_P);}
int  _ib_isprint(int c)  { return ((__IB_ctype)[(UCHR)(c)] & (_IB_P | _IB_U | _IB_L | _IB_N | _IB_B));}
int  _ib_isgraph(int c)  { return ((__IB_ctype)[(UCHR)(c)] & (_IB_P | _IB_U | _IB_L | _IB_N));}
int  _ib_iscntrl(int c)  { return ((__IB_ctype)[(UCHR)(c)] & _IB_C);}
int  _ib_toupper(int c)  { return ((__IB_trans_upper[(UCHR)(c)]));}
int  _ib_tolower(int c)  { return ((__IB_trans_lower[(UCHR)(c)]));}
int  _ib_isascii(int c)  { return (!((c) & ~0177));}
int  _ib_toascii(int c)  { return ((c) & 0177);}


//////////////////// LANGUAGE CLASS

LANGUAGE::LANGUAGE()
{
  if ((Which = Lang2Id(NULL) ) == 0)
    Which = DEFAULT_LANGUAGE;
}

LANGUAGE::LANGUAGE(const LANGUAGE& OtherLanguage)
{
  Which = OtherLanguage.Which;
}

LANGUAGE::LANGUAGE(int Id)
{
  Which = Id;
}

LANGUAGE::LANGUAGE(const STRING& Language)
{
  Which = Lang2Id(Language);
}

LANGUAGE::LANGUAGE(const char *Language)
{
  Which = Lang2Id(Language);
}

LANGUAGE& LANGUAGE::operator=(const LANGUAGE& OtherLanguage)
{
  Which = OtherLanguage.Which;
  return *this;
}

LANGUAGE& LANGUAGE::operator=(const STRING& Language)
{
  Which = LangX2Id(Language);
  return *this;
}

LANGUAGE& LANGUAGE::operator=(int Id)
{
  Which = Id;
  return *this;
}

LANGUAGE& LANGUAGE::operator=(const char *Language)
{
  Which = LangX2Id(Language);
  return *this;
}

GDT_BOOLEAN LANGUAGE::Read(FILE *Fp)
{
  ::Read(&Which, Fp);
  return Which <= MAX_LANGUAGE;
}


void LANGUAGE::Write(FILE *Fp) const
{
  ::Write(Which, Fp);
}



LANGUAGE::~LANGUAGE()
{
}

//////////////////////// LOCALE

LOCALE GlobalLocale;

void SetGlobalLocale(const LOCALE& NewLocale)
{
  GlobalLocale = NewLocale;
}

static const char *LocaleId2Lang(INT Id)       { return Id2Lang (Id/256);              }
static const char *LocaleId2Language(INT Id)   { return Id2Language (Id/256);          }
static const char *LocaleId2Charset(INT Id)    { return Id2Charset (Id % 256);         }
static const char *LocaleId2CharsetReg(INT Id) { return Id2CharsetRegistry (Id % 256); }

LOCALE::LOCALE()
{
  Which = 0;
}

LOCALE::LOCALE(INT Id)
{
  Which = Id;
}


LOCALE::LOCALE(const char *Name)
{
  Set(Name);
}


LOCALE::LOCALE(const STRING& Name)
{
  Set(Name);
}


LOCALE& LOCALE::Set(const STRING& name)
{
  return Set(name.c_str());
}

LOCALE& LOCALE::Set(const char *name)
{
  Which = 0;
  if (name && *name)
    {
      char tmp[127];
      char *tcp;

      strcpy(tmp, Locale2Cannon(name)); 
      if ((tcp = strchr(tmp, '.')) != NULL)
	*tcp++ = '\0';
      Which = Lang2Id(tmp) * 256L;
      if (tcp)
	Which += Charset2Id ( tcp );
    }
  return *this;
}


LOCALE& LOCALE::operator =(const LANGUAGE& Other)
{
  Which = Which & 0xFF;
  Which += (Other.Which)*256L;
  return *this;
}

LOCALE& LOCALE::operator =(const CHARSET& Other)
{
  Which = Which & 0xFFFF00;
  Which += Other.Which;
  return *this;
}


LOCALE& LOCALE::SetLanguage(const LANGUAGE& Other)
{
  *this = Other;
  return *this;
}

LOCALE& LOCALE::SetCharset(const CHARSET& Other)
{
  *this = Other;
  return *this;
}


BYTE  LOCALE::GetCharsetId() const
{
  return (Which%256);
}

INT   LOCALE::GetLanguageId() const
{
  return (Which/256);
}


CHARSET LOCALE::Charset() const
{
  return CHARSET(Which%256);
}


LANGUAGE LOCALE::Language() const
{
  return LANGUAGE(Which/256);
}

const char *LOCALE::GetLanguageCode () const
{
  return LocaleId2Lang(Which);
}

const char *LOCALE::GetLanguageName () const
{
  return LocaleId2Language(Which);
}

const char *LOCALE::GetCharsetName () const
{
  return LocaleId2Charset(Which);
}

const char *LOCALE::GetCharsetCode () const
{
  return LocaleId2CharsetReg(Which);
}


STRING LOCALE::LocaleName() const
{
  STRING name;
  return name.form("%s.%s", Id2Lang(Which/256), Id2Charset(Which & 0xFF));
}


GDT_BOOLEAN LOCALE::Read(FILE *Fp)
{
  UINT4 id;

  ::Read(&id, Fp);
  Which = id;
  return Which != 0;
}


void LOCALE::Write(FILE *Fp) const
{
  ::Write((UINT4)Which, Fp);
}



LOCALE::~LOCALE()
{
}

void       Write(const LOCALE& Locale, PFILE Fp) { Locale.Write(Fp);           }
GDT_BOOLEAN Read(LOCALE *LocalePtr, PFILE Fp)    { return LocalePtr->Read(Fp); }


#ifdef MAIN_STUB
main(int argc, char **argv)
{
  int code = Lang2Id(argv[1]);

  printf("%s ---> %s\n",  argv[1], Locale2Lang(argv[1])) ;
  printf("%s --> %d (%s)\n", argv[1], code, Id2Language((short)code));

  STRING Name;
  for (int i=0; i < 11; i++)
    {
      CHARSET charset(i);
      charset.GetSet(&Name);
	
      printf("\n static const OCTET cclass_%s[] = {\n", (const char *)Name);
      for (int x=0; x < 256; x++)
 	{
	  if (x && (x % 8) == 0)
	    printf("\n");
	  printf(" '\\%03o',", charset[x]); 
	}
       printf("\n};\n");
    }
}
#endif
