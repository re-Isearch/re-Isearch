/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)string.cxx  1.93 04/17/01 00:38:17 BSN"
/* ########################################################################

   Copyright (c) 1995-1997 : Edward C. Zimmermann. All Rights Reserved.
   Copyright (c) 1995-1997 : Basis Systeme netzwerk. All Rights Reserved.

  This notice is intended as a precaution against inadvertent publication
  and does not constitute an admission or acknowledgement that publication
  has occurred or constitute a waiver of confidentiality.

  This software is the proprietary and confidential property of Basis
  Systeme netzwerk, Munich.

  Basis Systeme netzwerk, Brecherspitzstr. 8, D-81541 Munich, Germany.
  tel: +49 (89) 692 8120
  fax: +49 (89) 692 8150

  ######################################################################## 

This module is NOW placed into the public domain: 2020. Edward C. Zimmermann


   ######################################################################## */
#ident "(C) Copyright 1994-1998, Edward C. Zimmermann and Basis Systeme netzwerk, Munich."

/************************************************************************
************************************************************************/

#ifdef __GNUG__
#pragma implementation "string.hxx"
#endif

/*
 * About ref counting:
 *  1) all empty strings use g_strEmpty, nRefs = -1 (set in Init())
 *  2) AllocBuffer() sets nRefs to 1, Lock() increments it by one
 *  3) Unlock() decrements nRefs and frees memory if it goes to 0
 */

#define HEADROOM(_x,_y) (((_x)/(_y) + 1)*(_y))

// ===========================================================================
// headers, declarations, constants
// ===========================================================================

#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "common.hxx"
#include "string.hxx"
#include "mmap.hxx"
#include "magic.hxx"
#ifdef _WIN32
#include <io.h>
#endif
#include "date.hxx"
#include "numbers.hxx"

#define NDEBUG 1
#ifndef NDEBUG
# include <assert.h>
# undef wxASSERT
# define wxASSERT(x) {assert(x);}
#endif

#define DELETEA(p)      if ( (p) != NULL ) delete [] p

// ---------------------------------------------------------------------------
// static class variables definition
// ---------------------------------------------------------------------------

#ifdef  STD_STRING_COMPATIBILITY
  const size_t STRING::npos = STRING_MAXLEN;
#endif

// ===========================================================================
// static class data, special inlines
// ===========================================================================

// for an empty string, GetStringData() will return this address
#if WANT_I18_STRING
/*
  INT2  nRefs;        // reference count
  UINT4 nDataLength;  // actual string length
  UINT4 nAllocLength; // allocated memory size
  UINT2 nCharset;     // Charset used
*/
static const INT2 g_strEmpty[] = {
 -1, -1, // Ref count
  0,  0, // Length
  0,  0, // nAlloc
  0,     // Charset
  0
};
// empty C style string: points to 'string data' byte of g_strEmpty
char *g_szNul = (char *)(&g_strEmpty[7]);

#else
static const INT4 g_strEmpty[] = {
	-1,     // ref count (locked)
         0,     // current length
         0,     // allocated memory
         0      // string data
};

// empty C style string: points to 'string data' byte of g_strEmpty
const char *g_szNul = (const char *)(&g_strEmpty[3]);

#endif


// empty string shares memory with g_strEmpty
const STRINGData * const g_strNul = (const STRINGData* const)&g_strEmpty;


// ===========================================================================
// global functions
// ===========================================================================

#ifdef  STD_STRING_COMPATIBILITY

#define   NAMESPACE

STRING operator <<(STRING& str, NAMESPACE istream& is)
{
  is >> str;
  return str;
}

NAMESPACE istream& operator>>(NAMESPACE istream& is, STRING& str)
{
  int w = is.width(0);
  str.Clear(); // was erase();
  if ( 1 /* is.ipfx(0) */ ) {
    NAMESPACE streambuf *sb = is.rdbuf();
    for ( ;; ) {
      int ch = sb->sbumpc ();
      if ( ch == EOF ) {
        is.clear( is.rdstate() | NAMESPACE ios::eofbit );
        break;
      }
      else if ( _ib_isspace(ch) ) {
        sb-> sputbackc(ch); // was sungetc();
        break;
      }
      str += (char)ch;
      if ( --w == 1 )
        break;
    }
  }

  // is.isfx();
  if ( str.Len() == 0 )
    is.clear(is.rdstate() | NAMESPACE ios::failbit);
  return is;
}

NAMESPACE ostream& operator<<(NAMESPACE ostream& os, const STRING& str)
{
  os.write(str.m_pchData, str.Len());
  return os;
}

#endif  //std::string compatibility

// ===========================================================================
// STRING class core
// ===========================================================================

// ---------------------------------------------------------------------------
// construction
// ---------------------------------------------------------------------------

// construct an empty string
STRING::STRING()
{
  Init();
}

// copy constructor
STRING::STRING(const STRING& stringSrc)
{
  wxASSERT( stringSrc.GetStringData()->IsValid() );

  if ( stringSrc.IsEmpty() ) {
    // nothing to do for an empty string
    Init();
  }
  else {
    m_pchData = stringSrc.m_pchData;            // share same data
    GetStringData()->Lock();                    // => one more copy
  }
}


// constructs string of <nLength> copies of character <ch>
STRING::STRING(char ch, size_t nLength)
{
  Init();

  if ( nLength > 0 ) {
    if (AllocBuffer(nLength))
      {
	wxASSERT( sizeof(char) == 1 );  // can't use memset if not
	memset(m_pchData, ch, nLength);
      }
  }
}

STRING::STRING(unsigned char ch, size_t nLength)
{
  Init();

  if ( nLength > 0 ) {
    if (AllocBuffer(nLength))
      {
	wxASSERT( sizeof(char) == 1 );  // can't use memset if not
	memset(m_pchData, ch, nLength);
      }
  }
}


STRING::STRING(const signed short ShortValue)
{
  Init();
  *this = ShortValue;
}

STRING::STRING(const unsigned short ShortValue)
{
  Init();
  *this = ShortValue;
}

STRING::STRING(const signed IntValue)
{
  Init();
  *this = IntValue;
}

STRING::STRING(const unsigned IntValue)
{
  Init();
  *this = IntValue;
}


STRING::STRING(const signed long LongValue)
{
  Init();
  *this = LongValue;
}

STRING::STRING(const unsigned long LongValue)
{
  Init();
  *this = LongValue;
}


STRING::STRING(const signed long long LongLongValue)
{
  Init();
  *this = LongLongValue;
}

STRING::STRING(const unsigned long long LongLongValue)
{
  Init();
  *this = LongLongValue;
}




STRING::STRING(const float FloatValue)
{
  Init();
  *this = FloatValue;
}

STRING::STRING(const double DoubleValue)
{
  Init();
  *this = DoubleValue;
}

STRING::STRING(const long double LongDoubleValue)
{
  Init();
  *this = LongDoubleValue;
}


// takes nLength elements of psz starting at nPos
void STRING::InitWith(const char *psz, size_t nPos, size_t nLength)
{
  Init();

  wxASSERT( nPos <= Strlen(psz) );

  if ( nLength == STRING_MAXLEN )
    nLength = Strlen(psz + nPos);

  if ( nLength > 0 ) {
    // trailing '\0' is written in AllocBuffer()
    if (AllocBuffer(nLength))
      memcpy(m_pchData, psz + nPos, nLength*sizeof(char));
  }
}
        
// take first nLength characters of C string psz
// (default value of STRING_MAXLEN means take all the string)
STRING::STRING(const char *psz, size_t nLength)
{
  if (psz == NULL || *psz == '\0')
    Init();
  else
    InitWith(psz, 0, nLength);
}

// the same as previous constructor, but for compilers using unsigned char
STRING::STRING(const unsigned char* psz, size_t nLength)
{
  InitWith((const char *)psz, 0, nLength);
} 


STRING::STRING(const char * const *CharVector)
{
  Init();
  for (size_t i=0; CharVector[i]; i++)
    {
      if (i) Cat(" ");
      Cat(CharVector[i]);
    }
}


STRING::STRING (const INT2 *IntVector)
{
  Init();
  *this = IntVector;
}

STRING::STRING (const INT4 *IntVector)
{
  Init();
  *this = IntVector;
}


 
#ifdef  STD_STRING_COMPATIBILITY

// ctor from a substring
STRING::STRING(const STRING& s, size_t nPos, size_t nLen)
{
  InitWith(s.c_str(), nPos, nLen == npos ? 0 : nLen);
}

// poor man's iterators are "void *" pointers
STRING::STRING(const void *pStart, const void *pEnd)
{
  InitWith((const char *)pStart, 0, 
           (const char *)pEnd - (const char *)pStart);
}

#endif  //std::string compatibility

// from wide string
STRING::STRING(const wchar_t *pwz)
{
  // first get necessary size
  size_t nLen = wcstombs(NULL, pwz, 0);

  // empty?
  if ( nLen != 0 ) {
    if (AllocBuffer(nLen))
      wcstombs(m_pchData, pwz, nLen);
  }
  else {
    Init();
  }
}

// ---------------------------------------------------------------------------
// memory allocation
// ---------------------------------------------------------------------------

// allocates memory needed to store a C string of length nLen
STRINGData *STRING::AllocBuffer(size_t nLen)
{
  wxASSERT( nLen >  0         );    //
  wxASSERT( nLen <= INT_MAX-1 );    // max size (enough room for 1 extra)

  // allocate memory:
  // 1) one extra character for '\0' termination
  // 2) sizeof(STRINGData) for housekeeping info
  size_t       want = HEADROOM(nLen,32);
  STRINGData*  pData;

//cerr << "nLen=" << nLen << " want=" << want << endl;

#ifdef __EXCEPTIONS
  try {
    pData = (STRINGData*)new char[sizeof(STRINGData) + (want+1)*sizeof(char)];
  } catch (...) {
    pData = NULL;
  }
#else
  pData = (STRINGData*)new char[sizeof(STRINGData) + (want+1)*sizeof(char)];
#endif

  if (pData)
    {
      pData->nRefs        = 1;
      pData->data()[nLen] = '\0';
      pData->nDataLength  = nLen;
      pData->nAllocLength = want;
      m_pchData           = pData->data();  // data starts after STRINGData
    }
  else
    message_log (LOG_ERRNO|LOG_PANIC, "Could not allocate string space for %ld characters.", (long)want);
  wxASSERT(want > nLen);
  return pData;
}

// releases the string memory and reinits it
void STRING::Reinit()
{
  GetStringData()->Unlock();
  Init();
}

// wrapper around STRING::Reinit
void STRING::Empty()
{
  if ( GetStringData()->nDataLength != 0 )
    {
      Reinit();
    }

  wxASSERT( GetStringData()->nDataLength == 0 );
  wxASSERT( GetStringData()->nAllocLength == 0 );
}

// wrapper around STRING::Empty
void STRING::Clear()
{
  STRINGData* ptr = GetStringData();
  if ( ptr == NULL)
    {
      message_log(LOG_PANIC, "String data corruption in Clear(). Call your vendor!");
    }
  else if (ptr->IsConstant())
    {
      Init();
    }
  else if ( ptr->IsShared())
    {
      Empty();
    }
  else if (ptr->nDataLength)
    {
      ptr->nDataLength = 0;
    }
}

// must be called before changing this string
bool STRING::CopyBeforeWrite()
{
  STRINGData* pData = GetStringData();

  if ( pData->IsShared() || pData->IsConstant() ) {
    pData->Unlock();                // memory not freed because shared
    if (AllocBuffer( pData->nDataLength ) == NULL)
      return false;
    memcpy(m_pchData, pData->data(), pData->nDataLength*sizeof(char));
  }
  wxASSERT( !pData->IsShared() );  // we must be the only owner
  return  !pData->IsShared();
}

// must be called before replacing contents of this string
bool STRING::AllocBeforeWrite(size_t nLen)
{
  wxASSERT( nLen != 0 );  // doesn't make any sense

  // must not share string and must have enough space
  register STRINGData* pData = GetStringData();  
  if ( pData->IsShared() || pData->IsConstant() || (nLen >= pData->nAllocLength) ) {
    // can't work with old buffer, get new one
    pData->Unlock();
    if (AllocBuffer(nLen) == NULL)
      {
	message_log (LOG_PANIC, "Can't allocate string space: %u KB", (unsigned)(nLen/1024));
	return false;
      }
  } else {
    GetStringData()->nDataLength = nLen;
    m_pchData[nLen] = '\0';
  }
  wxASSERT( !GetStringData()->IsShared() );  // we must be the only owner
  return !GetStringData()->IsShared();
}

// get the pointer to writable buffer of (at least) nLen bytes
char *STRING::GetWriteBuf(size_t nLen)
{
  AllocBeforeWrite(nLen);
  return m_pchData;
}

 // Copy into buffer 
void * STRING::Copy (void *buf, size_t len) const
{
  char *ptr = (char *)buf;
  const size_t my_length = Len();
  if (len == 0)
    {
      len = my_length;
      if (ptr)
	ptr[len] = '\0';
    }
  if (ptr == NULL)
    {
#ifdef __EXCEPTIONS
      try {
	ptr = new char [ len + 1 ];
      } catch (...) {
	ptr = NULL;
      }
#else
      ptr = new char [ len + 1 ];
#endif
      if (ptr == NULL)
	{
	  message_log (LOG_ERRNO, "Could not allocate string space for %u characters.", (unsigned)len+1);
	  len = 0;
	}
      else
	ptr[len] = '\0';
    }
  if (my_length < len)
    {
      memcpy(ptr, m_pchData, my_length*sizeof(char));
      memset(ptr+my_length, '\0', len - my_length);
    }
  else if (len)
    memcpy(ptr, m_pchData, len*sizeof(char));
  return (void *)ptr;
}

