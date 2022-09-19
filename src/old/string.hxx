#ifndef __STRING_HXX__
#define __STRING_HXX__ 1

#define WANT_I18_STRING 0

#ifdef __GNUG__
#pragma interface "string.hxx"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>

#include "gdt.h"
#include <iostream>
#include "ctype.hxx"

using namespace std;

typedef UINT4 STRINGINDEX;
typedef STRINGINDEX *PSTRINGINDEX;

#define NOT_FOUND       (-1)
#undef Copy

CHR *Copystring(const void *); // Forward declaration

// ---------------------------------------------------------------------------
// macros
// ---------------------------------------------------------------------------

/// compile the std::string compatibility functions
#define   STD_STRING_COMPATIBILITY

/// maximum possible length for a string means "take all string" everywhere
//  (as sizeof(StringData) is unknown here we substract 100)
#define   STRING_MAXLEN     (UINT_MAX - 100)

// 'naughty' cast
#define   STRINGCAST (char *)(const char *)

// NB: works only inside STRING class
#define wxASSERT(x) {;}
#define   ASSERT_VALID_INDEX(i) wxASSERT( (unsigned)(i) < Len() )

// ---------------------------------------------------------------------------
/** @name Global functions complementing standard C string library 
    @memo replacements for strlen() and portable strcasecmp()
 */
// ---------------------------------------------------------------------------

/// checks whether the passed in pointer is NULL and if the string is empty
inline GDT_BOOLEAN IsEmpty(const char *p) { return !p || !*p; }

/// safe version of strlen() (returns 0 if passed NULL pointer)
inline size_t Strlen(const char *psz) { return psz ? strlen(psz) : 0; }

/// portable strcasecmp/_stricmp
int Stricmp(const char *psz1, const char *psz2);
int Strnicmp(const char *psz1, const char *psz2, size_t len);

// ---------------------------------------------------------------------------
// string data prepended with some housekeeping info (used by String class),
// is never used directly (but had to be put here to allow inlining)
// ---------------------------------------------------------------------------
struct STRINGData
{
#if WANT_I18_STRING
  INT2  nRefs;        // reference count
  UINT4 nDataLength;  // actual string length
  UINT4 nAllocLength; // allocated memory size
  UINT2 nCharset;     // Charset used
#else
  INT4  nRefs;        // reference count
  UINT4 nDataLength;  // actual string length
  UINT4 nAllocLength; // allocated memory size
#endif

  // mimics declaration 'char data[nAllocLength]'
  char* data() const { return (char*)(this + 1); }  

  operator const char*() const { return data(); }
#ifndef SWIG
  operator const unsigned char*() const { return (unsigned char *)data(); }
#endif

  int RefCount() const { return (int)nRefs; }

  // empty string has a special ref count so it's never deleted
  GDT_BOOLEAN  IsConstant()const { return nRefs == -1; }
  GDT_BOOLEAN  IsShared()  const { return nRefs > 1;   }
  GDT_BOOLEAN  IsValid()   const { return nRefs != 0;  }

  // lock/unlock
  void  Lock()   { if ( nRefs >= 0 ) nRefs++;                        }
  void  Unlock() { if ( nRefs > 0  ) if (--nRefs == 0 ) delete[] this; }
};

extern const char * g_szNul; // global pointer to empty string
extern const STRINGData * const g_strNul; // global pointer to empty stringdata

class STRING {
  friend class ArraySTRING;

public:
  /** @name constructors & dtor */
  //@{
    /// ctor for an empty string
  STRING();
    /// copy ctor
  STRING(const STRING& stringSrc);        
    /// string containing nRepeat copies of ch
  STRING(char ch, size_t nRepeat = 1);       
  STRING(unsigned char ch, size_t nRepeat = 1);
    /// ctor takes first nLength characters from C string
  STRING(const char *psz, size_t nLength = STRING_MAXLEN);
    /// from C string (for compilers using unsigned char)
  STRING(const unsigned char* psz, size_t nLength = STRING_MAXLEN);
    /// from wide (UNICODE) string
  STRING(const wchar_t *pwz);
    /// dtor is not virtual, this class must not be inherited from!
  STRING(const char * const * CharVector);
  STRING(const INT2 *IntVector);
  STRING(const INT4 *IntVector);
  STRING(const short ShortValue);
  STRING(const unsigned short ShortValue);
  STRING(const signed IntValue);
  STRING(const unsigned IntValue);
  STRING(const signed long LongValue);
  STRING(const unsigned long LongValue);
  STRING(const signed long long LongLongValue);
  STRING(const unsigned long long LongLongValue);
  STRING(const float FloatValue);
  STRING(const double DoubleValue);
  STRING(const long double LongDoubleValue);
 ~STRING();
  //@}

  /** @name numeric typecasts */
  //@{
  // Explicit..
  GDT_BOOLEAN GetBool() const;
  short  GetShort(int base=0) const  { return (short)GetLong(base); }
  int    GetInt(int base=0) const    { return (int)GetLong(base);   }
  long   GetLong(int base=0) const   { return strtol(m_pchData, NULL, base); }
  long long GetLongLong(int base=0) const {
	return strtoll(m_pchData, NULL, base); }
  float  GetFloat() const            { return (float)GetDouble(); }
  double GetDouble() const           { return atof(m_pchData); }
  long double GetLongDouble() const  { return strtold(m_pchData, NULL); } 
  // casts
#ifndef SWIG
  operator bool () const             { return GetBool(); }
  operator short () const            { return (short)GetInt(); }
  operator unsigned short () const   { return (unsigned short)GetLong(); }
  operator int () const              { return GetInt(); }
  operator unsigned int () const     { return (unsigned int)GetInt(); }
  operator long () const             { return GetLong(); }
  operator unsigned long () const    { return (unsigned long)GetLong(); }
  operator long long() const         { return GetLongLong(); }
  operator unsigned long long() const { return (unsigned long long)
	GetLongLong(); }
  operator float () const            { return GetFloat(); }
  operator double () const           { return GetDouble(); }
  operator long double() const       { return GetLongDouble(); }
#endif
  //@}

