/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
#ifndef _IB_CTYPE_H
#define _IB_CTYPE_H
#define  OEM_VERSION

#define IsDotInWord(_x)     _ib_isdot(_x)
#define IsTermChar(_x)     (_ib_isalnum(_x))
#define IsAfterDotChar(_x) (_ib_isalpha(_x))

#define IsWordSep(_x)   (!IsTermChar(_x))

#if 1
// Experimental
# define IsTermWhite(_x) (_ib_isspace(_x) || (_x) == '-' || \
	(_x) == ',' || (_x) == ':' || (_x) == ';' || \
	(_x) == '\'' || (_x) == '"')
#else
# define IsTermWhite(_x) (_ib_isspace(_x) || (_x) == '-')
#endif /* Experimental */

extern "C" int _ib_isdot(const int Ch);


#ifndef _private_lang
#ifndef OCTET
# define OCTET char
#endif

extern const OCTET *__IB_ctype;
extern const OCTET *__IB_trans_upper;
extern const OCTET *__IB_trans_lower;
extern const OCTET *__IB_cclass;

#ifdef OEM_VERSION
extern "C" int (* const _ib_IsTermChr)(const unsigned char *ptr);
extern "C" int (* const _ib_IsSearchableTerm) (const char *field, const unsigned char *ptr, unsigned int len);
extern "C" int (* const _ib_IsExcludedSearchTerm) (const char *fieldname, const unsigned char *ptr, unsigned int len);
extern "C" int (* const _ib_IsExcludedIndexTerm)  (const char *word, unsigned int length);

extern "C" int (*_ib_ResolveBinPath)   (const char *Filename, char *buffer, int length);
extern "C" int (*_ib_ResolveConfigPath)(const char *Filename, char *buffer, int length);

#endif /* OEM_VERSION */

/* Character Classification Macros (8-bit sets) */

#define  _IB_U 0x00000001  /* Upper case */
#define  _IB_L 0x00000002  /* Lower case */
#define  _IB_N 0x00000004  /* Numeral (digit) */
#define  _IB_S 0x00000008  /* Spacing character */
#define  _IB_P 0x00000010  /* Punctuation */
#define  _IB_C 0x00000020  /* Control character */
#define  _IB_B 0x00000040  /* Blank */
#define  _IB_X 0x00000080  /* heXadecimal digit */

#ifdef INLINE_CC_MACROS

#define  _ib_isalpha(c)  ((__IB_ctype)[(UCHR)(c)] & (_IB_U | _IB_L))
#define  _ib_isupper(c)  ((__IB_ctype)[(UCHR)(c)] & _IB_U)
#define  _ib_islower(c)  ((__IB_ctype)[(UCHR)(c)] & _IB_L)
#define  _ib_isdigit(c)  ((__IB_ctype)[(UCHR)(c)] & _IB_N)
#define  _ib_isxdigit(c) ((__IB_ctype)[(UCHR)(c)] & _IB_X)
#define  _ib_isalnum(c)  ((__IB_ctype)[(UCHR)(c)] & (_IB_U | _IB_L | _IB_N))
#define  _ib_isspace(c)  ((__IB_ctype)[(UCHR)(c)] & _IB_S)
#define  _ib_ispunct(c)  ((__IB_ctype)[(UCHR)(c)] & _IB_P)
#define  _ib_isprint(c)  ((__IB_ctype)[(UCHR)(c)] & (_IB_P | _IB_U | _IB_L | _IB_N | _IB_B))
#define  _ib_isgraph(c)  ((__IB_ctype)[(UCHR)(c)] & (_IB_P | _IB_U | _IB_L | _IB_N))
#define  _ib_iscntrl(c)  ((__IB_ctype)[(UCHR)(c)] & _IB_C)

#define  _ib_iswhite(c)  ((__IB_ctype)[(UCHR)(c)] & (_IB_S | _IB_C | _IB_B))

#define  _ib_islatin1(c) _ib_islatin1(CHARSET(), c)

#define  _ib_toupper(c)  ((__IB_trans_upper[(UCHR)(c)]))
#define  _ib_tolower(c)  ((__IB_trans_lower[(UCHR)(c)]))
#define  _ib_isascii(c)  (!((c) & ~0177))
#define  _ib_toascii(c)  ((c) & 0177)

#define  _ib_diff(c1,c2) (__IB_cclass[(UCHR)(c1)] - __IB_cclass[(UCHR)(c2)])

#else /* function calls */

extern int  _ib_iswhite(int c) ; 
extern int  _ib_isalpha(int c) ; 
extern int  _ib_isupper(int c) ; 
extern int  _ib_islower(int c) ;
extern int  _ib_isdigit(int c) ;
extern int  _ib_isxdigit(int c);
extern int  _ib_isalnum(int c) ;
extern int  _ib_isspace(int c) ;
extern int  _ib_ispunct(int c) ;
extern int  _ib_isprint(int c) ;
extern int  _ib_isgraph(int c) ;
extern int  _ib_iscntrl(int c) ;
extern int  _ib_toupper(int c) ;
extern int  _ib_tolower(int c) ;
extern int  _ib_isascii(int c) ;
extern int  _ib_toascii(int c) ;

#define  _ib_islatin1(c) _ib_islatin1(CHARSET(), c)

// #define  _ib_toupper(c)  ((__IB_trans_upper[(UCHR)(c)]))
// #define  _ib_tolower(c)  ((__IB_trans_lower[(UCHR)(c)]))

extern int  _ib_toupper(int c) ; 
extern int  _ib_tolower(int c) ; 



//#define  _ib_isascii(c)  (!((c) & ~0177))
//#define  _ib_toascii(c)  ((c) & 0177)

#define  _ib_diff(c1,c2) (__IB_cclass[(UCHR)(c1)] - __IB_cclass[(UCHR)(c2)])

#endif /* INLINE_CC_MACROS */
#endif /* _private_lang */

inline int IsTermChr(const UCHR *Buffer) {
#ifdef OEM_VERSION
    if (_ib_IsTermChr) return _ib_IsTermChr ((const unsigned char *)Buffer);
#endif
    return Buffer && (IsTermChar(Buffer[0]) || (IsDotInWord(Buffer[0]) && IsAfterDotChar(Buffer[1])));
}
inline int IsTermChr(const CHR *Buffer) {
  return IsTermChr((const UCHR *)Buffer);
}
inline int IsTermChr(const UCHR Ch) {
  return IsTermChar(Ch);
}
inline int IsTermChr(const CHR Ch) {
  return IsTermChar(Ch);
}

inline int IsTermChr(const wchar_t Ch) {
  return iswalnum(Ch); // Alpha numeric
}

#endif