PCHR STRING::GetCString (CHR *buf, size_t len) const
{
  PCHR ptr = (PCHR)Copy((void *)buf, len);
  if (len)
    ptr[len] = '\0';
  return ptr;
}

PUCHR STRING::GetUCString (UCHR *buf, size_t len) const
{
  PUCHR ptr = (PUCHR)Copy((void *)buf, len);
  if (len)
    ptr[len] = '\0';
  return ptr;
}

// @@@ edz: BUGFIX
STRINGINDEX STRING::AppendFile (const STRING& FileName) const
{
  PFILE Fp = FileName.fopen("ab");
  if (Fp == NULL)
    {
      // Force open..
      STRING dirs;
      dirs = FileName;
      RemoveFileName(&dirs);
      if (MkDirs(dirs, 00644))
	Fp = FileName.fopen("ab");
    }
  STRINGINDEX res = WriteFile (Fp);
  if (Fp) fclose(Fp);
  return res;
}

STRINGINDEX STRING::WriteFile (const STRING& FileName) const
{
  PFILE Fp = FileName.fopen("wb");

  if (Fp == NULL)
    {
      // Force open..
      STRING dirs;
      dirs = FileName;
      RemoveFileName(&dirs);
      if (MkDirs(dirs, 00644))
	Fp = FileName.fopen("wb");
    }
  STRINGINDEX res = WriteFile (Fp);
  if (Fp) fclose(Fp);
  return res;
}

STRINGINDEX STRING::WriteFile (PFILE fp) const
{
  STRINGINDEX len = 0;
  if (fp)
    {
      const size_t length = Len();
      if (length)
	{
	  size_t res;
	  size_t newlength = length;
	  // See fwrite(3s) 
	  do {
	    res = (size_t)fwrite (&m_pchData[len], sizeof(UCHR), newlength, fp);
	    len += res;
	    newlength -= res;
	  } while (res && newlength > 0);
	}
    }
  return len;
}

// @@@ edz: BUGFIX
// Read a file into a STRING
STRINGINDEX STRING::ReadFile (const STRING& FileName)
{
  Clear();
  return CatFile (FileName);
}

STRINGINDEX STRING::ReadFile (PFILE Fp)
{
  Clear();
  return CatFile (Fp);
}


STRINGINDEX STRING::CatFile (const STRING& FileName)
{
  FILE *Fp = FileName.fopen("rb");
  STRINGINDEX res = 0;
  if (Fp != NULL)
    {
      res = CatFile (Fp);
      fclose(Fp);
    }
  return res;
}

STRINGINDEX STRING::CatFile (PFILE fp)
{
  size_t length = Len();
  const size_t old_length = length;
  if (fp)
    {
      // Make sure its a regular file
      struct stat sb;
      if ((fstat(fileno(fp), &sb) >= 0) && ((sb.st_mode & S_IFMT) == S_IFREG))
	{
	  const unsigned long max_size = (unsigned long)UINT_MAX - length - 1024;
	  if ((unsigned long)sb.st_size > max_size) 
	    {
	      message_log (LOG_ERROR, "%luKb+%luKb exceeds the string capacity (%luKb) of this platform.",
		(long)length/1024, (long)sb.st_size/1024, max_size/1024);
	    }
          else if (sb.st_size)
	    {
	      size_t addlength = (size_t)sb.st_size;
	      size_t nLen = addlength + length+1; 
	      register STRINGData* pData = GetStringData();
	      if ( pData->IsShared() || pData->IsConstant() || (nLen >= pData->nAllocLength) )
		{
		  AllocBuffer(nLen);
		  memcpy(m_pchData, pData->data(), length*sizeof(char));
		  pData->Unlock();
		} 
	      size_t res;
	      // See fread(3s)
//cerr << "Read Loop for " << addlength << " characters" << endl;
	      do {
		res = fread (&m_pchData[length], sizeof(UCHR), addlength, fp);
		length += res;
		addlength -= res;
	      } while (res && addlength > 0);
	      m_pchData[length] = '\0';
	      GetStringData()->nDataLength= length;
	    }
	}
    }
  return length - old_length;
}


void STRING::Write(FILE *fp) const
{
  const size_t length = Len();
  if (length < 256)
    {
      putObjID(objBCPL, fp);
      ::Write((CHR)length, fp);
    }
  else
    {
      putObjID(objSTRING, fp);
      ::Write((UINT2)length, fp);
    }
  if (length)
    {
      fwrite(m_pchData, sizeof(UCHR), length, fp);
    }
}

// Tricky !!!!!!!!!
int STRING::RawWrite(int fd) const
{
  int bytes = 0;
  if (fd != -1)
    {
      int         res;
      STRINGData *pData   = GetStringData();
      STRINGData  Kludge = { -1, pData->nDataLength, 0 };

      if ((res = ::write(fd, (void *)&Kludge, sizeof(STRINGData))) == -1)
	return res;
      bytes += res;
      if ((res = ::write(fd, m_pchData, sizeof(UCHR)*pData->nDataLength + 1)) != -1)
	bytes += res;
    }
  return bytes;
}

int STRING::RawRead(int fd, off_t offset)
{
  if (-1 == lseek(fd, offset, SEEK_SET))
    return -1;
  return RawRead(fd);
}


int STRING::RawRead(int fd)
{
  size_t     bytes = 0;

  if (fd != -1)
    {
      STRINGData Data;
      int        res;

      errno = 0;
      if ((res =  _sys_read(fd, (void *)&Data, sizeof(STRINGData))) == -1)
	return -1; // ERROR

      if (res != sizeof(STRINGData))
	{
	  if (res != 0)
	    message_log (LOG_PANIC, "Short read in STRING::RawRead(%d) %d < %u ",
		fd, res, (unsigned)sizeof(STRINGData) );
	  Clear();
	  return -1;
	}

      size_t     len = Data.nDataLength + 1;

      AllocBeforeWrite(len);
      if ((res = _sys_read(fd, m_pchData, len)) != -1)
	{
	  bytes += res;
	  GetStringData()->nDataLength = res-1;
	}
       else
	GetStringData()->nDataLength = 0;
    }
  return bytes; 
}

int STRING::RawRead(const void *Ptr)
{
  const STRINGData *pData = (const STRINGData *)Ptr;

  GetStringData()->Unlock();
  m_pchData = pData->data();  
  return sizeof(STRINGData) + (pData->nDataLength)*sizeof(UCHR) + 1;
}


int STRING::RawRead(void *ptr, size_t i)
{
  if (ptr == NULL)
    return -1;
  const MMAP  *map = (const MMAP *)ptr;

  if (!map->Ok())
    {
      message_log (LOG_PANIC, "Can't read from memory mapped file! Corrupt MMAP?" );
      return -1;
    }
  const size_t  fLength = map->Size();
  const size_t  dOffset  = i + sizeof(STRINGData);
  if (dOffset > fLength)
    {
      message_log (LOG_PANIC, "Lookup bounds error in memory mapped STRING::RawRead (%ld>%ld)!",
	(long)dOffset, (long)fLength);
      return -1;
    }
#if 1
  *this = (char *)(map->Ptr() + dOffset);
  return sizeof(STRINGData) + Len() + 1;
#else
  UCHR  *tcp = map->Ptr() + i;
  const STRINGData * const pData = (const STRINGData * const)tcp;

//cerr << "If pData is null..." << endl;
  if (pData == NULL)
    {
      message_log (LOG_PANIC, "Null pointer in mapped I/O?");
      return -1;
    }
//cerr << "Check nRefs..." << endl;
  if (pData->nRefs != -1)
    {
      message_log (LOG_PANIC, "Wrong Refcount on memory mapped STRING (%d)", pData->nRefs);
      return -1;
    }
//cerr << "Check Bound..." << endl;

  if ((pData->nDataLength + dOffset) > fLength)
    {
//cerr << "Bound err!" << endl;
      message_log (LOG_PANIC, "Lookup bounds error (%d) in memory mapped STRING::RawRead! Truncated file (%ld>%ld)?",
	i, pData->nDataLength + sizeof(STRINGData) + i, file_len);
    }

//cerr << "Unlock" << endl;
  GetStringData()->Unlock();
  m_pchData = pData->data();

//cerr << "Raw Read from MAPED " << *this << endl;

  return sizeof(STRINGData) + (pData->nDataLength)*sizeof(UCHR) + 1;
#endif
}


bool STRING::Read(FILE *fp)
{
  obj_t obj = getObjID(fp); // It it really a string?
  size_t length = 0;

  if (obj == objBCPL)
    length = (UCHR)fgetc (fp);
  else if (obj == objSTRING)
    length = getINT2 (fp);
  else
    PushBackObjID (obj, fp);
  STRING::Fread(fp, length);

  return (obj == objBCPL || obj == objSTRING);
}

// dtor frees memory if no other strings use it
STRING::~STRING()
{
  GetStringData()->Unlock();
}

int STRING::sprintf(const char *pszFormat, ...)
{
  va_list argptr;
  va_start(argptr, pszFormat);
  int iLen = PrintfV(pszFormat, argptr);
  va_end(argptr);
  return iLen;
}

STRING& STRING::form(const char *pszFormat, ...)
{
  if (pszFormat != NULL)
    {
      va_list argptr;
      va_start(argptr, pszFormat);
      PrintfV(pszFormat, argptr);
      va_end(argptr);
    }
  else
    message_log (LOG_PANIC, "Null format passed to STRING::form()");
  return *this;
}

#if 0
static char *encode64(char *ptr, size_t siz, unsigned long num)
{
  char tmp[13];
  /* 64 characters */
  char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz-";
  const size_t modulo = sizeof(chars)/sizeof(char) - 1;
  const size_t pad    = siz - 1;
  unsigned long val = num;
  size_t i = 0, j;

  do {
    tmp[i++] = chars[ val % modulo ];
    val /= modulo;
  } while (val);
  for (j =i; j < pad; j++)
    tmp[j] = '0';
  for (j = 0; j < pad; j++)
    ptr[j] = tmp[pad -  j - 1];
  ptr[j] = '\0';
  return ptr;
}

STRING STRING::Key() const
{
  char ptr[6];
  return encode64(ptr, sizeof(ptr), Hash())
}

#endif


// C-String escape encode/decocde
STRING STRING::Escape () const
{
  STRING Temp;
  const size_t len = Len();

  for (size_t i = 0; i < len; i++)
    {
      UCHR Ch;
      switch (Ch =  (UCHR)m_pchData[i])
	{
	  case '\0': Temp += "\\0";  break;
	  case '\n': Temp += "\\n";  break;
	  case '\t': Temp += "\\t";  break;
	  case '\v': Temp += "\\v";  break;
	  case '\b': Temp += "\\b";  break;
	  case '\r': Temp += "\\r";  break;
	  case '\f': Temp += "\\f";  break;
	  case '\a': Temp += "\\a";  break;
	  case '\\': Temp += "\\\\"; break;
	  case '"':  Temp += "\\\""; break;
	  case '\'': Temp += "\\'";  break;
	  // Special Case to protect Wildcards
	  // and operators..
	  case '?' : case '*': case '~': case '=':
	  case '<' : case '>': case '#' :
	    Temp += "\\"; Temp += Ch; break;
	  // Special (Private) Cases
	  case '\033': Temp += "\\E"; break;
	  // Default
	  default: {
	    // Is it a control sequence?
	    if (!_ib_isprint(Ch) && !_ib_isspace(Ch))
	      {
		// Escape as Octal
		char buf[5];
		::sprintf(buf, "\\%03o", (unsigned)Ch);
		Temp += buf;
	      }
	    else
	      Temp += Ch;
         }
     }
   }
  return Temp;
}

STRING STRING::w3Encode() const
{
  const char HEX[] = "0123456789ABCDEF";
  const size_t len = Len();
  STRING dest ('\0', len*3 + 16);

  size_t j = 0;
  for (register size_t i = 0; i < len; i++)
    {
      unsigned char Ch =  (unsigned char )m_pchData[i];
      if (Ch <= 32 || strchr("%+&=?<>[]()", Ch))
	{
	  dest.m_pchData[j++] = '%';
	  dest.m_pchData[j++] = HEX[(Ch & '\377') >> 4];
	  dest.m_pchData[j++] = HEX[(Ch & '\377') % 16];
	}
      else
	dest.m_pchData[j++] = (char)Ch;
    }
  dest.GetStringData()->nDataLength = j;
  return dest;
}

STRING& STRING::cEncode ()
{
  return *this = Escape();
}