  /** @name generic attributes & operations */
  //@{
    /// as standard strlen()
  size_t Len() const { return GetStringData()->nDataLength; }
    // Compat. with old String library..
  size_t GetLength() const { return Len(); }
  friend size_t Strlen(const STRING& Str) { return Str.Len(); } 
   /// How much space
  size_t Capacity () const  { return GetStringData()->nAllocLength; }
    /// string contains any characters?
  int RefCount() const { return GetStringData()->nRefs; }
  GDT_BOOLEAN IsEmpty() const { return GetStringData()->nDataLength == 0;  }

  GDT_BOOLEAN operator !() const { return !GetBool(); }

    /// Clear string (so that its Empty)
  void Clear();
    /// reinitialize string (and free data!)
  void Empty();
    /// Is an ascii value
  GDT_BOOLEAN IsAscii() const;
    /// Is Plain Word (no numbers, spaces or ..)
  GDT_BOOLEAN IsPlainWord() const;
    /// Is a number
  GDT_BOOLEAN IsNumber() const;
    /// Is a dot number (xxx.xxx.xxx)
  GDT_BOOLEAN IsDotNumber() const;
    /// A Range of Numerbers? 
  GDT_BOOLEAN IsNumberRange() const;
    /// Is a date?
  GDT_BOOLEAN IsDate() const;
    /// Date Range
  GDT_BOOLEAN IsDateRange() const;
    /// Is it currency?
  GDT_BOOLEAN IsCurrency() const;
    /// Is a Box?
  GDT_BOOLEAN IsGeoBoundedBox() const;
    /// Is a word
  GDT_BOOLEAN IsWord() const;
    /// Is printable
  GDT_BOOLEAN IsPrint() const;
   /// Is a file path? (Absolute path or/or file exists)
  GDT_BOOLEAN IsFilePath() const;
  //@}

  /** @ Copies */
  //@{
  // Copy into buffer
  void *Copy (void *ptr = NULL, size_t len = 0) const;
  PCHR GetCString (CHR *ptr = NULL, size_t len = 0) const;
  PUCHR GetUCString (UCHR *ptr = NULL, size_t len = 0) const;
  // Dup as a C-String Remember to delete [] !!
  PCHR    NewCString (size_t len=0) const  { return GetCString(NULL, len);  }
  PUCHR   NewUCString (size_t len=0) const { return GetUCString(NULL, len); }
  // Clone and Dup (Special purpose)
  STRING *Clone() const                    { return new STRING(*this);      }
  STRING  Dup() const                      { return STRING(m_pchData, Len());}
  //@}

  // Start count with 0
#if 0
  CHR GetAt(size_t n) const { return n < Len() ? m_pchData[n] : 0; ]
  void SetAt(size_t n, CHR c) { if (n < Len()) {CopyBeforeWrite(); m_pchData[n] = c;} }
#else
  CHR GetAt(size_t n) const { return (*this)[(unsigned)n]; }
  void SetAt(size_t n, CHR c) { (*this)[n] = c; } 
#endif

  /** @name data access (1 based) */
  //@{
    UCHR GetUChr (size_t n) const
        { return n <= Len() ? m_pchData[n-1] : 0; } // 1 is the first Nassib element
    CHR GetChr (size_t n) const
        { return n <= Len() ? m_pchData[n-1] : 0; } // 1 is the first Nassib element
    void SetChr (const STRINGINDEX Index, const UCHR NewChr);

  //@}

  /** @name data access (all indexes are 0 based) */
  //@{
    /// read access
    char  GetChar(size_t n) const
  	  { return n < Len() ? m_pchData[n] : 0; }
    /// read/write access
    char& GetWritableChar(size_t n)
  	  { ASSERT_VALID_INDEX( n ); CopyBeforeWrite(); return m_pchData[n]; }
    /// write access
    void  SetChar(size_t n, char ch)
      { ASSERT_VALID_INDEX( n ); CopyBeforeWrite(); m_pchData[n] = ch; }

    /// get last character
    char  Last() const { return m_pchData[Len() - 1]; }
    /// get writable last character
    char& Last() { CopyBeforeWrite(); return m_pchData[Len()-1]; }

    /// operator version of GetChar
    char  operator[](signed n) const {ASSERT_VALID_INDEX( n ); return m_pchData[n]; }
    char  operator[](unsigned n) const {ASSERT_VALID_INDEX( n ); return m_pchData[n]; }
    /// operator version of GetWritableChar
    char& operator[](unsigned n) {ASSERT_VALID_INDEX( n ); CopyBeforeWrite(); return m_pchData[n]; }

    /// implicit conversion to C string
#ifndef SWIG
    operator const void *() const{ return (const void *)m_pchData; }
#endif
    operator const char*() const { return m_pchData; } 
#ifndef SWIG
    operator const unsigned char*() const { return (unsigned char *)m_pchData; }
    /// explicit conversion to C string (use this with printf()!)
#endif
    const char* const c_str() const { return (const char * const)m_pchData; }
    const unsigned char * c_ustr() const { return (const unsigned char * const)m_pchData; }
    ///
    const char* GetData() const { return m_pchData; }
    const char* GetData(size_t n) const { return &m_pchData[n]; }
  //@}

  /** @name overloaded assignment */
  //@{
    ///
  STRING& operator=(const STRING& stringSrc);
    ///
  STRING& operator=(char ch);
    ///
  STRING& operator=(const char *psz);
    ///
  STRING& operator=(const unsigned char* psz);
    ///
  STRING& operator=(const wchar_t *pwz);
    ///
  STRING& operator=(const signed short ShortValue);
    ///
  STRING& operator=(const unsigned short ShortValue);
    ///
  STRING& operator=(const signed IntValue);
    ///
  STRING& operator=(const unsigned IntValue);
    ///
  STRING& operator=(const signed long LongValue);
    ///
  STRING& operator=(const unsigned long LongValue);
    ///
  STRING& operator=(const signed long long LongLongValue);
    ///
  STRING& operator=(const unsigned long long LongLongValue);
    ///
  STRING& operator=(const float FloatValue);
    ///
  STRING& operator=(const double DoubleValue);
    ///
  STRING& operator=(const long double DoubleValue);
    ///
  STRING& operator=(const INT4 *IntVector);
    ///
  STRING& operator=(const INT2 *IntVector);
  //@}

