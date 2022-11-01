/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#ifndef _LANG_CODES_HXX
#define _LANG_CODES_HXX 1

class STRING;
#ifndef STRING_HXX
#include "string.hxx"
#endif

#ifndef OCTET
# define OCTET char
#endif

// General functions
const char *Locale2Lang(const char *name);
SHORT       Locale2Id(const char *name);
SHORT       Lang2Id (const STRING& name);
SHORT       Lang2Id (const char *name);
SHORT       LangX2Id (const STRING& name);
SHORT       LangX2Id (const char *name);
SHORT       Language2Id (const char *name);
const char *Lang2Language (const STRING& name);
const char *Lang2Language (const char *name);
const char *Id2Lang (const SHORT Code);
const char *Id2Language (const SHORT Code);

/*
const char *Id2Country (const SHORT Code);
SHORT       iso3166Code2Id (const char *Code);
const char *Id2iso3166Code2 (const SHORT Code);
*/

BYTE        Charset2Id (const STRING& Name);
BYTE        Charset2Id (const char *Name);
const CHR  *Id2Charset(BYTE CharsetId);

int         UTFTo(UINT2 *dest, const char *buf, size_t len);

class CHARSET {
friend class LOCALE;
friend class MDTREC;
public:
  CHARSET();
  CHARSET(const CHARSET& OtherCharset);
  CHARSET(const char *Name);
  CHARSET(const STRING& Name);
  CHARSET(BYTE Id);

  bool Ok() const { return Which != 0xFF; }

  BYTE SetSet ();
  BYTE SetSet (const CHARSET& Other);
  BYTE SetSet (const char *name);
  BYTE SetSet (const STRING& Name);
  BYTE SetSet (BYTE Id);
  BYTE GetSet (STRING *Name) const;

  CHARSET& operator=(const CHARSET& set) { SetSet(set); return *this; }
  CHARSET& operator=(const STRING& name) { SetSet(name); return *this; }
  CHARSET& operator=(BYTE Id)            { SetSet(Id); return *this; }
  CHARSET& operator=(const char *name)   { SetSet(name); return *this; }

  operator const CHR *() const { return Id2Charset(Which); }
  operator STRING () const     { return (STRING)Id2Charset(Which); }
  operator int () const        { return Which; }

  const STRING& HtmlCat (const CHR ch, STRING *StringBufferPtr) const;
  const STRING& HtmlCat (const STRING& Input, bool Anchor=true) const;
  const STRING& HtmlCat (const STRING& Input, STRING *StringBufferPtr) const;
  const STRING& HtmlCat (const STRING& Input, STRING *StringBufferPtr, bool Anchor) const;

  STRING  ToLower(const STRING& String) const;
  STRING  ToUpper(const STRING& String) const;
  STRING& MakeLower(STRING *StringPtr) const { return *StringPtr = ToLower(*StringPtr); }
  STRING& MakeUpper(STRING *StringPtr) const { return *StringPtr = ToUpper(*StringPtr); }

  // Character classification functions
  int ib_isalpha(int c) const  { return ctype[(UCHR)c] & (_IB_U | _IB_L); }
  int ib_isupper(int c) const  { return ctype[(UCHR)c] & _IB_U; }
  int ib_islower(int c) const  { return ctype[(UCHR)c] & _IB_L; }
  int ib_isdigit(int c) const  { return ctype[(UCHR)c] & _IB_N; }
  int ib_isxdigit(int c) const { return ctype[(UCHR)c] & _IB_X; }
  int ib_isalnum(int c) const  { return ctype[(UCHR)c] & (_IB_U | _IB_L | _IB_N); }
  int ib_isspace(int c) const  { return ctype[(UCHR)c] & _IB_S; }
  int ib_ispunct(int c) const  { return ctype[(UCHR)c] & _IB_P; }
  int ib_isprint(int c) const  { return ctype[(UCHR)c] & (_IB_P | _IB_U | _IB_L | _IB_N | _IB_B); }
  int ib_isgraph(int c) const  { return ctype[(UCHR)c] & (_IB_P | _IB_U | _IB_L | _IB_N); }
  int ib_iscntrl(int c)  const { return ctype[(UCHR)c] & _IB_C; }
  int ib_iswhite(int c) const  { return ctype[(UCHR)c] & (_IB_S | _IB_C | _IB_B); }
  int ib_isascii(int c) const  { return !(CharTab[(UCHR)c] & ~0177); }
  int ib_islatin1(int c) const { return !(CharTab[(UCHR)c] & ~0377); }

  int ib_toupper(int c) const  { return (upper[(UCHR)(c)]); }
  int ib_tolower(int c) const  { return (lower[(UCHR)(c)]); }
  int ib_toascii(int c) const  { return ascii[(UCHR)c] & 0177; }

  int operator()(int c) const  { return CharTab[(BYTE)cclass[(UCHR)c]]; }
  int operator[](int c) const  { return (BYTE)cclass[(UCHR)c];  }

  int isTermChr(int c) const   { return ib_isalpha(c); }
  int isWordSep(int c) const   { return !isTermChr(c); }
  int isTermWhite(int c) const { return ib_iswhite(c) || c == '-'; }