STRING STRING::unEscape () const
{
  const size_t len = Len();
  STRING dest ('\0', len);

  size_t j = 0;
  for (register size_t i = 0; i < len; i++, j++)
    {
      char Ch;
      if ((Ch = m_pchData[i]) == '\\')
	{
	  switch (Ch = m_pchData[++i])
	    {
	      case 'n': Ch = '\n'; break;
	      case 't': Ch = '\t'; break;
	      case 'v': Ch = '\v'; break;
	      case 'b': Ch = '\b'; break;
	      case 'r': Ch = '\r'; break;
	      case 'f': Ch = '\f'; break;
	      case 'a': Ch = '\a'; break;
	      // Special (Private) Cases
	      case 'E': Ch = '\033'; break;
	      // Hex Esacpes \xhh
	      case 'x': case 'X':
		{
		  // Hex
		  register int val = 0;
		  for (register int count=0;count<2;count++)
		    {
		      int digit;
		      Ch = m_pchData[++i];
		      if (Ch >= '0' && Ch <= '9')
			digit = Ch - '0';
		      else if (Ch >= 'a' && Ch <= 'f')
			digit = Ch - 'a' + 10;
		      else if (Ch >= 'A' && Ch <= 'F')
			digit = Ch - 'A' + 10;
		      else {
			i--; // Backup
			break;
		      }
		      val <<= 4 ;
		      val += digit;
		    }
		  Ch = val;
		}
	      // Octal Escapes \ooo
	      case '0': case '1': case '2': case '3':
	      case '4': case '5': case '6': case '7':
		{
		  register int val = Ch - '0';
		  for (register int count = 0; count < 3; count++)
		    {
		      Ch = m_pchData[i++];
		      if (Ch >= '0' && Ch <= '7')
			{
			  val <<= 3;
			  val += Ch - '0';
			}
		      else
			{
			  i--; // Backup
			  break;
			}
		    }
		  Ch = val;
		}
	    }
	}
      else
	Ch = m_pchData[i];
//  if (j != i)
      dest.m_pchData[j] = Ch;
    }
  dest.m_pchData[dest.GetStringData()->nDataLength = j] = '\0';
  return dest;
}

STRING& STRING::cDecode ()
{
  return *this = unEscape();
}

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
# define __get16bits(d) (*((const UINT2 *) (d)))
#else
# define __get16bits(d) ((((UINT4)(((const BYTE *)(d))[1])) << 8)\
                       +(UINT4)(((const BYTE *)(d))[0]) )
#endif

static UINT4 SuperFastHash (const char * data, int len)
{
   UINT4 hash = len, tmp;
   int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += __get16bits (data);
        tmp    = (__get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (UINT2);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += __get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (UINT2)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += __get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;
    return hash;
}
#undef get16bits

// Hash
unsigned STRING::CaseHash() const
{
  STRING foo = *this;

  return foo.MakeLower().Hash();
}

unsigned STRING::Hash() const
{
  return SuperFastHash(m_pchData, Len());
}

UINT8 STRING::CRC64() const
{
  return ::CRC64(m_pchData, Len());
}

/* Return a 32-bit CRC of the contents of the buffer. */

UINT4 STRING::CRC32() const
{
 UINT4 crctable[256] = {
 0x00000000L, 0x77073096L, 0xEE0E612CL, 0x990951BAL,
 0x076DC419L, 0x706AF48FL, 0xE963A535L, 0x9E6495A3L,
 0x0EDB8832L, 0x79DCB8A4L, 0xE0D5E91EL, 0x97D2D988L,
 0x09B64C2BL, 0x7EB17CBDL, 0xE7B82D07L, 0x90BF1D91L,
 0x1DB71064L, 0x6AB020F2L, 0xF3B97148L, 0x84BE41DEL,
 0x1ADAD47DL, 0x6DDDE4EBL, 0xF4D4B551L, 0x83D385C7L,
 0x136C9856L, 0x646BA8C0L, 0xFD62F97AL, 0x8A65C9ECL,
 0x14015C4FL, 0x63066CD9L, 0xFA0F3D63L, 0x8D080DF5L,
 0x3B6E20C8L, 0x4C69105EL, 0xD56041E4L, 0xA2677172L,
 0x3C03E4D1L, 0x4B04D447L, 0xD20D85FDL, 0xA50AB56BL,
 0x35B5A8FAL, 0x42B2986CL, 0xDBBBC9D6L, 0xACBCF940L,
 0x32D86CE3L, 0x45DF5C75L, 0xDCD60DCFL, 0xABD13D59L,
 0x26D930ACL, 0x51DE003AL, 0xC8D75180L, 0xBFD06116L,
 0x21B4F4B5L, 0x56B3C423L, 0xCFBA9599L, 0xB8BDA50FL,
 0x2802B89EL, 0x5F058808L, 0xC60CD9B2L, 0xB10BE924L,
 0x2F6F7C87L, 0x58684C11L, 0xC1611DABL, 0xB6662D3DL,
 0x76DC4190L, 0x01DB7106L, 0x98D220BCL, 0xEFD5102AL,
 0x71B18589L, 0x06B6B51FL, 0x9FBFE4A5L, 0xE8B8D433L,
 0x7807C9A2L, 0x0F00F934L, 0x9609A88EL, 0xE10E9818L,
 0x7F6A0DBBL, 0x086D3D2DL, 0x91646C97L, 0xE6635C01L,
 0x6B6B51F4L, 0x1C6C6162L, 0x856530D8L, 0xF262004EL,
 0x6C0695EDL, 0x1B01A57BL, 0x8208F4C1L, 0xF50FC457L,
 0x65B0D9C6L, 0x12B7E950L, 0x8BBEB8EAL, 0xFCB9887CL,
 0x62DD1DDFL, 0x15DA2D49L, 0x8CD37CF3L, 0xFBD44C65L,
 0x4DB26158L, 0x3AB551CEL, 0xA3BC0074L, 0xD4BB30E2L,
 0x4ADFA541L, 0x3DD895D7L, 0xA4D1C46DL, 0xD3D6F4FBL,
 0x4369E96AL, 0x346ED9FCL, 0xAD678846L, 0xDA60B8D0L,
 0x44042D73L, 0x33031DE5L, 0xAA0A4C5FL, 0xDD0D7CC9L,
 0x5005713CL, 0x270241AAL, 0xBE0B1010L, 0xC90C2086L,
 0x5768B525L, 0x206F85B3L, 0xB966D409L, 0xCE61E49FL,
 0x5EDEF90EL, 0x29D9C998L, 0xB0D09822L, 0xC7D7A8B4L,
 0x59B33D17L, 0x2EB40D81L, 0xB7BD5C3BL, 0xC0BA6CADL,
 0xEDB88320L, 0x9ABFB3B6L, 0x03B6E20CL, 0x74B1D29AL,
 0xEAD54739L, 0x9DD277AFL, 0x04DB2615L, 0x73DC1683L,
 0xE3630B12L, 0x94643B84L, 0x0D6D6A3EL, 0x7A6A5AA8L,
 0xE40ECF0BL, 0x9309FF9DL, 0x0A00AE27L, 0x7D079EB1L,
 0xF00F9344L, 0x8708A3D2L, 0x1E01F268L, 0x6906C2FEL,
 0xF762575DL, 0x806567CBL, 0x196C3671L, 0x6E6B06E7L,
 0xFED41B76L, 0x89D32BE0L, 0x10DA7A5AL, 0x67DD4ACCL,
 0xF9B9DF6FL, 0x8EBEEFF9L, 0x17B7BE43L, 0x60B08ED5L,
 0xD6D6A3E8L, 0xA1D1937EL, 0x38D8C2C4L, 0x4FDFF252L,
 0xD1BB67F1L, 0xA6BC5767L, 0x3FB506DDL, 0x48B2364BL,
 0xD80D2BDAL, 0xAF0A1B4CL, 0x36034AF6L, 0x41047A60L,
 0xDF60EFC3L, 0xA867DF55L, 0x316E8EEFL, 0x4669BE79L,
 0xCB61B38CL, 0xBC66831AL, 0x256FD2A0L, 0x5268E236L,
 0xCC0C7795L, 0xBB0B4703L, 0x220216B9L, 0x5505262FL,
 0xC5BA3BBEL, 0xB2BD0B28L, 0x2BB45A92L, 0x5CB36A04L,
 0xC2D7FFA7L, 0xB5D0CF31L, 0x2CD99E8BL, 0x5BDEAE1DL,
 0x9B64C2B0L, 0xEC63F226L, 0x756AA39CL, 0x026D930AL,
 0x9C0906A9L, 0xEB0E363FL, 0x72076785L, 0x05005713L,
 0x95BF4A82L, 0xE2B87A14L, 0x7BB12BAEL, 0x0CB61B38L,
 0x92D28E9BL, 0xE5D5BE0DL, 0x7CDCEFB7L, 0x0BDBDF21L,
 0x86D3D2D4L, 0xF1D4E242L, 0x68DDB3F8L, 0x1FDA836EL,
 0x81BE16CDL, 0xF6B9265BL, 0x6FB077E1L, 0x18B74777L,
 0x88085AE6L, 0xFF0F6A70L, 0x66063BCAL, 0x11010B5CL,
 0x8F659EFFL, 0xF862AE69L, 0x616BFFD3L, 0x166CCF45L,
 0xA00AE278L, 0xD70DD2EEL, 0x4E048354L, 0x3903B3C2L,
 0xA7672661L, 0xD06016F7L, 0x4969474DL, 0x3E6E77DBL,
 0xAED16A4AL, 0xD9D65ADCL, 0x40DF0B66L, 0x37D83BF0L,
 0xA9BCAE53L, 0xDEBB9EC5L, 0x47B2CF7FL, 0x30B5FFE9L,
 0xBDBDF21CL, 0xCABAC28AL, 0x53B39330L, 0x24B4A3A6L,
 0xBAD03605L, 0xCDD70693L, 0x54DE5729L, 0x23D967BFL,
 0xB3667A2EL, 0xC4614AB8L, 0x5D681B02L, 0x2A6F2B94L,
 0xB40BBE37L, 0xC30C8EA1L, 0x5A05DF1BL, 0x2D02EF8DL
 };
 unsigned       crc32val = 0;
 const size_t   length   = Len();
  
 for (size_t i = 0;  i < length;  i ++)
   crc32val = crctable[(crc32val ^  m_pchData[i]) & 0xff] ^ (crc32val >> 8);
 return crc32val;
}

UINT2 STRING::CRC16() const
{
  unsigned short CRC16_LookupHigh[16] = {
        0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70,
        0x81, 0x91, 0xA1, 0xB1, 0xC1, 0xD1, 0xE1, 0xF1 };
  unsigned short CRC16_LookupLow[16] = {
        0x00, 0x21, 0x42, 0x63, 0x84, 0xA5, 0xC6, 0xE7,
        0x08, 0x29, 0x4A, 0x6B, 0x8C, 0xAD, 0xCE, 0xEF };

  unsigned char CRC16_High = 0xFF;
  unsigned char CRC16_Low  = 0xFF;

#define CRC16_Update4Bits(_v ) { \
	unsigned char	t = CRC16_High >> 4; \
	t = t ^ ((unsigned char)_v); \
	CRC16_High = (CRC16_High << 4) | (CRC16_Low >> 4); \
	CRC16_Low = CRC16_Low << 4; \
	CRC16_High = CRC16_High ^ CRC16_LookupHigh[t]; \
	CRC16_Low = CRC16_Low ^ CRC16_LookupLow[t]; }
#define CRC16_Update( _v ) { \
	unsigned char val = _v; \
	CRC16_Update4Bits( val >> 4 );\
	CRC16_Update4Bits( val & 0x0F ); }

 const size_t   length   = Len();

 for (size_t i = 0;  i < length;  i ++)
    CRC16_Update(m_pchData[i]);

  UINT2 crc16 = CRC16_High;
  crc16 <<= 4;
  crc16 |= CRC16_Low;

  return crc16 /* ((UINT2)CRC16_High << 4) | CRC16_Low */ ;
}


// ---------------------------------------------------------------------------
// data access
// ---------------------------------------------------------------------------

// all functions are inline in string.h


// ---------------------------------------------------------------------------
// Insert, EraseBefore, EraseAfter (Nassib Style)
// ---------------------------------------------------------------------------

// If InsertionPt > length_ then pad the bits in between with ' '
STRING& STRING::Insert (const STRINGINDEX InsertionPt, const STRING& OtherString)
{
  const size_t length = Len();
  const size_t olength = OtherString.Len();

  if (olength == 0 || InsertionPt < 1)
    {
      return *this;
    }
  if (InsertionPt == length)
    {
      return *this += OtherString; // Insert after current string
    }

  size_t Nlength = length + olength;
  if (InsertionPt > Nlength)
    Nlength = InsertionPt;

  const size_t want      = HEADROOM(Nlength,32);
  STRINGData* pData      = NULL;

#ifdef __EXCEPTIONS
  try {
    pData      = (STRINGData*)new char[sizeof(STRINGData) + (want+1)*sizeof(char)];
  } catch (...) {
    pData      = NULL;
  }
#else
   pData      = (STRINGData*)new char[sizeof(STRINGData) + (want+1)*sizeof(char)];
#endif
  if (pData == NULL)
    {
      message_log (LOG_ERRNO|LOG_PANIC, "Could not allocate string space %ld", (long)want);
      return *this;
    }

  pData->nRefs           = 1;
  pData->data()[Nlength] = '\0';
  pData->nDataLength     = Nlength;
  pData->nAllocLength    = want;
  char *ptr              = pData->data();
  if (InsertionPt > length)
    {
      if (length)
	memcpy (ptr,  m_pchData, length);
      memset (ptr+length, ' ', Nlength-length);
    }
   else
    memcpy (ptr, m_pchData, InsertionPt - 1);
  memcpy (ptr + InsertionPt - 1, OtherString.m_pchData, olength);
  if (length >= InsertionPt)
    memcpy (ptr + InsertionPt - 1 + olength, m_pchData + InsertionPt - 1, length - InsertionPt + 1);

  GetStringData()->Unlock(); // Unlock me..
  m_pchData = ptr;  // Set me

  return *this;
}