  STRING& Cat(const STRING&);
  STRING& Cat(const char *);
  STRING& Cat(char);
  STRING& Cat(unsigned char);
  STRING& Cat(int);
  STRING& Cat(long);
  STRING& Cat(unsigned long);
  STRING& Cat(long long);
  STRING& Cat(unsigned long long);
  STRING& Cat(float);
  STRING& Cat(double);

  STRING& HexCat(long long);
  STRING& OctCat(long long);
  STRING& DecCat(long long x) { return Cat (x); }
  
  /** @name string concatenation */
  //@{
    /** @name in place concatenation */
    //@{
      /// string += string
  STRING& operator+=(const STRING& string);
      /// string += C string
  STRING& operator+=(const char *psz);
      /// string += char
  STRING& operator+=(char ch);
  STRING& operator+=(unsigned char ch);
   /** These do arithmetic if string IsNumber() **/
     /// string += short
  STRING& operator+=(short val);
     /// string += int
  STRING& operator+=(int val);
     /// string += long
  STRING& operator+=(long val);
     /// string += float 
  STRING& operator+=(float val);
     /// string += double
  STRING& operator+=(double val);
      /// string += 2-byte integer vector
  STRING& operator+=(const INT2 *IntVector);
      /// string += 4-byte integer vector
  STRING& operator+=(const INT4 *IntVector);
    //@}
    /** @name concatenate and return the result
        left to right associativity of << allows to write 
        things like "str << str1 << str2 << ..."          */
    //@{
      /// as +=
  STRING& operator <<(const STRING& string);
  STRING& operator <<(const char *psz);
  STRING& operator <<(char ch);
  STRING& operator <<(unsigned char _c) { return *this << (char)_c;}
  STRING& operator <<(short _i)         { return *this << (long)_i; }
  STRING& operator <<(unsigned short _i){ return *this << (unsigned long)_i; }
  STRING& operator <<(int _i)           { return *this << (long)_i; };
  STRING& operator <<(unsigned _i)      { return *this << (unsigned long)_i; };
  STRING& operator <<(long LongValue);
  STRING& operator <<(unsigned long LongValue);
  STRING& operator <<(long long _i);
  STRING& operator <<(unsigned long long _i);
  STRING& operator <<(float _x)        { return *this << (double)_x; }
  STRING& operator <<(double DoubleValue);
  STRING& operator <<(PFILE in);
    //@}
    
    /** @name return resulting string */
    //@{
      ///
  friend STRING operator+(const STRING& string1,  const STRING& string2);
      ///
  friend STRING operator+(const STRING& string, char ch);
      ///
  friend STRING operator+(char ch, const STRING& string);
      ///
  friend STRING operator+(const STRING& string, const char *psz);
      ///
  friend STRING operator+(const char *psz, const STRING& string);
    //@}
  //@}
  
  /** @name string comparison */
  //@{
    /** 
    case-sensitive comparaison
    @return 0 if equal, +1 if greater or -1 if less
    @see CmpNoCase, IsSameAs
    */
   int  FieldCmp(const char *psz) const { return FieldCmp(c_str(), psz); }
   int  FieldCmp(const STRING& Other) const { return FieldCmp(c_str(), Other.c_str()); }
   int  FieldCmp(const char *s1, const char *s2) const;

  int  Cmp(const char *psz) const { return strcmp(c_str(), psz); }
    /**
    case-insensitive comparaison, return code as for STRING::Cmp()
    @see: Cmp, IsSameAs
    */
  int  CmpNoCase(const char *psz) const { return Stricmp(c_str(), psz); }
    /**
    test for string equality, case-sensitive (default) or not
    @param   bCase is TRUE by default (case matters)
    @return  TRUE if strings are equal, FALSE otherwise
    @see     Cmp, CmpNoCase
    */
  GDT_BOOLEAN IsSameAs(const char *psz, GDT_BOOLEAN bCase = GDT_TRUE) const 
    { return !(bCase ? Cmp(psz) : CmpNoCase(psz)); }
  //@}
  
  /** @name other standard string operations */
  //@{
    /** @name simple sub-string extraction
     */
    //@{
      /** 
      return substring starting at nFirst of length 
      nCount (or till the end if nCount = default value)
      */
  STRING Mid(size_t nFirst, size_t nCount = STRING_MAXLEN) const;  
      /// get first nCount characters
  STRING Left(size_t nCount) const;
      /// get all characters before the first occurence of ch
      /// (returns the whole string if ch not found)
  STRING Left(char ch) const;
      /// get all characters before the last occurence of ch
      /// (returns empty string if ch not found)
  STRING Before(char ch) const;
      /// get all characters after the first occurence of ch
      /// (returns empty string if ch not found)
  STRING After(char ch, GDT_BOOLEAN Last=GDT_FALSE) const;
      /// get last nCount characters
  STRING Right(size_t nCount) const;
      /// get all characters after the last occurence of ch
      /// (returns the whole string if ch not found)
  STRING Right(char ch) const;
      /// String without the quoes
  STRING DeQuote() const; 
      // Escape Characters, eg. \n --> \\n
  STRING Escape() const;
  STRING unEscape() const;
      /// Encode white space into a single line
  STRING PackOf() const;
      // Decode a Packed() string back
  STRING unPackOf() const;
    //@}
    
    /** @name case conversion */
    //@{ 
      ///
  GDT_BOOLEAN IsUpper() const;
  STRING& MakeUpper();
  STRING& ToUpper() { return MakeUpper(); }
      ///
  GDT_BOOLEAN IsLower() const;
  STRING& MakeLower();
  STRING& ToLower() { return MakeLower(); }
     ///
  STRING& MakeAscii() { return ToAscii(); }
  STRING& ToAscii();
  STRING& ToPrint ();
    //@}

    /** @name trimming/padding whitespace (either side) and truncating */
    //@{
      /// remove spaces from left or from right (default) side
  STRING& Trim(GDT_BOOLEAN bFromRight = GDT_TRUE);
      /// remove multiple spaces
  STRING& Pack ();
     /// remove VT100 sequences
  STRING& VT100Strip();
     /// remove XML comments
  inline STRING XMLCommentStrip(const STRING& ) const;
  STRING& XMLCommentStrip();
     /// add nCount copies chPad in the beginning or at the end (default)
  STRING& Pad(size_t nCount, char chPad = ' ', GDT_BOOLEAN bFromRight = GDT_TRUE);
      /// truncate string to given length
  STRING& Truncate(size_t uiLen);
    //@}
    