  void   ToUTF(STRING *Buffer, const STRING& From) const;
  char  *ToUTF(char *buffer, const char *From) const;
  STRING ToUTF(const STRING& From) const;

  size_t ReadUCS (UINT2 *buf, size_t DataFileSize, PFILE Fp) const;
  UINT2  UCS(CHR Ch) const     { return CharTab[(UCHR)Ch];}
  UINT2  UCS(UCHR Ch) const    { return CharTab[Ch];      }

  bool Read(FILE *Fp);
  void        Write(FILE *Fp) const;

  // These two are special purpose utility functions
  int (* SisCompare ()) (const void *, const void *)         { return cclassComp; }
  int (* SisKeys())     (const void *, const void *)         { return sisComp;    }
  int (* Compare())     (const void *, const void *, size_t) { return nComp;      }

  // Destruction
  ~CHARSET();
private:
  int (*cclassComp) (const void *, const void *);
  int (*sisComp)    (const void *, const void *);
  int (*nComp)      (const void *, const void *, size_t);
  const OCTET *ctype;
  const OCTET *upper;
  const OCTET *lower;
  const OCTET *ascii; // Without diacriticals
  const char          *sound;
  const OCTET         *cclass;
  const UINT2         *CharTab; // buffer with 8-bit maps
  BYTE                 Which; // Which set
};

// Inlines
inline bool operator==(const CHARSET& s1, const CHARSET& s2)
  { return (int)s1 == (int)s2; }
inline bool operator==(const CHARSET& s1, const STRING& name)
  { return (int)s1 == (int)(Charset2Id(name)); }
inline bool operator==(const STRING& name, const CHARSET& s1)
  { return (int)s1 == (int)(Charset2Id(name)); }
inline bool operator==(const CHARSET& s1, const char *name)
  { return (int)s1 == (int)(Charset2Id(name)); }
inline bool operator==(const char *name, const CHARSET& s1)
  { return (int)s1 == (int)(Charset2Id(name)); }
inline bool operator==(const CHARSET& s1, BYTE Id)
  { return (int)s1 == (int)Id; }
inline bool operator==(BYTE Id, const CHARSET& s1)
  { return (int)s1 == (int)Id; }
inline bool operator!=(const CHARSET& s1, const CHARSET& s2)
  { return (int)s1 != (int)s2; }
inline bool operator!=(const CHARSET& s1, const STRING& name)
  { return (int)s1 != (int)(Charset2Id(name)); }
inline bool operator!=(const STRING& name, const CHARSET& s1)
  { return (int)s1 != (int)(Charset2Id(name)); }
inline bool operator!=(const CHARSET& s1, const char *name)
  { return (int)s1 != (int)(Charset2Id(name)); }
inline bool operator!=(const char *name, const CHARSET& s1)
  { return (int)s1 != (int)(Charset2Id(name)); }
inline bool operator!=(const CHARSET& s1, BYTE Id)
  { return (int)s1 != (int)Id; }
inline bool operator!=(BYTE Id, const CHARSET& s1)
  { return (int)s1 != (int)Id; }

// Classification
inline int  ib_isalpha(const CHARSET& set, int c)  { return set.ib_isalpha(c); }
inline int  ib_isupper(const CHARSET& set, int c)  { return set.ib_isupper(c); }
inline int  ib_islower(const CHARSET& set, int c)  { return set.ib_islower(c); }
inline int  ib_isdigit(const CHARSET& set, int c)  { return set.ib_isdigit(c); }
inline int  ib_isxdigit(const CHARSET& set, int c) { return set.ib_isxdigit(c);}
inline int  ib_isalnum(const CHARSET& set, int c)  { return set.ib_isalnum(c); }
inline int  ib_isspace(const CHARSET& set, int c)  { return set.ib_isspace(c); }
inline int  ib_ispunct(const CHARSET& set, int c)  { return set.ib_ispunct(c); }
inline int  ib_isprint(const CHARSET& set, int c)  { return set.ib_isprint(c); }
inline int  ib_isgraph(const CHARSET& set, int c)  { return set.ib_isgraph(c); }
inline int  ib_iscntrl(const CHARSET& set, int c)  { return set.ib_iscntrl(c); }
inline int  ib_iswhite(const CHARSET& set, int c)  { return set.ib_iswhite(c); }
inline int  ib_toupper(const CHARSET& set, int c)  { return set.ib_toupper(c); }
inline int  ib_tolower(const CHARSET& set, int c)  { return set.ib_tolower(c); }
inline int  ib_isascii(const CHARSET& set, int c)  { return set.ib_isascii(c); }
inline int  ib_toascii(const CHARSET& set, int c)  { return set.ib_toascii(c); }
inline int  ib_islatin1(const CHARSET& set, int c) { return set.ib_islatin1(c);}
inline int  ib_diff(const CHARSET& set, int c1, int c2) { return set[c1] - set[c2]; }

inline int  isWordSep(const CHARSET& set, int c)   { return set.isWordSep(c); }
inline int  isTermChr(const CHARSET& set, int c)   { return set.isTermChr(c); }
inline int  isTermWhite(const CHARSET& set, int c) { return set.isTermWhite(c); }