STRING& STRING::EraseBefore (const STRINGINDEX Index)
{
  size_t length = Len();

  if ((Index > 1) && (length != 0))
    {
      if (Index <= length)
	{
	  length -= (Index - 1); // New length
	  CopyBeforeWrite();
	  memmove(m_pchData, &m_pchData[Index - 1], length);
	  m_pchData[length] = '\0';
	  GetStringData()->nDataLength = length;
	}
      else
	Clear();
    }
  return *this;
}

// We Zap at the length or the first Nul, whichever comes first
STRING& STRING::EraseAfterNul (const STRINGINDEX Index)
{
  size_t c_length = 0;
  size_t s_length = Len();

  while (m_pchData[c_length] && (c_length < Index) && (c_length < s_length))
    c_length++;
  return EraseAfter(c_length);
}
STRING& STRING::EraseAfterNul () { return EraseAfterNul(Len()); }


STRING& STRING::EraseAfter (const STRINGINDEX Index)
{
  size_t len = Len();
  if (Index < 1)
    len = 0;
  else if (Index < len)
    len = Index;
  else
    return *this; // Nothing to do
  return Truncate(len);
}


// ---------------------------------------------------------------------------
// Special Copy
// ---------------------------------------------------------------------------

// Special Code
void STRING::SetTermLower (const UCHR *Ptr, const STRINGINDEX len)
{
  size_t i = 0;

  if (len)
    {
      AllocBeforeWrite(len);
      for(UCHR ch; i < len && (ch = Ptr[i]) != '\0';i++)
	m_pchData[i] = !IsTermChr(Ptr+i) ? ' ' : _ib_tolower(ch);
    }
  if (i != 0)
    {
      GetStringData()->nDataLength = i;
      m_pchData[i] = '\0';
    }
  else
   Clear(); //  Looks like we need to just zap things
}

// Basic Assignment
STRING& STRING::Assign (const CHR *CString)
{
  return AssignCopy(Strlen(CString), CString);
}

STRING& STRING::Assign (const CHR * CString, const size_t MaxLen)
{
  size_t i = 0;
  if (CString)
    {
      while(i < MaxLen && CString[i])
	i++;
    }
  return AssignCopy(i, CString);
}

STRING& STRING::Assign (const STRING& OtherString)
{
  return *this = OtherString;
}



// ---------------------------------------------------------------------------
// assignment operators
// ---------------------------------------------------------------------------

// helper function: does real copy 
STRING& STRING::AssignCopy(size_t nSrcLen, const char *pszSrcData)
{
  if (nSrcLen > 0)
    {
      AllocBeforeWrite(nSrcLen);
      memcpy(m_pchData, pszSrcData, nSrcLen*sizeof(char));
//    GetStringData()->nDataLength = nSrcLen;
//    m_pchData[nSrcLen] = '\0';
    }
  else
    Reinit();
  return *this;
}

// assigns one string to another
STRING& STRING::operator= (const STRING& stringSrc)
{
  // Don't set NulString
  if (stringSrc.IsEmpty())
    {
      if (!IsEmpty()) Clear();
    }
  // don't copy string over itself
  else if ( m_pchData != stringSrc.m_pchData )
    {
      if (m_pchData)
	GetStringData()->Unlock(); // Unlock me..
      m_pchData = stringSrc.m_pchData;
      if (m_pchData)
	GetStringData()->Lock(); // lock us
    }
  return *this;
}

// assigns a single character
STRING& STRING::operator =(char ch)
{
  return AssignCopy(1, &ch);
}

// assigns C string
STRING& STRING::operator =(const char *psz)
{
  return Assign(psz);
}

// same as 'signed char' variant
STRING& STRING::operator =(const unsigned char* psz)
{
  return Assign((const char *)psz);
}

STRING& STRING::operator =(const wchar_t *pwz)
{
  STRING str(pwz);
  return *this = str;
}

STRING& STRING::operator =(const signed short ShortValue)
{
  return *this = (signed int)ShortValue;
}

STRING& STRING::operator =(const unsigned short ShortValue)
{
  return *this = (unsigned int)ShortValue;
}


STRING& STRING::operator =(const signed IntValue)
{
  char s[16];

  ::sprintf(s, "%i", IntValue);
  return Assign(s);
}

STRING& STRING::operator =(const unsigned IntValue)
{
  char s[16];

  ::sprintf(s, "%u", IntValue);
  return Assign(s);
}


STRING& STRING::operator=(const signed long LongValue)
{
  char s[16];

  ::sprintf(s, "%ld", LongValue);
  return Assign(s);
}


STRING& STRING::operator=(const unsigned long LongValue)
{
  char s[16];

  ::sprintf(s, "%lu", LongValue);
  return Assign(s);
}

STRING& STRING::operator=(const signed long long LongLongValue)
{
  char s[16];
#ifdef _WIN32
  ::sprintf(s, "%I64d", (__int64)LongLongValue);
#else
  ::sprintf(s, "%lld", LongLongValue);
#endif
  return Assign(s);
}


STRING& STRING::operator=(const unsigned long long LongLongValue)
{
  char s[16];

#ifdef _WIN32
  ::sprintf(s, "%I64u", (__int64)LongLongValue);
#else
  ::sprintf(s, "%llu", LongLongValue);
#endif
  return Assign(s);
}


STRING& STRING::operator=(const float val)
{
  if ((val - (long)val) == 0.0)
    {
      return *this = (long)val;
    }

  char s[32];

  ::sprintf(s, "%f", val);
  return Assign(s);
}

STRING& STRING::operator=(const double val)
{
  if ((val - (long)val) == 0.0)
    {
      return *this = (long)val;
    }

  char s[32];

  ::sprintf(s, "%g", val);
  return Assign(s);
}

STRING& STRING::operator=(const long double val)
{
  if ((val - (long)val) == 0.0)
    {
      return *this = (long long)val;
    }

  char s[64];
  
  ::sprintf(s, "%Lg", val);
  return Assign(s);
}


STRING& STRING::operator=(const INT2 *IntVector)
{
  STRING Temp;

  Temp +=IntVector;
  return *this = Temp;
}
 
STRING& STRING::operator=(const INT4 *IntVector) 
{
  STRING Temp;

  Temp +=IntVector;
  return *this = Temp;
}


// ---------------------------------------------------------------------------
// string concatenation
// ---------------------------------------------------------------------------

// concatenate two sources
// NB: assume that 'this' is a new STRING object
void STRING::ConcatCopy(int nSrc1Len, const char *pszSrc1Data,
                        int nSrc2Len, const char *pszSrc2Data)
{
  size_t nNewLen = nSrc1Len + nSrc2Len;
  if ( nNewLen != 0 )
  {
    AllocBuffer(nNewLen);
    wxASSERT(GetStringData()->nDataLength == nNewLen);
    wxASSERT(nNewLen < 65536);
    if (nSrc1Len)
      {
	if (nSrc1Len == sizeof(char))
	  m_pchData[0] = *pszSrc1Data;
	else
	  memcpy(m_pchData, pszSrc1Data, nSrc1Len*sizeof(char));
      }
    if (nSrc2Len)
      {
	if (nSrc2Len == sizeof(char))
	  m_pchData[nSrc1Len] = *pszSrc2Data;
	else
	  memcpy(m_pchData + nSrc1Len, pszSrc2Data, nSrc2Len*sizeof(char));
      }
    wxASSERT(GetStringData()->nDataLength == nNewLen);
    wxASSERT(m_pchData[GetStringData()->nDataLength] == '\0');
  }
}

// add something to this string
void STRING::ConcatSelf(int nSrcLen, const char *pszSrcData)
{
  // concatenating an empty string is a NOP
  if ( nSrcLen != 0 )
    {
      register STRINGData *pData = GetStringData();

      // alloc new buffer if current is too small
      if ( pData->IsShared() || pData->IsConstant() || pData->nDataLength + nSrcLen > pData->nAllocLength )
	{
	  // we have to grow the buffer, use the ConcatCopy routine
	  // (which will allocate memory)
	  STRINGData* pOldData = pData;
	  ConcatCopy(pOldData->nDataLength, m_pchData, nSrcLen, pszSrcData);
	  pOldData->Unlock();
	}
      else if ( nSrcLen == sizeof(char))
	{
	  // Just like below but without need to call memcpy 
	  m_pchData[pData->nDataLength++] = *pszSrcData;
	  m_pchData[pData->nDataLength] = '\0';   // put terminating '\0'
	}
      else // Multiple octets
	{
	  // fast concatenation when buffer big enough
	  memcpy(m_pchData + pData->nDataLength, pszSrcData, nSrcLen*sizeof(char));
	  pData->nDataLength += nSrcLen;

	  // should be enough space
	  wxASSERT( pData->nDataLength <= pData->nAllocLength );

	  m_pchData[pData->nDataLength] = '\0';   // put terminating '\0'
    }
  }
}

/*
 * string may be concatenated with other string, C string or a character
 */

STRING& STRING::operator+=(const STRING& string)
{
  ConcatSelf(string.Len(), string);
  return *this;
}

STRING& STRING::operator+=(const char *psz)
{
  ConcatSelf(Strlen(psz), psz);
  return *this;
}

STRING& STRING::operator+=(char ch)
{
  ConcatSelf(sizeof(char), &ch);
  return *this;
}

STRING& STRING::operator+=(unsigned char ch)
{
  ConcatSelf(sizeof(char), (const char *)&ch);
  return *this;
}

STRING& STRING::operator+=(short val)
{
  return *this += (long)val;
}

STRING& STRING::operator+=(int val)
{
  return *this += (long)val;
}

STRING& STRING::operator+=(long val)
{
  if (IsNumber())
    *this = GetDouble() + val;
  else
    *this += STRING(val);
  return *this;
}

STRING& STRING::operator+=(float val)
{
  return *this += (double)val;
}

STRING& STRING::operator+=(double val)
{
  if (IsNumber())
    *this = GetDouble() + val;
  else
    *this += STRING(val);
  return *this;
}

STRING& STRING::operator+=(const INT2 *IntVector)
{
  if (IntVector)
    {
      for (size_t i=0; IntVector[i] != -1; i++)
	{
	  if (i || Len())
	    *this += '.';
	  Cat( (int)IntVector[i] );
	}
    }
  return *this;
}

STRING& STRING::operator+=(const INT4 *IntVector)
{
  if (IntVector)
    {
      for (size_t i=0; IntVector[i] != -1; i++)
        {
	  if (i || Len())
	    *this += '.';
	  Cat ( (long)IntVector[i] );
        }
    }
  return *this;
}

STRING& STRING::Cat(const STRING& string)
{
  return *this += string;
}

STRING& STRING::Cat(const char *psz)
{
  return *this += psz;
}


STRING& STRING::Cat(char ch)
{
  ConcatSelf(sizeof(char), &ch);
  return *this;
}

STRING& STRING::Cat(unsigned char ch)
{
  ConcatSelf(sizeof(char), (const char *)&ch);
  return *this;
}

STRING& STRING::Cat(int Val)
{
  return Cat( STRING(Val) );
}

STRING& STRING::Cat(long Val)
{
  return Cat( STRING(Val) );
}


STRING& STRING::Cat(unsigned long Val)
{
  return Cat( STRING(Val) );
}


STRING& STRING::Cat(long long Val)
{
  return Cat( STRING(Val) );
}

STRING& STRING::Cat(unsigned long long Val)
{
  return Cat( STRING(Val) );
}


STRING& STRING::Cat(float Val)
{
  return Cat( STRING(Val) );
}

STRING& STRING::Cat(double Val)
{
  return Cat( STRING(Val) );
}

STRING& STRING::HexCat(long long val)
{
  char tmp[26];
#ifdef _WIN32
  sprintf(tmp, "%I64x", val);
#else
  sprintf(tmp, "%llx", val);
#endif
  return Cat (tmp);
}

STRING& STRING::OctCat(long long val)
{
  char tmp[26];
#ifdef _WIN32
  sprintf(tmp, "%I64o", val);
#else
  sprintf(tmp, "%llo", val);
#endif
  return Cat (tmp);
}


/*
 * Same as above but return the result
 */  

STRING& STRING::operator<<(const STRING& string)
{
  ConcatSelf(string.Len(), string);
  return *this;
}

STRING& STRING::operator<<(const char *psz)
{
  ConcatSelf(Strlen(psz), psz);
  return *this;
}

STRING& STRING::operator <<(char ch)
{
  ConcatSelf(sizeof(char), (const char *)&ch);

  return *this;
}

STRING& STRING::operator <<(long LongValue)
{
  Cat(LongValue);
  return *this;
}

STRING& STRING::operator <<(unsigned long LongValue)
{
  Cat(LongValue);
  return *this;
}


STRING& STRING::operator <<(long long LongLongValue)
{
  Cat(LongLongValue);
  return *this;
}

STRING& STRING::operator <<(unsigned long long LongLongValue)
{
  Cat(LongLongValue);
  return *this;
}



STRING& STRING::operator <<(double DoubleValue)
{
  Cat(DoubleValue);
  return *this;
}


STRING& STRING::operator <<(PFILE in)
{
  int ch;
  while ((ch = fgetc(in)) != EOF && ch != '\n')
    Cat((char)ch);
  return *this;  
}


/* 
 * concatenation functions come in 5 flavours:
 *  string + string
 *  char   + string      and      string + char
 *  C str  + string      and      string + C str
 */