    /** @name searching and replacing */
    //@{
      /// searching (return starting index, or -1 if not found)
  int Find(char ch, GDT_BOOLEAN bFromEnd = GDT_FALSE) const;   // like strchr/strrchr
      /// searching (return starting index, or -1 if not found)
  int Find(const STRING& Sub, size_t Start = 0) const;
  int Find(const char *pszSub, size_t Start = 0) const;
  friend const char *strchr(const STRING& str, int ch);
  friend const char *strrchr(const STRING& str, int ch);
  friend const char *strstr(const STRING& str, const char *sub);
  friend const char *strstr(const STRING& str, const STRING& sub);
  friend const char *strstr(const char *str, const STRING& sub);

   size_t Count() const; // How many \,/ or ':'
   size_t Count(CHR Ch) const; // How many Ch are in the string?
   size_t Count(const char *str) const;
   size_t Count(const STRING& str) const;

      /**
      replace first (or all) occurences of substring with another one
      @param  bReplaceAll: global replace (default) or only the first occurence
      @return the number of replacements made
      */
  UINT Replace(const char *szOld, const char *szNew, GDT_BOOLEAN bReplaceAll = GDT_TRUE);

  STRINGINDEX FirstWhiteSpace() const;

  STRINGINDEX Search (const STRING& OtherString, const STRINGINDEX Start = 1) const;
  STRINGINDEX Search (const CHR *CString, const STRINGINDEX Start = 1) const;
  STRINGINDEX Search (const CHR Character, const STRINGINDEX Start = 1) const;
  STRINGINDEX Search (const UCHR Character, const STRINGINDEX Start = 1) const;

  STRINGINDEX SearchAny (const STRING& OtherString, const STRINGINDEX Start = 1) const;
  STRINGINDEX SearchAny (const CHR *CString, const STRINGINDEX Start = 1) const;
  STRINGINDEX SearchAny (const CHR Character, const STRINGINDEX Start = 1) const;
  STRINGINDEX SearchAny (const UCHR Character, const STRINGINDEX Start = 1) const;

  STRINGINDEX SearchReverse (const STRING& OtherString) const;
  STRINGINDEX SearchReverse (const CHR *CString) const;
  STRINGINDEX SearchReverse (const CHR Character) const;
  STRINGINDEX SearchReverse (const UCHR Character) const;

  STRINGINDEX SearchN(const STRING& String, const STRINGINDEX Count, const STRINGINDEX Start = 1) const;
  STRINGINDEX SearchN(const CHR* CString, const STRINGINDEX Count, const STRINGINDEX Start = 1) const;
  STRINGINDEX SearchN(const UCHR Character, const STRINGINDEX Count, const STRINGINDEX Start = 1) const;

//STRINGINDEX SearchReverseN(const STRING& String, const STRINGINDEX Count, const STRINGINDEX Start = 1) const;
//STRINGINDEX SearchReverseN(const CHR* CString, const STRINGINDEX Count, const STRINGINDEX Start = 1) const;
//STRINGINDEX SearchReverseN(const UCHR Character, const STRINGINDEX Count, const STRINGINDEX Start = 1) const;


  // Glob
  INT IsWild () const;
  GDT_BOOLEAN IsPlain() const; // No Wild chars: near opposite of IsWild
  GDT_BOOLEAN MatchWild(const STRING& OtherString) const;
  GDT_BOOLEAN MatchWild(const CHR *CString) const;
  GDT_BOOLEAN Glob(const STRING& OtherString) const;
  GDT_BOOLEAN Glob(const CHR *CString) const;
  GDT_BOOLEAN FieldMatch(const STRING& OtherString) const;
  GDT_BOOLEAN FieldMatch(const CHR *CString) const;

    //@}
  //@}

  // URLs..
  STRING w3Encode() const;

  // C-String escape encode/decocde
  STRING& cEncode ();
  STRING& cDecode ();
  // Hash
  unsigned CaseHash() const;
  unsigned Hash() const;
  UINT8    CRC64() const;
  UINT4    CRC32() const;
  UINT2    CRC16() const;

  /** @name formated output */
  //@{
    /// as sprintf(), returns the number of characters written or < 0 on error
  int Printf(const char *pszFormat, ...);
    /// as vprintf(), returns the number of characters written or < 0 on error
  int PrintfV(const char* pszFormat, va_list argptr);
  //@}
  
  // get writable buffer of at least nLen characters
  char *GetWriteBuf(size_t nLen);

  /** @name wxWindows compatibility functions */
  //@{
    /// values for second parameter of CompareTo function
  enum caseCompare {exact, ignoreCase};
    /// values for first parameter of Strip function
  enum stripType {leading = 0x1, trailing = 0x2, both = 0x3};
    /// same as Printf()

    /// No longer inline because of C++ warning
  int sprintf(const char *pszFormat, ...);
   /// Like above but returns string
  STRING& form(const char *pszFormat, ...); 

    /// same as Cmp
  inline int CompareTo(const char* psz, caseCompare cmp = exact) const
  { return cmp == exact ? Cmp(psz) : CmpNoCase(psz); }

    /// same as Mid (substring extraction)
  inline STRING  operator()(size_t start, size_t len = STRING_MAXLEN) const { return Mid(start, len); }

    /// same as += or <<
  inline STRING& Append(const char* psz) { return *this << psz; }
  inline STRING& Append(char ch, int count = 1) { STRING str(ch, count); return (*this) += str; }

    ///
  STRING& Prepend(const STRING& str) { *this = str + *this; return *this; }
    /// same as Len
  size_t Length() const { return Len(); }
    /// same as MakeLower
  void LowerCase() { MakeLower(); }
    /// same as MakeUpper
  void UpperCase() { MakeUpper(); }
    /// same as Trim except that it doesn't change this string
  STRING Strip(stripType w = trailing) const;