// Set up the language
class LANGUAGE {
friend class LOCALE;
friend class MDTREC;
public:
  LANGUAGE();
  LANGUAGE(const LANGUAGE& OtherLanguage);
  LANGUAGE(const char *Name);
  LANGUAGE(const STRING& Name);
  LANGUAGE(int Id);

  operator const CHR *() const { return Id2Language(Which); }
  operator STRING () const     { return (STRING)Id2Language(Which); }
  operator int () const        { return Which; }

  const CHR * Name() const     { return Id2Language(Which); } // Language Name
  const CHR * Code() const     { return Id2Lang(Which); } // ISO Lang code

  LANGUAGE& operator=(const LANGUAGE& Lang);
  LANGUAGE& operator=(const STRING& Lang);
  LANGUAGE& operator=(int Id);
  LANGUAGE& operator=(const char *name);

  bool Read(FILE *Fp);
  void        Write(FILE *Fp) const;

  ~LANGUAGE();
private:
  UINT2 Which;
};

// Language Comparison inlines...
inline bool operator==(const LANGUAGE& s1, const LANGUAGE& s2)
  { return (int)s1 == (int)s2; }
inline bool operator==(const LANGUAGE& s1, const STRING& name)
  { return (int)s1 == (int)(LangX2Id(name)); }
inline bool operator==(const STRING& name, const LANGUAGE& s1)
  { return (int)s1 == (int)(LangX2Id(name)); }
inline bool operator==(const LANGUAGE& s1, const char *name)
  { return (int)s1 == (int)(LangX2Id(name)); }
inline bool operator==(const char *name, const LANGUAGE& s1)
  { return (int)s1 == (int)(LangX2Id(name)); }
inline bool operator==(const LANGUAGE& s1, SHORT Id)
  { return (int)s1 == (int)Id; }
inline bool operator==(SHORT Id, const LANGUAGE& s1)
  { return (int)s1 == (int)Id; }
inline bool operator!=(const LANGUAGE& s1, const LANGUAGE& s2)
  { return (int)s1 != (int)s2; }
inline bool operator!=(const LANGUAGE& s1, const STRING& name)
  { return (int)s1 != (int)(LangX2Id(name)); }
inline bool operator!=(const STRING& name, const LANGUAGE& s1)
  { return (int)s1 != (int)(LangX2Id(name)); }
inline bool operator!=(const LANGUAGE& s1, const char *name)
  { return (int)s1 != (int)(LangX2Id(name)); }
inline bool operator!=(const char *name, const LANGUAGE& s1)
  { return (int)s1 != (int)(LangX2Id(name)); }
inline bool operator!=(const LANGUAGE& s1, SHORT Id)
  { return (int)s1 != (int)Id; }
inline bool operator!=(BYTE Id, const LANGUAGE& s1)
  { return (int)s1 != (int)Id; }


class LOCALE {
  friend class MDTREC;
public:
  LOCALE();
  LOCALE(INT Id);
  LOCALE(const char *Name);
  LOCALE(const STRING& Name);

  LOCALE& Set(const STRING& Locale);
  LOCALE& Set(const char *Locale);

  LOCALE& operator = (const STRING& name) { return Set(name); }
  LOCALE& operator = (const char *name)   { return Set(name); }

  LOCALE& operator =(const LOCALE& Other) { Which = Other.Which; return *this;}
  LOCALE& operator =(const LANGUAGE& Other);
  LOCALE& operator =(const CHARSET& Other);

  LOCALE& SetLanguage(const LANGUAGE& Other);
  LOCALE& SetCharset(const CHARSET& Other);

  const char *GetLanguageCode () const; // ISO Language Codes
  const char *GetLanguageName () const; // Long Language Name
  const char *GetCharsetCode () const; // IANA names
  const char *GetCharsetName () const; // Registry Name

  BYTE        GetCharsetId() const;
  INT         GetLanguageId() const;

  bool Ok() const { return Which != 0; };

  CHARSET  Charset() const;
  LANGUAGE Language() const;
  STRING   LocaleName() const;
  INT      Id() const { return Which; }

  operator STRING () const   { return LocaleName(); }
  operator CHARSET () const  { return Charset();    }
  operator LANGUAGE () const { return Language();   }
  operator INT () const      { return Id();         }

  bool operator==(const LOCALE& Other) const { return Which == Other.Which; }
  bool operator!=(const LOCALE& Other) const { return Which != Other.Which; }

  bool Read(FILE *Fp);
  void        Write(FILE *Fp) const;

  ~LOCALE();
private:
  UINT4 Which;
};

void       Write(const LOCALE& Locale, PFILE Fp);
bool Read(LOCALE *LocalePtr, PFILE Fp);

extern LOCALE GlobalLocale; 

void        SetGlobalLocale(const LOCALE& NewLocale);

#ifndef _C_LANGUAGE
bool SetGlobalCharset (const STRING& Name);
bool SetGlobalCharset (BYTE Charset = 0xFF);
BYTE        GetGlobalCharset (STRING *StringBuffer = NULL);
#endif

#endif