STRING operator+(const STRING& string1, const STRING& string2)
{
  STRING s;
  size_t len = string1.GetLength() + string2.GetLength();

  s.ConcatCopy(string1.GetStringData()->nDataLength, string1.m_pchData,
               string2.GetStringData()->nDataLength, string2.m_pchData);

  if (s.GetLength() != len)
    cerr << "PANIC PANIC PANIC len=" << len << " + len = " << s.GetLength() << endl;
  return s;
}

STRING operator+(const STRING& string1, char ch)
{
  STRING s;
  s.ConcatCopy(string1.GetStringData()->nDataLength, string1.m_pchData, 1, &ch);
  return s;
}

STRING operator+(char ch, const STRING& string)
{
  STRING s;
  s.ConcatCopy(1, &ch, string.GetStringData()->nDataLength, string.m_pchData);
  return s;
}

STRING operator+(const STRING& string, const char *psz)
{
  STRING s;
  s.ConcatCopy(string.GetStringData()->nDataLength, string.m_pchData,
               Strlen(psz), psz);
  return s;
}

STRING operator+(const char *psz, const STRING& string)
{
  STRING s;
  s.ConcatCopy(Strlen(psz), psz,
               string.GetStringData()->nDataLength, string.m_pchData);
  return s;
}


// ===========================================================================
// other common string functions
// ===========================================================================

// ---------------------------------------------------------------------------
// simple sub-string extraction
// ---------------------------------------------------------------------------

// helper function: clone the data attached to this string
void STRING::AllocCopy(STRING& dest, int nCopyLen, int nCopyIndex) const
{
  if (nCopyLen > 0)
    {
      int LeftToCopy = Len() - nCopyIndex;
      if (LeftToCopy > 0)
        {
	  size_t want = (LeftToCopy < nCopyLen) ? LeftToCopy : nCopyLen;
	  dest.AllocBeforeWrite(want);
	  memcpy(dest.m_pchData, m_pchData + nCopyIndex, want*sizeof(char));
	  return;
        }
    }
  dest.Clear();
}

// extract string of length nCount starting at nFirst
// default value of nCount is 0 and means "till the end"
STRING STRING::Mid(size_t nFirst, size_t nCount) const
{
  // out-of-bounds requests return sensible things
  if ( nCount == 0 )
    nCount = GetStringData()->nDataLength - nFirst;

  if ( nFirst + nCount > (size_t)GetStringData()->nDataLength )
    nCount = GetStringData()->nDataLength - nFirst;
  if ( nFirst > (size_t)GetStringData()->nDataLength )
    nCount = 0;

  STRING dest;
  AllocCopy(dest, nCount, nFirst);
  return dest;
}



// 
STRING STRING::Substring(const size_t start, const size_t To) const
{
  STRING dest;
  const size_t len = GetStringData()->nDataLength;
  if (len && (start <= To || To == 0))
    {
      const size_t end = ((To > len || To == 0) ? len : To);
      const size_t nCopyLen = end - start;
      AllocCopy(dest, nCopyLen, start);
    }
  return dest;
}


// extract nCount last (rightmost) characters
STRING STRING::Right(size_t nCount) const
{
  if ( nCount > (size_t)GetStringData()->nDataLength )
    nCount = GetStringData()->nDataLength;

  STRING dest;
  AllocCopy(dest, nCount, GetStringData()->nDataLength - nCount);
  return dest;
}

// get all characters after the last occurence of ch
// (returns the whole string if ch not found)
STRING STRING::Right(char ch) const
{
  STRING str;
  int iPos = Find(ch, true);
  if ( iPos == NOT_FOUND )
    str = *this;
  else
    str = c_str() + iPos + 1;

  return str;
}

// extract nCount first (leftmost) characters
STRING STRING::Left(size_t nCount) const
{
  if ( nCount > (size_t)GetStringData()->nDataLength )
    nCount = GetStringData()->nDataLength;

  STRING dest;
  AllocCopy(dest, nCount, 0);
  return dest;
}

// get all characters before the first occurence of ch
// (returns the whole string if ch not found)
STRING STRING::Left(char ch) const
{
  STRING str;
  for ( const char *pc = m_pchData; *pc != '\0' && *pc != ch; pc++ )
    str += *pc;

  return str;
}

/// get all characters before the last occurence of ch
/// (returns empty string if ch not found)
STRING STRING::Before(char ch) const
{
  STRING str;
  int iPos = Find(ch, true);
  if ( iPos != NOT_FOUND && iPos != 0 )
    str = STRING(c_str(), iPos);

  return str;
}

/// get all characters after the first occurence of ch
/// (returns empty string if ch not found)
STRING STRING::After(char ch, bool Last) const
{
  STRING str;
  int iPos = Find(ch, Last);
  if ( iPos != NOT_FOUND )
    str = c_str() + iPos + 1;

  return str;
}

// replace first (or all) occurences of some substring with another one
UINT STRING::Replace(const char *szOld, const char *szNew, bool bReplaceAll)
{
  UINT uiCount = 0;   // count of replacements made

  UINT uiOldLen = Strlen(szOld);

  STRING strTemp;
  const char *pCurrent = m_pchData;
  const char *pSubstr;           
  while ( *pCurrent != '\0' ) {
    pSubstr = ::strstr(pCurrent, szOld);
    if ( pSubstr == NULL ) {
      // strTemp is unused if no replacements were made, so avoid the copy
      if ( uiCount == 0 )
        return 0;

      strTemp += pCurrent;    // copy the rest
      break;                  // exit the loop
    }
    else {
      // take chars before match
      strTemp.ConcatSelf(pSubstr - pCurrent, pCurrent);
      strTemp += szNew;
      pCurrent = pSubstr + uiOldLen;  // restart after match

      uiCount++;

      // stop now?
      if ( !bReplaceAll ) {
        strTemp += pCurrent;    // copy the rest
        break;                  // exit the loop
      }
    }
  }

  // only done if there were replacements, otherwise would have returned above
  *this = strTemp;

  return uiCount;
}

bool STRING::IsAscii() const
{
  for (const unsigned char *s = (const unsigned char*) *this; *s; s++)
    {
      if(!_ib_isascii(*s))
        return(false);
    }
  return(true);
}
  
bool STRING::IsWord() const
{
  for (const unsigned char *s = (const unsigned char*) *this; *s; s++)
    {
      if(!_ib_isalpha(*s))
	return(false);
    }
  return(true);
}

// Plain Word. No numbers, spaces or .
bool STRING::IsPlainWord() const
{
  for (const unsigned char *s = (const unsigned char*) *this; *s; s++)
    {
      if (!_ib_isalpha(*s)) return false;
    }
  return true;
}

//
// Number:
//
//  o   a decimal significand consisting of a sequence of decimal digits
//         optionally containing a decimal-point character, or
//  o   a hexadecimal significand consisting of a ``0X'' or ``0x'' followed
//         by a sequence of hexadecimal digits optionally containing a decimal-
//         point character.
//

bool STRING::IsNumber() const
{
  size_t               dots   = 0;
  size_t               digits = 0;
  const unsigned char *s = (const unsigned char*) *this;
  int                  hex  = 0;

  while (isspace(*s)) s++;

  if (*s == '0' && (s[1] == 'x' || s[1] == 'X'))
    hex = 1;
  else if (*s == '-' || *s == '+')
    s++; // signed number


  for (; *s; s++)
    {
      if(hex ? !_ib_isxdigit(*s) : !_ib_isdigit(*s))
	{
	  if (*s == '.')
	    dots++;
	  else if (!isspace(*s) || s[1]) // Don't be tricked by trailing space!
	    return(false);
	}
       else digits++;
    }

  return (dots < 2 && digits);
}

bool STRING::IsNumberRange() const
{
  const NUMERICALRANGE Range(*this);
  return Range.Ok();
}

#if 0
bool STRING::IsTelephoneNumber() const
{
  const size_t length = Len();
  size_t       saw_digit = 0;
  size_t       dots      = 0;
  size_t       hex       = 0;
  const unsigned char *ptr = (const unsigned char *)*this;

}

#endif

bool STRING::IsDotNumber() const
{
  const size_t length = Len();
  size_t       saw_digit = 0;
  size_t       dots      = 0;
  size_t       hex       = 0;
  const unsigned char *ptr = (const unsigned char *)*this;

  for (size_t i=0; i < length; i++)
    {
      if (ptr[i] == ':')
	{
	  hex++;
	  dots++;
	}
      else if (ptr[i] == '.')
	{
	  if (i > 0 && ptr[i-1] == '.')
	    return(false); // .. is not allowed, probably is a range
	  dots++;
	}
      else if (!_ib_isxdigit(ptr[i]))
	{
	  if (i > 0) return(false);
	}
      else saw_digit++;
    }
  if ((saw_digit > 1) && ((dots > 1) || (hex == dots)))
    return true;
  return false;
}

bool STRING::IsCurrency() const
{
  return MONETARYOBJ (m_pchData).Ok();  
}

bool STRING::IsFilePath() const
{
  const size_t len = Len();
  for (size_t i=0; i < len; i++)
    {
      if (m_pchData[i] == '\\' || m_pchData[i] == '/')
	{
	  return FileExists(m_pchData);
	}
    }
  return false;
}

bool STRING::IsDate() const
{
  if ((Len() > 5 || Len() < 512) && !IsFilePath())
    {
      return SRCH_DATE(*this).Ok();
    }
  return false;
}

bool STRING::IsDateRange() const
{
  if (Len() > 10)
    return DATERANGE(*this).Ok();
  return false;
}


// Look for RECT{ .. } 
bool STRING::IsGeoBoundedBox() const
{
  if (Len() < 512) // Max 512 byte long strings!
    {
       const STRINGINDEX pos =  SearchAny("RECT{");
       return (pos != 0 && SearchAny('}', pos + 4) != 0);
    }
  return false;
}

bool STRING::IsPrint() const
{
  for (const unsigned char *s = (const unsigned char*) *this; *s; s++)
    {
      if(!_ib_isprint(*s))
	return(false);
    }
  return(true);
}


bool STRING::GetBool() const
{
  // Tricky polymorphism guessing
  bool result = false; // Nothing is false...
  if (!IsEmpty())
    { 
      if (IsNumber())
	{
	  result = (GetDouble() != 0.0);
	}
      else
	{
	  char ch = toupper(m_pchData[0]);
	  // True if T[rue], T[ack], Y[es], J[a], S[i], E[in], D[a],.....
	  if (ch == 'T' || ch == 'Y' || ch == 'J' || ch == 'S' || ch == 'E' || ch == 'D')
	    result = true;
	  else if (ch == 'N' || ch == 'A') // No, Nein, Aus etc...
	    result = false;
	  else if (CaseEquals("On") || CaseEquals("Oui")) // O case can be On or Off
	    result = true;
	  // else Off, ...
	}
    }
  return result;
}


STRING STRING::Strip(stripType w) const
{
    STRING s = *this;
    if ( w & leading ) s.Trim(false);
    if ( w & trailing ) s.Trim(true);
    return s;
}

// ---------------------------------------------------------------------------
// case conversion
// ---------------------------------------------------------------------------

bool STRING::IsUpper() const
{
  const size_t len = Len();
 
  for (size_t i=0; i < len; i++)
    {
      if (! _ib_isupper(m_pchData[i] ))
	return false;
    } 
  return true;
} 



STRING& STRING::MakeUpper()
{
  const size_t len = Len();
  unsigned char *p = (unsigned char *)m_pchData;

  AllocBeforeWrite(len);
  for (size_t i=0; i < len; i++)
    {
      m_pchData[i] = _ib_toupper(p[i]);
    }
  return *this;
}

bool STRING::IsLower() const
{
  const size_t len = Len();
 
  for (size_t i=0; i < len; i++)
    {
      if (! _ib_islower(m_pchData[i] ))
	return false;
    } 
  return true;
} 



STRING& STRING::MakeLower()
{
  const size_t len = Len();
  unsigned char *p = (unsigned char *)m_pchData;

  AllocBeforeWrite(len);
  for (size_t i=0; i < len; i++)
    m_pchData[i] = _ib_tolower(p[i]);
  return *this;
}

STRING& STRING::ToAscii()
{
  const size_t len = Len();
  unsigned char *p = (unsigned char *)m_pchData;

  AllocBeforeWrite(len);
  for (size_t i=0; i < len; i++)
    m_pchData[i] = _ib_isascii(p[i]) ? p[i] : _ib_toascii(p[i]);
  return *this;
}

STRING& STRING::ToPrint ()
{
  const size_t len = Len();
  unsigned char *p = (unsigned char *)m_pchData;

  AllocBeforeWrite(len);
  for (size_t i=0; i < len; i++)
    m_pchData[i] = _ib_isprint(p[i]) ? p[i] : ' ';
  return *this;
}


// ---------------------------------------------------------------------------
// trimming and padding
// ---------------------------------------------------------------------------

// trims spaces (in the sense of isspace) from left or right side
STRING& STRING::Trim(bool bFromRight)
{
  if (GetStringData()->nDataLength < 1)
    return *this; // Nothing to do

  CopyBeforeWrite();

  if ( bFromRight )
  {
    // find last non-space character
    char *psz = m_pchData + GetStringData()->nDataLength - 1;
    while ( _ib_isspace(*psz) && (psz >= m_pchData) )
      psz--;

    // truncate at trailing space start
    *++psz = '\0';
    GetStringData()->nDataLength = psz - m_pchData;
  }
  else
  {
    // find first non-space character
    const char *psz = m_pchData;
    while ( _ib_isspace(*psz) )
      psz++;

    // fix up data and length
    int nDataLength = GetStringData()->nDataLength - (psz - m_pchData);
    memmove(m_pchData, psz, (nDataLength + 1)*sizeof(char));
    GetStringData()->nDataLength = nDataLength;
  }

  return *this;
}