    /// same as Find (more general variants not yet supported)
  size_t Index(const char* psz) const { return Find(psz); }
  size_t Index(char ch)         const { return Find(ch);  }
    /// same as Truncate
  STRING& Remove(size_t pos) { return Truncate(pos); }
  STRING& RemoveLast() { return Truncate(Len() - 1); }
    /// same as IsEmpty
  GDT_BOOLEAN IsNull() const { return IsEmpty(); }
  //@}

#ifdef  STD_STRING_COMPATIBILITY
  /** @name std::string compatibility functions */
  
  /// an 'invalid' value for string index
  static const size_t npos;
        
  //@{
    /** @name constructors */
    //@{
      /// take nLen chars starting at nPos
      STRING(const STRING& s, size_t nPos, size_t nLen = npos);
      /// take all characters from pStart to pEnd
      STRING(const void *pStart, const void *pEnd);
    //@}
    /** @name lib.string.capacity */
    //@{
      /// return the length of the string
      size_t size() const { return Len(); }
      /// return the length of the string
      size_t length() const { return Len(); }
      /// return the maximum size of the string
      size_t max_size() const { return STRING_MAXLEN; } 
      /// resize the string, filling the space with c if c != 0
      void resize(size_t nSize, char ch = '\0');
      /// delete the contents of the string
      void clear() { Empty(); }
      /// returns true if the string is empty
      GDT_BOOLEAN empty() const { return IsEmpty(); }
    //@}
    /** @name lib.string.access */
    //@{
      /// return the character at position n
      char at(size_t n) const { return GetChar(n); }
      /// returns the writable character at position n
      char& at(size_t n) { return GetWritableChar(n); }
    //@}
    /** @name lib.string.modifiers */
    //@{
      /** @name append something to the end of this one */
      //@{
        /// append a string
        STRING& append(const STRING& str) { return (*this) += str; }
        /// append elements str[pos], ..., str[pos+n]
        STRING& append(const STRING& str, size_t pos, size_t n) 
          { ConcatSelf(n, str.c_str() + pos); return *this; }
        /// append first n (or all if n == npos) characters of sz
        STRING& append(const char *sz, size_t n = npos) 
          { ConcatSelf(n == npos ? Strlen(sz) : n, sz); return *this; }

        /// append n copies of ch
        STRING& append(size_t n, char ch) { return Pad(n, ch); }
      //@}
        
      /** @name replaces the contents of this string with another one */
      //@{
        /// same as `this_string = str'
        STRING& assign(const STRING& str) { return (*this) = str; }
        /// same as ` = str[pos..pos + n]
        STRING& assign(const STRING& str, size_t pos, size_t n) 
          { return *this = STRING((const char *)str + pos, n); }
        /// same as `= first n (or all if n == npos) characters of sz'
        STRING& assign(const char *sz, size_t n = npos) 
          { return *this = STRING(sz, n); }
        /// same as `= n copies of ch'
        STRING& assign(size_t n, char ch) 
          { return *this = STRING(ch, n); }

      //@}
        
      /** @name inserts something at position nPos into this one */  
      //@{
        /// insert another string
        STRING& insert(size_t nPos, const STRING& str);
        /// insert n chars of str starting at nStart (in str)
        STRING& insert(size_t nPos, const STRING& str, size_t nStart, size_t n)
	  	    { return insert(nPos, STRING((const char *)str + nStart, n)); }

        /// insert first n (or all if n == npos) characters of sz
        STRING& insert(size_t nPos, const char *sz, size_t n = npos)
          { return insert(nPos, STRING(sz, n)); }
        /// insert n copies of ch
        STRING& insert(size_t nPos, size_t n, char ch) 
          { return insert(nPos, STRING(ch, n)); }

      //@}
      
      /** @name deletes a part of the string */
      //@{
        /// delete characters from nStart to nStart + nLen
        STRING& erase(size_t nStart = 0, size_t nLen = npos);
      //@}
      
      /** @name replaces a substring of this string with another one */
      //@{
         /// replaces the substring of length nLen starting at nStart
         STRING& replace(size_t nStart, size_t nLen, const char* sz);
         /// replaces the substring with nCount copies of ch
         STRING& replace(size_t nStart, size_t nLen, size_t nCount, char ch);
         /// replaces a substring with another substring
         STRING& replace(size_t nStart, size_t nLen, 
                         const STRING& str, size_t nStart2, size_t nLen2);
         /// replaces the substring with first nCount chars of sz
         STRING& replace(size_t nStart, size_t nLen, 
                         const char* sz, size_t nCount);
      //@}
    //@}
         
    /// swap two strings
    void swap(STRING& str);

    /** @name string operations */
    //@{
      /** All find() functions take the nStart argument which specifies
          the position to start the search on, the default value is 0.
          
          All functions return npos if there were no match.
          
          @name string search 
      */
      //@{
        /**
            @name find a match for the string/character in this string 
        */
        //@{
          /// find a substring
          size_t find(const STRING& str, size_t nStart = 0) const;
          /// find first n characters of sz
          size_t find(const char* sz, size_t nStart = 0, size_t n = npos) const;
          /// find the first occurence of character ch after nStart
          size_t find(char ch, size_t nStart = 0) const;

	  // wxWin compatibility
	  inline GDT_BOOLEAN Contains(const STRING& str) { return (Find(str) != -1); }

        //@}
        
        /** 
          @name rfind() family is exactly like find() but works right to left
        */
        //@{
        /// as find, but from the end
        size_t rfind(const STRING& str, size_t nStart = npos) const;
        /// as find, but from the end
        size_t rfind(const char* sz, size_t nStart = npos, 
                     size_t n = npos) const;
        /// as find, but from the end
        size_t rfind(char ch, size_t nStart = npos) const;
        //@}
        
        /**
          @name find first/last occurence of any character in the set
        */
        //@{
          ///
          size_t find_first_of(const STRING& str, size_t nStart = 0) const;
          ///
          size_t find_first_of(const char* sz, size_t nStart = 0) const;
          /// same as find(char, size_t)
          size_t find_first_of(char c, size_t nStart = 0) const;
          
          ///
          size_t find_last_of (const STRING& str, size_t nStart = npos) const;
          ///
          size_t find_last_of (const char* s, size_t nStart = npos) const;
          /// same as rfind(char, size_t)
          size_t find_last_of (char c, size_t nStart = npos) const;
        //@}
        
        /**
          @name find first/last occurence of any character not in the set
        */
        //@{
          ///
          size_t find_first_not_of(const STRING& str, size_t nStart = 0) const;
          ///
          size_t find_first_not_of(const char* s, size_t nStart = 0) const;
          ///
          size_t find_first_not_of(char ch, size_t nStart = 0) const;
          
          ///
          size_t find_last_not_of(const STRING& str, size_t nStart=npos) const;
          ///
          size_t find_last_not_of(const char* s, size_t nStart = npos) const;
          ///
          size_t find_last_not_of(char ch, size_t nStart = npos) const;
        //@}
      //@}
      
      /** 
        All compare functions return -1, 0 or 1 if the [sub]string 
        is less, equal or greater than the compare() argument.
        
        @name comparison
      */
      //@{
        /// just like strcmp()
        int compare(const STRING& str) const { return Cmp(str); }
        /// comparaison with a substring
        int compare(size_t nStart, size_t nLen, const STRING& str) const;
        /// comparaison of 2 substrings
        int compare(size_t nStart, size_t nLen,
                    const STRING& str, size_t nStart2, size_t nLen2) const;
        /// just like strcmp()
        int compare(const char* sz) const { return Cmp(sz); }
        /// substring comparaison with first nCount characters of sz
        int compare(size_t nStart, size_t nLen,
                    const char* sz, size_t nCount = npos) const;
      //@}
    STRING substr(size_t nStart = 0, size_t nLen = npos) const;
    //@}
  //@}
#endif

  INT FieldCompare(const STRING& str) const;
  // Case dependent
  INT Compare(const STRING& str) const { return Cmp(str);};
  INT Compare (const STRING& OtherString, size_t len) const
	{ return strncmp(c_str(), OtherString.c_str(), len); }
  INT Compare (const char *psz, size_t len) const
	{ return strncmp(c_str(), psz, len); }
  INT Compare(const char *str) const   { return Cmp(str); }
  GDT_BOOLEAN Equals(const STRING& str) const { return Len() == str.Len() && Cmp(str) == 0;}
  GDT_BOOLEAN Equals(const char *str) const   { return Cmp(str) == 0;                  }
  GDT_BOOLEAN Equals(const char val) const    { return Len() <= 1 && *m_pchData == val;}
  GDT_BOOLEAN Equals(const int val) const     { return val == GetInt();                }
  GDT_BOOLEAN Equals(const long val) const    { return val == GetLong();               }
  GDT_BOOLEAN Equals(const float val) const   { return val == GetFloat();              }
  GDT_BOOLEAN Equals(const double val) const  { return val == GetDouble();             }
  // Case independent
  INT CaseCompare(const STRING& str) const { return CmpNoCase(str);};
  INT CaseCompare(const char *str) const   { return CmpNoCase(str);};
  INT CaseCompare (const STRING& OtherString, size_t len) const
	{ return Strnicmp(c_str(), OtherString.c_str(), len); }
  INT CaseCompare (const char *psz, size_t len) const
	{ return Strnicmp(c_str(), psz, len); }

  GDT_BOOLEAN CaseEquals(const STRING& str) const { return Len() == Strlen(str) && CmpNoCase(str) == 0;};
  GDT_BOOLEAN CaseEquals(const char *str) const   { return Len() == Strlen(str) && CmpNoCase(str) == 0;};
  GDT_BOOLEAN CaseEquals(const char val) const    { return Len() <= 1 && _ib_tolower(val) == _ib_tolower(*m_pchData);};
  GDT_BOOLEAN CaseEquals(const int val) const     { return val == GetInt();};
  GDT_BOOLEAN CaseEquals(const long val) const    { return val == GetLong();};
  GDT_BOOLEAN CaseEquals(const float val) const   { return val == GetFloat();};
  GDT_BOOLEAN CaseEquals(const double val) const  { return val == GetDouble();};

// More Nassib String functions (1 is first element)
  STRING& Insert (const STRINGINDEX InsertionPoint, const STRING& OtherString);
  STRING& EraseBefore (const STRINGINDEX Index);
  STRING& EraseAfter (const STRINGINDEX Index);
  STRING  SubString (const STRINGINDEX Start, const STRINGINDEX End) const;
//
  STRING& EraseAfterNul();
  STRING& EraseAfterNul(const STRINGINDEX Index); // Like EraseAfter
// Special Code
  void SetTermLower (const UCHR *Ptr, const STRINGINDEX Len);
// Basic Assignment
  STRING& Assign (const CHR * CString);
  STRING& Assign (const CHR * CString, const size_t MaxLen);
  STRING& Assign (const STRING& OtherString);

  // effectively copies data to string
  STRING& AssignCopy(size_t, const char *);

// I/O
  STRINGINDEX WriteFile (PFILE Fp) const;
  STRINGINDEX WriteFile (const STRING& FileName) const;
  STRINGINDEX AppendFile (const STRING& FileName) const;
  STRINGINDEX ReadFile (PFILE Fp);
  STRINGINDEX ReadFile (const STRING& FileName);
  STRINGINDEX CatFile  (const STRING& FileName);
  STRINGINDEX CatFile  (PFILE Fp);
  PFILE fopen (const STRING& Type) const { return ::fopen(m_pchData, Type.m_pchData);}
  PFILE fopen (const CHR *Type) const { return ::fopen(m_pchData, Type); }
  PFILE Fopen (const CHR *Type) const { return fopen(Type);}
  INT Unlink () const;
  STRINGINDEX Fread(const STRING& Filename, size_t Len, off_t Offset=0L);
  STRINGINDEX Fread(PFILE fp, size_t Len);
  STRINGINDEX Fread(FILE *fp, size_t Len, off_t Offset);
  GDT_BOOLEAN FGet (PFILE FilePointer);
  GDT_BOOLEAN FGet (PFILE FilePointer, const STRINGINDEX MaxCharacters);
  GDT_BOOLEAN FGetMultiLine(PFILE FilePointer, const STRINGINDEX MaxCharacters = 65535);