STRING& STRING::XMLCommentStrip()
{
  const size_t length = Len();
  size_t       len = 0;

  /* expect at least <!-- --> */
  if (length < 8) return *this;

  UCHR Ch;
  CopyBeforeWrite();

  for (size_t x = 0; x < length; x++)
    {
      Ch =  (UCHR)m_pchData[x];

      if (_ib_isspace(Ch) && (x+1)< length &&  _ib_isspace(m_pchData[x+1]) )
	continue;
      // Strip comments
      if (Ch == '<' && (x+2 < length) && m_pchData[x+1] == '!'  &&
		m_pchData[x+2] == '-' && m_pchData[x+3] == '-')
        {
          /* comment */
          x += 2;
          /* look for --> */
          while (x < length && (m_pchData[x-2] != '-' || m_pchData[x-1] != '-' || m_pchData[x] != '>'))
            x++;
          if (x < length) x++;
        }
      if (x >= length)
        break;
      if (!(_ib_isspace(m_pchData[x]) && len > 0 && _ib_isspace(m_pchData[len-1])))
	{
	  if (_ib_isspace(Ch =  m_pchData[x]))
	    {
	      if (len == 0) continue; // don't start with a space
	      Ch = ' ';
	     }
          m_pchData[len++] = Ch;
	}
    }
   if (len > 0 &&  m_pchData[len-1] == ' ')
    len--;
   GetStringData()->nDataLength = len;
   m_pchData[len] = '\0';
  return *this;
}

// Zap VT100 Esacpe
STRING& STRING::VT100Strip()
{
  const size_t length = Len();
  if (length)
    {
      size_t len = 0;
      UCHR Ch;

      CopyBeforeWrite();
      for (size_t x = 0; x < length; x++)
	{
	  Ch =  (UCHR)m_pchData[x];
	  if (Ch == '\033' &&  m_pchData[x+1] == '[')
	    {
	      size_t y = x + 2;
	      // \E[ --> \E[?m or \Em
	      while (_ib_isdigit( (UCHR)m_pchData[y]) ||  m_pchData[y] == ';')
		y++;
	      if ( m_pchData[y] == 'm')
		{
		  Ch =  (UCHR)m_pchData[x = ++y];
		}
	      // Else was not a sequence for me
	    }
	  if (x != len)
	    m_pchData[len] = Ch;
	  len++;
	}
      GetStringData()->nDataLength = len;
      m_pchData[len] = '\0';
    }
  return *this;
}



// Remove whitespace 
STRING& STRING::removeWhiteSpace()
{
  const size_t length = Len();
  if (length)
    {
      size_t len = 0;

      CopyBeforeWrite();
      for (size_t x = 0; x < length; x++)
        {
          UCHR Ch =  (UCHR)m_pchData[x];
          if (!_ib_isspace(Ch))
	    m_pchData[len++] = Ch;
        }
      GetStringData()->nDataLength = len;
      m_pchData[len] = '\0';
    }
  return *this;
}


// Remove duplicate ' 's
STRING& STRING::Pack ()
{
  const size_t length = Len();
  if (length)
    {
      size_t len = 0;
      bool SawSpace = true;
      UCHR Ch;

      CopyBeforeWrite();
      for (size_t x = 0; x < length; x++)
	{
	  Ch =  (UCHR)m_pchData[x];
	  if (_ib_isspace(Ch))
	    {
	      if (!SawSpace)
		 m_pchData[len++] = ' ';
	      SawSpace = true;
	    }
	  else
	    {
	      SawSpace = false;
	      if (len != x)
		 m_pchData[len] = Ch;
	      len++;
	    }
	}
      if (SawSpace && len)
	len--; // Strip trailing space
      GetStringData()->nDataLength = len;
      m_pchData[len] = '\0';
    }
  return *this;
}

STRING STRING::DeQuote() const
{
  if (m_pchData[0] != '\'' && m_pchData[0] != '"')
    return *this; // Not quoted
  if (m_pchData[0] != m_pchData[Len() - 1])
    return *this; // Not quoted.
  return Mid(1, Len()-1);
}

const char DLE = 0x10;

STRING STRING::PackOf() const
{
  STRING tmp;
  const size_t length = Len();

  for (size_t i = 0; i < length; i++)
    {
      char ch = m_pchData[i];
      if (_ib_isspace((unsigned char)ch))
	{
	  tmp += DLE;
	  tmp += char(ch + 0x80);
	}
      else
	tmp += ch;
   }
   return tmp;
}

STRING STRING::unPackOf() const
{
  STRING tmp;
  const size_t length = Len();

  for (size_t i = 0; i < length; i++)
    {
      if (m_pchData[i] == DLE)
	tmp += char(m_pchData[++i] - 0x80);
      else
	tmp += m_pchData[i];
   }
  return tmp;
}


// Nassib stype Index starts with 1!!
void STRING::SetChr (const STRINGINDEX Index, const UCHR NewChr)
{
  if (Index > 0)
    {
      const int padding = Index - Len();
      if (padding > 0)
	{
	  STRING s(' ', padding);
	  *this += s;
	}
      CopyBeforeWrite();
      m_pchData[Index-1] = NewChr;
    }
}


// adds nCount characters chPad to the string from either side
STRING& STRING::Pad(size_t nCount, char chPad, bool bFromRight)
{
  STRING s(chPad, nCount);

  if ( bFromRight )
    *this += s;
  else
  {
    s += *this;
    *this = s;
  }

  return *this;
}

// truncate the string
STRING& STRING::Truncate(size_t uiLen)
{
  if (uiLen)
   {
      CopyBeforeWrite();
      *(m_pchData + uiLen) = '\0';
      GetStringData()->nDataLength = uiLen;
   }
  else
    Clear();
  return *this;
}

// ---------------------------------------------------------------------------
// finding (return NOT_FOUND if not found and index otherwise)
// ---------------------------------------------------------------------------

STRINGINDEX STRING::FirstWhiteSpace() const
{
  const size_t len = Len();
  for (size_t i = 0; i < len; i++)
    {
       if (isspace(m_pchData[i]))
        return i+1;
    }
  return 0;
}

// Search for XML paths as A\B\C or A/B/C or A|B|C
size_t STRING::Count() const
{
  size_t       count = 0;
  const size_t len = Len();

  for (size_t i=0; i< len; i++)
    {
      char Ch;
      if ((Ch = m_pchData[i]) == '\\' || Ch == '/' || Ch == '|')
	count++;
    }
  return count;
}

// How many Ch are in the string?
size_t STRING::Count(CHR Ch) const
{
  const size_t len = Len();
  size_t       count = 0;

  for (size_t i=0; i< len; i++)
    if (m_pchData[i] == Ch) count++;
  return count;
}

size_t STRING::Count(const char *str) const
{
  const int    len   = (int)Len();
  const size_t slen  = strlen(str);
  size_t       count = 0;

  for(int pos=0; pos < len;) {
    if ((pos = Find(str, pos)) == NOT_FOUND)
      break;
    pos += slen;
    count++;
  } 
  return count;
}

size_t STRING::Count(const STRING& str) const
{
  const int len      = (int)Len();
  size_t       count = 0;
  const size_t slen  = str.Len();

  for(int pos=0; pos < len;) {
    if ((pos = Find(str, pos)) == NOT_FOUND)
      break;
    pos += slen;
    count++;
  }
  return count;
}

// find a character
int STRING::Find(char ch, bool bFromEnd) const
{
  const size_t len = Len();
  if (bFromEnd)
    {
      for (int i=len-1; i >=0; i--)
	{
	  if (ch == m_pchData[i])
	    return i;
	}
    }
  for (size_t i = 0; i < len; i++)
    {
       if (ch == m_pchData[i])
	return i;
    }
  return NOT_FOUND;
}

const char *strchr(const STRING& str, int ch)
{
  int i = str.Find((char)ch, false);
  if (i == NOT_FOUND)
    return NULL;
  return str.GetData((size_t)i);
}

const char *strrchr(const STRING& str, int ch)
{
  int i = str.Find((char)ch, true);
  if (i == NOT_FOUND)
    return NULL;
  return str.GetData((size_t)i);
}

const char *strstr(const STRING& str, const STRING& sub)
{
  int i = str.Find(sub);
  if (i == NOT_FOUND)
    return NULL;
  return str.GetData((size_t)i);
}

const char *strstr(const STRING& str, const char *pszSub)
{
  int i = str.Find(pszSub);
  if (i == NOT_FOUND)
    return NULL;
  return str.GetData((size_t)i);
}

const char *strstr(const char *str, const STRING& sub)
{
  return strstr(str, sub.GetData());
}


// find a sub-string
int STRING::Find(const char *pszSub, size_t start) const
{
  const size_t length = Len();
  const size_t pLen = Strlen(pszSub);
  if (pLen > 0 && pLen <= length)
    {
      const size_t bound = length - pLen;
      for (size_t i = start; i <= bound; i++)
        {
          if ((char)m_pchData[i] == *pszSub)
	    {
	      if (memcmp(&m_pchData[i], pszSub, pLen) == 0)
		return i;
	    }
        }
    }
  return NOT_FOUND;
}

int STRING::Find(const STRING& Sub, size_t start) const
{
  const size_t length = Len();
  const size_t pLen = Sub.Len();
  if (pLen > 0 && pLen <= length)
    {
      const size_t bound = length - pLen;
      for (size_t i = start; i <= bound; i++)
	{
	  if (m_pchData[i] == *(Sub.m_pchData))
	    {
	      if (memcmp(&m_pchData[i], Sub.m_pchData, pLen) == 0)
		return i;
	    }
	}
    }
  return NOT_FOUND;
}


// Search at most N characters of the STRING
STRINGINDEX STRING::SearchN(const STRING& OtherString, const STRINGINDEX Count, const STRINGINDEX Start) const
{
  const STRINGINDEX olength = OtherString.Len();
  if (olength == 1)
    return SearchN(OtherString[0], Count, Start);

  const STRINGINDEX length = Len(); // Limit
  if (olength > 0 && olength <= length)
    {
      const char *otherptr = OtherString.m_pchData;
      const STRINGINDEX index = Start > 0 ? Start-1 : 0;
      // Limit to Count characters..
      STRINGINDEX bound = length - olength;
      if (Count > 0 && bound - index > Count)
	bound = Count - index;
      for (STRINGINDEX x = index; x <= bound; x++)
	{
	  if (memcmp(&m_pchData[x], otherptr, olength) == 0)
	    return (x+1);
	}
    }
  return 0; // NOT FOUND
}


STRINGINDEX STRING::SearchN(const CHR* CString, const STRINGINDEX Count, const STRINGINDEX Start) const
{
  const STRINGINDEX olength = Strlen(CString);
  if (olength == 1)
    return SearchN(*CString, Count, Start);

  const STRINGINDEX length = Len();
  if (olength > 0 && olength <= length)
    {
      const STRINGINDEX index = Start > 0 ? Start-1 : 0;
      STRINGINDEX bound = length - olength;

      if (Count > 0 && bound - index > Count)
	bound = Count - index;
      for (STRINGINDEX x = Start-1; x <= bound; x++)
        {
	  if (m_pchData[x] == *CString && memcmp(&m_pchData[x], CString, olength) == 0)
	    return (x+1);
        }
    }
  return 0; // NOT FOUND
}

STRINGINDEX STRING::SearchN(const UCHR Character, const STRINGINDEX Count, const STRINGINDEX Start) const
{
  register STRINGINDEX length = Len();
  register STRINGINDEX x = Start > 0 ? Start-1 : 0;
  if (Count > 0 && length - x > Count)
     length = Count + x;
  while (x < length)
    {
      if (m_pchData[x++] == Character)
	return x;
    }
  return 0; // NOTFOUND

}


// 1 is first element..
STRINGINDEX STRING::Search (const STRING& OtherString, const STRINGINDEX Start) const
{
  const STRINGINDEX olength = OtherString.Len();
  if (olength == 1)
    return Search(OtherString[0], Start);

  const STRINGINDEX length = Len();
  if (olength > 0 && olength <= length)
    {
      const char *otherptr = OtherString.m_pchData;
      const STRINGINDEX bound = length - olength;

      for (STRINGINDEX i = Start-1; i <= bound; i++)
	{
	  if (memcmp(&m_pchData[i], otherptr, olength) == 0)
	    return (i+1);
	}
    }
  return 0; // NOT FOUND

}

STRINGINDEX STRING::Search (const CHR *CString, const STRINGINDEX Start) const
{
  const STRINGINDEX length = Len();
  STRINGINDEX SLen = CString? strlen (CString) : 0;
  if (SLen > 0 && SLen <= length)
    {
      STRINGINDEX bound = length - SLen;
      for (STRINGINDEX x = Start - 1; x <= bound; x++)
	{
	  if (m_pchData[x] == *CString && memcmp(&m_pchData[x], CString, SLen) == 0)
	    return (x + 1);
	}
    }
  return 0;

}

STRINGINDEX STRING::Search (const CHR Character, const STRINGINDEX Start) const
{
  const STRINGINDEX length = Len();
  register STRINGINDEX x = Start ? Start - 1 : 0;
  while (x < length)
    {
      if (m_pchData[x++] == Character)
	return x;
    }
  return 0; // NOTFOUND

}