  void Print () const;
  void Print (STRINGINDEX Conline) const;
  void Print (PFILE FilePointer) const;
  void Print (PFILE FilePointer, STRINGINDEX Conline) const;

// IO Streams
  friend ostream& operator <<(ostream& os, const STRING& str);
  friend istream& operator >>(istream& os, STRING& str);
  friend STRING operator <<(STRING& str, istream& os);

// Write and Read FastLoad STRING objects..
  void Write(FILE *fp) const;
  GDT_BOOLEAN Read(FILE *fp);

// Write out RAW objects for Memory mapping
  int RawWrite(int fd) const; // returns number of bytes written
  int RawRead(int fd); // return number of bytes read
  int RawRead(int fd, off_t offset);
  int RawRead(const void *ptr); 
  int RawRead(void *map, size_t i);

protected:
  // points to data preceded by STRINGData structure with ref count info
  char *m_pchData;

  // accessor to string data
  STRINGData* GetStringData() const { return (STRINGData*)m_pchData-1; }

  // string (re)initialization functions
    // initializes the string to the empty value (must be called only from
    // ctors, use Reinit() otherwise)
  void Init() { m_pchData = (char *)g_szNul; }
    // initializaes the string with (a part of) C-string
  void InitWith(const char *psz, size_t nPos = 0, size_t nLen = STRING_MAXLEN);
    // as Init, but also frees old data
  inline void Reinit(); 

  // memory allocation
    // allocates memory for string of lenght nLen
  STRINGData  *AllocBuffer(size_t nLen);
    // copies data to another string
  void AllocCopy(STRING&, int, int) const;
  
  // append a (sub)string
  void ConcatCopy(int nLen1, const char *src1, int nLen2, const char *src2);
  void ConcatSelf(int nLen, const char *src);

  // functions called before writing to the string: they copy it if there 
  // other references (should be the only owner when writing)
  GDT_BOOLEAN  CopyBeforeWrite();
  GDT_BOOLEAN  AllocBeforeWrite(size_t);

};

extern const STRING& NulString;

// ----------------------------------------------------------------------------
/** The string array uses it's knowledge of internal structure of the String
    class to optimize string storage. Normally, we would store pointers to
    string, but as String is, in fact, itself a pointer (sizeof(String) is
    sizeof(char *)) we store these pointers instead. The cast to "String *"
    is really all we need to turn such pointer into a string!

    Of course, it can be called a dirty hack, but we use twice less memory 
    and this approach is also more speed efficient, so it's probably worth it.

    Usage notes: when a string is added/inserted, a new copy of it is created,
    so the original string may be safely deleted. When a string is retrieved
    from the array (operator[] or Item() method), a reference is returned.

    @name ArraySTRING
    @memo probably the most commonly used array type - array of strings
 */
// ----------------------------------------------------------------------------
#include "strlist.hxx"

class ArraySTRING {
public:
  /** @name ctors and dtor */
  //@{
    /// default ctor
  ArraySTRING();
    /// Default Count
  ArraySTRING(size_t size);
    /// copy ctor
  ArraySTRING(const ArraySTRING& array);
    /// From List
  ArraySTRING(const STRLIST& list);
    /// assignment operator
  ArraySTRING& operator=(const ArraySTRING& src);
    /// not virtual, this class can't be derived from
 ~ArraySTRING();
  //@}

  /** @name memory management */
  //@{
    /// empties the list, but doesn't release memory
  void Empty();
    /// empties the list and releases memory
  void Clear();
    /// preallocates memory for given number of items
  void Alloc(size_t nCount);
  //@}

  /** @name simple accessors */
  //@{
    /// number of elements in the array
  UINT  Count() const   { return m_nCount;      }
    /// is it empty?
  GDT_BOOLEAN  IsEmpty() const { return m_nCount == 0; }
  //@}

  /** @name items access (range checking is done in debug version) */
  //@{
    /// get item at position uiIndex
  STRING& Item(size_t nIndex) const
    { wxASSERT( nIndex < m_nCount ); return *(STRING *)&(m_pItems[nIndex]); }
    /// same as Item()
  STRING& operator[](size_t nIndex) const { return Item(nIndex); }
    /// get last item
  STRING& Last() const { wxASSERT( !IsEmpty() ); return Item(Count() - 1); }
  //@}
  // Compat. with STRLIST (start count with 1)
  STRING  GetEntry(const size_t Index) const
    { return (Index > 0 && Index <=  m_nCount) ? Item(Index-1) : NulString; }
  GDT_BOOLEAN GetEntry(const size_t Index, STRING* StringEntry) const
    { return !(*StringEntry = GetEntry(Index)).IsEmpty(); }
  void SetEntry(const size_t Index, const STRING& src)
    { if (Index > 0) Replace(src, Index-1); }

  /** @name item management */
  //@{
    /**
      Search the element in the array, starting from the either side
      @param if bFromEnd reverse search direction
      @param if bCase, comparaison is case sensitive (default)
      @return index of the first item matched or NOT_FOUND
      @see NOT_FOUND
     */
  int  Index (const char *sz, GDT_BOOLEAN bCase = GDT_TRUE, GDT_BOOLEAN bFromEnd = GDT_FALSE) const;
    /// add new element at the end
  void Add   (const STRING& str);
    /// add new element at given position
  void Insert(const STRING& str, UINT uiIndex);
    /// Replace element at given position
  void Replace(const STRING& src, size_t nIndex);

    /// remove first item matching this value
  void Remove(const char *sz);
    /// remove item by index
  void Remove(size_t nIndex);
  //@}

  /// sort array elements
  void Sort(GDT_BOOLEAN bCase = GDT_TRUE, GDT_BOOLEAN bReverse = GDT_FALSE);
  void Sort(size_t offset, GDT_BOOLEAN bCase = GDT_TRUE, GDT_BOOLEAN bReverse = GDT_FALSE);
  void UniqueSort(GDT_BOOLEAN bCase = GDT_TRUE, GDT_BOOLEAN bReverse = GDT_FALSE);
  void UniqueSort(size_t offset, GDT_BOOLEAN bCase = GDT_TRUE, GDT_BOOLEAN bReverse = GDT_FALSE);

private:
  void    Grow();     // makes array bigger if needed
  void    Free();     // free the string stored

  size_t  m_nSize,    // current size of the array
          m_nCount;   // current number of elements

  char  **m_pItems;   // pointer to data
};