STRINGINDEX STRING::Search (const UCHR Character, const STRINGINDEX Start) const
{
  return Search((CHR)Character, Start);
}



#if 0
// Search to Max length
//
//

STRINGINDEX STRING::Search (const CHR Character, const STRINGINDEX Start, STRINGINDEX Length) const
{
  const STRINGINDEX length = Length ? min(Len(), Length) : Len() ;
  register STRINGINDEX x = Start ? Start - 1 : 0;
  while (x < length)
    {
      if (m_pchData[x++] == Character)
        return x;
    }
  return 0; // NOTFOUND

}

STRINGINDEX STRING::SearchAny (const STRING& OtherString, const STRINGINDEX Start, const STRINGINDEX Length) const
{
  const STRINGINDEX olength = OtherString.Len();
  if (olength == 1)
    return Search(OtherString[0], Start, Length);

  const STRINGINDEX length = Len();
  if (olength > 0 && olength <= length)
    {
      const char *otherptr = OtherString.m_pchData;
      const STRINGINDEX  limit = length - olength;
      const STRINGINDEX bound = min(limit, Length);
      for (STRINGINDEX x = Start-1; x <= bound; x++)
        {
          if (Strnicmp(&m_pchData[x], otherptr, olength) == 0)
            return (x+1);
        }
    }
  return 0; // NOT FOUND
}
#endif


STRINGINDEX STRING::SearchAny (const STRING& OtherString, const STRINGINDEX Start) const
{
  const STRINGINDEX olength = OtherString.Len();
  if (olength == 1)
    return Search(OtherString[0], Start);

  const STRINGINDEX length = Len();
  if (olength > 0 && olength <= length)
    {
      const char *otherptr = OtherString.m_pchData;
      const STRINGINDEX bound = length - olength;
      for (STRINGINDEX x = Start-1; x <= bound; x++)
        {
          if (Strnicmp(&m_pchData[x], otherptr, olength) == 0)
            return (x+1);
        }
    }
  return 0; // NOT FOUND
}

STRINGINDEX STRING::SearchAny (const CHR *CString, const STRINGINDEX Start) const
{
 const STRINGINDEX length = Len();
  STRINGINDEX SLen = CString? strlen (CString) : 0;
  if (SLen > 0 && SLen <= length)
    {
      STRINGINDEX bound = length - SLen;
      for (STRINGINDEX x = Start - 1; x <= bound; x++)
        {
          if (Strnicmp(&m_pchData[x], CString, SLen) == 0)
            return (x + 1);
        }
    }
  return 0;
}

STRINGINDEX STRING::SearchAny (const CHR Character, const STRINGINDEX Start) const
{
  return SearchAny((UCHR)Character, Start);
}

STRINGINDEX STRING::SearchAny (const UCHR Character, const STRINGINDEX Start) const
{
  const STRINGINDEX length = Len();
  const UCHR ch = _ib_tolower(Character);
  for (STRINGINDEX x = Start ? Start - 1 : 0; x < length; x++)
    {
      if (_ib_tolower((unsigned char)m_pchData[x]) == ch)
        {
          return x+1;
        }
    }
  return 0; // NOTFOUND
}


STRINGINDEX STRING::SearchReverse (const STRING& OtherString) const
{
  const STRINGINDEX length = Len();
  const STRINGINDEX olength = OtherString.Len();
  if (olength > 0 && olength <= length)
    {
      const char *otherptr = OtherString.m_pchData;
      STRINGINDEX x = length - olength;
      do {
        if (memcmp(otherptr, &m_pchData[x], olength) == 0)
          return (x + 1);
      } while (x-- > 0);
    }
  return 0;
}

STRINGINDEX STRING::SearchReverse (const CHR *CString) const
{
  const STRINGINDEX length = Len();
  const STRINGINDEX olength = Strlen(CString);
  if (olength > 0 && olength <= length)
    {
      STRINGINDEX x = length - olength;
      do {
        if (memcmp(CString, &m_pchData[x], olength) == 0)
          return (x + 1);
      } while (x-- > 0);
    }
  return 0;
}

STRINGINDEX STRING::SearchReverse (const CHR Character) const
{
  int found = Find((char)Character, true);
  if (found >= 0)
    return found+1;
  return 0; // NOTFOUND
}

STRINGINDEX STRING::SearchReverse (const UCHR Character) const
{
  int found = Find((char)Character, true);
  if (found >= 0)
    return found+1;
  return 0; // NOTFOUND
}

// Returns position of Wild character..
INT STRING::IsWild () const
{
  const size_t length = Len();
  for (size_t i=0; i<length; i++)
    {
      int Ch;
      switch ((Ch = m_pchData[i]))
	{
	  case '?': case '*': case '[': case '{':  /* } */
	    return i+1;
	  case '\\': if ((Ch = m_pchData[++i]) == '\\' || Ch == '?' || Ch == '*' || Ch == '[' || Ch == '{') /* } */
	    i++;
	}        /* switch() */
    }          /* while() */
  return 0;
}

// No Wild characters..
bool STRING::IsPlain () const
{
  const size_t length = Len();
  for (size_t i=0; i<length; i++)
    {
      switch (m_pchData[i])
        {
          case '?': case '*': case '[': case '{':  /* } */
            return false; 
        }        /* switch() */
    }          /* while() */
  return true;
}


bool STRING::MatchWild(const STRING& OtherString) const
{
  return MatchWild(OtherString.m_pchData);
}


bool STRING::Glob(const STRING& OtherString) const
{
  return ::Glob((const UCHR *)m_pchData, (const UCHR *)OtherString.m_pchData);
}

bool STRING::Glob(const CHR *CString) const
{
  return ::Glob((const UCHR *)m_pchData, (const UCHR *)CString);
}

bool STRING::MatchWild(const CHR *CString) const
{
  bool     match = false;
  char           *argv[256]; // At most 256 patterns (hardwired)
#ifdef __GNUC__
  const size_t    max_pat = Len()+1;
  char            pat[max_pat]; 
#else
  const size_t    max_pat = BUFSIZ;
  char            pat[BUFSIZ];  // Pattern at most 1024 bytes (hardwired)
#endif
  int argc = 0;

  strncpy(pat, m_pchData, max_pat);
  pat[max_pat-1] = '\0';

#if 0
  while ( argv[argc++] = strsep(&pat, "|")) != NULL)
    /* loop */;
#else
  char          *ctxt;

  argv[argc++] = strtok_r(pat, "|", &ctxt);
  while ((argv[argc++] = strtok_r(NULL, "|", &ctxt)) != NULL && argc < 256)
    /* loop */;
#endif
  argv[argc] = NULL;


  for(argc=0; argv[argc] && !match; argc++)
    match = ::Glob((const UCHR *)argv[argc], (const UCHR *)CString);

  return match;
}


// ---------------------------------------------------------------------------
// formatted output
// ---------------------------------------------------------------------------
int STRING::Printf(const char *pszFormat, ...)
{
  va_list argptr;
  va_start(argptr, pszFormat);

  int iLen = PrintfV(pszFormat, argptr);

  va_end(argptr);

  return iLen;
}

int STRING::PrintfV(const char* pszFormat, va_list argptr)
{
  const size_t maxL = strlen(pszFormat) + 3*BUFSIZ;
  AllocBeforeWrite(maxL);
#ifdef NO_VSPRINTF
  return GetStringData()->nDataLength = vsprintf(m_pchData, pszFormat, argptr);
#else
  return GetStringData()->nDataLength = vsnprintf(m_pchData, maxL, pszFormat, argptr);
#endif
}


// ---------------------------------------------------------------------------
// More I/O
// ---------------------------------------------------------------------------
INT STRING::Unlink () const { return UnlinkFile(m_pchData); }

STRINGINDEX STRING::Fread(const STRING& Filename, size_t Len, off_t Offset)
{
  STRINGINDEX res = 0;
  FILE *fp = Filename.fopen("rb");
  if (fp)
    {
      // Freshly opened so always at Offet==0
      res = Offset > 0 ? Fread(fp, Len, Offset) : Fread(fp, Len);
      fclose(fp);
    }
  return res;
}

STRINGINDEX STRING::Fread(FILE *fp, size_t Len, off_t Offset)
{
  if (-1 == fseek (fp, Offset, SEEK_SET))
    {
      Clear(); // Error
      if (Offset) message_log (LOG_DEBUG, "Can't Fread %lu bytes: Seek to %ld in stream failed", (long)Len, Offset);
      return 0;
    }
  return Fread(fp, Len);
}

STRINGINDEX STRING::Fread(FILE *fp, size_t Len)
{
  STRINGINDEX length = 0;
  if (fp && Len > 0)
    {
      size_t res = 1;
      // See fread(3s)

      AllocBeforeWrite(HEADROOM(Len, 64));
      do {
	res = fread (&m_pchData[length], sizeof(char), Len, fp);
	length += res;
	Len -= res;
      } while (res && Len > 0);
      m_pchData[length] = '\0';
      // Added June 2006 to be O end
      for (STRINGINDEX i=0; i< length ; i++)
	{ if (m_pchData[i] == '\0') length = i; }
      // End
      GetStringData()->nDataLength = length;
    }
  else
   {
      Clear();
   }
  return length;
}

bool STRING::FGet (PFILE FilePointer)
{
  int ch;
  Clear();
  while ((ch = getc(FilePointer)) != EOF && ch != '\n')
    {
      if (ch == '\r')
	{
	  ch = getc(FilePointer);
	  if (ch != '\n')
	    ungetc(ch, FilePointer);
	  break;
	}
      else
	Cat ((CHR)ch);
    }
  return !IsEmpty();
}


bool STRING::FGet (PFILE FilePointer, const STRINGINDEX MaxCharacters)
{
  size_t length  = 0;
  bool Ok = false;

  AllocBeforeWrite(MaxCharacters+2); // Make buffer large enough

  if (FilePointer && fgets (m_pchData, MaxCharacters + 1, FilePointer))
    {
      length = strlen (m_pchData) - 1;
      while (length > 0 && (m_pchData[length] == '\n' || m_pchData[length] == '\r') )
	length--;
      length++;
      Ok = true;
    }

  m_pchData[length] = '\0';
  GetStringData()->nDataLength = length;
  return Ok;
}


bool STRING::FGetMultiLine(PFILE FilePointer, const STRINGINDEX MaxCharacters)
{
  STRING Temp;
  STRINGINDEX Max = MaxCharacters;
  bool res = (FilePointer != NULL); // a 0 length string is TRUE

  Clear();
  while (Max && (res = Temp.FGet (FilePointer, Max)) == true)
    {
      STRINGINDEX l = Temp.Len();
      if (Temp.GetChr(l) == '\\' && Temp.GetChr(l-1) != '\\')
	{
	  Temp.m_pchData[l-1] = '\0'; // Zap the slash (\) 
	  Cat (Temp);
	  Max -= (l-1);
	  if (Max == 0)
	    return false; // Have a continue but no more room!
	}
      else
	break; // We are done.. line not continued
    }
  return res;
}

void STRING::Print() const
{
  cout.write(m_pchData, Len());
}

void STRING::Print(STRINGINDEX Conline) const
{
  if (Conline <= 2)
    {
      Print();
      return;
    }
  const size_t length = Len();
  size_t       i = 0, bytes;
  do {
    if (i)
      cout << "\\" << endl;
    if ((bytes=Conline)+i > length)
      {
	bytes = length - i;
      }
    else
      {
	while (bytes > Conline/2 && !_ib_isspace(m_pchData[i+bytes-1]))
	  bytes--;
      }

    cout.write(&m_pchData[i], bytes);
    i += bytes;
  } while (i < length);
}

void STRING::Print(PFILE FilePointer) const
{
  const size_t length = Len();
  for (size_t x=0; x<length; x++)
    fputc(m_pchData[x], FilePointer);
}

void STRING::Print(PFILE FilePointer, STRINGINDEX Conline) const
{
  if (Conline <= 2)
    {
      Print(FilePointer);
      return;
    }
  const size_t length = Len();
  size_t       i = 0, bytes;
  do {
    if (i)
      fputs("\\\n", FilePointer);
    if ((bytes=Conline)+i > length)
      {
        bytes = length - i;
      }
    else
      {
	while (bytes > Conline/2 && !_ib_isspace(m_pchData[i+bytes-1]))
	  bytes--;
      }
    for (size_t x=0; x < bytes; x++)
      fputc(m_pchData[i+x], FilePointer);
    i += bytes;
  } while (i < length);

}


// ---------------------------------------------------------------------------
// standard C++ library string functions
// ---------------------------------------------------------------------------
#ifdef  STD_STRING_COMPATIBILITY

STRING& STRING::insert(size_t nPos, const STRING& str)
{
  wxASSERT( nPos <= Len() );

  STRING strTmp;
  char *pc = strTmp.GetWriteBuf(Len() + str.Len() + 1);
  strncpy(pc, c_str(), nPos);
  strcpy(pc + nPos, str);
  strcpy(pc + nPos + str.Len(), c_str() + nPos);
  *this = strTmp;
    
  return *this; 
}

size_t STRING::find(const STRING& str, size_t nStart) const
{
  wxASSERT( nStart <= Len() );
  return Find(str, nStart);
}

size_t STRING::find(const char* sz, size_t nStart, size_t n) const
{
  return find(STRING(sz, n == npos ? 0 : n), nStart);
}
        
size_t STRING::find(char ch, size_t nStart) const
{
  const size_t len = Len();
  for (size_t i = nStart; i < len; i++)
    {
       if (ch == m_pchData[i])
        return i;
    }
  return (size_t)NOT_FOUND;
}