// ---------------------------------------------------------------------------
// implementation of inline functions
// ---------------------------------------------------------------------------
// Case dependent comparison
inline INT Compare(const STRING& s1, const STRING& s2)     { return s1.Cmp(s2);        }
inline INT Compare(const STRING& s1, const char  * s2)     { return s1.Cmp(s2);        }
inline INT Compare(const char *s1, const STRING& s2)       { return -s2.Cmp(s1);       }
inline INT Compare(const char *s1, const char * s2)        { return strcmp(s1, s2);    }
inline INT Compare(const STRING& s1, const STRING& s2, size_t len) { return s1.Compare(s2, len);  }
inline INT Compare(const STRING& s1, const char  * s2, size_t len) { return s1.Compare(s2, len);  }
inline INT Compare(const char *s1, const STRING& s2, size_t len)   { return -s2.Compare(s1, len); }
inline INT Compare(const char *s1, const char * s2, size_t len)    { return strncmp(s1, s2, len); }
// Case INDEPENDENT
inline INT CaseCompare(const STRING& s1, const STRING& s2) { return s1.CmpNoCase(s2);  }
inline INT CaseCompare(const STRING& s1, const char  * s2) { return s1.CmpNoCase(s2);  }
inline INT CaseCompare(const char *s1, const STRING& s2)   { return -s2.CmpNoCase(s1); }
inline INT CaseCompare(const char *s1, const char * s2)    { return Stricmp(s1, s2);   }
inline INT CaseCompare(const STRING& s1, const STRING& s2, size_t len) { return s1.CaseCompare(s2, len);  }
inline INT CaseCompare(const STRING& s1, const char  * s2, size_t len) { return s1.CaseCompare(s2, len);  }
inline INT CaseCompare(const char *s1, const STRING& s2, size_t len)   { return -s2.CaseCompare(s1, len); }
inline INT CaseCompare(const char *s1, const char * s2, size_t len)    { return Strnicmp(s1, s2, len);    }
///
inline GDT_BOOLEAN operator==(const STRING& s1, const STRING& s2) { return s1.Equals(s2); }
///
inline GDT_BOOLEAN operator==(const STRING& s1, const char  * s2) { return s1.Equals(s2); }
///
inline GDT_BOOLEAN operator==(const char  * s1, const STRING& s2) { return s2.Equals(s1); }
///
inline GDT_BOOLEAN operator==(const char  s1, const STRING& s2)   { return s2.Equals(s1); }
///
inline GDT_BOOLEAN operator==(const STRING& s1, const char s2)    { return s1.Equals(s2); }
///
inline GDT_BOOLEAN operator^=(const STRING& s1, const STRING& s2) { return s1.CaseEquals(s2); }
///
inline GDT_BOOLEAN operator^=(const STRING& s1, const char  * s2) { return s1.CaseEquals(s2); }
///
inline GDT_BOOLEAN operator^=(const char *  s1, const STRING& s2) { return s2.CaseEquals(s1); }
///
inline GDT_BOOLEAN operator!=(const STRING& s1, const STRING& s2) { return !s1.Equals(s2); }
///
inline GDT_BOOLEAN operator!=(const STRING& s1, const char  * s2) { return !s1.Equals(s2); }
///
inline GDT_BOOLEAN operator!=(const char  * s1, const STRING& s2) { return !s2.Equals(s1); }
///
inline GDT_BOOLEAN operator< (const STRING& s1, const STRING& s2) { return s1.Cmp(s2) <  0; }
///
inline GDT_BOOLEAN operator< (const STRING& s1, const char  * s2) { return s1.Cmp(s2) <  0; }
///
inline GDT_BOOLEAN operator< (const char  * s1, const STRING& s2) { return s2.Cmp(s1) >  0; }
///
inline GDT_BOOLEAN operator> (const STRING& s1, const STRING& s2) { return s1.Cmp(s2) >  0; }
///
inline GDT_BOOLEAN operator> (const STRING& s1, const char  * s2) { return s1.Cmp(s2) >  0; }
///
inline GDT_BOOLEAN operator> (const char  * s1, const STRING& s2) { return s2.Cmp(s1) <  0; }
///
inline GDT_BOOLEAN operator<=(const STRING& s1, const STRING& s2) { return s1.Cmp(s2) <= 0; }
///
inline GDT_BOOLEAN operator<=(const STRING& s1, const char  * s2) { return s1.Cmp(s2) <= 0; }
///
inline GDT_BOOLEAN operator<=(const char  * s1, const STRING& s2) { return s2.Cmp(s1) >= 0; }
///
inline GDT_BOOLEAN operator>=(const STRING& s1, const STRING& s2) { return s1.Cmp(s2) >= 0; }
///
inline GDT_BOOLEAN operator>=(const STRING& s1, const char  * s2) { return s1.Cmp(s2) >= 0; }
///
inline GDT_BOOLEAN operator>=(const char  * s1, const STRING& s2) { return s2.Cmp(s1) <= 0; }


// Handle \n
#if defined(_MSDOS) || defined(_WIN32)
# define __NL "\r\n"
#else
# define __NL "\n"
#endif
inline STRING &endl(STRING &s) { s << __NL; return s; }
#undef __NL
   

typedef STRING *PSTRING;

void Write(const STRING& s, PFILE Fp);

GDT_BOOLEAN Read(PSTRING s, PFILE Fp);

inline STRING XMLCommentStrip(const STRING& Input) { return STRING(Input).XMLCommentStrip(); };

#if 0
// International String
class I18nSTRING: public STRING {
  I18nSTRING() { }
  I18nSTRING(const CHARSET& CharSet) { CharsetId = (int)CharSet; }
  I18nSTRING(const I18nSTRING& Val) {
     *this = Val;
  }
  I18nSTRING(const STRING& Input, const CHARSET& Set) {
     CHARSET(CharsetId = (int)Set).ToUTF(&Value, Input);
  }
  I18nSTRING& operator=(const I18nSTRING& Src) {
    Value     = Src.Value;
    CharsetId = Src.CharsetId;
    return *this;
  }
  I18nSTRING& operator=(const STRING& Src) {
    CHARSET(CharsetId).ToUTF(&Value, Src);
    return *this; 
  }

private:
  STRING Value;
  BYTE   CharsetId;
}
#endif /* 0 */

#endif