size_t STRING::rfind(const STRING& str, size_t nStart) const
{
  wxASSERT( nStart <= Len() );

  // # could be quicker than that
  const char *p = c_str() + (nStart == npos ? Len() : nStart);
  while ( p >= c_str() + str.Len() ) {
    if ( strncmp(p - str.Len(), str, str.Len()) == 0 )
      return p - str.Len() - c_str();
    p--;
  }
  
  return npos;
}
        
size_t STRING::rfind(const char* sz, size_t nStart, size_t n) const
{
  return rfind(STRING(sz, n == npos ? 0 : n), nStart);
}

size_t STRING::rfind(char ch, size_t nStart) const
{
  wxASSERT( nStart <= Len() );

  const char *p = ::strrchr(c_str() + nStart, ch);
  
  return p == NULL ? npos : p - c_str();
}

STRING STRING::substr(size_t nStart, size_t nLen) const
{
  // npos means 'take all'
  if ( nLen == npos )
    nLen = 0;

  wxASSERT( nStart + nLen <= Len() );

  return STRING(c_str() + nStart, nLen == npos ? 0 : nLen);
}

STRING& STRING::erase(size_t nStart, size_t nLen)
{
  STRING strTmp(c_str(), nStart);
  if ( nLen != npos ) {
    wxASSERT( nStart + nLen <= Len() );

    strTmp.append(c_str() + nStart + nLen);
  }

  *this = strTmp;
  return *this;
}

STRING& STRING::replace(size_t nStart, size_t nLen, const char *sz)
{
  wxASSERT( nStart + nLen <= Strlen(sz) );

  STRING strTmp;
  if ( nStart != 0 )
    strTmp.append(c_str(), nStart);
  strTmp += sz;
  strTmp.append(c_str() + nStart + nLen);
  
  *this = strTmp;
  return *this;
}

STRING& STRING::replace(size_t nStart, size_t nLen, size_t nCount, char ch)
{
  return replace(nStart, nLen, STRING(ch, nCount));
}

STRING& STRING::replace(size_t nStart, size_t nLen, 
                        const STRING& str, size_t nStart2, size_t nLen2)
{
  return replace(nStart, nLen, str.substr(nStart2, nLen2));
}

STRING& STRING::replace(size_t nStart, size_t nLen, 
                        const char* sz, size_t nCount)
{
  return replace(nStart, nLen, STRING(sz, nCount));
}

#endif  //std::string compatibility

static STRING _nulstring;
const  STRING& NulString = _nulstring;


// ============================================================================
// ArrayString
// ============================================================================

// size increment = max(50% of current size, ARRAY_MAXSIZE_INCREMENT)
#define   ARRAY_MAXSIZE_INCREMENT       4096
#ifndef   ARRAY_DEFAULT_INITIAL_SIZE    // also defined in dynarray.h
  #define   ARRAY_DEFAULT_INITIAL_SIZE    (16)
#endif

#define   STRING(p)   ((STRING *)(&(p)))

// ctor
ArraySTRING::ArraySTRING()
{
  m_nSize  =
  m_nCount = 0;
  m_pItems = NULL;
}

ArraySTRING::ArraySTRING(size_t Size)
{
  m_nCount = 0;
  if ( m_nSize != 0 )
    m_pItems = new char *[m_nSize];
  else
    m_pItems = NULL;
  m_nSize  = Size;
}


// copy ctor
ArraySTRING::ArraySTRING(const ArraySTRING& src)
{
  m_nSize  = src.m_nSize;
  m_nCount = src.m_nCount;

  if ( m_nSize != 0 )
    m_pItems = new char *[m_nSize];
  else
    m_pItems = NULL;

  if ( m_nCount != 0 )
    memcpy(m_pItems, src.m_pItems, m_nCount*sizeof(char *));
}


ArraySTRING::ArraySTRING(const STRLIST& List)
{
  m_nSize= List.GetTotalEntries();
  m_nCount = 0;
  if ( m_nSize)
    {
      m_pItems = new char *[m_nSize];
      for (const STRLIST *p = List.Next(); p != &List; p = p->Next())
	Add(p->Value());
    }
  else
    m_pItems = NULL;
}

// copy operator
ArraySTRING& ArraySTRING::operator=(const ArraySTRING& src)
{
  DELETEA(m_pItems);

  m_nSize  = src.m_nSize;
  m_nCount = src.m_nCount;

  if ( m_nSize != 0 )
    m_pItems = new char *[m_nSize];
  else
    m_pItems = NULL;

  if ( m_nCount != 0 )
    memcpy(m_pItems, src.m_pItems, m_nCount*sizeof(char *));

  return *this;
}

// grow the array
void ArraySTRING::Grow()
{
  // only do it if no more place
  if( m_nCount == m_nSize ) {
    if( m_nSize == 0 ) {
      // was empty, alloc some memory
      m_nSize = ARRAY_DEFAULT_INITIAL_SIZE;
      m_pItems = new char *[m_nSize];
    }
    else {
      // add 50% but not too much
      size_t nIncrement = m_nSize >> 1;
      if ( nIncrement > ARRAY_MAXSIZE_INCREMENT )
        nIncrement = ARRAY_MAXSIZE_INCREMENT;
      m_nSize += nIncrement;
      char **pNew = new char *[m_nSize];

      // copy data to new location
      memcpy(pNew, m_pItems, m_nCount*sizeof(char *));

      // delete old memory (but do not release the strings!)
      DELETEA(m_pItems);

      m_pItems = pNew;
    }
  }
}

void ArraySTRING::Free()
{
  for ( size_t n = 0; n < m_nCount; n++ ) {
    STRING(m_pItems[n])->GetStringData()->Unlock();
  }
}

// deletes all the strings from the list
void ArraySTRING::Empty()
{
  Free();

  m_nCount = 0;
}

// as Empty, but also frees memory
void ArraySTRING::Clear()
{
  Free();

  m_nSize  = 
  m_nCount = 0;

  DELETEA(m_pItems);
  m_pItems = NULL;
}

// dtor
ArraySTRING::~ArraySTRING()
{
  Free();

  DELETEA(m_pItems);
}

// pre-allocates memory (frees the previous data!)
void ArraySTRING::Alloc(size_t nSize)
{
  wxASSERT( nSize > 0 );

  // only if old buffer was not big enough
  if ( nSize > m_nSize ) {
    Free();
    DELETEA(m_pItems);
    m_pItems = new char *[nSize];
    m_nSize  = nSize;
  }

  m_nCount = 0;
}

// searches the array for an item (forward or backwards)
int ArraySTRING::Index(const char *sz, bool bCase, bool bFromEnd) const
{
  if ( bFromEnd ) {
    if ( m_nCount > 0 ) {
      UINT ui = m_nCount;
      do {
        if ( STRING(m_pItems[--ui])->IsSameAs(sz, bCase) )
          return ui;
      }
      while ( ui != 0 );
    }
  }
  else {
    for( UINT ui = 0; ui < m_nCount; ui++ ) {
      if( STRING(m_pItems[ui])->IsSameAs(sz, bCase) )
        return ui;
    }
  }

  return NOT_FOUND;
}

// add item at the end
void ArraySTRING::Add(const STRING& src)
{
  Grow();

  // the string data must not be deleted!
  src.GetStringData()->Lock();
  m_pItems[m_nCount++] = (char *)src.c_str();
}



void ArraySTRING::Replace(const STRING& src, size_t nIndex)
{
  if (nIndex == m_nCount)
    {
      Add(src);
    }
  else
    {
      Item(nIndex).GetStringData()->Unlock();
      src.GetStringData()->Lock();
      m_pItems[nIndex] = (char *)src.c_str();
    }
}

// add item at the given position
void ArraySTRING::Insert(const STRING& src, UINT nIndex)
{
//  wxCHECK( nIndex <= m_nCount );

  Grow();

  memmove(&m_pItems[nIndex + 1], &m_pItems[nIndex], 
          (m_nCount - nIndex)*sizeof(char *));

  src.GetStringData()->Lock();
  m_pItems[nIndex] = (char *)src.c_str();

  m_nCount++;
}

// removes item from array (by index)
void ArraySTRING::Remove(size_t nIndex)
{
//  wxCHECK( nIndex <= m_nCount );

  // release our lock
  Item(nIndex).GetStringData()->Unlock();

  memmove(&m_pItems[nIndex], &m_pItems[nIndex + 1], 
          (m_nCount - nIndex - 1)*sizeof(char *));
  m_nCount--;
}

// removes item from array (by value)
void ArraySTRING::Remove(const char *sz)
{
  int iIndex = Index(sz);

//  wxCHECK( iIndex != NOT_FOUND );

  Remove((size_t)iIndex);
}

#if 0

int _strcmp(register const char *s1, register const char *s2)
{
  while (*s1 && *s2 && (
	*s1 == *s2 || ((*s1 == '\\' || *s1 == '/') && (*s2 == '\\' || *s2 == '/')))) {
    do { s2++; } while (*s2 == '\\' || *s2 == '/');
    do { s1++; } while (*s1 == '\\' || *s1 == '/');
  }
  return(*s1 - *s2);
}


#endif


// Fields are case independent and \\ and / are the same
// multiple / also don't make a difference.
// Trailing / is significant
int STRING::FieldCmp(const char *s1, const char *s2) const
{
  // Handle NULL and same pointer
  if (s1 == s2)   return  0;
  if (s1 == NULL) return -1;
  if (s2 == NULL) return  1;

//cerr << s1 << " versus " << s2;

  // Find first difference
  while (*s1 && ((toupper(*s1) == toupper(*s2)) ||
	( (*s1 == '/' || *s1 == '\\') &&
	  (*s2 == '/' || *s2 == '\\'))) )
    {
      if (*s1 == '/' || *s1 == '\\')
        {
          // Note:  x//y//z matches x/y/z
	  // but play/ is not the same as play
	  while ((*s1 == '\\' || *s1 == '/') && *(s1+1) != '\0') s1++;
	  while ((*s2 == '\\' || *s2 == '/') && *(s2+1) != '\0') s2++;
	  if (*s1 == '/' || *s2 == '/') break;
	  if (*s1 == '\\'|| *s2 == '\\') break;
        }
      else   
        s1++, s2++;
    }
//cerr << " == " << (int)(*s1 - *s2) << endl;
  return *s1 - *s2;
}


/*--  Sort Functions -*/
static int fCmp (const void *x, const void *y)
{
  return strcmp( *((const char **) x), *((const char **) y));
}

static int fCmpReverse (const void *x, const void *y)
{
  return strcmp( *((const char **) y), *((const char **) x));
}

static int fCmpNoCase (const void *x, const void *y)
{
  return Stricmp( *((const char **) x), *((const char **) y));
}

static int fCmpNoCaseReverse (const void *x, const void *y)
{
  return Stricmp( *((const char **) y), *((const char **) x));
}

// sort array elements using passed comparaison function
void ArraySTRING::Sort(bool bCase, bool bReverse)
{
  Sort(0, bCase, bReverse);
}

void ArraySTRING::Sort(size_t offset, bool bCase, bool bReverse)
{
  int (*Compare) (const void *, const void *);

  if (bCase && bReverse) Compare = fCmpNoCaseReverse;
  else if (bCase)        Compare = fCmpNoCase;
  else if (bReverse)     Compare = fCmpReverse;
  else                   Compare = fCmp; 

  if ( m_nCount > (1 + offset) )
    QSORT(m_pItems + offset, m_nCount - offset, sizeof(char *), Compare);
}


// sort array elements using passed comparaison function
void ArraySTRING::UniqueSort(bool bCase, bool bReverse)
{
  UniqueSort(0, bCase, bReverse);
}

void ArraySTRING::UniqueSort(size_t offset, bool bCase, bool bReverse)
{
  int (*Compare) (const void *, const void *);

  if (bCase && bReverse) Compare = fCmpNoCaseReverse;
  else if (bCase)        Compare = fCmpNoCase;
  else if (bReverse)     Compare = fCmpReverse;
  else                   Compare = fCmp;

  if ( m_nCount > (1 + offset) )
    {
      QSORT(m_pItems + offset, m_nCount - offset, sizeof(char *), Compare);
      // Now its sorted BUT we need to remove dups..
      const char *ptr1, *ptr2 = m_pItems[offset];
      for (size_t i = offset + 1; i <  m_nCount - offset ; i++)
	{
	  ptr2 = m_pItems[i];
	  if (Compare(&ptr1, &ptr2) == 0) Remove(i);
	  else                            ptr1 = ptr2;
	}
    }
}


// Common stuff...
int Stricmp(const char *psz1, const char *psz2)
{
  return StrCaseCmp(psz1, psz2);
}
int Strnicmp(const char *psz1, const char *psz2, size_t len)
{
  return StrNCaseCmp(psz1, psz2, len);
}
bool Read(STRING *s, FILE *fp) { return s->Read(fp); }
void Write(const STRING& s, FILE *fp) { s.Write(fp); }


UINT8 CRC64(const STRING& Contents)
{
  return CRC64(Contents.c_str(), Contents.Length());
}


#if MAIN_STUB
STRING test(const STRING& me)
{
  STRING Temp = me;
  return Temp;
}


main()
{
  STRING x, z;
  {
    STRING y = test("Passing String");
    x = y;
    y = "Setting String";
    z = y;
  }
  z.Cat('1');
  cerr << x << endl;
  cerr << z << endl;

}
#endif

