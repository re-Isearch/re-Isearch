/* Copyright (c) 2020-22 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
#pragma ident  "@(#)index.cxx"

#define PHONETIC_SKIP_INITIALS 1

/*-@@@
File:   	index.cxx
Description:	Class INDEX
@@@*/
/*
TODO:

  Write the .inx files for a term using gpcomp sort order. This
  saves having to sort for a term.....
*/
#define BOUNDARY '#'

#define USE_PLATFORM_IND_INX 0 /* Indexes are always same order */

#define OPTIM 0 /* NOT DONE YET */

#ifdef O_BUILD_IB64
# define VERY_COMMON_TERM                   500000
# define DEFAULT_TOO_MANY_RECORDS_THRESHOLD  50000
#else
# define VERY_COMMON_TERM                   100000
# define DEFAULT_TOO_MANY_RECORDS_THRESHOLD  10000
#endif

extern int _ib_defaultMaxCPU_ticks;
extern int _ib_defaultMaxQueryCPU_ticks;


#define XXX_DEBUG 1
#undef XXX_DEBUG /* NO DEBUG */

#define PositionOf(_x) (((_x) * sizeof(GPTYPE)) + 2 + OPTIM)


/*
// $Log: index.cxx,v $
// Revision 1.4  2007/10/14 12:01:14  edz
// Fixed sub-index literal bug
//
// Revision 1.3  2007/06/20 08:08:31  edz
// sync
//
// Revision 1.2  2007/06/19 06:24:03  edz
// sync
//
// Revision 1.1  2007/05/15 15:47:23  edz
// Initial revision
//
// Revision 1.13  1996/05/10  13:57:12  edz
// Added some dict code.
//
*/
#define EDZ_MAP 1

/* Add to /etc/magic:
# Iindex
0	byte	0x69	Isearch Index
>1	byte	0x00	- version 1
>1	byte	0x01	- version 1 (hyphenation)
>1	byte	0x02	- version 2
>1	byte	0x03	- version 2 (hyphenation)
>1      byte    0x04    - version 3
>1      byte    0x05    - version 3 (hyphenation)
*/
/* Idea: Add to .inx
//  1) for each gp also a single char
//    first_char mapped to lowercase
//    this will save us quite a few indirect buffer
//    lookups..
// Or (maybe better idea)
//  1) Build a .ind file 4x256 bytes that contains
//      - an address into .inx file where the
//        first entry into the .inx file indirect
//        starts with that letter, eg.
//        ind['A'] -> first .inx (seek) that starts
//        with 'A'
// Move .ind to the first 2048 bytes of .inx ==>
// first gp is at byte 2050.
*/

#include <stdlib.h>
#ifndef __GNUG__
# ifndef NO_ALLOCA_H
#  include <alloca.h>
# endif
#endif
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
//#include <sys/file.h>
#include <sys/stat.h>
#include "common.hxx"
#include "index.hxx"
#include "irset.hxx"
#include "soundex.hxx"
#include "metaphone.hxx"
#include "doctype.hxx"
#include <assert.h>
#include "lock.hxx"
//#include "dfdt.hxx"
#include <iomanip>
#include "magic.hxx"
#include "lang-codes.hxx"
#include "dictionary.hxx"

#include "filemap.hxx" /* NEW INDEX STRATEGY */
#include "mergeunit.hxx" /* NEW INDEX STRATEGY */
#include "stoplist.hxx"

#ifdef _WIN32
# define EIDRM  82              /* Identifier removed */
#endif


#define __Low32(_x)  (UINT4)((_x) &   0x000000FFFFFFFFLL)
#define __High32(_x) (UINT4)((((_x) & 0xFFFFFFFF000000LL) >> 32) & 0xFFFF)


#define __DEBUG__ 0

#if __DEBUG__
# include "fpt.hxx" /* DEBUG */
#endif

#ifdef _WIN32
static const char _mode_rt[] = "rt";
#else
static const char _mode_rt[] = "r";
#endif

static const char CommonWordsFileExtension[] = ".cwi";

static STRING DbIndexingStatisticsSection ("IndexingStatistics");
static STRING DbTotalBytes                ("sisByteTotal");
static STRING DbTotalWords                ("sisWordTotal");
static STRING DbTotalWordsTruncated       ("WordsTruncated");
static STRING DbLongestWord               ("LongestWord");


#ifndef  FIND_CONCAT_WORDS
# define FIND_CONCAT_WORDS 1 /* Default behaviour */
#endif

#ifdef _WIN32
# define PAGESIZE 4096
#endif

#ifndef PAGESIZE
# ifdef _SC_PAGESIZE
#  define PAGESIZE sysconf(_SC_PAGESIZE)
# else
#  ifdef PAGE_SIZE
#    define PAGESIZE PAGE_SIZE
@  else
#    define PAGESIZE (1U<<12) /* 4096 */
#  endif
# endif
#endif
static const unsigned long long maxWordsCapacity = 536870911ULL << sizeof(GPTYPE)/sizeof(INT4);

/// Need to move from INT_MAX to max addressable by kernel
#ifndef INT_MAX
# define INT_MAX 2147483647U
#endif

#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC    1000000
#endif

#define DONT_LOCK 1

#ifndef DONT_LOCK
#if 0
/*
 * flock operations.
 */
# ifndef LOCK_SH
#  define LOCK_SH      1  /* shared lock */
#  define LOCK_EX      2  /* exclusive lock */
#  define LOCK_NB      4  /* don't block when locking */
#  define LOCK_UN      8  /* unlock */
# endif
#  include <sys/file.h>
#  define lockfd(f,nb)	flock(f, LOCK_EX | (nb ? LOCK_NB : 0), 0)
#  define unlockfd(f)	flock(f, LOCK_UN)
extern "C"
  {
    int flock(int fd, int operation);
  }
#else          /* new way */
#  include <unistd.h>
#  define lockfd(f,nb)	lockf(f, (nb ? F_TLOCK : F_LOCK), 0)
#  define unlockfd(f)	lockf(f, F_ULOCK, 0)
#endif
#endif

static const STRING Section                      ("DbSearch");
static const STRING WaterlimitEntry              ("PhraseWaterlimit");
static const STRING FindConcatEntry              ("FindConcatWords");
static const STRING ForceEntry                   ("Force");
static const STRING ThresholdEntry               ("Threshold");
static const STRING DontStoreHitCoordinatesEntry ("Freeform");
static const STRING PhoneticAlgorithmEntry       ("Phonetic");
static const STRING MaxTimeEntry                 ("MaxTermSearchTime");
static const STRING MaxSearchTimeEntry           ("MaxSearchTime");


static const STRING CommonWordsSection ("CommonWords");

static const char *_lock = ".lock";

inline int putGPTYPE(const GPTYPE c, PFILE fp) { return Write(c, fp); }

#define AWWW 1
#define INDEX_VERSION 2 /* Supports up to version 15 */

// Should we ever need to go beyond version 15 we need a new encoding!!!!
// see how we write the 2-byte encoding magic below
const INT IndexVersion = (((INDEX_VERSION-1) * 2)) & 0x1F; // Increment * 2


// Private bsearch
static char * __priv_bsearch(const void *key,  const void *base0, size_t nmemb, size_t size, int (*compar)(const void *, const void*))
{
  register const char *base = (const char *)base0;
  register int lim, cmp;

  for (lim = nmemb; lim != 0; lim >>= 1) {
      register void *p = (void *)(base + (lim >> 1) * size);
      if ((cmp = (*compar)(key, p)) == 0)
	return (char *)(p);
      else if (cmp > 0) {  /* key > p: move right */
	base = (const char *)p + size;
	lim--;
      } /* else move left */
  }
  return (NULL);
}



static inline size_t TermExtract(UCHR *Buffer, size_t Length=StringCompLength)
{
  size_t len = Length;
  for (size_t z = 0; z< len; z++)
    {
      if (!IsTermChr(Buffer+z))
        len = z;
      else if (_ib_isupper(Buffer[z]))
        Buffer[z] = _ib_tolower(Buffer[z]);
    }
  Buffer[len] = '\0';
  return len;
}


// Zap all non acceptable characters
static inline size_t BufferClean(UCHR *Buffer, size_t Length=StringCompLength,
                                 bool ToLower = false)
{
  size_t len = Length;
  for (size_t z = 0; z<Length; z++)
    {
      if (!IsTermChr(&Buffer[z]))
        {
          Buffer[z] = ' ';
          if (len == Length)
            len = z;
        }
      else if (ToLower)
        Buffer[z] = _ib_tolower(Buffer[z]);
    }
  return len;
}


static inline STRING TermClean(const STRING String, bool ToLower = false)
{
  STRING Result;
  UCHR *Buffer = String.NewUCString();
  BufferClean(Buffer, String.GetLength(), ToLower);
  Result = Buffer;
  delete[] Buffer;
  return Result;
}

INDEX::INDEX (const PIDBOBJ DbParent, const STRING& NewFileName, size_t CacheSize)
{
  if (DbParent == NULL)
    {
      message_log (LOG_PANIC, "Bogus request to create an orphaned INDEX class for '%s'!",
            NewFileName.c_str());
      return;
    }

  OK            = true;
  ActiveIndexing= false;
  Parent        = DbParent;
  IndexFileName = NewFileName;
  DebugMode     = false;
  useSoundex    = false; // Use DoubleMetaphone for Phonetic
  TermAliases   = NULL;
  StopWords     = NULL;
  CommonWords   = NULL;
#ifdef _WIN32
  MergeStatus   = iMerge;
#else
  MergeStatus   = iNothing;
#endif
  TooManyRecordsThreshold = DEFAULT_TOO_MANY_RECORDS_THRESHOLD;
  MaxCPU_ticks            = _ib_defaultMaxCPU_ticks;
  MaxQueryCPU_ticks       = _ib_defaultMaxQueryCPU_ticks;

  SisFileName   = Parent->ComposeDbFn(DbExtDict);

  {
    STRING tmp;

    Parent->ProfileGetString(Section, MaxTimeEntry, NulString, &tmp);
    if (tmp.IsNumber())
      {
        int seconds = tmp.GetInt();
	if (seconds > 1)
	  MaxCPU_ticks = seconds * CLOCKS_PER_SEC;
      }
    Parent->ProfileGetString(Section, MaxSearchTimeEntry, NulString, &tmp);
    if (tmp.IsNumber())
      {
        clock_t ticks = tmp.GetInt()*CLOCKS_PER_SEC;
        if (ticks > MaxCPU_ticks)
          MaxQueryCPU_ticks = ticks; 
	else
	  MaxQueryCPU_ticks = MaxCPU_ticks*2 + ticks;
      }

    if (tmp.GetLength())
      findConcatWords = tmp.GetBool();
    else
      findConcatWords = FIND_CONCAT_WORDS;

    Parent->ProfileGetString(Section, FindConcatEntry, NulString, &tmp);
    if (tmp.GetLength())
      findConcatWords = tmp.GetBool();
    else
      findConcatWords = FIND_CONCAT_WORDS;
    Parent->ProfileGetString(FindConcatEntry, ForceEntry, NulString, &tmp);
    forceConcatWords = tmp.GetLength() ? tmp.GetBool() : false;

    Parent->ProfileGetString(Section, DontStoreHitCoordinatesEntry, NulString, &tmp);
    storeHitCoordinates = tmp.IsEmpty() ? true : !tmp.GetBool();

    Parent->ProfileGetString(Section, PhoneticAlgorithmEntry, NulString, &tmp);
    if (tmp.GetLength() && _ib_tolower(tmp.GetChr(1)) == 's')
     useSoundex = true;
  }

  if (MaxCPU_ticks > CLOCKS_PER_SEC)
    message_log (LOG_DEBUG, "Term search limit set to %.1f sec / Query total %1.f CPU.",
	MaxCPU_ticks*1.0/CLOCKS_PER_SEC, MaxQueryCPU_ticks*1.0/CLOCKS_PER_SEC);

  message_log (LOG_DEBUG, "Creating Index of '%s' with CacheSize=%ld", NewFileName.c_str(),
        (long)CacheSize);

  IndexingTotalBytesCount= Parent->ProfileGetGPTYPE(DbIndexingStatisticsSection, DbTotalBytes);
  IndexingTotalWordsCount= Parent->ProfileGetGPTYPE(DbIndexingStatisticsSection, DbTotalWords);
  IndexingWordsTruncated = Parent->ProfileGetGPTYPE(DbIndexingStatisticsSection, DbTotalWordsTruncated);
  IndexingWordsLongestLength = Parent->ProfileGetGPTYPE(DbIndexingStatisticsSection, DbLongestWord);


  const bool host = IsBigEndian();

  const  int indexTypus = (sizeof(GPTYPE) == 4) ? 0 : 1;

  IndexMagic = (indexTypus << 14) | ((host ? objINDEXl : objINDEXm) << 5) |
               (IndexVersion + OPTIM);

  if (IndexFileName.GetLength() == 0)
    {
      message_log (LOG_WARN, "Index called with Nothing???");
      Parent->SetErrorCode(0);
    }
  else if (GetFileSize(IndexFileName) % sizeof(GPTYPE) == sizeof(IndexMagic))
    {
      // New generation index
      PFILE fp = ffopen (IndexFileName, "rb");
      if (fp)
        {
          IndexMagic = getINT2(fp);
          ffclose (fp);
          if (INDEX_VERSION == 1)
            {
              // Old versions ? Update magic to new magic system
              if ((IndexMagic == ((objINDEXl << 8) | (6 + OPTIM))) || IndexMagic == ((objINDEXm << 8) | (6 + OPTIM)) )
		{
		  message_log (LOG_INFO, "Old version index (%d), updating ..", (int)IndexMagic);
                  IndexMagic = ((indexTypus << 14) | ((IndexMagic & 0xFF00 >> 8) << 5) | (IndexVersion + OPTIM));
		}
            }
          if (((IndexMagic & 0xFF00) >> 14) != indexTypus)
            {
              message_log (LOG_INFO, "ERROR: %u indexes (%x) are not compatible with %u-bit libs",
                    (unsigned)(32*(indexTypus+1)), (unsigned)IndexMagic,  (unsigned)(8*sizeof(GPTYPE)));
              Parent->SetErrorCode(-8*(int)sizeof(GPTYPE));
              OK = false;
            }
          else
            {
              int bits = (IndexMagic & 0xFF00)>>14;
              message_log (LOG_DEBUG, "%u-bit Indexes v.%d (%sEndian)",
                    (unsigned)(32*(bits+1)), (int)(IndexMagic&0x1F),
                    ((IndexMagic>> 5) & 0xFF) == objINDEXm ? "Little" : "Big");
            }
        }
      else
        {
          message_log (LOG_ERRNO, "Couldn't access '%s' (Index)", IndexFileName.c_str());
          Parent->SetErrorCode(2);
          OK = false;
        }
    }
  else
    message_log (LOG_DEBUG, "NO INDEX MAGIC");

  wrongEndian = (((IndexMagic>> 5) & 0xFF) == objINDEXm) ? host : !host;
  if (wrongEndian != host && ((IndexMagic >> 5) & 0xFF) != objINDEXl)
    {
      message_log (LOG_ERROR, "Index magic is corrupt (%x)?", IndexMagic);
      Parent->SetErrorCode(1);
      OK = false;
    }
  else if (wrongEndian)
    {
      const char *little = "littleEndian";
      const char *big    = "bigEndian";
      message_log (LOG_INFO, "Platform is %s. Index is %s.",  host ? big : little, host ? little : big);
    }
  SIScount = 0;

  SetCache = CacheSize ? new RCACHE(Parent, CacheSize) : NULL;
  if (SetCache)
    {
      message_log (LOG_DEBUG, "RCACHE(Parent, %ld)", (long)CacheSize);
      if (Parent->UsePersistantCache())
        {
          FILE *fp = fopen(Parent->PersistantCacheName(), "rb");
          if (fp)
            {
              SetCache->Read(fp);
              fclose(fp);
            }
        }
    }
  FieldCache = new FCACHE(Parent);

  NumFieldCache   = NULL;

  IndexNum = 0;
  MemorySISCache = NULL;
  MemoryIndexCache = NULL;
  SisLimit = DefaultSisLength;
  ClippingThreshold = 0;

  // if (Parent) Parent->ProfileWriteString("TermAliases", NulString, NulString);

  {
    STRING s;

    if (Parent) Parent->ProfileGetString(CommonWordsSection, ThresholdEntry,  NulString, &s);
    if ((CommonWordsThreshold = s.GetInt()) <= 0)
      {
        CommonWordsThreshold = VERY_COMMON_TERM;
        if (Parent) Parent->ProfileWriteString(CommonWordsSection, ThresholdEntry, VERY_COMMON_TERM);
      }
    else
      message_log (LOG_DEBUG, "Common words threshold is %lu", (unsigned long)CommonWordsThreshold);

    s.Clear();
    if (Parent)
      Parent->ProfileGetString(Section, WaterlimitEntry, NulString, &s);

    if ((PhraseWaterlimit = s.GetInt()) <= 0)
      {
        PhraseWaterlimit = CommonWordsThreshold/50;
        if (Parent) Parent->ProfileWriteString(Section, WaterlimitEntry, PhraseWaterlimit);
      }
    else
      message_log (LOG_DEBUG, "Phrase heuristic water limit is %lu", (long)PhraseWaterlimit);
  }
}


STRING INDEX::Description() const
  {
    STRING result;

    result.form ("\
Low level index <database>.ini Options:\n\
 [%s]\n\
 %s=<nnn>\t# Max. time in seconds to search for a term (advice) [default %d].\n\
 %s=<nnn>\t# Max. time in seconds (nnn) to run a query (advice) [default %d].\n\
 %s=<bool>\t# True/False: Search \"flow-er\"? Not found then \"flower\".\n\
 %s=nnn\t# At this point we go into heuristic modus for phrases.\n\
 %s=<bool>\t# Should we NOT store hits (no proximity etc.)?\n\
 %s=[soundex|metaphone] # Algorithm to use for phonetic term searches.\n\n\
 [%s]\n\
 %s=<bool>\t# Force search of XX-YY-ZZ for XXYYZZ if no match.\n\n\
 [TermAliases]\n\
 <Term>=<TermAlias>\t# To map one term to another\n\n\
 [%s]\n\
 %s=nnnn\t# Frequency to call common\n\
 Words=<word1> <word2> .... # A list of common words with space seperators\n\n"
                 , Section.c_str()
		 , MaxTimeEntry.c_str(), MaxCPU_ticks/CLOCKS_PER_SEC
		 , MaxSearchTimeEntry.c_str(),  MaxQueryCPU_ticks/CLOCKS_PER_SEC
                 , FindConcatEntry.c_str()
                 , WaterlimitEntry.c_str()
		 , DontStoreHitCoordinatesEntry.c_str()
		 , PhoneticAlgorithmEntry.c_str()
                 , FindConcatEntry.c_str()
                 , ForceEntry.c_str()
		 , CommonWordsSection.c_str()
		 , ThresholdEntry.c_str()
 );
    result << "\
Geospatial RECT{North West South East} [Note the canonical order]\n\
queries need to have their data fields defined via Gpoly-types or:\n\
 [BOUNDING-BOX]\n\
 North=<Numeric Field for North Coordinates [NORTHBC]>\n\
 East= <Numeric Field for East Coordinatesi [EASTBC]>\n\
 West= <Numeric Field for West Coordinatesi [WESTBC]>\n\
 South=<Numeric Field for South Coordinates [SOUTHBC]>\n\n";


    result << "Stopwords are used during search on the basis of STOPLISTS.\n"
    << STOPLIST().Description();
    return result;
  }


void INDEX::DumpPersistantCache()
{
  // Save the cache between sessions?
  if (SetCache && SetCache->Modified())
    {
      const STRING    Fn(Parent->PersistantCacheName());
      if (!Fn.IsEmpty())
        {
          STRING          tmp(Fn);
          if (!FileExists(tmp.Cat(".tmp")))
            {
              FILE           *fp = fopen(tmp, "wb");
              if (fp)
                {
                  SetCache->Write(fp);
                  fclose(fp);
                  if (RenameFile(tmp, Fn) == -1)
                    {
                      message_log(LOG_ERRNO, "Could not install cache %s into %s", tmp.c_str(), Fn.c_str());
#ifdef _WIN32
                      ::chmod (tmp.c_str(), S_IWRITE); // Ignore result
#endif
                      if (::remove(tmp) == -1)
                        message_log (LOG_ERRNO, "Could not remove '%s' (Old Persistant Cache). Please remove by hand!", tmp.c_str());
                      Parent->SetErrorCode(2);
                    }
#ifndef MSDOS
                  /*
                   else	// Set the cache as world writable!
                     chmod(fn.c_str(), S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
                   */
#endif
                }
              else
                {
                  message_log(LOG_NOTICE, "Can't write temporary file %s", tmp.c_str());
                  Parent->SetErrorCode(2);
                }
            }
          else
            {
              message_log (LOG_ERROR, "Search Cache temporary file '%s' already exists. Deadlock condition?", tmp.c_str());
              Parent->SetErrorCode(2);
            }
        }
    }
}



void INDEX::SetCacheSize(size_t newCacheSize)
{
  RCACHE *newSetCache = NULL;

  if (newCacheSize > 0)
    {
      try
        {
          newSetCache = new RCACHE(Parent, newCacheSize);
        }
      catch (...)
        {
          message_log (LOG_ERRNO, "INDEX::SetCacheSize(%ld) allocation failed.", (long)newCacheSize);
          newSetCache = NULL;
        }
    }
  if (SetCache)
    {
      if (Parent->UsePersistantCache())
        {
          DumpPersistantCache();
          if (newSetCache)
            {
              FILE *fp = fopen(Parent->PersistantCacheName(), "rb");
              if (fp)
                {
                  newSetCache->Read(fp);
                  fclose(fp);
                }
            }
        }
      delete SetCache;
    }
  SetCache = newSetCache;
}

void INDEX::SetSisLimit(size_t NewLimit)
{
  if (NewLimit > 2 && NewLimit < StringCompLength)
    SisLimit = NewLimit;
  message_log (LOG_INFO, "SISLimit is %d", SisLimit);
}


 // Max in Seconds
void   INDEX::SetMaximumTermCPU_sec(size_t newMax) { MaxCPU_ticks = newMax * CLOCKS_PER_SEC; }
size_t INDEX::GetMaximumTermCPU_sec() const        { return MaxCPU_ticks/CLOCKS_PER_SEC;     }

void   INDEX::SetMaximumQueryCPU_sec(size_t newMax) { MaxQueryCPU_ticks = newMax * CLOCKS_PER_SEC; }
size_t INDEX::GetMaximumQueryCPU_sec() const        { return MaxQueryCPU_ticks/CLOCKS_PER_SEC;     }


void   INDEX::SetMaxRecordsAdvice(size_t Limit)
{
  TooManyRecordsThreshold = ((Limit > 0) ? Limit : DEFAULT_TOO_MANY_RECORDS_THRESHOLD);
}

size_t INDEX::GetMaxRecordsAdvice() const
{
  return TooManyRecordsThreshold;
}



bool INDEX::IsSystemFile (const STRING&) const
{
  return false;
}


PFILE INDEX::ffopen (const STRING& FileName, const CHR* Type) const
{
  return Parent->ffopen (FileName, Type);
}

INT INDEX::ffclose (PFILE FilePointer) const
{
  return Parent->ffclose (FilePointer);
}


bool INDEX::SetDateRange(const DATERANGE& Range)
{
  DateRange = Range;
  if (DebugMode) message_log(LOG_DEBUG, "Date Range Set: %s", (const char *)((STRING)Range));
  return DateRange.Ok();
}

bool INDEX::GetDateRange(DATERANGE *Range) const
{
  if (Range)
    *Range = DateRange;
  return DateRange.Ok();
}


bool INDEX::SetIndexNum(INT Num)
{
  if (Parent == NULL)
    {
      message_log (LOG_ERROR, "Can't set the Index number in orphan'd INDEX");
      return false;
    }
  MDT *mdt = Parent->GetMainMdt();
  if (mdt == NULL)
    {
      message_log (LOG_ERROR, "Can't set the index number, lost MDT in INDEX?");
      return false;
    }
  if (mdt->SetIndexNum(Num) == false)
    {
      message_log (LOG_ERRNO, "Couldn't set index number to %d.", Num);
      Parent->SetErrorCode(2);
      return false;
    }
  return true;
}

INT INDEX::GetIndexNum() const
{
  return Parent->GetMainMdt()->GetIndexNum();
}

void INDEX::SetDocTypePtr(const PDOCTYPE NewDocTypePtr)
{
  DocTypePtr = NewDocTypePtr;
}

PDOCTYPE INDEX::GetDocTypePtr() const
{
  return DocTypePtr;
}



bool INDEX::IsWrongEndian() const
{
  return wrongEndian;
}

INT INDEX::Version() const
{
    extern int _ib_ctype_signature_;
//  IndexMagic = ( (host ? objINDEXl : objINDEXm) << 8) | IndexVersion + OPTIM;

    if (((INDEX_VERSION)>1) && (((IndexMagic & 0x1F))/2 != (IndexVersion/2)))
      return -1;
    return IndexVersion + 100*(_ib_ctype_signature_) + 10000*sizeof(GPTYPE);
}

// Term, Buffer, Length
static inline INT _strncasecmp(const UCHR *p1, const UCHR *p2, const INT n,
// What's next, length of the second term/phrase
                               bool *look = NULL, size_t *length = NULL)
{
  const UCHR *p2_Start = p2;
  int         diff = 0, q = 0;
  INT         x = 0;
  while (*p1 && *p2 &&
         ((q = (IsTermWhite(*p1) && IsTermWhite(*p2))) ||
          ( ((*p1 == '.') || (*p1 == ',')) && ((*p2 == '.') || (*p2 == ','))) ||
          (!IsTermChar(*p1) && !IsTermChar(*p2)) ||
          (diff = ((UCHR) _ib_tolower (*p1) - (UCHR) _ib_tolower (*p2))) == 0) )
    {
      if (q)
        {
          // Skip over spaces
          do { p2++; }
          while (IsTermWhite(*p2));
          do { p1++, x++; }
          while (x < n && IsTermWhite(*p1));
        }
      else
        {
          p1++, p2++, x++;
        }

      if (x >= n)
        break;
    }
  if (look) *look = IsTermChar(*p2);
  if (x < n && *p2 == '\0')
    {
      diff = *p1;
    }
  if (diff == 0 && length) *length = p2 - p2_Start - 1;
  return diff;
}


INT INDEX::GpFwrite (const GPTYPE Gp, FILE *Stream) const
{
#if USE_PLATFORM_IND_INX
  return GpFwrite(&Gp, 1, Stream);
#else
  GPTYPE gp = Gp;
  if (wrongEndian) GpSwab(&gp);
  return fwrite (&gp, sizeof(GPTYPE), 1, Stream);
#endif
}

INT INDEX::GpFwrite (const GPTYPE * const Ptr, size_t NumElements, FILE *Stream) const
  {
    size_t len = 0;
#if USE_PLATFORM_IND_INX
    for (size_t i=0; i< NumElements; i++)
      {
        if (Write(Ptr[i], Stream) == 0)
          break;
        len++;
      }
    return len;
#else
    if (wrongEndian)
      {
        for (; len < NumElements; len++)
          {
            if (GpFwrite (Ptr[len], Stream) == 0)
              break;
          }
      }
    else
      {
        // See fwrite(3s)
        size_t res;
        do
          {
            res = (size_t)fwrite (&Ptr[len], sizeof(GPTYPE), NumElements, Stream);
            len += res;
            NumElements -= res;
          }
        while (res && NumElements > 0);
      }
    return (INT)len;
#endif
  }


INT INDEX::GpFread (GPTYPE *Ptr, FILE *Stream) const
  {
#if USE_PLATFORM_IND_INX
    GPTYPE gp;

    Read(&gp, Stream);
    if (Ptr) *Ptr = gp;
    return 1;
#else
    const size_t x = fread (Ptr, sizeof(GPTYPE), 1, Stream);
    if (x && wrongEndian) GpSwab (Ptr);
    return x;
#endif
  }

INT INDEX::GpFread (GPTYPE *Ptr, size_t NumElements, FILE *Stream) const
  {
#if USE_PLATFORM_IND_INX
    size_t len =0;
    while (len++<NumElements)
      if (GpFread(Ptr++, Stream) == 0)
        break;
#else
    // See fread(3s)
    size_t res, len = 0;
    do
      {
        res = (size_t)fread (&Ptr[len], sizeof(GPTYPE), NumElements, Stream);
        len += res;
        NumElements -= res;
      }
    while (res && NumElements > 0);
    if (len && wrongEndian)
      {
        for (size_t y = 0; y < len; y++)
          GpSwab (Ptr + y);
      }
#endif
    return len;
  }

GPTYPE INDEX::GpFread(FILE *Stream) const
  {
    GPTYPE gp = (GPTYPE)(-1);
    GpFread(&gp, Stream);
    return gp;
  }


GPTYPE INDEX::GpFread(off_t Pos, FILE *Stream) const
  {
    if (-1 == fseek(Stream, PositionOf(Pos), SEEK_SET))
      {
        message_log(LOG_ERRNO,
#ifdef _WIN32
	  "Can't seek to %I64d in index.", (__int64)PositionOf(Pos));
#else
	  "Can't seek to %lld in index." , (long long)PositionOf(Pos));
#endif
        Parent->SetErrorCode(2);
        return (GPTYPE)-1;
      }
    return GpFread(Stream);
  }

INT INDEX::GpFread(GPTYPE *Ptr, size_t NumElements, off_t Pos, FILE *Stream) const
  {
    if (-1 == fseek(Stream, PositionOf(Pos), SEEK_SET))
      {
        message_log(LOG_ERRNO,
#ifdef _WIN32
	  "Can't seek to %I64d in index.", (__int64)PositionOf(Pos));

#else
	  "Can't seek to %lld in index." , (long long)PositionOf(Pos));
#endif
        Parent->SetErrorCode(2);
        return 0;
      }
    return GpFread(Ptr, NumElements, Stream); /* @@ */
  }

GPTYPE * const INDEX::GpPtr(size_t SubIndex, GPTYPE Pos) const
{
  if (MemoryIndexCache)
    return (GPTYPE *)((unsigned char *)MemoryIndexCache->Ptr(SubIndex) + PositionOf(Pos));
  return NULL;
}

GPTYPE INDEX::GpOf(GPTYPE *Ptr, size_t Index) const
{
  if (wrongEndian)
    return GpSwab(Ptr[Index]);
  return Ptr[Index];
}

static inline int SisCompare(const UCHR *p1, const UCHR *p2, int MaxLen = StringCompLength)
{
  int x = 0, diff;
  while ((diff = _ib_diff(*p1, *p2)) == 0 &&  __IB_cclass[*p1])
    {
      if (++x >= MaxLen) break;
      p1++, p2++;
    }
  return diff;
}

#if 1
#define SisCompare2 SisCompare
#else
static int SisCompare2(const UCHR *p1, const UCHR *p2, int MaxLen = StringCompLength)
{
  int x = 0, diff;

  while ((diff = _ib_diff(*p1, *p2)) == 0 &&  __IB_cclass[*p1] && __IB_cclass[*p2])
    {
      if (++x >= MaxLen) break;
      p1++, p2++;
    }
  if (diff == 0 && x < MaxLen)
    {
      return __IB_cclass[*p1] - __IB_cclass[*p2];
    }
  return diff;
}
#endif


#if 0
bool INDEX::ExtractFieldTable(INT Number)
{
  //
}
#endif

NUMERICOBJ INDEX::encodeHash(const STRING& String) const
{
  STRING     buffer ( String.DeQuote().Pack().Trim(STRING::both) );
  NUMERICOBJ val = buffer.CRC64();
  NUMERICOBJ off = 1.0/(double)(buffer.GetLength() % 10000);
  return val + off;
}

NUMERICOBJ INDEX::encodeCaseHash(const STRING& String) const
{
  STRING     buffer ( String.DeQuote().Pack().Trim(STRING::both) );

  buffer.MakeLower();
  NUMERICOBJ val = buffer.CRC64();
  NUMERICOBJ off = 1.0/(double)(buffer.GetLength() % 10000);
  return val + off;
}

NUMERICOBJ INDEX::encodeLexiHash(const STRING& String) const
{
  STRING      buffer ( String.DeQuote().Pack().Trim(STRING::both) );
  size_t      blen = buffer.GetLength();
  UINT8       x = 0;
  char        tmp[9];

  if (blen > 8) blen = 8;

  buffer.MakeLower();
#ifdef BSD
  bzero(tmp, sizeof(tmp));
#else
  memset(tmp, '\0', sizeof(tmp));
#endif
  strncpy(tmp, buffer.c_str(), blen);

  for (size_t i = 0; i < sizeof(x); i++)
    x = (x << 8) + tmp[i];

  return (long double)x +  1.0/((double)(buffer.GetLength() % 10000));
}


extern "C" long double (*_IB_private_hash)(const char *, const char *, size_t );

FILE *INDEX::OpenForAppend(const STRING& FieldName, FIELDTYPE FieldType)
{
  FILE       *fp = NULL;
  INT         type = (INT)FieldType;
  STRING      FileName;

  if (Parent == NULL || FieldName.IsEmpty())
    return NULL;

  if (Parent->DfdtGetFileName(FieldName, FieldType, &FileName) == false)
    {
      message_log (LOG_ERROR, "Could not get a filename for '%s' of type %s. DFD Defect?",
	FieldName.c_str(), FieldType.c_str());
    }
  else // We want the right method to open for append
    switch (type)
      {
        case FIELDTYPE::ttl:
        case FIELDTYPE::numerical:
	case FIELDTYPE::phonhash:
	case FIELDTYPE::phonhash2:
	case FIELDTYPE::metaphone:
	case FIELDTYPE::metaphone2:
	case FIELDTYPE::hash:
	case FIELDTYPE::casehash:
	case FIELDTYPE::currency:
	case FIELDTYPE::lexi:
	case FIELDTYPE::dotnumber:
	  fp = NUMERICLIST().OpenForAppend(FileName);
	  break;
	case FIELDTYPE::db_hnsw:
	  /* NOT YET IMPLEMENTED */
	  break;
	case FIELDTYPE::privhash:
	  if (_IB_private_hash)
	    fp = NUMERICLIST().OpenForAppend(FileName);
	  else
	    FileName.Clear(); // Can't add to an undefined ...
          break;
        case FIELDTYPE::ttl_expires:
        case FIELDTYPE::time:
        case FIELDTYPE::date:
          fp = DATELIST().OpenForAppend(FileName);
          break;
        case FIELDTYPE::gpoly:
          fp = GPOLYLIST().OpenForAppend(FileName);
          break;
        case FIELDTYPE::box:
          fp = BBOXLIST().OpenForAppend(FileName);
          break;
        case FIELDTYPE::numericalrange:
          fp = INTERVALLIST().OpenForAppend(FileName);
          break;
        default:
          message_log (LOG_WARN, "No OpenForAppend method defined for type %s, using default", FieldType.c_str());
        case FIELDTYPE::daterange:
	  // fall into..
	case FIELDTYPE::any:
	case FIELDTYPE::text:
          fp = fopen(FileName, "ab");
      }
  return fp;
}


bool INDEX::WriteFieldData (const RECORD& Record, const GPTYPE GpOffset)
{
  FILE  *fp;
  STRING FileName;

  const DFT *dftPtr = Record.GetDftPtr ();
  const size_t total = dftPtr->GetTotalEntries ();
  size_t errors = 0; // shared in the threads below

  if (DebugMode && total)
    message_log (LOG_DEBUG, "Writing field data, %d elements", total);

  DOCTYPE *DocTypePtr =  Parent->GetDocTypePtr ( Record.GetDocumentType() );
  BUFFER Buffer;

#pragma omp parallel for 
  for (size_t x = 1; x <= total; x++)
    {
      const STRING    FieldName ( dftPtr->GetFieldName (x) );
      const FIELDTYPE FieldType ( Parent->GetFieldType(FieldName) );

      bool  res = Parent->DfdtGetFileName (FieldName, &FileName);

      if (res == false)
        {
	  if (Parent->checkFieldName(FieldName)) { 
            message_log (LOG_ERROR, "Can't append '%s' field data, DFD Defect!", FieldName.c_str());
            errors++;
	  } // else disallowed characters in name

          continue;
        }
      else
        {
          if (DebugMode) message_log (LOG_DEBUG, "Dumping %s FCs to %s",
                                 FieldName.c_str(), FileName.c_str());
          if ((fp = fopen (FileName.c_str(), "ab")) == NULL)
            {
              message_log (LOG_ERRNO, "Can't append '%s' field data to '%s'!",
                    FieldName.c_str(), FileName.c_str());
              errors++;
              continue; // ERROR
            }
        }
#ifndef DONT_LOCK
      int fd = fileno(fp);
      if (-1 == lockfd(fd, 1))
        {
          message_log (LOG_ERRNO, "Can't lock %s", FieldName.c_str());
        }
#endif

      dftPtr->WriteFct(x, fp, GpOffset);
#ifndef DONT_LOCK
      if (-1 == unlockfd(fd))
        {
          message_log (LOG_ERRNO, "Can't unlock %s", FieldName.c_str());
        }
#endif
      fclose (fp);

      if (DocTypePtr == NULL)
        {
          message_log (LOG_PANIC, "Null pointer to doctype in Write????");
          continue;
        }

      if (!FieldType.Ok())
        {
          message_log (LOG_WARN, "Unknown Fieldtype keyword specified for '%s'.", FieldName.c_str());
          continue;
        }

      // If field is text we are done
      if (FieldType.IsText())
        {
          continue;
        }


      INT         type = (INT)FieldType;
      message_log (LOG_DEBUG, "Handle fieldtype: %d", type);
      switch (type)
	{
#if 0
	  case FIELDTYPE::db_string:
	    db_stringPtr = new DB_STRING();
	    db_stringPtr->OpenFieldForAppend(FieldName);
	    break;
          case FIELDTYPE::callback:
	  case FIELDTYPE::callback1:
	  case FIELDTYPE::callback2:
	  case FIELDTYPE::callback3:
	  case FIELDTYPE::callback4:
	  case FIELDTYPE::callback5:
	  case FIELDTYPE::callback6:
	  case FIELDTYPE::callback7:
	    db_callbackPtr = new DB_CALLBACK((int)type-(int)callback);
	    db_callbackPtr->OpenFieldForAppend(FieldName);
	    break;
#endif
	  default:
	    if ((fp = OpenForAppend(FieldName, FieldType)) == NULL)
	      {
		message_log (LOG_ERRNO, "Can't append '%s' %s-field data!", FieldName.c_str(), FieldType.c_str());
		errors++;
		continue;
	      }
	}

      const  DF        *dfPtr     = dftPtr->GetEntryPtr(x);
      const  FCLIST    *fcListPtr = dfPtr->GetFcListPtr ();
//      BUFFER Buffer (127, sizeof(UCHR));
      int    items = 0;

      //static const char message[] = "Field '%s' is %s. Appended %d item(s) to '%s'";

      for (const FCLIST *p = fcListPtr->Next(); p!=fcListPtr; p=p->Next())
        {
          const FC     fc   = p->Value();
          const GPTYPE gp   = fc.GetFieldStart() + GpOffset;
          const size_t flen = fc.GetLength();

          if (type == FIELDTYPE::unknown)
	    {
              break;
	    }
          GetIndirectBuffer(gp, (UCHR *)Buffer.Want(flen+1, sizeof(UCHR)), 0, flen);

          // File format: <gp><data_object>
          switch (type)
            {
            case FIELDTYPE::isbn:
            {

            }
            break;
#if 0
            case FIELDTYPE::db_string:
            {
              STRING value (DocTypePtr->ParseString(FieldName, Record, Buffer);
                            if (db_stringPtr->Write(value))
                            items++;
                          }
                        break;
      case FIELDTYPE::callback:  case FIELDTYPE::callback1: case FIELDTYPE::callback2:
      case FIELDTYPE::callback3: case FIELDTYPE::callback4: case FIELDTYPE::callback5:
        case FIELDTYPE::callback6: case FIELDTYPE::callback7:
          {
            STRING value (DocTypePtr->ParseString(FieldName, Record, Buffer);
                          if (db_callbackPtr->Write(value))
                            items++;
                          }
                        break;
#endif
          case FIELDTYPE::ttl_expires:
          {
            NUMERICOBJ val (DocTypePtr->ParseTTL(FieldName, Buffer));
              int        ttl = val;
              if (ttl > 0)
                {
                  // Becomes a FIELDTYPE::date type(!)
                  SRCH_DATE expiryDate;
                  expiryDate.SetNow();
                  DATEFLD(gp, expiryDate.PlusNminutes(ttl)).Write(fp);
                  items++;
                }
              break;

            }
            case FIELDTYPE::time:
            {
              NUMERICOBJ val (DocTypePtr->ParseComputed(FieldName, Buffer));
              time_t     time = val;

              // Numeric computed value for time_t struct (seconds since 1970)
              // this becomes a FIELDTYPE::date type(!)
              const SRCH_DATE date ( &time );
              if (date.Ok())
                {
                  DATEFLD(gp, date).Write(fp);
                  items++;
                }
              break;

            }
            case FIELDTYPE::date:
            {
              const SRCH_DATE date ( DocTypePtr->ParseDate(Buffer, FieldName) );
              if (date.Ok())
                {
                  // Store only whole date
                  DATEFLD(gp, date).Write(fp);
                  items++;
                }
              break;
            }
            case FIELDTYPE::daterange:
            {
              const DATERANGE range ( DocTypePtr->ParseDateRange(Buffer, FieldName) );
              if (range.Ok())
                {
                  DATERNGFLD(gp, range).Write(fp);
                  items++;
                }
              break;
            }

	    case FIELDTYPE::db_hnsw: /* HNSW */
	    {
	      message_log (LOG_ERROR, "HNSW Not Yet Implemented");
	      break;
	    }

	    case FIELDTYPE::metaphone:
	    case FIELDTYPE::phonhash:
	    {
	      WORDSLIST List;
	      if (DocTypePtr->ParseWords2(Buffer, &List))
		{
		  UINT8 v;
		  for (WORDSLIST *ptr = List.Next(); ptr != &List; ptr=ptr->Next())
		    {
		      const STRING name (  ptr->Value().Word() );
#ifdef PHONETIC_SKIP_INITIALS
		      if (name.GetLength() <= 1 || name.GetChr(2) == '.')
			continue;
#endif
		      const size_t off  = ptr->Value().Offset();
		      if (type == FIELDTYPE::phonhash)
			{
			  NUMERICFLD(gp+off, SoundexEncode( name )).Write(fp); // Primary key
			  items++;
			}
		      else if ((v = DoubleMetaphone ( name )) != 0)
			{
			  UINT4        low  = __Low32(v);
			  UINT4        high = __High32(v);
			  NUMERICFLD(gp+off, low).Write(fp); // Primary key
			  if (high)
			    NUMERICFLD(gp+off+1, high).Write(fp); // Optional secondary key
			  items++;
			}
		    }
		}
	      else message_log (LOG_WARN, "Empty metaphone field '%s': %s", FieldName.c_str(), (const char *)Buffer);
              break;
	    }
            case FIELDTYPE::metaphone2:
            {
              UINT8  v =  DoubleMetaphone(DocTypePtr->ParseBuffer(Buffer));
	      if (v != 0)
		{
		  UINT4        low  = __Low32(v);
		  UINT4        high = __High32(v);
		  NUMERICFLD(gp, low).Write(fp);
		  if (high)  NUMERICFLD(gp+1, high).Write(fp);
		  items++;
		}
              break;
            }
	    case FIELDTYPE::phonhash2:
	    {
	      const NUMERICOBJ val ( DocTypePtr->ParsePhonhash(Buffer) );
	      NUMERICFLD(gp, val).Write(fp);
	      items++;
	      break;
	    }
	    case FIELDTYPE::hash:
	    {
	      const NUMERICOBJ val = encodeHash( DocTypePtr->ParseBuffer(Buffer) ); 
	      NUMERICFLD(gp, val).Write(fp);
	      items++;
	      break;
	    }
            case FIELDTYPE::casehash:
            {
              const NUMERICOBJ val = encodeCaseHash( DocTypePtr->ParseBuffer(Buffer) );
              NUMERICFLD(gp, val).Write(fp);
              items++;
              break;
            }
	    case FIELDTYPE::lexi:
	    {
	      const NUMERICOBJ val = encodeLexiHash( DocTypePtr->ParseBuffer(Buffer) );
	      NUMERICFLD(gp, val).Write(fp);
	      items++;
	      break;

	    }
            case FIELDTYPE::privhash:
            {
	      if (_IB_private_hash)
		{
		   STRING content (  DocTypePtr->ParseBuffer(Buffer) );
		   const NUMERICOBJ val = _IB_private_hash(FieldName, content, content.Len()); 
		   NUMERICFLD(gp, val).Write(fp);   
		   items++;
		}
              break;
            }
	    case FIELDTYPE::currency:
	    {
	      const MONETARYOBJ currency ( DocTypePtr->ParseCurrency(FieldName, Buffer) );
              if (currency.Ok())
                {
                  NUMERICFLD(gp, currency).Write(fp);
                  items++;
                }
              break;
	    }
	    case FIELDTYPE::dotnumber:
            case FIELDTYPE::numerical:
            {
	      STRING           sValue ( Buffer.c_ustr() );
              const NUMERICOBJ val (  DocTypePtr->ParseNumeric(sValue) );
              if (val.Ok())
                {
                  NUMERICFLD(gp, val).Write(fp);
                  items++;
                }
              else message_log (LOG_WARN,
#ifdef _WIN32
			"In '%s' \"%s\" not numeric (%I64d) // %ld",
#else
			"In '%s' \"%s\" not numeric (%lld) // %ld",
#endif
			FieldName.c_str(), sValue.Trim().c_str(), (long long)gp, (long)val);
              break;
            }
            case FIELDTYPE::ttl:
            case FIELDTYPE::computed:
            {
              NUMERICOBJ val ( type == FIELDTYPE::ttl ?
                               DocTypePtr->ParseTTL(FieldName, Buffer) :
                               DocTypePtr->ParseComputed(FieldName, Buffer));
              if (val.Ok())
                {
                  NUMERICFLD(gp, val).Write(fp);
                  items++;
                }
              break;
            }
            case FIELDTYPE::numericalrange:
            {
              DOUBLE fStart = 0, fEnd = 0;
              if (DocTypePtr->ParseRange(Buffer, FieldName, &fStart, &fEnd))
                {
                  INTERVALFLD intervalfield;
                  // Double pair
                  intervalfield.SetGlobalStart(gp);
                  intervalfield.SetStartValue(fStart);
                  intervalfield.SetEndValue(fEnd);
                  Write(intervalfield, fp);
                  items++;
                }
              break;
            }
            case FIELDTYPE::box:
            {
              BBOXFLD bbox;
              if (DocTypePtr->ParseBBox(Buffer, FieldName, &bbox))
                {
                  Write(bbox, fp);
                  items++;
#if 0
		  if (bbox.GetExtent() != 0)
                    {
                      STRING fn;
                      STRING extent ("@Extent");
                      STRING extentField ( FieldName + extent );
                      if (Parent->DfdtGetFileName(extentField, FIELDTYPE::numerical, &fn))
                        {
                          FILE *fpE = NUMERICLIST().OpenForAppend(fn);
                          Write(bbox.getExtent(), fp);
                          fclose(fpE);
                        }
                      if (Parent->DfdtGetFileName(extent, FIELDTYPE::numerical, &fn))
                        {
                          FILE *fpE = NUMERICLIST().OpenForAppend(fn);
                          Write(bbox.getExtent(), fp);
                          fclose(fpE);
                        }
                    }
#endif
		}
              break;
            }
            case FIELDTYPE::gpoly:
            {
              GPOLYFLD gpoly;
              if (DocTypePtr->ParseGPoly(Buffer, FieldName, &gpoly))
                {
                  Write(gpoly, fp);
                  items++;
                }
              break;
            }
            default:
              message_log (LOG_WARN, "Unimplemented/Unsupported data-type '%s' specified for field '%s'",
                    FieldType.c_str(), FieldName.c_str());
              type = FIELDTYPE::unknown;
            }
        } // for()
      fclose(fp);
      if (items)
        message_log (LOG_DEBUG, "Appended %d item(s) of type '%s' for field '%s'",
              items, FieldType.c_str(), FieldName.c_str() );
      else if (type != FIELDTYPE::unknown)
        message_log (LOG_WARN, "Despite definition, field '%s' had no valid items of type '%s'.",
              FieldName.c_str(), FieldType.c_str() );
    } // for()

  Buffer.Free("INDEX::WriteFieldData", "Buffer");
  if (total && DebugMode) message_log (LOG_DEBUG, "%d errors", errors);
  return errors == 0;
}

void INDEX::SetMergeStatus(t_merge_status a)
{
  MergeStatus=a;
}

bool INDEX::IsEmpty() const
{
    // Like GetTotalWords() below but we just need 1 word
    STRING s;
    const INT count = GetIndexNum();

    for (int j = count; j>= 0; j--)
      {
        if (j)
          s.form("%s.%d", IndexFileName.c_str(), j);
        else if (count)
          break;
        else
          s = IndexFileName;
        if (GetFileSize(s) >= (off_t)sizeof(GPTYPE))
          return false;
        if (!FileExists(s)) message_log (LOG_ERRNO, "Can't access %s", s.c_str());
      }
    return true;
}

off_t INDEX::GetTotalWords() const
  {
    STRING s;
    const INT count = GetIndexNum();
    off_t Words = 0;

    for (int j = count; j>= 0; j--)
      {
        if (j)
          s.form("%s.%d", IndexFileName.c_str(), j);
        else if (count)
          break;
        else
          s = IndexFileName;
        Words += (GetFileSize(s)/sizeof(GPTYPE));
      }
    return Words;
  }

off_t INDEX::GetTotalUniqueWords() const
  {
    STRING s;
    const INT count = GetIndexNum();
    off_t Words = 0;
    FILE *fp;

    for (int j = count; j >= 0; j--)
      {
        if (j)
          s.form("%s.%d", SisFileName.c_str(), j);
        else if (count)
          break;
        else
          s = SisFileName;
        if ((fp = ffopen(s, "rb")) != NULL)
          {
            const size_t compLength = (unsigned char)fgetc(fp) + sizeof(GPTYPE) + 1;
            Words += (GetFileSize(fp)/compLength);
            ffclose(fp);
          }
      }
    return Words;
  }


bool INDEX::CreateCentroid()
{
  bool result = false;
  STRING      fn ( Parent->ComposeDbFn(DbExtCentroid) );
  STRING      out_fn (fn);

  if (!Exists(fn))
    {
      // Compressed extensions
      const char * const exts[] = { ".bz2", ".z", ".Z", NULL };
      for (size_t i=0; exts[i]; i++)
        {
          fn = out_fn + exts[i];
          if (FileExists(fn))
            break;
        }
      if (!FileExists(fn))
        fn = Parent->ComposeDbFn( DbExtCentroidCompressed );
    }
  // Do we really need to generate it?
  if (Exists(fn))
    {
      // Compare the dates.
      SRCH_DATE   centroid_mod, db_mod;
      if (centroid_mod.SetTimeOfFile(fn) && db_mod.SetTimeOfFile( Parent->ComposeDbFn(DbExtMdt)))
        {
          if (db_mod <= centroid_mod)
            {
              // The centroid is newer than the database so its done..
              message_log (LOG_INFO, "Existing Centroid \"%s\" is up-to-date.", fn.c_str());
              return true;
            }
        }
      if (fn != out_fn)
        UnlinkFile(fn); // Remove the compressed version
    }
  FILE *fp = fopen(out_fn, "w");
  if (fp)
    {
      message_log (LOG_INFO, "Writing Centroid to \"%s\".", out_fn.c_str());
      result = WriteCentroid(fp);
      fclose(fp);
    }
  return result;
}


bool INDEX::WriteCentroid(FILE* fp)
{
  int       len, pos, old_pos = 0;
  const int maxCompLength = StringCompLength*4 + 4096;

  int compLength = maxCompLength;

  int count = 0, charset;
  FILE *inp;
  char tmp[256];

  // Merge
  MergeIndexFiles();
  const size_t sub_indexes = GetIndexNum(); // should be <=1 but who knows...
  STRING sis;

  // Write Header
  fprintf(fp, "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>\n\
          <!-- generator=\"IB %s %s\" -->\n\
          <!DOCTYPE Locator SYSTEM \"centroid.dtd\">\n<centroid \
src=\"%s\" version=\"0.1\">\n",
          __IB_Version,  __HostPlatform,  Parent-> GetDbFileStem().c_str() );

  fprintf(fp, "<title>%s</title>\n", Parent->GetTitle().c_str());
  {
    STRING val;
    if (Parent->GetHTTP_server(&val))
      fprintf(fp, "<server>%s/%s</server>\n", val.c_str(),
		Parent->DbName().c_str());
    else
      fprintf(fp, "<server>%s</server>\n", Parent-> GetDbFileStem().c_str());
    // fprintf(fp, "<maintainer>%s</maintainer>\n", Parent->GetMaintainer().c_str());
    if (!(val = Parent->GetCopyright()).IsEmpty())
      fprintf(fp, "<copyright>%s</copyright>\n", val.c_str());
  }

  CHARSET Charset;
  BUFFER  buffer;
  char   *outword = (char *)(buffer.Want(1024));

  for (size_t i=1; i<= sub_indexes || (sub_indexes == 0 && i == 1); i++)
    {
      if (sub_indexes <= 1)
        sis = SisFileName;
      else
        sis.form("%s.%d", SisFileName.c_str(), i);

      if ((inp = ffopen(sis, "rb")) == NULL)
        continue;
      compLength = (unsigned char)fgetc(inp);
      charset = (unsigned char)fgetc(inp);
      outword = (char *)(buffer.Want(compLength*3 +1));

      // Write Words
      fprintf(fp, " <words slots=\"%lu\" slot_length=\"%d\" source_charset=\"%s\">\n",
              (unsigned long)(GetFileSize(inp)/(compLength+sizeof(GPTYPE)+1)),
              (int)compLength,
              Id2Charset(charset) );

      Charset.SetSet(charset);
      while ((len = fgetc(inp)) != EOF)
        {
          GPTYPE p;
          int    truncated;

          if ((fread(tmp, 1, compLength, inp) != compLength) ||
		 fread(&p, sizeof(GPTYPE), 1, inp) < 1) /* Position in .inx */
	    {
               message_log (LOG_ERROR, "Read error on %s. Centroid Truncated!", sis.c_str());
	       break;
	    }
          pos = GP(&p, 0);

          truncated = 0;
          if (len >= compLength)
            {
              SCANLIST ScanList; // Need to scan for truncated words
              tmp[len] = '\0';
              // Scan can fail if, for example, the sources have been removed
              truncated = Scan (&ScanList, NulString, tmp, -1, false,
                                /* Handle the specific subindex if given */
                                (sub_indexes <= 1) ? 0 : i );

              if (truncated)
                {
                  // Did we find something?
                  const atomicSCANLIST *sptr = ScanList.GetPtratomicSCANLIST();
                  for (const atomicSCANLIST *ptr = sptr->Next(); ptr!=sptr; ptr=ptr->Next())
                    {
// IDEA: Instead of <word freq=number> term </> .. 
// we do <word> term <freq>number </freq></word>
                      STRING Term (ptr->Term());
                      outword = (char *)(buffer.Want(Term.GetLength()*3 + 1));
                      fprintf(fp, "  <word freq=\"%ld\">%s</word>\n",
                              (long)(ptr->Frequency()), Charset.ToUTF(outword, Term ) );
                      count++;
                    }
                }
              else tmp[len++] = '*'; // Write as Right truncated word
            }
          if (truncated == 0)
            {
              outword = (char *)(buffer.Want(len*3 + 1));
              tmp[len] = '\0';
              if (pos <= old_pos && pos > 0)
                {
#ifdef WIN32
		  message_log (LOG_PANIC, "SIS '%s' corrupt! Gp %I64x < %I64x",
                        sis.c_str(), (UINT8)pos, (UINT8)old_pos);

#else
                  message_log (LOG_PANIC, "SIS '%s' corrupt! Gp %llx < %llx",
                        sis.c_str(), (long long)pos, (long long)old_pos);
#endif
                }
              else
                {
                  fprintf(fp, "  <word freq=\"%d\">%s</word>\n",
                          old_pos ? (int)(pos - old_pos) : pos +1,
                          Charset.ToUTF(outword, tmp));
                  count++;
                }
            }
          old_pos = pos;
        }
      ffclose(inp);
      fprintf(fp, " </words>\n");
    }

  // Dump fieldnames and types

  DFDT *Dfdtptr = Parent->GetDfdt ();

  if (Dfdtptr)
    {
      const size_t fields = Dfdtptr->GetTotalEntries();
      if (fields)
        {
          DFD Dfd;
          fprintf(fp, " <fields count=\"%d\">\n", (int)fields);
          STRLIST  sList;
          SCANLIST scanList;
          STRING   fieldName;
          FIELDTYPE fieldType;
          size_t   count;
          for (size_t x=1; x<=fields; x++)
            {
              Dfdtptr->GetEntry(x, &Dfd);
              fieldName = Dfd.GetFieldName();
              fieldType = Dfd.GetFieldType();
              STRING typeName ( fieldType.c_str() );

              sList.Clear();
              scanList.Clear();
              if (fieldType.IsString()) // IsText etc.
                {
                  count = Scan(&scanList, fieldName, 0, INT_MAX/2, 0);
                  const atomicSCANLIST *sptr = scanList.GetPtratomicSCANLIST();

                  fprintf(fp, "  <%s type=\"string\" elements=\"%u\">\n",
                          fieldName.c_str(), (unsigned)count);
                  for (const atomicSCANLIST *ptr = sptr->Next(); ptr!=sptr; ptr=ptr->Next())
                    fprintf(fp, "    <word freq=\"%u\">%s</word>\n",
                            (unsigned)(ptr->Frequency()), Charset.ToUTF(outword, ptr->Term() ) );
                  fprintf(fp, "  </%s>\n", fieldName.c_str());
                  continue;
                }
              else switch ( fieldType)
                  {
                  case FIELDTYPE::unknown:
                    fprintf(fp, "  <%s type=\"%s\" />\n", fieldName.c_str(), fieldType.c_str());
                    continue;
                  case FIELDTYPE::date:
                    count = DateScan(&sList, fieldName);
                    break;
                  case FIELDTYPE::numerical:
                    count = NumericalScan(&sList, fieldName);
                    break;
                  case FIELDTYPE::gpoly:
                    count = GPolyScan(&sList, fieldName);
                    break;
                  default:
                    count = Scan(&sList, fieldName, 0, INT_MAX/2, 0);
                    break;
                  }
              fprintf(fp, "  <%s type=\"%s\" elements=\"%u\">\n",
                      fieldName.c_str(), typeName.c_str(), (unsigned)count);
              for (STRLIST *lptr = sList.Next(); lptr != &sList; lptr = lptr->Next())
                fprintf(fp, "\t<%s>%s</%s>\n",
                        typeName.c_str(),
                        lptr->Value().c_str(),
                        typeName.c_str());
              fprintf(fp, "  </%s>\n", fieldName.c_str());
            }
          fprintf(fp, " </fields>\n");
        }
    }
  fprintf(fp, "</centroid>\n");
  return count != 0;
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

  do
    {
      tmp[i++] = chars[ val % modulo ];
      val /= modulo;
    }
  while (val);
  for (j =i; j < pad; j++)
    tmp[j] = '0';
  for (j = 0; j < pad; j++)
    ptr[j] = tmp[pad -  j - 1];
  ptr[j] = '\0';
  return ptr;
}
#endif


// Main Indexing Loop
bool INDEX::AddRecordList (FILE *RecordListFp)
{
  if (ActiveIndexing)
    {
      Parent->SetErrorCode(29); // Locked db
      message_log (LOG_ERROR, "Indexing already running on '%s' (Indexing or Importing).",
	Parent->GetDbFileStem ().c_str());
      return false;
    }
  if (RecordListFp == NULL)
    {
      message_log (LOG_PANIC, "INDEX::AddRecordList(<NULL>)!");
      return false;
    }
  else
    {
      // Sanity checks
      off_t qlen = ::GetFileSize(RecordListFp);
      off_t pos  = ftell(RecordListFp);

      if (qlen < (off_t)(sizeof(RECORD)/2))
        {
          message_log (LOG_WARN, "Record index queue empty (len=%d). Nothing to do!", (int)qlen);
          return true; // Nothing to do
        }
      else if ((qlen - pos) < (off_t)(sizeof(RECORD)/2))
        {
          message_log (LOG_INFO, "Nothing new to process in record index queue!");
          return true;
        }
      if (DebugMode)
        message_log (LOG_INFO, "Adding to Index from Queue Stream (%lu bytes) from byte %lu",
              (unsigned long)qlen, (unsigned long)pos);
    }

  UINT4        IndexingMemory = (UINT4)
                                ((3*(long long)(Parent->GetIndexingMemory ())/(3+sizeof(GPTYPE))));
  int          tries = 0;
  UCHR        *MemoryData = NULL;
  GPTYPE       MemoryDataLength = 0;
  GPTYPE      *MemoryIndex;

memory_allocation: // This is where we try to get memory

  if (++tries > 4)
    {
      Parent->SetErrorCode(2); // "Temporary system error";
      message_log (LOG_PANIC, "Can't get indexing memory. Giving up.");
      return false;
    }
  IndexingMemory /= tries;


  // Reserve StringCompLength EXTRA
  const UINT4 DataMemorySize = IndexingMemory - StringCompLength - 1;
  const UINT4 MemorySizeKb = sizeof(UCHR)*DataMemorySize/1024;

  // Was DEBUG
  if (DebugMode || (tries>1))
    message_log (LOG_INFO, "MEMORY: Want to allocate %ld%cB (%lu of %lu pages) for read buffer.",
          MemorySizeKb < 1024 ? (long)MemorySizeKb : (long)MemorySizeKb/1024L,
          MemorySizeKb < 1024 ? 'K' : 'M',
          (unsigned long) DataMemorySize/PAGESIZE,
          (unsigned long)_IB_GetFreeMemory()/PAGESIZE );
  try
    {
      MemoryData = new UCHR[IndexingMemory];
    }
  catch (...)
    {
      MemoryData = NULL;
    }
  if (MemoryData == NULL)
    {
      message_log (LOG_WARN|LOG_ERRNO, "Could not allocate %ldk bytes for read buffer",
            (unsigned long)((IndexingMemory*sizeof(UCHR))/1024L));
      goto memory_allocation;
    }
  memset(MemoryData, 0, IndexingMemory*sizeof(UCHR));

  // Assume average wordlength is 3 or more characters (leaving off the single
  // letter words)
  UINT4 IndexMemorySize = (UINT4) ((IndexingMemory / 3) -1); // jw patch

  if (DebugMode || (tries>1))
    message_log (LOG_INFO, "MEMORY: Want to allocate %ldk term pointers (from %lu pages available memory).",
          IndexMemorySize/1024, (unsigned long)_IB_GetFreeMemory()/PAGESIZE );
  try
    {
      MemoryIndex = new GPTYPE[IndexMemorySize]; // jw patch
    }
  catch (...)
    {
      MemoryIndex = NULL;
    }
  if (MemoryIndex == NULL)
    {
      if (MemoryData)
	{
	  delete[] MemoryData;
	  MemoryData = NULL;
	}
      message_log (LOG_WARN|LOG_ERRNO, "Could not allocate %ld %d-byte term pointers (%ld Mb)",
	IndexMemorySize, sizeof(GPTYPE), (IndexMemorySize*sizeof(GPTYPE))/(1024L*1024L));
      goto memory_allocation;
    }

  message_log (tries > 1 ? LOG_NOTICE : LOG_DEBUG,
	"MEMORY:%luMB allocated. System has %lu Pages core still available.",
        (long)((IndexingMemory + (sizeof(GPTYPE)*(long long)IndexMemorySize))/(1024*1024)) ,
        (unsigned long)_IB_GetFreeMemory()/PAGESIZE);

  INT FirstRecord = 1;
  INT CurrentRecord;
  INT MemoryIndexLength;
  RECORD record;
  STRING DataFileName;
  STRING OldDataFileName;
  STRING MdtrecFilePath;
  MDTREC mdtrec ( Parent->GetMainMdt() );
  // MDTREC lastmdtrec (MainMdt);

  INT Error;
  GPTYPE Offset = 0;
  GPTYPE endOffset = 0;
  GPTYPE oldEnd = 0;
  GPTYPE TrueGlobalStart = 0;
  GPTYPE OldGlobalStart;
  GPTYPE RecordStart, RecordEnd;

#if 1
  INODE     inode;
#else
  long  host = _IB_Hostid();
  ino_t st_ino = 0;
  int   st_dev = -1;  
#endif

  bool Break    = false;
  bool Ok       = true;
  bool checkKey = (Parent->GetMainMdt()->GetTotalEntries () != 0);
  bool isLinked = false;
  bool Override =  Parent->GetOverride();
  MMAP MyMemoryMap;

  {
    STRING s;
    int    id = (BYTE)GetGlobalCharset(&s);
    message_log (LOG_DEBUG, "Indexing using Charset (#%d) %s", id, s.c_str());
  }


  ActiveIndexing = true;

  for (;;)
    {
      int RecordFlag = BOUNDARY; // In case empty queue

      CurrentRecord = FirstRecord;
      Error = 0;
      MemoryDataLength = 0;
      MemoryIndexLength = 0;

      // TODO:: Add a byte counter to give some running status counts of bytes
      // indexed

      OldGlobalStart = Parent->GetMainMdt()->GetNextGlobal();

      FCT Regions;
      DOCTYPE_ID Doctype;
      STRING Key;

      for (;;)
        {

          //
          // NOTE:  We could make ParseFields in the doctype take a buffer
          // this would save a reading but since we memory map it should
          // not make a difference. We also don't need or want entity resolution
          // for the field parser BUT its needed by the word parsers!
          //
          if (Break || ((RecordFlag = getc(RecordListFp)) == BOUNDARY))
            {
              static const char skipping_msg[] =
#ifdef _WIN32
			"Skipping %s %s [%I64d-%I64d]";
#else
			"Skipping %s %s [%lld-%lld]";
#endif
              if (!Break)
                {
                  if (record.Read (RecordListFp) == false)
                    {
                      message_log (LOG_PANIC, "Read error on indexer input record queue");
                      break;
                    }
                }
#if 0 /* APRIL 2008 // Temp can' do this   */
              if (record.IsBadRecord())
                {
                  if (DebugMode || record.GetDocumentType().IsDefined())
                    message_log(LOG_INFO, skipping_msg,
                         record.GetDocumentType().ClassName(false).c_str(),
                         record.GetFullFileName ().c_str(),
                         (long long)record.GetRecordStart (), (long long)record.GetRecordEnd ());
                  continue;
                }
#endif
              DataFileName = record.GetFullFileName ();

              if ((Doctype = record.GetDocumentType()) == NulDoctype)
                {
                  message_log(LOG_INFO, skipping_msg, "", DataFileName.c_str(),
                       (long long)record.GetRecordStart (), (long long)record.GetRecordEnd ());
                  continue; // Skip NULL docs
                }
              if (Doctype.Name == ZeroDoctype)
                {
                  if (DebugMode) message_log(LOG_INFO, skipping_msg, "",
                                        DataFileName.c_str(),
                                        (long long)record.GetRecordStart (), (long long)record.GetRecordEnd ());
                  continue; // Its also a NULL
                }

              Break = false;

              if (DebugMode)
                message_log(LOG_DEBUG, "Parse %s fields in %s", Doctype.Name.c_str(), DataFileName.c_str());

              Parent->IndexingStatus (IndexingStatusParsingRecord, DataFileName);
              Parent->ParseFields (&record);

              Doctype = record.GetDocumentType(); // Parsefield can change the doctype!

              Parent->SelectRegions (record, &Regions);
              if (DataFileName != OldDataFileName)
                {

// cerr << "OldDataFileName=" << OldDataFileName << endl;
//cerr << "DataFileName   =" << DataFileName << endl;

                  Parent->IndexingStatus (IndexingStatusIndexingDocument, DataFileName);
                  MyMemoryMap.CreateMap(DataFileName, MapSequential);
                  if ((Ok = MyMemoryMap.Ok()) == false)
                    message_log (LOG_ERRNO|LOG_INFO, "Error accessing/mapping %s, skipping..", DataFileName.c_str());
                  Parent->ffdispose(OldDataFileName); // Don't cache anymore...
                  OldDataFileName = DataFileName;

//cerr << "OldDataFileName=" << OldDataFileName << endl;

                  // What is the Pathname for the MDTREC?
                  MdtrecFilePath = (Parent->getUseRelativePaths() ?
                                    Parent->RelativizePathname(DataFileName) :
                                    DataFileName);
#if 0
                  // @@@@ Bug fix
                  if (MemoryDataLength) MemoryDataLength++;
                  // @@@@ End
#endif
                  TrueGlobalStart = Parent->GetMainMdt ()->GetNextGlobal ();

                  RecordStart = (Offset = record.GetRecordStart ());
                  RecordEnd   = (oldEnd = record.GetRecordEnd());
                  endOffset = 0;
                  // Use inode information
		  inode.Set(DataFileName);
		  isLinked = inode.isLinked();
		  //key = inode.Key();
                }
              else
                {
                  if ((RecordStart = record.GetRecordStart()) < oldEnd)
                    {
                      message_log (LOG_DEBUG, "Old End %lu > Start %lu",
                            (unsigned long)oldEnd, (unsigned long)RecordStart);
                      endOffset = 0;
                    }
                  else
                    {
                      endOffset = RecordStart - oldEnd - 1;
                    }
                  RecordEnd = oldEnd = record.GetRecordEnd();
                }
              if (!Ok)
                {
                  CurrentRecord++;
                }
              else
                {
#if 0
                  for (const FCLIST *r = Regions, *itor = r->Next(); itor != r; itor = itor->Next())
                    {
                      Fc = itor->Value();
                      if (Fc.GetLength())
                        {
                          const GPTYPE DataFileSize = Fc.GetFieldEnd();
                          if (DataFileSize == 0)
                            continue;
                          const GPTYPE start = record.GetRecordStart() + Fc.GetFieldStart();
                          const GPTYPE end   = start + DataFileSize;
                        }
                    }
#endif

                  const GPTYPE DataFileSize = ((RecordEnd == 0) ?
                                               MyMemoryMap.Size() - 1 /* added EDZ Sun Nov 16 --> */ - RecordStart:
                                               RecordEnd - RecordStart ) + 1;
                  if (DataFileSize == 0)
                    continue;
                  if (DataFileSize > DataMemorySize)
                    {
                      // This should NEVER happen since we now adjust the
                      // memory in "idb.cxx" to be at least as large as
                      // the largest record!
                      message_log (LOG_PANIC, "Internal Buffers too small (%ld>%ld)!",
                            (long)DataFileSize, (long)DataMemorySize);
                      exit (1);
                    }
                  if ((DataFileSize + MemoryDataLength) > DataMemorySize)
                    {
                      Break = true;
                      break;
                    }
                  size_t bytes = DataFileSize;

                  if (bytes > (MyMemoryMap.Size() - RecordStart)) 
                    {
		      // Dec 2007: Added test to make sure that the API was correctly used.
		      // Len-1 and not Len. This is not really an error but not right either! 
                      if ((bytes = MyMemoryMap.Size() - RecordStart) == DataFileSize-1 && bytes > 0)
			{
			  if (RecordEnd)
			    message_log (LOG_NOTICE, "Record End position set to length, should be length-1!");
			  else
			    message_log (LOG_NOTICE, "Can't read last byte of '%s' (read %lu of %lu bytes from %lu)?",
				DataFileName.c_str(),
				bytes, (unsigned long)DataFileSize, (unsigned long)RecordStart);
			}
		      else
			{
			  // This is an error condition
                          message_log (LOG_NOTICE|LOG_ERROR, "Wanted %ld bytes but only %u bytes availble from '%s'(%ld-%ld)",
                            (long)DataFileSize, bytes, DataFileName.c_str(), (long)RecordStart, (long)RecordEnd);
			  Error++; // Tag as error?
			}
                    }
//cerr << "MAP from " << RecordStart << endl;
                  PUCHR p = (PUCHR)memcpy(MemoryData + MemoryDataLength,
                                          MyMemoryMap.Ptr() + RecordStart, bytes);
                  if (DebugMode) message_log (LOG_DEBUG, "Copied %u bytes from '%s' (%lu)", bytes,
                                         DataFileName.c_str(), (unsigned long)RecordStart );
                  memset(p+bytes, ' ', StringCompLength); // Pad an EXTRA StringCompLength

                  // This is where the buffers get indexed...
                  const size_t GpListSize = BuildGpList (
                                              Doctype,
                                              MemoryDataLength,
                                              MemoryData,
                                              MemoryDataLength + bytes,
                                              MemoryIndex + MemoryIndexLength,
                                              IndexMemorySize - MemoryIndexLength);
#if 0
                  if (GpListSize == 0 && Doctype.)
                    {
                      message_log (LOG_WARN, "No words were indexed in %s record '%s'(%ld-%ld) bytes. Skipping!",
                            Doctype.Name.c_str(), DataFileName.c_str(), RecordStart, RecordStart+bytes);
                    }
                  else
#endif
                    if (endOffset <= TrueGlobalStart)
                      {
                        if (DebugMode) message_log (LOG_DEBUG, "Indexed %d words (%s)", GpListSize, Doctype.c_str());
                        mdtrec.SetDocumentType ( Doctype );
                        if (record.IsBadRecord())
                          mdtrec.SetDeleted( true);
                        else
                          mdtrec.SetDeleted( (Doctype.Name == NilDoctype) || GpListSize == 0); /* KLUDGE ALERT */
                        mdtrec.SetLocale  ( record.GetLocale()   );
                        mdtrec.SetPriority ( record.GetPriority() );
                        mdtrec.SetCategory ( record.GetCategory() );
//cerr << "***** DEBUG: Setting MDTREC fullfilename = " <<  MdtrecFilePath << endl;
                        mdtrec.SetFullFileName (  MdtrecFilePath );

#if __DEBUG__
                         /cerr << "...... " << endl;
#endif

			mdtrec.SetOrigPathname ( record.GetOrigPathname() ); // New June 2008
//cerr << "### Record Orig = " <<  record.GetOrigPathname() << endl;
                        mdtrec.SetLocalRecordStart (RecordStart);
                        mdtrec.SetLocalRecordEnd ( (RecordStart == 0) && (RecordEnd == 0) ?
                                                   bytes - 1 :
                                                   RecordEnd );
                        mdtrec.SetGlobalFileStart (TrueGlobalStart - endOffset); // Minus endOffset!!!
                        mdtrec.SetDate (record.GetDate ());	// Date
                        mdtrec.SetDateModified (record.GetDateModified ());       // Date
                        mdtrec.SetDateCreated (record.GetDateCreated ());       // Date

                        // Something that needs to be added somewhere:
                        // If record already contains a user-defined key,
                        // we need to make sure that it is unique!
                        if (record.GetKey(&Key).IsEmpty())
                          {
                            // Create our own network independent handle
                            // [This might not always work]
			    Key = inode.Key(RecordStart, RecordEnd);
			    if (DebugMode)
			      {
				if (Parent->GetMainMdt()->LookupByKey(Key))
				  {
				    message_log (LOG_PANIC, "Duplicate KEY '%s'. Inode defective?", Key.c_str());
				    checkKey = true;
				  }
				else
			          message_log (LOG_DEBUG, "Created key '%s'", Key.c_str());
			      }
                          }
                        else
                          checkKey = true;
                        if (checkKey || isLinked)
                          {
                            if (DebugMode)
                              message_log(LOG_DEBUG, "%s Key '%s'", Override ? "Setting"  : "Getting Unique", Key.c_str());
                            Parent->GetMainMdt ()->GetUniqueKey (&Key, Override);
                          }

                        mdtrec.SetKey (Key);
                        MemoryDataLength += bytes;
                        MemoryIndexLength += GpListSize;

                        const GPTYPE position = mdtrec.GetGlobalFileStart () + mdtrec.GetLocalRecordStart ();

                        const size_t nr = Parent->GetMainMdt ()->AddEntry (mdtrec);

                        // We actually want to give the key here
                        Parent->IndexingStatus (IndexingStatusRecordAdded, Key, nr);

                        if (WriteFieldData (record, position) // edz: moved position to *before* AddEntry()
                            == false)
                          {
                            message_log (LOG_ERROR, "Errors writing field data!");
                            Error++;
                          }
                      }
                    else
                      {
                        message_log (LOG_ERROR|LOG_PANIC,
#ifdef _WIN32
			"Internal [%s] record overflow (%I64d > %I64d) [%I64d-%I64d]."
#else
			"Internal [%s] record overflow (%lld > %lld) [%lld-%lld]."
#endif
                              , Doctype.Name.c_str(),
                              (long long)endOffset,
                              (long long)TrueGlobalStart,
                              (long long)RecordStart,
                              (long long)RecordEnd);
                        // Error++;
                      }
                  CurrentRecord++;
                }
              if (Break) break; // Pass Done
            }
          else break; // Pass Done
        } // for ()

      if (Error == 0)
        {
          Parent->IndexingStatus (IndexingStatusIndexing, MemoryIndexLength);
          if (MemoryIndexLength)
            {
//	      clock_t start    = clock();
              TermSort (MemoryData, MemoryIndex, MemoryIndexLength);
//	      clock_t end      = clock();
//	      static const double factor = sqrt(CLOCKS_PER_SEC);
//	      message_log (LOG_INFO, "Processed %.2f words/s (%.4f) CPU.",
//		MemoryIndexLength*factor/(end/factor - start/factor), (end-start)/(double)CLOCKS_PER_SEC);

              if (false == FlushIndexFiles(MemoryData,
                                               MemoryIndex,
					       MemoryIndexLength,
					       OldGlobalStart /* Added EDZ Sun Nov 16 --> */ + Offset))
                {
                  Error++; // ERROR
                }
              else if ( MergeStatus==iMerge && GetIndexNum() > 1) // Merge while indexing
                MergeIndexFiles(); // Merge while we work..
            }
        }
      FirstRecord = CurrentRecord;

      if (RecordFlag != BOUNDARY || Error) break; // Done
    } // for (;;)
  Parent->ffdispose(DataFileName); // Don't cache anymore...

#if __DEBUG__
  Parent->GetMainFpt()->Dump();
#endif

  if (MemoryData)  { delete[]MemoryData; MemoryData = NULL; }
  if (MemoryIndex) { delete[]MemoryIndex; MemoryIndex = NULL; }

  if (DebugMode) message_log (LOG_INFO, "Sort numeric field data...");
  SortNumericFieldData();

  // now, do our *experimental* merge
  if ((IndexNum = GetIndexNum()) == 1)
    {
      STRING s;

      // Move .inx.1 to .inx
      s.form("%s.%d", IndexFileName.c_str(), IndexNum);
      if (RenameFile(s, IndexFileName) == 0)
        {
          if (DebugMode) message_log(LOG_DEBUG, "Moved %s to %s", s.c_str(), IndexFileName.c_str());
          SetIndexNum(0);
          // Move the .sis.1 file...
          if (RenameFile( s.form("%s.1", SisFileName.c_str()), SisFileName) == 0)
            {
              if (DebugMode) message_log(LOG_DEBUG, "Moved %s to %s", s.c_str(), SisFileName.c_str());
            }
          else
            message_log(LOG_ERRNO, "Could not move %s to %s", s.c_str(), SisFileName.c_str());
        }
      else
        {
          message_log (LOG_ERRNO, "Could not move %s to %s", s.c_str(), IndexFileName.c_str());
        }
    }
  else if (MergeStatus==iOptimize)
    MergeIndexFiles();
  else if (MergeStatus==iCollapse || (MergeStatus==iIncremental && IndexNum > 2))
    CollapseIndexFiles();
  else if (IndexNum > 0 && Exists(IndexFileName))
    {
      if (DebugMode) message_log(LOG_DEBUG, "Removing '%s'", IndexFileName.c_str());
      if (UnlinkFile(IndexFileName) == -1) // Remove .inx
        message_log (LOG_ERRNO, "Could not remove '%s' (old Index)", IndexFileName.c_str());
      if (Exists(SisFileName))
        {
          if (DebugMode) message_log(LOG_DEBUG, "Removing '%s'", SisFileName.c_str());
          if (UnlinkFile(SisFileName) == -1) // Zap old..
            message_log (LOG_ERRNO, "Could not remove '%s' (old Sis)", SisFileName.c_str());
        }
    }

  ActiveIndexing = false;
  return Error? false : true;
}

// Ship a "" to flush..
#define _SisFlush(dp) SisWrite("", dp)
INT INDEX::SisWrite(const STRING Str, FILE *dp)
{
  if (SIScount++ == 0 || lastSIString.GetLength() == 0)
    {
      lastSIString = Str;
      return 0;
    }

  if (Str.GetLength() == 0 ||
      SisCompare((const UCHR *)lastSIString, (const UCHR *)Str, SisLimit) < 0)
    {
      size_t len;
      const unsigned char *ptr = (const unsigned char *)lastSIString;
      for (len = 0; len < SisLimit; len++)
        {
          if (!IsTermChr(ptr+len))
            break;
        } // for
      putc(len & 0xFF, dp);
      // Write SisLimit so that we can bsearch on it..
      fwrite(ptr, sizeof(UCHR), SisLimit, dp);
      ::putGPTYPE (SIScount-1, dp); // Write which SIS word..
      lastSIString = Str;
      return 1;
    }
  return 0;
}

#ifdef _WIN32
#define getpid (int)GetCurrentProcessId  /* cast DWORD to int */
#endif


/* NEW INDEXING STRATEGY */
bool INDEX::CollapseIndexFiles()
{
  const int mypid = getpid();

  const INT LocalIndexNum=GetIndexNum();
  if (LocalIndexNum <= 1) return true; // Done;

  const size_t First=LocalIndexNum-1;
  const size_t Second=LocalIndexNum;

  message_log(LOG_INFO, "Collapsing Sub-Indexes %u and %u -> %u", (unsigned)First, (unsigned)Second, (unsigned)First);

  STRING TmpIndexFile, TmpSisFile, OutFile;
  STRING Current;

//  Parent->IndexingStatus(IndexingStatusMerging);

  MMAP SisFiles[2];
  MMAP InxFiles[2];

  size_t maxSisLength = 0;

  unsigned char *Inx_buff[2];
  struct
    {
      const unsigned char   *buf;
      int                    dsiz;
      size_t                 compLength;
      size_t                 total; // Number of elements
      size_t                 length; // Length of buffer
      BYTE                   charsetId;
    } SisInfo[2];

  {
    STRING s;
    unsigned long long total_words = 0;

    for (size_t j=0, i=First; i<=Second; i++)
      {
        // MAP Inx
        if (InxFiles[j].CreateMap(s.form("%s.%d", IndexFileName.c_str(), i), MapSequential) == false)
          {
            // Can't map
            // Could call there some alternative method to fold via
            // file I/O
            // -- first need to see that search can use alternative file I/O.
          }
        if ((Inx_buff[j] = InxFiles[j].Ptr()) == NULL)
          {
            message_log (((GetFileSize(s) >  (0x7fffffff-100)) ? LOG_INFO : LOG_ERRNO),
                  "CollapseIndexFiles: Could not Map '%s'. Won't collapse %d+%d->%d.",
                  s.c_str(), First, Second, First);
            return false;
          }
        total_words += ((GPTYPE)InxFiles[j].Size())/sizeof(GPTYPE);

        if (DebugMode) message_log (LOG_DEBUG, "Mapped %s", s.c_str());
        // MAP SIS
        s.form("%s.%d", SisFileName.c_str(), i);
        SisFiles[j].CreateMap(s, MapSequential);
        const unsigned char *Map = SisFiles[j].Ptr();
        if (Map == NULL)
          {
            message_log (LOG_ERRNO, "Collapse: Could not Map '%s'", s.c_str());
            return false;
          }
        if (DebugMode) message_log (LOG_DEBUG, "Mapped %s", s.c_str());

        const size_t compLength = (unsigned char)Map[0];
        //size of a sistring record
        const size_t dsiz = compLength + sizeof(GPTYPE)+1;
        SisInfo[j].charsetId = (BYTE)Map[1];
        SisInfo[j].buf       = Map + 2;
        SisInfo[j].compLength= compLength;
        SisInfo[j].dsiz      = dsiz;
        SisInfo[j].total     = (SisFiles[j].Size() - 2)/ dsiz;
        SisInfo[j].length  = (SisInfo[j].total)*dsiz;
        j++;

        if (compLength > maxSisLength)
          maxSisLength = compLength;
      } // for()
    if (total_words >=  maxWordsCapacity)
      {
#ifdef _WIN32
        message_log (LOG_ERROR, "Sorry can't merge %I64u words (Capacity with %d-bit files systems is %I64u)" 
              , (UINT8)total_words, sizeof(GPTYPE) *8, (UINT8)maxWordsCapacity);
#else
	message_log (LOG_ERROR, "Sorry can't merge %I64u words (Capacity with %d-bit files systems is %I64u)"
		,total_words, sizeof(GPTYPE) *8, maxWordsCapacity);
#endif
        return false;
      }
#ifdef _WIN32
    message_log (LOG_INFO, "Merging %I64u words from %u indexes.", (UINT8)total_words, Second-First+1);
#else
    message_log (LOG_INFO, "Merging %llu words from %u indexes.", total_words, Second-First+1);
#endif

  }
  if (SisInfo[0].charsetId != SisInfo[0].charsetId)
    {
      message_log (LOG_WARN, "Can't collapse sub-indices with different base charsets (%d != %d) at this time.",
            SisInfo[0].charsetId, SisInfo[1].charsetId);
      return false;
    }

  // Name of the Temp Index (for output)
  TmpIndexFile.form("%s.%d.%d", IndexFileName.c_str(), First, mypid);
  // Name of the Temp Sis (for output)
  TmpSisFile.form("%s.%d.%d", SisFileName.c_str(), First, mypid);

  FILE *inx_fp=fopen(TmpIndexFile,"wb");
  if (inx_fp == NULL)
    {
      message_log (LOG_ERRNO, "Couldn't open '%s' for writing (index).", TmpIndexFile.c_str());
      return false;
    }

  FILE *sis_fp = fopen(TmpSisFile, "wb");
  if (sis_fp == NULL)
    {
      fclose(inx_fp);
      if (UnlinkFile(TmpIndexFile) == -1) // Remove trash
        message_log (LOG_ERRNO, "Could not remove '%s' (Trash)", TmpIndexFile.c_str());
      message_log (LOG_ERRNO, "Couldn't open '%s' for writing (sis).", TmpSisFile.c_str());
      return false;

    }
  SetGlobalCharset( SisInfo[0].charsetId );

  fputc(maxSisLength & 0xFF, sis_fp);
  fputc(SisInfo[0].charsetId, sis_fp); // CHARACTER SET
  // Write Magic to index
  putINT2((INT2)IndexMagic, inx_fp);

  // Loop through
  long sis_count = 0;
  size_t pos1 = 0, pos2 = 0;
  const unsigned char *term1 = NULL;
  const unsigned char *term2 = NULL;

  if (DebugMode) message_log (LOG_DEBUG, "Dumping to (sis/inx).%d.%d", First, mypid);
  if (CommonWords) CommonWords->Clear();
  for (;;)
    {
      const unsigned char  *ptr = (const unsigned char *)""; // ="" just to keep lint quiet
      int                   dif = 0;
      size_t                add = 0;

      if (pos1 >= SisInfo[0].total)
        {
          dif = -1;
          term2 = SisInfo[1].buf + pos2*SisInfo[1].dsiz;
        }
      if (pos2 >= SisInfo[1].total)
        {
          if (dif == -1)
            break; // Done
          dif = 1;
          term1 = SisInfo[0].buf + pos1*SisInfo[0].dsiz;
        }
      if (dif == 0)
        {
          term1 = SisInfo[0].buf + pos1*SisInfo[0].dsiz;
          term2 = SisInfo[1].buf + pos2*SisInfo[1].dsiz;
          size_t len = *term1 > *term2 ? *term1 : *term2;
          dif =  SisCompare2(&term2[1], &term1[1], len);
        }
      if (dif >= 0)
        {
          // Copy Inx bits
          // Make sure not the first element...
          const GPTYPE start = ((long)term1 - (long)SisInfo[0].buf) ?
                               GP((unsigned char *)term1, -(int)sizeof(GPTYPE)) + 1 : 0;
          // Make sure not the last element...
          const GPTYPE end = GP(&term1[SisInfo[0].compLength+1], 0);

          if (PositionOf(end) > InxFiles[0].Size())
            {
              message_log(LOG_PANIC, "Collapse(1): Sub-Index %d is corrupt (overflow): %d (%ld > %ld) [%ld]",
                   First, start, PositionOf(end), InxFiles[0].Size(), end);
              add = 0;
            }
          else
            {
              const off_t entries = end-start+1;
              const off_t position = PositionOf(start);
              // Write the region of the .inx
              if (DebugMode) message_log (LOG_DEBUG, "Writing Index#1 %ld entries (%ld)", entries,  position);
              add = fwrite(Inx_buff[0]+position, sizeof(GPTYPE), entries, inx_fp);
              if ((off_t)add != entries)
                message_log(LOG_ERRNO, "Write error to %s (Temporary Index) (%ld!=%ld)",
                     TmpIndexFile.c_str(), (long)add, entries);
            }
#if XXX_DEBUG
# undef  XXX_DEBUG
# define XXX_DEBUG(X)	if (DebugMode) { static UCHR buffer[127]; \
	    message_log (LOG_INFO, "(%d) Start = %d, End = %d, Total = %d, TERM=%s First=%ld", \
		X, start, end, end-start+1, term##X+1, GP(Inx_buff[(X)-1], PositionOf(start)) ); \
	    for (GPTYPE i=0; i <= end-start; i++) { \
	      const GPTYPE off = PositionOf(start+i); \
	      const unsigned char *tp = Inx_buff[(X)-1]; \
	      const GPTYPE gp = GP(tp, off); \
	      const INT    got = GetIndirectBuffer (gp, buffer, 0, sizeof(buffer)/sizeof(UCHR)-1); \
	      if (got == 0) continue; \
	      if (got < 0) { \
		cerr << "READ ERROR!!!!! Can't get GP=" << gp << " Expected \"" <<  (char *)(term##X+1)  << "\"" << endl; \
		cerr << "Dump, i = " << (int)i << " end-start = " << (int)(end-start) << " off = " << off << endl; \
		abort();\
	      } \
	      TermExtract(buffer, (*term##X) + 1); \
	      if (strcmp( (char *) term##X+1, (char *)buffer) != 0) \
	      message_log (LOG_PANIC, "Expected %s at GP=%ld TERM = \"%s\" (Indirect Buffer)", \
		term##X+1, gp, (const char *)buffer); \
	    } }
#else
# undef  XXX_DEBUG
# define XXX_DEBUG(X)
#endif
          XXX_DEBUG(1);
          pos1++;
          ptr = term1;
        }
      if (dif <= 0)
        {
          // Copy Inx bit
          const GPTYPE end = GP(&term2[SisInfo[1].compLength+1], 0);
          // Make sure not the first element...
          const GPTYPE start = (term2 - SisInfo[1].buf) ? GP(term2, -(int)sizeof(GPTYPE)) + 1 : 0;
          if (2+end*sizeof(GPTYPE) > InxFiles[1].Size())
            {
              message_log(LOG_PANIC, "Collapse(2): Sub-Index %d is corrupt (overflow): %d", First, start);
            }
          else
            {
              const size_t entries = end-start+1;
              const off_t position = PositionOf(start);
              if (DebugMode)
                message_log (LOG_DEBUG, "Writing Index#2 %ld entries (%ld)", entries, position);
              size_t     wrote = fwrite(Inx_buff[1]+position, sizeof(GPTYPE), entries, inx_fp);
              if (wrote != entries)
                message_log(LOG_ERRNO, "Write error to %s (Temporary Index) (%ld!=%ld)",
                     TmpIndexFile.c_str(), (long)wrote, entries);
              add += wrote;
            }
          XXX_DEBUG(2);
          pos2++;
          if (dif != 0) ptr = term2;
        }
      if (add != 0)
        {
          fwrite(ptr, sizeof(UCHR), maxSisLength+1, sis_fp);
          sis_count += add;
          ::putGPTYPE (sis_count-1, sis_fp); // Write which SIS word..
          if (First<= 1 && add >  (size_t)CommonWordsThreshold)
            {
              if (CommonWords == NULL)
                CommonWords = new STRLIST();
              const char *word = (const char *)(ptr+1);
              CommonWords->AddEntry(word);
              message_log (LOG_INFO, "Term '%s' is common: frequency %u (>%d)!", word, add, CommonWordsThreshold);
            }
//	  if (DebugMode) message_log (LOG_DEBUG, "Added '%s'[%d] (%d)", word, *ptr, add);
        }
    }
  if (DebugMode) message_log (LOG_DEBUG, "%ld Terms dumped", sis_count);

  fclose(inx_fp);
  fclose(sis_fp);

  // We are finished and MS Windows really needs them unmaped before we can write!
  // in Unix it does not hurt.
  const char unmap_errmsg[] = "Could not unmap %s.%d";
  if (InxFiles[0].Unmap() == false) message_log (LOG_ERRNO, unmap_errmsg, IndexFileName.c_str(), First);
  if (SisFiles[0].Unmap() == false) message_log (LOG_ERRNO, unmap_errmsg, SisFileName.c_str(), First);
  if (InxFiles[1].Unmap() == false) message_log (LOG_ERRNO, unmap_errmsg, IndexFileName.c_str(), Second);
  if (SisFiles[1].Unmap() == false) message_log (LOG_ERRNO, unmap_errmsg, SisFileName.c_str(), Second);

  static const char err_msg[] =  "Could not remove '%s'(%s). Please remove by hand!";
  //////////////////////////////////////////////////////////////////////////
  // Decrement count ....
  //////////////////////////////////////////////////////////////////////////
  if ((LocalIndexNum == 2) && (LocalIndexNum==GetIndexNum()))
    {
      if (DebugMode) message_log(LOG_DEBUG, "Creating %s", IndexFileName.c_str());
      if (RenameFile(TmpIndexFile, IndexFileName) != 0) // Install new .inx
        {
          message_log(LOG_ERRNO, "Could not install '%'s as '%s'", TmpIndexFile.c_str(), IndexFileName.c_str());
          return false;
        }
      if (DebugMode) message_log(LOG_DEBUG, "Creating %s", SisFileName.c_str());
      if (RenameFile(TmpSisFile, SisFileName) != 0)
        {
          message_log(LOG_ERRNO, "Could not install '%s' as '%s'", TmpSisFile.c_str(), SisFileName.c_str());
          return false;
        }
      SetIndexNum(0);

      // clean up old files
      for (size_t i=First; i<=Second; i++)
        {
          // Zap .inx.*
          TmpIndexFile.form("%s.%d", IndexFileName.c_str(), i);
          if (DebugMode) message_log(LOG_DEBUG, "Deleting %s", TmpIndexFile.c_str());
          if (UnlinkFile(TmpIndexFile) == -1)
	    {
              message_log (LOG_ERRNO, err_msg, TmpIndexFile.c_str(), "Index(2)");
	      AddtoGarbageFileList(TmpIndexFile);
	    }
          // Zap .sis.*
          TmpIndexFile.form("%s.%d", SisFileName.c_str(), i);
          if (DebugMode) message_log(LOG_DEBUG, "Deleting %s", TmpIndexFile.c_str());
          if (UnlinkFile(TmpIndexFile) == -1)
	    {
              message_log (LOG_ERRNO, err_msg, TmpIndexFile.c_str(), "SIS(2)");
	      AddtoGarbageFileList(TmpIndexFile);
	    }
        }
    }
  else
    {
      // Zap .sis.
      STRING Tmp;

      Tmp.form("%s.%d", SisFileName.c_str(), First);
      if (DebugMode) message_log(LOG_DEBUG, "Creating collapsed %s", Tmp.c_str());
      if (RenameFile(TmpSisFile,Tmp) != 0)
        {
          message_log(LOG_ERRNO, "Could not install '%s' as '%s'", TmpSisFile.c_str(), Tmp.c_str());
          return false;
        }
      Tmp.form("%s.%d", SisFileName.c_str(), Second);
      if (DebugMode) message_log(LOG_DEBUG, "Deleting %s", Tmp.c_str());
      if (UnlinkFile(Tmp) == -1)
	{
          message_log (LOG_ERRNO, err_msg, Tmp.c_str(), "SIS");
	  AddtoGarbageFileList(Tmp);
	}

      Tmp.form("%s.%d", IndexFileName.c_str(), First);
      if (DebugMode) message_log(LOG_DEBUG, "Creating collapsed %s", Tmp.c_str());
      if (RenameFile(TmpIndexFile,Tmp) == 0)
        {
          if (LocalIndexNum==GetIndexNum())
            SetIndexNum(First); // Decrement
          Tmp.form("%s.%d", IndexFileName.c_str(), Second);
          if (DebugMode) message_log(LOG_DEBUG, "Deleting %s", Tmp.c_str());
          if (UnlinkFile(Tmp) == -1)
	    {
              message_log (LOG_ERRNO, err_msg, Tmp.c_str(), "Index");
	      AddtoGarbageFileList(Tmp);
	    }
        }
      else
        {
          message_log(LOG_ERRNO, "Could not install '%s' as '%s'", TmpIndexFile.c_str(), Tmp.c_str());
          return false;
        }
    }
  return true;
}

bool INDEX::MergeIndexFiles()
{
  if ((IndexNum=GetIndexNum()) < 1)
    {
      return true;
    }
  Parent->IndexingStatus (IndexingStatusMerging, IndexNum);

  FILE *fj;
  char hostname[256], tmp[256];

  if (getHostname (hostname, sizeof(hostname)-1) == -1)
    strcpy(hostname, "localhost");

  // Write own lock
  STRING lockName = IndexFileName + _lock;
  if ((fj = fopen(lockName, _mode_rt)) != NULL)
    {
      const STRING db = Parent->GetDbFileStem ();
      long pid = 0;
      long uid = 0;
      // Get PID
      tmp[0] = '\0';
      if (fscanf(fj, "%ld %ld %s\n", &pid, &uid, tmp) != 3)
        message_log (LOG_WARN, "Optimizer lockfile '%s' is corrupt!", lockName.c_str());
      fclose(fj);
      if (tmp[0] && strcasecmp(hostname, tmp) != 0 && strcmp(hostname, tmp) != 0)
        {
          message_log(LOG_ERROR, "\
               There might be a merge of %s running on %s (owned by uid=%ld and pid=%ld). If its a stale lock \
               you can remove it (%s) by hand.", db.c_str(), tmp, uid, pid, lockName.c_str());
          return false;
        }

      if (IsRunningProcess(pid))
        {
#ifdef _WIN32
          message_log(LOG_ERROR, "You already have a merge of %s running (pid=%ld)! Check locks",
               db.c_str(), pid);
#else
          if (IsMyProcess(uid))
            message_log(LOG_ERROR, "You already have a merge of %s running (pid=%ld)! Check locks",
                 db.c_str(), pid);
          else
            message_log (LOG_ERROR, "Index optimizer on %s already running by %s (pid=%ld). Check locks",
                  db.c_str(), ProcessOwner(uid).c_str(), pid);
#endif
          return false;
        }
      else if (pid)
        {
          // Stale Lock
#ifdef _WIN32
          message_log (LOG_WARN,  "An optimizer process broke down (pid=%ld). Index might be corrupt.", pid);
#else
          message_log (LOG_WARN, "An optimizer process run by %s broke down (pid=%ld). \
                Index might be corrupt.", ProcessOwner(uid).c_str(), pid );
#endif

          const INT y = GetIndexNum();
          for (INT i = 1; i <= y; i++)
            {
              STRING s;
              if (UnlinkFile (s = STRING().form("%s.%d.%d", IndexFileName.c_str(), i, pid) ) == -1)
                message_log (LOG_ERRNO, "Could not unlink '%s'", s.c_str());
              if (UnlinkFile (s = STRING().form("%s.%d.%d", SisFileName.c_str(), i, pid) ) == -1)
                message_log (LOG_ERRNO, "Could not unlink '%s'", s.c_str());
              errno = 0;
            }
        }
    }
  else if (FileExists(lockName))
    message_log (LOG_WARN, "Can't read optimizer lockfile '%s'.", lockName.c_str());
  if ((fj = fopen(lockName, "wt")) != NULL)
    {
#ifdef _WIN32
# define getuid(_x) 0
#endif
      fprintf(fj, "%ld %ld %s\n", (long)getpid(), (long)getuid (), hostname);
      fclose(fj);
    }
  else
    {
      message_log(LOG_WARN|LOG_ERRNO, "Can't set optimizer lock '%s'!", lockName.c_str());
    }

// message_log (LOG_INFO, "%d Sub-Indexes to Merge", IndexNum);
  // Collapse down to 1...
  do
    {
      if (CollapseIndexFiles() == false)
        break;
    }
  while ((IndexNum=GetIndexNum()) > 1);

  if (UnlinkFile(lockName) == -1) // Zap the lock!
    message_log (LOG_ERRNO, "Could not remove optimizer lock '%s'!", lockName.c_str());
  return true;
}				// end function

#if 0
// Private method:
//  -- Used by FlushIndexFiles(...)
//     and Import Index
//
STRING INDEX::GetFlushIndexSlot()
{
  STRING TmpIndexFileName;
  if (IndexNum == 0)
    {
      struct stat st_buf;
      // Appending to an optimized index?
      if (stat(IndexFileName.c_str(), &st_buf) == 0)
        {
          // Is it linked? st_nlink
          if (st_buf.st_nlink == 1)
            {
              // Back up .inx to .inx.1 and increment
              do
                {
                  TmpIndexFileName.form("%s.%d", IndexFileName.c_str(), ++IndexNum);
                }
              while (Exists(TmpIndexFileName));
              FileLink(IndexFileName, TmpIndexFileName); // Link it
              if (DebugMode) message_log(LOG_DEBUG, "Linked %s to %s", IndexFileName.c_str(), TmpIndexFileName.c_str());
              // Link .sis.*
              TmpIndexFileName.form("%s.%d", SisFileName.c_str(), IndexNum);
              FileLink(SisFileName, TmpIndexFileName);
            }
          else
            {
              message_log(LOG_NOTICE, "%s already linked (n=%d)?", IndexFileName.c_str(), st_buf.st_nlink);
            }
        }
    }
  // Look for a free slot (other indexer running?)...
  int tries = 0;
  do {
    TmpIndexFileName.form("%s.%d", IndexFileName.c_str(), ++IndexNum);
    tries++;
  } while (Exists(TmpIndexFileName));
  message_log ((tries > 2 && MergeStatus) ? LOG_WARN : LOG_DEBUG,
          "%d indexer slots were occupied on '%s'.%s", IndexNum, IndexFileName.c_str(),
  return TmpIndexFileName;
}

#endif

bool INDEX::FlushIndexFiles(PUCHR MemoryData,
                                   PGPTYPE NewMemoryIndex, GPTYPE MemoryIndexLength, GPTYPE GlobalStart)
{
  Parent->ffGC();
  Parent->IndexingStatus (IndexingStatusFlushing);
  // Open index file
  // in this new implementation, we will never do an in-line merge
  STRING TmpIndexFileName;
  if (IndexNum == 0)
    {
      struct stat st_buf;
      // Appending to an optimized index?
      if (stat(IndexFileName.c_str(), &st_buf) == 0)
        {
          // Is it linked? st_nlink
          if (st_buf.st_nlink == 1)
            {
              // Back up .inx to .inx.1 and increment
              do
                {
                  TmpIndexFileName.form("%s.%d", IndexFileName.c_str(), ++IndexNum);
                }
              while (Exists(TmpIndexFileName));
              FileLink(IndexFileName, TmpIndexFileName); // Link it
              if (DebugMode) message_log(LOG_DEBUG, "Linked %s to %s", IndexFileName.c_str(), TmpIndexFileName.c_str());
              // Link .sis.*
              TmpIndexFileName.form("%s.%d", SisFileName.c_str(), IndexNum);
              FileLink(SisFileName, TmpIndexFileName);
            }
          else
            {
              message_log(LOG_NOTICE, "%s already linked (n=%d)?", IndexFileName.c_str(), st_buf.st_nlink);
            }
        }
    }

  // Look for a free slot (other indexer running?)...
  {
    int tries = 0;
    do
      {
        TmpIndexFileName.form("%s.%d", IndexFileName.c_str(), ++IndexNum);
        tries++;
      }
    while (Exists(TmpIndexFileName));
    message_log ((tries > 2 && MergeStatus) ? LOG_WARN : LOG_DEBUG,
          "%d indexer slots were occupied on '%s'.%s", IndexNum, IndexFileName.c_str(),
          IndexNum > 2 ? " Unmerged or other indexer running?" : "");
  }

  if (DebugMode) message_log(LOG_DEBUG, "Dumping index to %s", TmpIndexFileName.c_str());

  PFILE fp = fopen(TmpIndexFileName, "wb");
  if (!fp)
    {
      message_log(LOG_ERRNO, "Can't write to %s!", TmpIndexFileName.c_str());
      return false;
    }
  // Mark index number for other processes (search engine would just skip it)
  SetIndexNum(IndexNum);

  // Dump out index
  // Write Magic to index
  putINT2((INT2)IndexMagic, fp);

  TmpIndexFileName.form ("%s.%d", SisFileName.c_str(), IndexNum);
  if (DebugMode) message_log (LOG_DEBUG, "Dumping SIStrings to '%s'", TmpIndexFileName.c_str());

  FILE *dp = fopen(TmpIndexFileName, "wb");
  if (dp == NULL)
    message_log (LOG_ERRNO, "Could not open %s for writing!", TmpIndexFileName.c_str());
  int repeat = 0;
#define SISDEBUG 0
#if SISDEBUG
  GPTYPE last_i = 0;
#endif		/* SISDEBUG */
  GPTYPE sis_offset = 0;
#if OPTIM
  GPTYPE charset = 0;
#endif		/* OPTIM */
  if (dp)
    {
      const BYTE cset = GetGlobalCharset();
#if 0
      /* Need some magic now to handle the version */
      fputc(0, dp);  // Start off with a ZERO
      fputc(sizeof(GPTYPE)*8 + 0, dp); // Now write the size of our GPTYPE + version modulo 8.
#endif
      fputc(SisLimit & 0xFF, dp);
      fputc(cset, dp); // CHARACTER SET
      message_log (LOG_DEBUG, "Dumping strings using charset ID#%d", (int)cset);
    }
//GPTYPE old_i = 0;
  for (GPTYPE i=0; i<=MemoryIndexLength; i++)
    {
      if (i == MemoryIndexLength)
        {
          repeat = 1; // To spit out the last sis word
        }
      else
        {
#if OPTIM
          GpFwrite(sis_offset|(charset << 24), fp);
#endif		/* OPTIM */
          if (i > 0)
            {

#if 0
	      // Term Pairs   auto mobile transport -> "auto mobile" "mobile transport"
              const UCHR *ptr0 = (const UCHR *)&MemoryData[NewMemoryIndex[i]];
              const UCHR *ptr1 = ptr0;
              while (*ptr1) ptr1++;

              // Look for next work (max 27 bytes away)
              for (size_t ii=0; ii < 27 && *ptr1 == '\0'; ii++) ptr1++;
              STRING pair(ptr0);
              if (IsTermChr(*ptr1))
                {
                  pair.Cat(" "); pair.Cat(ptr1);
                }
              cerr << "Pair: '" << pair <<  endl;
#endif

#if 1 /* NEW CODE */
	      // We can use strncmp since we really only want to know if its a different
	      // word or not. 
              repeat = strncmp((const char *)&MemoryData[ NewMemoryIndex[i-1] ],
                               (const char *)&MemoryData[ NewMemoryIndex[i] ], SisLimit);
#else
              repeat = SisCompare(&MemoryData[ NewMemoryIndex[i-1] ], &MemoryData[ NewMemoryIndex[i] ], SisLimit);
#endif
            }
        }

      bool includedTerm = true; // OK to add to index
      if (repeat && dp)
        {
          // Write the SIS String.. See SisWrite() above..
#if 1 /* NEW CODE */
          const char *ptr = (const char *)(MemoryData+NewMemoryIndex[i-1]);
          const size_t len = strlen(ptr); // We can do strlen since we zap to '\0' in ParseWords...

          if (_ib_IsExcludedIndexTerm && _ib_IsExcludedIndexTerm(ptr, len))
            includedTerm = false;

          // We now have the length of the word = len
          IndexingTotalBytesCount += len;
          IndexingTotalWordsCount++;

          // we then can caculate TotalWordBytes/TotalWordCount for the
          // average word length and use it to "estimate" a word metric
          if (len > SisLimit) IndexingWordsTruncated++;;
          if (len > IndexingWordsLongestLength)  IndexingWordsLongestLength = len;

          if (includedTerm)
            putc((len > SisLimit ? SisLimit : len) & 0xFF, dp); // Write the SIS String length (max SisLimit)
#else
          size_t len;
          const UCHR *ptr = MemoryData+NewMemoryIndex[i-1];
          for (len = 0; len < SisLimit; len++)
            {
              if (!IsTermChr(ptr+len))
                break;
//	else if (isNumerical && !_ib_isdigit(ptr[len])) isNumerical = false;
            } // for
          // Is the term numerical?
//      if (isNumerical) number = atol((const char *)ptr);

          if (includedTerm)
            putc(len & 0xFF, dp); // Write the SIS String length
#endif

          if (includedTerm)
            {
              // Write SisLimit so that we can bsearch on it..
              sis_offset += 1 + fwrite(ptr, sizeof(UCHR), SisLimit, dp);
              ::putGPTYPE (i-1, dp);// Write which SIS word..
            }
        } // if repeat

      // Here we write the GP of the term...
      if (includedTerm && i < MemoryIndexLength)
        GpFwrite (NewMemoryIndex[i]+GlobalStart, fp);
    }
  if (dp)
    fclose(dp);
  fclose(fp);

  // Flush DFDT
  STRING DFDTFN = Parent->ComposeDbFn(DbExtDfd);
  if (DebugMode) message_log (LOG_DEBUG, "Flushing DFDT to '%s'", DFDTFN.c_str());
  Parent->GetMainDfdt ()->Flush( DFDTFN );

  if (Parent)
    {
      Parent->ProfileWriteString(DbIndexingStatisticsSection, DbTotalBytes, IndexingTotalBytesCount);
      Parent->ProfileWriteString(DbIndexingStatisticsSection, DbTotalWords, IndexingTotalWordsCount);
      Parent->ProfileWriteString(DbIndexingStatisticsSection, "AvgBytesWord",
                                 (IndexingTotalBytesCount + IndexingTotalWordsCount/2)/IndexingTotalWordsCount);
      Parent->ProfileWriteString(DbIndexingStatisticsSection, DbTotalWordsTruncated, IndexingWordsTruncated);
      Parent->ProfileWriteString(DbIndexingStatisticsSection, DbLongestWord, IndexingWordsLongestLength);
    }


  return true;
}

bool INDEX::SetStoplist (const LOCALE& Locale)
{
  return SetStoplist (Locale.LocaleName());
}

bool INDEX::SetStoplist(const STRING& Value)
{
  if (Value.IsEmpty() || Value.Equals("<NULL>"))
    {
//cerr << "Stopwords Empty/NULL" << endl;
      if (StopWords) delete StopWords;
      StopWords = NULL;
      return true;
    }
#if _WIN32
  message_log (LOG_WARN, "WIN32: '%s' Stoplists are not supported at this time.", Value.c_str());
  return false;
#else
  if (StopWords == NULL)
    {
//cerr << "Create Stopwords" << endl;
      StopWords = new STOPLIST ();
    }
//cerr << "Set Stop = " << Value << endl;
  return StopWords->Load (Value);
#endif
}

bool INDEX::IsStopWord (const STRING& Word) const
  {
    return IsStopWord(NulString, Word);
  }

bool INDEX::IsStopWord (const STRING& Field, const STRING& Word) const
  {
    STRINGINDEX len;

    if ((len = Word.GetLength()) > StringCompLength)
      len = StringCompLength;
    return IsStopWord(Field.c_str(), (const UCHR *)Word, len, 0);
  }

bool INDEX::IsStopWord (const UCHR *WordStart, STRINGINDEX WordMaximum, INT Limit) const
  {
    return IsStopWord(NULL, WordStart, WordMaximum, Limit);
  }

#if 0
bool INDEX::IsStopWord (const char *FieldName, const UCHR *WordStart, STRINGINDEX WordMaximum, INT Limit) const
  {
    return IsStopWord(NULL, FieldName, WordStart, WordMaximum, Limit);
  }
#endif

bool INDEX::IsStopWord (const char *FieldName, const UCHR *WordStart, STRINGINDEX WordMaximum, INT Limit) const
{
    STRINGINDEX WordLength = 0;

    if (WordMaximum == 0) return true; // 0 length words are always stoped!

    if (WordStart == NULL || *WordStart == '\0')
      return true;

    // ' and - can be in term!
    while ((WordLength < WordMaximum) &&
           (IsTermChr(WordStart+WordLength) || WordStart[WordLength] == '\'' ||
            WordStart[WordLength] == '-') )
      {
        WordLength++;
      }

// Locale_id = Parent ? Parent->GetLocale().Id() : 0

    if ( _ib_IsExcludedSearchTerm && _ib_IsExcludedSearchTerm(FieldName, WordStart, WordLength))
      return true;

    if (_ib_IsSearchableTerm && _ib_IsSearchableTerm(FieldName, WordStart, WordLength))
      return false;

    // Special case .1 1.12 etc.
    if (_ib_ispunct(WordStart[WordLength]) && _ib_isalnum(WordStart[WordLength+1]))
      return false;
    // Another Special Case: C++, C&X etc.
    else if ( WordLength == 1 && _ib_ispunct(WordStart[1]))
      return false;
    // stuff..
    else if (Limit && ((Limit<0 && (WordLength+Limit==0)) || ((INT)WordLength > Limit)))
      return true;

    if (StopWords == NULL)
      return false;

    return StopWords->InList (WordStart, WordLength);
}



// Uses ParseWords from the parent IDB class which, in turn, will call 
// the routines in the doctype handlers... 
//
size_t INDEX::BuildGpList(const DOCTYPE_ID& Doctype,
                          GPTYPE Start, PUCHR Data, GPTYPE DataLength,
                          PGPTYPE Index, GPTYPE IndexLength)
{
  if (IndexLength == 0 || DataLength <= Start)
    {
      if (DataLength < Start)
        message_log (LOG_WARN,
#ifdef _WIN32
	"GpList data=%I64d <= %I64d (start). Buffer overrun (Index length=%I64d)?" 
#else
	"GpList data=%lld <= %lld (start). Buffer overrun (Index length=%lld)?"
#endif
              , (long long)DataLength, (long long)Start, (long long)IndexLength);
      else
        message_log (LOG_DEBUG,
#ifdef _WIN32
		"BuildGpList, Length=%I64d"
#else
		"BuildGpList, Length=%lld"
#endif
		, (long long)IndexLength);
      return 0;
    }

  return ( Parent->ParseWords(Doctype, Data + Start,
                              DataLength - Start, Start, Index,
                              IndexLength) );   // Convert parameters to what ParseWords() wants
}


// @@: edz@nonmonotonic.com
// The operator application was
//     Op1->Op(*Op2);
//     Stack << *Op1;
// But now changed to
//     Op2->Op(*Op1);
//     Stack << *Op2;
//
// So X Y AndNot means X AndNot Y.
//
// March 2008
// Was Stack << *Op1; delete Op1;
// Now is Stack << Op1; // Saves making a copy
//
/* Define What to return on Error */
# define ERROR_SET ((PIRSET)0)	/* Null */


PIRSET INDEX::Search (const QUERY& Query)
{
  enum NormalizationMethods Method = Query.GetNormalizationMethod();
  SQUERY                    sQuery (Query.GetSQUERY());


#if 0
  if (sQuery.isOpQuery(OperatorAnd))
    {
      cerr << "Its an AND Query..." << endl;
    }
#endif

  // Flip OPSTACK upside-down to convert so we can
  // pop from it in RPN order.
  OPSTACK Stack;

  sQuery.GetOpstack (&Stack);
  Stack.Reverse ();

  // Pop OPOBJ's, converting OPERAND's to result sets, and
  // executing OPERATOR's
  OPSTACK      TempStack;
  POPOBJ       OpPtr;
  PIRSET       NewIrset = NULL;
  ATTRLIST     Attrlist;
  STRING Term, FieldName;
  INT          Relation = -1,Structure=-1;
  bool  gotRelation;
  FIELDTYPE    FieldType;
  FIELDTYPE    aFieldType; // Advised Field Type
  INT          TermWeight;
  POPOBJ       Op1, Op2;
  t_Operator op_t;
  clock_t      startClock = clock();

  message_log (LOG_DEBUG, "Entering Search loop");

  int terms = 0; // Term count (not stopwords)
  while (Stack >> OpPtr)
    {
      if (OpPtr->GetOpType () == TypeOperator)
        {
          if ((op_t = OpPtr->GetOperatorType ()) == OperatorNOT)
            {
              // Unary operators are "Special Case"
              TempStack >> Op1;
              if (Op1 == NULL)
                {
                  message_log (LOG_DEBUG, "108   Malformed query (Complement of NIL undefined)");
                  Parent->SetErrorCode(108);
                  return ERROR_SET; // Malformed query 
                }
              Stack << Op1->Not ();
            }
	  else if (op_t == OperatorSibling)
	    {
	      TempStack >> Op1;
	      if (Op1 == NULL)
		{
		  Parent->SetErrorCode(108);
		  return ERROR_SET;
		}
	      Stack << Op1->Sibling ();
	    }
          else if (op_t == OperatorReduce || op_t == OperatorHitCount || op_t == OperatorTrim ||
		op_t == OperatorBoostScore )
            {
              FLOAT metric =  OpPtr->GetOperatorMetric();
              if ((op_t == OperatorReduce || op_t == OperatorTrim) && metric < 0)
                {
                  Parent->SetErrorCode(107); // "Query type not supported";
                  return ERROR_SET;
                }
              TempStack >> Op1;
              if (Op1 == NULL)
                {
                  Parent->SetErrorCode(108);
                  return ERROR_SET;
                }
	      if (op_t == OperatorHitCount)
		Stack << Op1->HitCount(metric);
	      else if (op_t == OperatorReduce)
		Stack << Op1->Reduce(metric);
	      else if (op_t == OperatorBoostScore)
		Stack << Op1->BoostScore(metric);
	      else
		Stack << Op1->Trim(metric);
            }
          else if (op_t == OperatorWithinFile || op_t == OperatorWithKey ||
		   op_t == OperatorSortBy ||
		   op_t == OperatorWithinFileExtension || op_t == OperatorWithinDoctype ||
                   op_t == OperatorNotWithin ||
                   op_t == OperatorWithin || op_t == OperatorXWithin ||
		   op_t == OperatorInclusive || op_t == OperatorInside)
            {
              STRING fieldName ( OpPtr->GetOperatorString() );
              // Unary operators are "Special Case"
              TempStack >> Op1;
              if (Op1 == NULL)
                {
                  Parent->SetErrorCode(108);
                  return ERROR_SET; // Complement of NIL is undefined
                }
              switch (op_t)
                {
		case OperatorWithinFileExtension:
		  // Very special case!
		  fieldName = "*." + OpPtr->GetOperatorString(); 
                case OperatorWithinFile:
                  // Very special case!
#if 1
		  Op1->WithinFile (fieldName);
#else
                  Stack << *Op1; Op2 = Op1->WithinFile( fieldName);
                  if (Op2) { Stack << Op2; Op2 = NULL;} // was here (also below): *Op2; delete Op2 // 2008 MEMO
#endif
                  break;
                case OperatorWithKey:
                  // Very special case!
#if 1
		  Op1->WithKey(fieldName);
#else
                  Stack << *Op1; Op2 = Op1->WithinKey(fieldName);
		  if (Op2) { Stack << Op2; Op2 = NULL;}
		  // if (Op2) { Stack << Op1->And(*Op2) /* was Stack << Op2 */; Op2 = NULL;}
#endif
		  break;
		case OperatorWithinDoctype:
		  // Very special case!
		  Stack << *Op1; Op2 = Op1->WithinDoctype( fieldName);
		  if (Op2) { Stack << Op2; Op2 = NULL;}
                  break;
                  // Normal unary operators:
                case OperatorNotWithin: Op1->Not     ( fieldName ); break;
                case OperatorWithin:    Op1->Within  ( fieldName ); break;
                case OperatorInside:    Op1->Inside  ( fieldName ); break;
		case OperatorInclusive: Op1->Inclusive(fieldName ); break;
                case OperatorXWithin:   Op1->XWithin ( fieldName ); break;
		case OperatorSortBy:    Op1->SortBy  ( fieldName ); break;
                default: message_log (LOG_PANIC, "INTERNAL ERROR: Unknown Unary Operator %d!", (int)op_t );
                }
	      Stack << Op1;
	      Op1 = NULL;
            }
          else if (op_t != OperatorNoop && op_t != OperatorERR)
            {
              TempStack >> Op1;
              TempStack >> Op2;
              // Make sure that the RPN stack is not defective!
              if (Op1 == NULL || Op2 == NULL)
                {
                  if (Op1) delete Op1;
                  if (Op2) delete Op2;
                  message_log (LOG_DEBUG, "108   Malformed Boolean query  (NIL Term)");
                  Parent->SetErrorCode(108);
                  return ERROR_SET;	// Error

                }
              else
                {
#define SwapOp(Op1,Op2) { \
		  const size_t t1 =((IRSET *)Op1)->GetHitTotal(); \
		  const size_t t2 =((IRSET *)Op2)->GetHitTotal(); \
                  if (t1 > t2) { POPOBJ Op = Op1; Op1 = Op2; Op2 = Op; } }

                  switch (OpPtr->GetOperatorType () )
                    {
                    case OperatorOr:
                      if (Op1->GetTotalEntries () == 0)
			{
                          Stack << Op2;
			  Op2 = NULL;
			  delete Op1; // April 2008. !!!
			}
                      else
			{
                          Stack << (Op1->Or (*Op2));
			}
                      break;
                    case OperatorAnd:
		      SwapOp(Op1,Op2);
		      Stack << (Op1->And(*Op2)); 
                      break;
                    case OperatorNotAnd:
                      // A B NOTAND := B A ANDNOT
		      { POPOBJ Op = Op1; Op1 = Op2; Op2 = Op; }
		    case OperatorAndNot:
		      Stack << (Op1->AndNot (*Op2));
                      break;
		    case OperatorJoinR:
		      // Like AND but NOT symetric
		      { POPOBJ Op = Op1; Op1 = Op2; Op2 = Op; }
		    case OperatorJoin:
		    case OperatorJoinL:
                      // Like AND but NOT symetric 
                      Stack << (Op1->Join (*Op2));
                      break;
                    case OperatorXor:
                      Stack << (Op1->Xor (*Op2));
                      break;
                    case OperatorXnor:
                      // symetric XNOR := XOR NOT
                      Stack << (Op1->Xor (*Op2)->Not());
                      break;
                    case OperatorNor:
                      // symetric NOR := OR NOT
                      Stack << (Op1->Or (*Op2)->Not());
                      break;
                    case OperatorNand:
                      // symetric NAND := AND NOT
                      Stack << (Op1->And (*Op2)->Not());
                      break;
                    case OperatorNeighbor:
		      SwapOp(Op1,Op2);
                      Stack << (Op1->Neighbor (*Op2));
                      break;
                    case OperatorNear:
                    {
                      FLOAT metric =  OpPtr->GetOperatorMetric();
                      if (metric) // A metric specified is a char prox
                        Stack << (Op1->CharProx(*Op2, metric));
                      else
                        Stack << (Op1->Near(*Op2));
                    }
                    break;
                    case OperatorProximity:
		      SwapOp(Op1,Op2);
                      Stack << (Op1->CharProx(*Op2,  OpPtr->GetOperatorMetric() ));
                      break;
                    case OperatorFar:
		      SwapOp(Op1,Op2);
                      Stack << (Op1->Far (*Op2));
                      break;
                    case OperatorAfter:
                    {
                      FLOAT metric =  OpPtr->GetOperatorMetric();
                      if (metric)
                        Stack << (Op1->CharProx(*Op2, metric, OPOBJ::AFTER));
                      else
                        Stack << (Op1->After (*Op2));
                    }
                    break;
                    case OperatorBefore:
                    {
                      FLOAT metric =  OpPtr->GetOperatorMetric();
                      if (metric)
                        Stack << (Op1->CharProx(*Op2, metric, OPOBJ::BEFORE));
                      else
                        Stack << (Op1->Before (*Op2));
                    }
                    break;
                    case OperatorAdj:
		      SwapOp(Op1,Op2);
                      Stack << (Op1->Adj (*Op2));
                      break;
                    case OperatorFollows:
                      Stack << (Op1->Follows (*Op2));
                      break;
                    case OperatorPrecedes:
                      Stack << (Op1->Precedes (*Op2));
                      break;
                    case OperatorAndWithin:
		      SwapOp(Op1,Op2);
                      Stack << (Op1->Within (*Op2, OpPtr->GetOperatorString() ));
                      break;
                    case OperatorBeforeWithin:
                      Stack << (Op1->BeforeWithin (*Op2, OpPtr->GetOperatorString() ));
                      break;
                    case OperatorAfterWithin:
                      Stack << (Op1->AfterWithin (*Op2, OpPtr->GetOperatorString() ));
                      break;
                    case OperatorOrWithin:
		      SwapOp(Op1,Op2);
                      Stack << (Op1->Or (*Op2)->Within(OpPtr->GetOperatorString() ));
                      break;
                    case OperatorPeer:
		      SwapOp(Op1,Op2);
		      Stack << (Op1->Peer (*Op2));
                      break;
                    case OperatorAfterPeer:
                      Stack << (Op1->AfterPeer (*Op2));
                      break;
                    case OperatorBeforePeer:
                      Stack << (Op1->BeforePeer (*Op2));
                      break;
                    case OperatorXPeer:
		      SwapOp(Op1,Op2);
                      Stack << (Op1->XPeer (*Op2));
                      break;

                    default:
                      message_log (LOG_ERROR, "RPN Stack contains bogus ops.");
                      // Bad case
                      if (Op1) delete Op1;
                      if (Op2) delete Op2;
		      Parent->SetErrorCode(110); // "Operator unsupported"
                      return ERROR_SET;     // Error
                    }
                }
              if (Op2) delete Op2;
            }
	  if ((clock() - startClock) > MaxQueryCPU_ticks)
	    {
	      message_log (LOG_INFO, "Search time limit (%u ms) exceeded: Unpredictable partial results available",
                        (int)((MaxQueryCPU_ticks*1000.0)/CLOCKS_PER_SEC) );
	      // "Resources exhausted - unpredictable partial results available";
	      Parent->SetErrorCode(32);
	      break;
	    }
	  delete OpPtr; // ADDED: 2008 March 
        }
      else if (OpPtr->GetOpType () == TypeOperand)
        {
          if (OpPtr->GetOperandType () == TypeRset)
            {
	      TempStack << OpPtr;
            }
          else if (OpPtr->GetOperandType () == TypeTerm)
            {
              terms++; // Increment count
              OpPtr->GetTerm (&Term);
              // Term Alias hook!
              if (TermAliases)
                {
                  STRING NewTerm;
                  if (TermAliases->GetValue(Term, &NewTerm))
                    Term = NewTerm;
                }
              else
                Parent->ProfileGetString("TermAliases", Term, Term, &Term);
              if (Term.IsEmpty())
                {
                  // Empty Term so don't bother
                  NewIrset = new IRSET (Parent);
                  goto error;
                }

              OpPtr->GetAttributes (&Attrlist);

              // NOTES:
              // We need to think about the difference between explicit and
              // implicit fieldtypes
              // if sometime types in a number and its a number field type the
              // search should be handled as a number
              // if someone type in a gpoly and its a gpoly then it should be treated
              // as a gpoly.
              // This is afterall the kind of polymorphism we want.. I think?

              if (Attrlist.AttrGetFieldName (&FieldName))
                {
                  // Check that fieldname is valid..
                  if (! Parent->FieldExists(FieldName))
                    {
                      Parent->SetErrorCode(114); // "Unsupported Use attribute"
                      // Field does not exist so don't bother searching
                      NewIrset = new IRSET (Parent);
                      goto error;
                    }
                }

              FieldType = Attrlist.AttrGetFieldType();
              aFieldType = Parent->GetFieldType(FieldName); // What it also is

	      // Did we specify a relation?
              gotRelation = Attrlist.AttrGetRelation(&Relation);
//  cerr << "Did we get a relation for " << FieldName << " ? " << gotRelation << endl;

              // Is undefined??
              if (gotRelation && !(FieldName.IsEmpty() || FieldType.Defined()))
                {
//cerr << "Checking.. " << endl;
		  const DFDT *Dfdt =  Parent->GetMainDfdt();
                  if (Term.GetChr(1) == '#') // Force (historical)
                    {
                      char ch = Term.GetChr(2);
                      FieldType = (ch == '{' || ch == '[') ? FIELDTYPE::daterange : FIELDTYPE::date;
                    }
		  else if (Term.IsGeoBoundedBox())
		    {
		      if (Dfdt->TypeFieldExists(FIELDTYPE::box, FieldName))
			FieldType = FIELDTYPE::box;
		    }
                  else if (Term.IsDotNumber() && Dfdt->TypeFieldExists(FIELDTYPE::dotnumber, FieldName))
                    {
                      FieldType = FIELDTYPE::dotnumber;
                    }
		  else if (Term.IsNumberRange() && Dfdt->TypeFieldExists(FIELDTYPE::numericalrange, FieldName))
		    {
//cerr << "Is number range" <<endl;
		      FieldType = FIELDTYPE::numericalrange;
		    }
                  else if (Term.IsDate() && Dfdt->DateFieldExists(FieldName))
                    {
//cerr << "Is date " << endl;
                      FieldType = FIELDTYPE::date;
                    }
		  else if (Term.IsNumber())
		    {
//cerr << "IS Number " << endl;
		      // Number could be money but money should be money...
		      if (Dfdt->NumericFieldExists(FieldName))
			{
			  FieldType = FIELDTYPE::numerical;
			}
		      else if (Dfdt->TypeFieldExists(FIELDTYPE::currency, FieldName))
			{
			  FieldType = FIELDTYPE::currency;
			}
		    }
		  else if (Term.IsCurrency())
		    {
//cerr << "Is money" << endl;
//cerr << " but " << Dfdt->DateFieldExists(FieldName) << endl;

		      if (Dfdt->TypeFieldExists(FIELDTYPE::currency, FieldName))
			FieldType = FIELDTYPE::currency;
		    }
		  else if (Term.IsDateRange())
		    {
//cerr << "Is date range" << endl;
		      if (Dfdt->TypeFieldExists(FIELDTYPE::daterange, FieldName))
			FieldType = FIELDTYPE::daterange;
                    }
                  else
                    FieldType = aFieldType;
                  if ((FieldType.IsDate() | FieldType.IsDateRange()) &&
                      (Term.Search('/') || Term.Search('[') || Term.Search('}')))
                    {
                      Structure=ZStructDateRange;
                    }
		  if (FieldType.Defined())
		    message_log (LOG_DEBUG, "Unspecified field '%s' is '%s' (Rel=%d)", FieldName.c_str(), FieldType.c_str(), Relation);
		  else
		    message_log (LOG_DEBUG, "Field '%s' has unknown type. Assuming any.", FieldName.c_str());
//cerr << "Fieldtype=" << FieldType.c_str() << endl;
                }

	      // Here is where we search.......................

              if (FieldName.IsEmpty() && Term.SearchAny("RECT{") /*}*/ == 1)
                {
                  // Pass {c1,c2,c3,c4} to the routine
                  NewIrset=BoundingRectangle( Term.c_str() + 4);
                  FieldType = FIELDTYPE::box;
                }
              else if (FieldType.IsBox() || (aFieldType.IsBox() && (Term.Search("{") || Term.Search("[") ))) /*"}"*/
                {
                  NUMBER N, So, W, E;
                  // Need to fix the search to search ONLY this field!
                  NewIrset=BoundingRectangle(Term, &N, &So, &W, &E); // <- Wrong!
                  // OK - NewIrset has the list of records that match the query.
                  // This is where we apply the ranking algorithm from Ken Lanfear
                  SetSpatialScores(NewIrset, FieldName, N,So,W,E);
                }
              else if (FieldType.IsDate() || (aFieldType.IsDate() && SRCH_DATE(Term).Ok()))
                {
                  if (gotRelation==false) Relation=ZRelEQ;
                  NewIrset=DateSearch(Term, FieldName, Relation);
                }
              else if (FieldType.IsDateRange())
                {
                  if (gotRelation==false) Relation=ZRelEQ;
                  if (Attrlist.AttrGetStructure(&Structure)==false)
                    Structure=ZStructDateRange;
                  NewIrset=DoDateSearch(Term,FieldName,Relation,Structure);
                }
              else if (FieldType.IsNumerical() || FieldType.IsComputed() ||
                       ((aFieldType.IsNumerical() || aFieldType.IsComputed()) && gotRelation && Term.IsNumber()))
                {
                  if (gotRelation==false) Relation=ZRelEQ;
//cerr << "Numeric Search for: " << Term << " rel=" << Relation << endl;
                  NewIrset=NumericSearch(Term.GetDouble(), FieldName, Relation);
                }
              else if (aFieldType.IsCurrency() || FieldType.IsCurrency()) // FIELDTYPE::currency
                {
                  if (gotRelation==false) Relation=ZRelEQ;
                  NewIrset=MonetarySearch( MONETARYOBJ(Term), FieldName, Relation);
                }
	      else if (aFieldType.IsLexiHash() || FieldType.IsLexiHash()) // FIELDTYPE::lexi
		{
		  if (gotRelation==false) Relation=ZRelEQ;
		  NewIrset=LexiHashSearch( Term, FieldName, Relation);
		}
              else if (Attrlist.AttrGetPhonetic ())
                {
                  NewIrset = TermSearch (Term, FieldName,
                                         Attrlist.AttrGetAlwaysMatches () ? PhoneticCase : Phonetic);
                }
              else if (Attrlist.AttrGetGlob() && Term.IsWild() != 0)
                {
                  NewIrset = GlobSearch (Term, FieldName, Attrlist.AttrGetAlwaysMatches ());
                }
              else if (Attrlist.AttrGetLeftAndRightTruncation())
                {
		  if (Term.GetLength() < 3)
		    {
		      // "9     Truncated words too short  (unspecified)"
		      message_log (LOG_DEBUG, "9 Term \"%s\": Left/Right truncated words too short.", Term.c_str());
		      Parent->SetErrorCode(9);
		      NewIrset = new IRSET (Parent);
		    }
		  else
		    {
		      STRING tmp;
		      tmp << "*" << Term << "*";
		      NewIrset = GlobSearch (tmp, FieldName, Attrlist.AttrGetAlwaysMatches ());
		    }
                }
              else if (Attrlist.AttrGetLeftTruncation() )
                {
		  if (Term.GetLength() < 3)
		    {
		      message_log (LOG_DEBUG, "9 Term \"%s\": Left truncated words too short.", Term.c_str());
		      Parent->SetErrorCode(9);
		      NewIrset = new IRSET (Parent);
		    }
		  else
		    {
		      const STRING tmp ( "*" + Term );
		      NewIrset = GlobSearch (tmp, FieldName, Attrlist.AttrGetAlwaysMatches ());
		    }
                }
              else if (Attrlist.AttrGetRightTruncation () ||
                       // 7 Oct 2003: Treat > and >= as Right Truncation
                       (Relation == ZRelGE) || (Relation == ZRelGT) )
                {
                  if (Term.GetLength() < 2)
                    {
                      // "9     Truncated words too short  (unspecified)"
                      message_log (LOG_DEBUG, "9 Term \"%s\": Right truncated words too short.", Term.c_str());
                      Parent->SetErrorCode(9);
                      NewIrset = new IRSET (Parent);
                    }
                  else
                    NewIrset = TermSearch (Term, FieldName,
                                           Attrlist.AttrGetAlwaysMatches () ? LeftAlwaysMatches : LeftMatch);
                }
	      else if (aFieldType.IsHash() || FieldType.IsHash()) // FIELDTYPE::hash
		{
		  if (gotRelation==false) Relation=ZRelEQ;
		  NewIrset=HashSearch( Term, FieldName, Relation, false);
		}
              else if (aFieldType.IsCaseHash() || FieldType.IsCaseHash()) // FIELDTYPE::casehash
                {
                  if (gotRelation==false) Relation=ZRelEQ;
                  NewIrset=HashSearch( Term, FieldName, Relation, true);
                }
               else if (aFieldType.IsPrivHash() && _IB_private_hash) // FIELDTYPE::privhash
                {
                  if (gotRelation==false) Relation=ZRelEQ;
		  const NUMERICOBJ val = _IB_private_hash(FieldName, Term, Term.Len());
		  message_log (LOG_DEBUG, "Numeric search \"%s\"->%F", Term.c_str(), (double)val);
                  NewIrset=NumericSearch( val, FieldName, Relation);
                }
	      else if (aFieldType.IsPhonetic())
		{
		  WORDSLIST Names;
		  size_t    count = 1;
		  if (Parent)
		    count = Parent->ParseWords2(Term, &Names); 
		  else if (Term.GetLength())
		    Names.AddEntry(WORDSOBJ(Term, 0));

		  IRSET *irset0 = NULL;

		  NewIrset = NULL;

		  if (gotRelation==false) Relation=ZRelEQ;
		  for (WORDSLIST *ptr = Names.Next(); ptr != &Names; ptr = ptr->Next())
		    {
		      UINT8 hash1 = 0;
		      UINT8 hash2 = 0;

		      STRING name = ptr->Value().Word();

		      if (name.GetLength() <= 1 || name.GetChr(2) == '.')
			{
#ifdef PHONETIC_SKIP_INITIALS
			  continue; // Skip initials
#else
			  irset0 = TermSearch (name, FieldName, LeftMatch);
#endif
			}
		      else
			{
			  // More than 1 character
			  if (aFieldType.Equals(FIELDTYPE::phonhash))
			    {
			      hash1 = SoundexEncode(name); 
			    }
		          else if (aFieldType.Equals (FIELDTYPE::metaphone))
			    {
			      UINT8 v = DoubleMetaphone ( name );
			      if (v) 
				{
				  hash1 = __Low32(v);
				  hash2 = __High32(v);
				}
			    }
		          irset0=NumericSearch( hash1, FieldName, Relation);
		          if (hash2 && hash2 != hash1)
			    {
			      IRSET *irset = NumericSearch( hash2, FieldName, Relation);
			      if (irset)
				{
				  irset0->Or(*irset);
				  delete irset;
				}
			    }
			}
		      if (NewIrset)
			{
			  NewIrset->Within(*irset0, FieldName);
			  if (irset0) delete irset0;
			  irset0 = NULL;
			}
		      else
			{
			  NewIrset = irset0;
			  irset0 = NULL; 
			}
		      if (NewIrset->IsEmpty())
			{
			  break;
			}
		  } // for()
	        }
              else if (Term.Search(' ') == 0 && IsStopWord(FieldName, Term))
                {
                  terms--; // Decrement, not a term
                  NewIrset = new IRSET (Parent);
                }
              else if (Attrlist.AttrGetAlwaysMatches ())
                {
                  NewIrset = TermSearch (Term, FieldName, AlwaysMatches);
                }
              else if (Attrlist.AttrGetExactTerm())
                {
                  NewIrset = TermSearch (Term, FieldName, ExactTerm);
                }
              else
                {
                  if (Attrlist.AttrGetFreeForm ())
                    NewIrset = TermSearch (Term, FieldName, FreeForm);
                  else
                    NewIrset = TermSearch (Term, FieldName, Exact);
                }
              if (NewIrset)
                {
                  if (FieldType.IsText())
                    {
                      // With Text fields treat <> as !
                      if (Relation == ZRelNE) NewIrset->Not();
                    }
                  TermWeight = Attrlist.AttrGetTermWeight ();
                  NewIrset->ComputeScores (TermWeight, Method);
                }
              else
                NewIrset = new IRSET (Parent);
error:
	      TempStack << NewIrset;
	      NewIrset = NULL; // Not needed but.. (April 2008)
	      delete OpPtr; // Added: 2008 March
            } /* TypeTerm*/
        } /* TypeOperand */
    }
  TempStack >> NewIrset;

#if 1
  if (!Stack.IsEmpty())
    {
      message_log (LOG_INFO, "Query stack was not empty. Malformed query.");
      Parent->SetErrorCode(108); // "Malformed query";
    }
  if (!TempStack.IsEmpty())
    {
      message_log (LOG_INFO, "Search stack was not empty. Malformed RPN.");
      Parent->SetErrorCode(108); // "Malformed query";
    }
#endif

  // Do we want to clip the set?
  if (!Query.isUnlimited())
    {
      const size_t clip = Query.GetMaximumResults();
      if (NewIrset->GetTotalEntries() > clip) {
	NewIrset->SortBy(Query.GetSortBy()); // Added 2024
	NewIrset->SetTotalEntries(clip);
      }
    }

  if (terms == 0)
    {
      message_log (LOG_DEBUG, "4     Terms only exclusion (stop) words   (unspecified)");
      Parent->SetErrorCode(4);
    }
  else if (Method == CosineMetricNormalization || Method == EuclideanNormalization) {
    NewIrset->ComputeScoresCosineMetricNormalization(1);
  }
  return NewIrset;
}

//private
PFILE INDEX:: GetFilePointer (const GPTYPE gp, const INT Off) const
  {
    PFILE fp = NULL;
    MDTREC Mdtrec;
    if (Parent && Parent->GetMainMdt ()->GetMdtRecord (gp, &Mdtrec))
      {
        const off_t Offset = gp - Mdtrec.GetGlobalFileStart () - Off;
        // Is Offset within record boundary?
        if (Offset >= (off_t)Mdtrec.GetLocalRecordStart())
          {
            if ((fp = ffopen ( Parent->ResolvePathname(Mdtrec.GetFullFileName ()), "rb")) != NULL)
              {
                if (-1 == fseek (fp, Offset, SEEK_SET))
                  {
                    // Seek Error
                    ffclose(fp);
                    fp = NULL;
                  }
              }
          }
      }
    return fp;
  }


//private
// Default Off = 0, Length = StringCompLength
//
// TODO: Know the character set of the record so we can use this when reading into
// UCS (when we go to UCS instead of char * buffers)
//
INT INDEX::GetIndirectBuffer (const GPTYPE gp, PUCHR Dest, INT Off, INT Length) const
{
    MDTREC Mdtrec;
    UCHR  *Buffer = Dest;

#ifndef _WIN32
    if (DebugMode)
      message_log (LOG_DEBUG, "Get Indirect buffer for %llx (off=%d)", (long long)gp, (int)Off);
#endif

    if (Parent && Parent->GetMainMdt ()->GetMdtRecord (gp, &Mdtrec))
      {
        int x = 0;
        // Added edz : 3 Sept 1999 : Don't bother with deleted records!!
        if (Mdtrec.GetDeleted())
          {
            Buffer[0] = '\000';
            return (INT)x;
          }
        off_t Offset = gp - Mdtrec.GetGlobalFileStart () - Off;
        off_t diff   = Offset - (off_t)Mdtrec.GetLocalRecordStart();

        // Is Offset within record boundary?
        if (Off == 1 && diff < 0 && Length+diff > 0)
          {
            memset(Dest, ' ', -diff);
            Buffer -= diff;
            Length += diff;
            Offset -= diff;
            diff = 0;
          }
        if (diff >= 0)
          {
            // Get Length Right
            if ((GPTYPE)(Length + Offset) > Mdtrec.GetLocalRecordEnd())
              {
                // Don't cross End-of-Record boundary
                Length =  Mdtrec.GetLocalRecordEnd() - Offset;
              }
            DOCTYPE     *DoctypePtr = Parent->GetDocTypePtr ( Mdtrec.GetDocumentType() );
            const STRING filename (  Parent->ResolvePathname(Mdtrec.GetFullFileName()) );
            if (DoctypePtr)
              {
                if (DebugMode)
                  message_log (LOG_DEBUG, "Getting indirect term via %s::GetTerm(%s,%lu,%d)",
                        DoctypePtr->Name().c_str(), filename.c_str(), (unsigned long)Offset, (int)Length );
                x = DoctypePtr->GetTerm(filename, (char *)Buffer, Offset, Length);
              }
            else
              {
                FILE *fp = ffopen (filename, "rb");
                if (fp != NULL)
                  {
                    x = pfread(fp, Buffer, Length, Offset);
                    ffclose(fp); // Close
                  }
              }
          }
        if (DebugMode)
          message_log(LOG_DEBUG, "*::GetTerm() returned %d of max. %d characters", x, Length);
        Buffer[x] = '\000';
        return (INT)x;
      }
    // Deleted record or Error!
    if (DebugMode)
      message_log (errno == EIDRM ? LOG_ERROR : LOG_DEBUG, "%s indirect buffer for GP=%lx, Off=%d, Length=%d%s",
            errno == 0 ? "Empty" : "Could not get",
            gp, Off, Length, errno == ENOENT ? ": Deleted record." : "!");
    Buffer[0] = '\000';
    return -1;
}

INT INDEX::GetIndirectTerm(const GPTYPE Gp, PUCHR Buffer, INT MaxLength) const
  {
    if (Buffer == NULL)
      {
        try
          {
            Buffer = new UCHR[MaxLength+1];
          }
        catch (...)
          {
            message_log (LOG_PANIC, "Memory Exhausted");
            return 0; // No memory
          }
      }
    INT length = GetIndirectBuffer(Gp, Buffer, 0, MaxLength);
    if (length > 0)
      {
        length = BufferClean(Buffer, length, true);
        Buffer[length] = '\0';
        return length;
      }
    // ERROR
    return 0;
  }

STRING INDEX::GetIndirectTerm(GPTYPE Gp) const
  {
    // Hack Alert!!
    STRING tmpString('\0', BUFSIZ);
    size_t Len = GetIndirectTerm(Gp, (PUCHR)(tmpString.GetData()), BUFSIZ-1);

    if (Len)
      {
        tmpString.Truncate(Len);
        return tmpString;
      }
    return NulString;
  }


// inline
static int gpcomp(const void* x, const void* y)
{
  return(*((GPTYPE *)x)-*((GPTYPE *)y));
}

PIRSET INDEX::MetaphoneSearch (const STRING& QueryTerm, const STRING& FieldName, bool useCase)
{
  int num_hits = 0;
  PGPTYPE  gplist = NULL;
  size_t   gplist_siz = 0;
  PIRSET   pirset = new IRSET (Parent);
  clock_t  startClock = clock();

  char n[16];

  // Reference Double Metaphone Hash
  const UINT8 mhash  = DoubleMetaphone (QueryTerm); // Get the Double Metaphone Hash for our term..

  // Copy lower case into a BCPL-style string...
  // n[0] contains the match length
  // n[1] contains the string length
  // &n[2] contains the lower case term..
  n[0] = '\0';
  n[1] = '\1';
  n[2] = _ib_tolower (QueryTerm.GetChr(1));
  n[3] = '\0';

  INT NumberOfIndexes =  GetIndexNum();
  FILE *fpi = NULL;

  FC Fc;
  IRESULT iresult;
  MDTREC mdtrec;
  size_t old_w = 0;

  bool IsDeleted = false;
  SRCH_DATE   rec_date;

  iresult.SetVirtualIndex( (UCHR)( Parent->GetVolume(NULL) ) );
  iresult.SetMdt (Parent->GetMainMdt() );
  iresult.SetHitCount (1);
  iresult.SetAuxCount (1);
  iresult.SetScore (0);

  const bool CheckField= (FieldName.GetLength() != 0);
// bool      Disk = true;
  int      FirstTime = 1;

  // Loop through all sub-indexes
  MMAP MemoryMap;
  for (INT jj = 1; (jj <= NumberOfIndexes) || (NumberOfIndexes == 0); jj++)
    {

      if (NumberOfIndexes == 0)
        {
          if ((fpi = ffopen (IndexFileName, "rb")) == NULL)
            continue;
          NumberOfIndexes = -1;
          MemoryMap.CreateMap(SisFileName);
        }
      else
        {
          if (fpi) ffclose(fpi); // Close old handle
          STRING s;
          s.form("%s.%d", IndexFileName.c_str(), jj);

          if ((fpi = ffopen(s, "rb")) == NULL)
            continue;
          s.form("%s.%d", SisFileName.c_str(), jj);
          MemoryMap.CreateMap(s);
        }
      if (!MemoryMap.Ok())
        continue;

      MemoryMap.Advise(MapRandom);
      const char *Map = (char *)MemoryMap.Ptr();
      const size_t size = MemoryMap.Size();
#if 1
     // Note: the sisfile should be 2 bytes longer than the number of SISentries.. each
     // compLength. If not.. then either 1) the file got truncated 2) old version?
     // Our test for tuncation is not too good but still.. in the future we should probably
     // give a warning message if its not 2 but for now ...   18 March 2009
                         // New edition indexes? Map[0] stores the compLength
      const int    off = (( ( size % (sizeof(GPTYPE) + Map[0] + 1) ) == 2) ? 2  :
                         // old edition?
                          ( size % (sizeof(GPTYPE) + StringCompLength + 1) == 0 ? 0 : 2) );
#else /* Was */
      const int  off = (size % (sizeof(GPTYPE)+StringCompLength+1) == 0 ? 0 : 2);
#endif
      const char *Buffer = Map + off;
      const size_t compLength = off ? (unsigned char)Map[0] : StringCompLength;
      const size_t dsiz = compLength + sizeof(GPTYPE)+1;
      if (off)
        {
          Charset.SetSet((BYTE)Map[1]);
          if (!Charset.Ok())
            message_log (LOG_ERROR, "Can't set Charset %d!", (int)((BYTE)Map[1]));
        }

      // Find any match..
      char *t = __priv_bsearch(&n, Buffer, size/dsiz, dsiz, Charset.SisKeys());
      if (t)
        {
          // We now have ANY Match...
          char *High = t; // Last Element in range
          char *Low = t; // First Element in range
          char *tp;
          // Scan back to low (replace later with bsearches)
          for (tp = (char *)t; tp >= Buffer; tp -= dsiz)
            {
              if (n[2] != tp[1])
                break;
              Low = tp;
            }

          // Now look forward ... (replace later)
          for (tp = (char *)t; tp < &Buffer[size]; tp += dsiz)
            {
              if (n[2] != tp[1])
                break;
              High = tp;
            }
          // We now have the range...
          // Determine the size of the index
          // const off_t Size = GetFileSize(fpi);
          // const INT Off = Size % sizeof (GPTYPE);
          for (tp = Low; tp <= High; tp += dsiz)
            {
              if (MetaphoneMatch(mhash, &tp[1]))
                {
                  // Double Metaphone Matched..
                  const GPTYPE end = GP((unsigned char *)tp, compLength+1);
                  // Make sure not the first element...
                  const GPTYPE start = ((long)tp - (long)Buffer) ? GP((unsigned char *)tp, -(int)sizeof(GPTYPE)) + 1 : 0;
                  int nhits = end - start + 1; // @@@@@@@@@@@ was -1  ( Mon Jul 17 13:51:14 MET DST 2000)
                  if (nhits <= 0)
                    {
                      continue; // Paranoia
                    }
                  if ((size_t)nhits > gplist_siz)
                    {
                      // Reallocate space..
                      if (gplist) delete[] gplist;
                      gplist_siz += nhits + 100;
                      try
                        {
                          gplist = new GPTYPE[gplist_siz];
                        }
                      catch (...)
                        {
                          gplist = NULL;
                          Parent->SetErrorCode(2); // Temp error
                          num_hits = nhits = 0; // Not enough memory
                        }
                    }
                  // Read the GPs..
                  if (nhits)
                    num_hits = GpFread(gplist, nhits, start, fpi); /* @@ */

                  // Now sort..
                  // if (num_hits > 1) QSORT(gplist, num_hits, sizeof(GPTYPE), gpcomp); // Speed up looking

                  if (CheckField && FirstTime)
                    {
                      FieldCache->SetFieldName(FieldName); // Note: Here we can adise we Disk
                      FirstTime = 0;
                    }
                  for (INT j=0; j<num_hits; j++)
                    {
		      if (j % 2001 == 2000 && (MaxCPU_ticks < (clock() - startClock)))
			{
			  CPU_ResourcesExhausted();
			  break; // Timeout
			}

                      if (CheckField && !FieldCache->ValidateInField(gplist[j]))
                        continue;      // Nope not in field..

                      size_t w = Parent->GetMainMdt ()->LookupByGp (gplist[j]);
                      if (w == 0)
                        continue; // Could not find GP
                      if (DateRange.Defined())
                        {
                          if (w != old_w)
                            {
                              if (Parent->GetMainMdt ()->GetEntry (w, &mdtrec))
                                {
                                  old_w = w;
                                  IsDeleted = mdtrec.GetDeleted();
                                  rec_date = mdtrec.GetDate();
                                }
                              else
                                IsDeleted = true; // Could not find record
                            }
                          if (IsDeleted)
                            continue; // Is deleted
                          // Check date range
                          if (!DateRange.Contains(rec_date))
                            continue; // Not in range
#if 1
                          iresult.SetDate (rec_date); // Set Date
#endif
                        } // date range check

                      if (useCase)
                        {
                          UCHR tmp[StringCompLength + 1];
                          // Need to get the Source..
                          if (GetIndirectBuffer (gplist[j], tmp) <= 0)
                            continue; // Error, but be gracefull
                          if (tmp[0] != QueryTerm.GetChr(1))
                            continue; // Bad case
                        }
                      // Bump up the count
                      iresult.SetMdtIndex (w);
                      // Code to mark "hit" coordinates
                      Fc.SetFieldStart(gplist[j]);
                      Fc.SetFieldEnd(gplist[j] + ((unsigned char)tp[0] - 1)); // Need to handle this differently
                      iresult.SetHitTable (Fc);
                      // Add entry
#if AWWW
                      pirset->FastAddEntry (iresult);
#else
                      pirset->AddEntry (iresult, true);
#endif
                      if (ClippingThreshold > 0 && pirset->GetTotalEntries() > ClippingThreshold)
                        break;
                    } // for()
                } // match
            } // for()
        } // if have something
    } /* for */
  // Close dangling handles..
  if (fpi) ffclose(fpi);

  // Clean up
  if (gplist) delete[] gplist; // Clean-up

#if AWWW
  pirset->MergeEntries(true);
#endif

  return pirset;
}


PIRSET INDEX::SoundexSearch (const STRING& QueryTerm, const STRING& FieldName, bool useCase)
{
  int num_hits = 0;
  PGPTYPE  gplist = NULL;
  size_t   gplist_siz = 0;
  PIRSET   pirset = new IRSET (Parent);

  char n[16];

  // Reference Hash
  UINT8 Number = SoundexEncode (QueryTerm); // Get the Soundex Hash for our term..
#if 1
//cerr << "SoundexHash=" << Number << endl;
#endif

  // Copy lower case into a BCPL-style string...
  // n[0] contains the match length
  // n[1] contains the string length
  // &n[2] contains the lower case term..
  n[0] = '\0';
  n[1] = '\1';
  n[2] = _ib_tolower (QueryTerm.GetChr(1));
  n[3] = '\0';

  INT NumberOfIndexes =  GetIndexNum();
  FILE *fpi = NULL;

  FC Fc;
  IRESULT iresult;
  MDTREC mdtrec;
  size_t old_w = 0;

  bool IsDeleted = false;
  SRCH_DATE   rec_date;

  iresult.SetVirtualIndex( (UCHR)( Parent->GetVolume(NULL) ) );
  iresult.SetMdt (Parent->GetMainMdt() );
  iresult.SetHitCount (1);
  iresult.SetAuxCount (1);
  iresult.SetScore (0);

  const bool CheckField= (FieldName.GetLength() != 0);
// bool      Disk = true;
  int      FirstTime = 1;

  // Loop through all sub-indexes
  MMAP MemoryMap;
  for (INT jj = 1; (jj <= NumberOfIndexes) || (NumberOfIndexes == 0); jj++)
    {

      if (NumberOfIndexes == 0)
        {
          if ((fpi = ffopen (IndexFileName, "rb")) == NULL)
            continue;
          NumberOfIndexes = -1;
          MemoryMap.CreateMap(SisFileName);
        }
      else
        {
          if (fpi) ffclose(fpi); // Close old handle
          STRING s;
          s.form("%s.%d", IndexFileName.c_str(), jj);
          if ((fpi = ffopen(s, "rb")) == NULL)
            continue;
          s.form("%s.%sd", SisFileName.c_str(), jj);
          MemoryMap.CreateMap(s);
        }
      if (!MemoryMap.Ok())
        continue;
      MemoryMap.Advise(MapRandom);
      const char *Map = (char *)MemoryMap.Ptr();
      const size_t size = MemoryMap.Size();

#if 1
                         // New edition indexes?
      const int    off = (( ( size % (sizeof(GPTYPE) + Map[0] + 1) ) == 2) ? 2  :
                         // old edition?
                          ( size % (sizeof(GPTYPE) + StringCompLength + 1) == 0 ? 0 : 2) );
#else /* was */
      const int  off = (size % (sizeof(GPTYPE)+StringCompLength+1) == 0 ? 0 : 2);
#endif
      const char *Buffer = Map + off;
      const size_t compLength = off ? (unsigned char)Map[0] : StringCompLength;
      const size_t dsiz = compLength + sizeof(GPTYPE)+1;
      if (off)
        {
          Charset.SetSet((BYTE)Map[1]);
          if (!Charset.Ok())
            message_log (LOG_ERROR, "Can't set Charset %d!", (int)((BYTE)Map[1]));
        }

      // Find any match..
      char *t = __priv_bsearch(&n, Buffer, size/dsiz, dsiz, Charset.SisKeys());
      if (t)
        {
          // We now have ANY Match...
          char *High = t; // Last Element in range
          char *Low = t; // First Element in range
          char *tp;
          // Scan back to low (replace later with bsearches)
          for (tp = (char *)t; tp >= Buffer; tp -= dsiz)
            {
              if (n[2] != tp[1])
                break;
              Low = tp;
            }

          // Now look forward ... (replace later)
          for (tp = (char *)t; tp < &Buffer[size]; tp += dsiz)
            {
              if (n[2] != tp[1])
                break;
              High = tp;
            }
          // We now have the range...
          // Determine the size of the index
          // const off_t Size = GetFileSize(fpi);
          // const INT Off = Size % sizeof (GPTYPE);
          for (tp = Low; tp <= High; tp += dsiz)
            {
	      // Compare encode terms
              if (Number == SoundexEncode(&tp[1]))
                {
                  // Soundex Matched..
                  const GPTYPE end = GP((unsigned char *)tp, compLength+1);
                  // Make sure not the first element...
                  const GPTYPE start = ((long)tp - (long)Buffer) ? GP((unsigned char *)tp, -(int)sizeof(GPTYPE)) + 1 : 0;
                  int nhits = end - start + 1; // @@@@@@@@@@@ was -1  ( Mon Jul 17 13:51:14 MET DST 2000)
                  if (nhits <= 0)
                    {
                      continue; // Paranoia
                    }
                  if ((size_t)nhits > gplist_siz)
                    {
                      // Reallocate space..
                      if (gplist) delete[] gplist;
                      gplist_siz += nhits + 100;
                      try
                        {
                          gplist = new GPTYPE[gplist_siz];
                        }
                      catch (...)
                        {
                          gplist = NULL;
                          Parent->SetErrorCode(2); // Temp error
                          num_hits = nhits = 0; // Not enough memory
                        }
                    }
                  // Read the GPs..
                  if (nhits)
                    num_hits = GpFread(gplist, nhits, start, fpi); /* @@ */

                  // Now sort..
                  // if (num_hits > 1) QSORT(gplist, num_hits, sizeof(GPTYPE), gpcomp); // Speed up looking

                  if (CheckField && FirstTime)
                    {
                      FieldCache->SetFieldName(FieldName); // Note: Here we can adise we Disk
                      FirstTime = 0;
                    }
                  for (INT j=0; j<num_hits; j++)
                    {

                      int     ok = 1;
                      if (CheckField)
                        ok = FieldCache->ValidateInField(gplist[j]);
                      if (!ok)
                        continue;      // Nope not in field..

                      size_t w = Parent->GetMainMdt ()->LookupByGp (gplist[j]);
                      if (w == 0)
                        continue; // Could not find GP
                      if (DateRange.Defined())
                        {
                          if (w != old_w)
                            {
                              if (Parent->GetMainMdt ()->GetEntry (w, &mdtrec))
                                {
                                  old_w = w;
                                  IsDeleted = mdtrec.GetDeleted();
                                  rec_date = mdtrec.GetDate();
                                }
                              else
                                IsDeleted = true; // Could not find record
                            }
                          if (IsDeleted)
                            continue; // Is deleted
                          // Check date range
                          if (!DateRange.Contains(rec_date))
                            continue; // Not in range
#if 1
                          iresult.SetDate (rec_date); // Set Date
#endif
                        } // date range check

                      if (useCase)
                        {
                          UCHR tmp[StringCompLength + 1];
                          // Need to get the Source..
                          if (GetIndirectBuffer (gplist[j], tmp) <= 0)
                            continue; // Error, but be gracefull
                          if (tmp[0] != QueryTerm.GetChr(1))
                            continue; // Bad case
                        }
                      // Bump up the count
                      iresult.SetMdtIndex (w);
                      // Code to mark "hit" coordinates
                      Fc.SetFieldStart(gplist[j]);
                      Fc.SetFieldEnd(gplist[j] + ((unsigned char)tp[0] - 1)); // Need to handle this differently
                      iresult.SetHitTable (Fc);
                      // Add entry
#if AWWW
                      pirset->FastAddEntry (iresult);
#else
                      pirset->AddEntry (iresult, true);
#endif
                      if (ClippingThreshold > 0 && pirset->GetTotalEntries() > ClippingThreshold)
                        break;
                    } // for()
                } // match
            } // for()
        } // if have something
    } /* for */
  // Close dangling handles..
  if (fpi) ffclose(fpi);

  // Clean up
  if (gplist) delete[] gplist; // Clean-up

#if AWWW
  pirset->MergeEntries(true);
#endif

  return pirset;
}

#if 0
//
// TODO:
// To handle the phrase *"ubular bicycle wheels" --> *"ubular" "bicycle wheels" PRECEDES
PIRSET INDEX::LeftTruncatedSearch(const STRING& QueryTerm, const STRING& FieldName, bool useCase)
{
  int             num_hits = 0;
  PGPTYPE         gplist = NULL;
  size_t          gplist_siz = 0;
  PIRSET          pirset = new IRSET(Parent);

  INT             NumberOfIndexes = GetIndexNum();
  FILE           *fpi = NULL;

  const bool CheckField= (FieldName.GetLength() != 0);

  FC              Fc;
  IRESULT         iresult;
  MDTREC          mdtrec;
  size_t          old_w = 0;

  STRING          Pattern;
  const size_t    TermLength = QueryTerm.GetLength();

  Pattern = "*" + QueryTerm;
  Pattern.ToLower();

  iresult.SetVirtualIndex((UCHR) (Parent->GetVolume(NULL)));
  iresult.SetMdt (Parent->GetMainMdt() );
  iresult.SetHitCount (1);
  iresult.SetAuxCount (1);
  iresult.SetScore (0);

// bool      Disk = true;
  bool      FirstTime = true;

  // Loop through all sub-indexes
  MMAP MemoryMap;
  for (INT jj = 1; (jj <= NumberOfIndexes) || (NumberOfIndexes == 0); jj++)
    {

      if (NumberOfIndexes == 0)
        {
          if ((fpi = ffopen(IndexFileName, "rb")) == NULL)
            continue; // Err next
          NumberOfIndexes = -1;
          MemoryMap.CreateMap(SisFileName);
        }
      else
        {
          if (fpi) ffclose(fpi);	// Close old handle
          STRING          s;
          s.form("%s.%d", IndexFileName.c_str(), jj);
          if ((fpi = ffopen(s, "rb")) == NULL)
            continue; // Err next
          s.form("%s.%d", SisFileName.c_str(), jj);
          MemoryMap.CreateMap(s);
        }

      if (!MemoryMap.Ok())
        continue;
      // Advise the memory manager that we want random access for binary search..
      MemoryMap.Advise(MapRandom); // Randon access
      const char *Map = (char *)MemoryMap.Ptr();
      const size_t size = MemoryMap.Size();

#if 1
                         // New edition indexes?
      const int    off = (( ( size % (sizeof(GPTYPE) + Map[0] + 1) ) == 2) ? 2  :
                         // old edition?
                          ( size % (sizeof(GPTYPE) + StringCompLength + 1) == 0 ? 0 : 2) );
#else
      const int  off = (size % (sizeof(GPTYPE)+StringCompLength+1) == 0 ? 0 : 2);
#endif
      const char *Buffer = Map + off;
      const size_t compLength = off ? (unsigned char)Map[0] : StringCompLength;
      const size_t dsiz = compLength + sizeof(GPTYPE)+1;

      if (off)
        {
          SetGlobalCharset( (BYTE)Map[1] );
        }

      // We now have the range...
      // Determine the size of the index
      const off_t      Size = GetFileSize(fpi);
      // const INT    Off = Size % sizeof(GPTYPE);
      char            tmp[StringCompLength+10];

      for (const char *tp = Buffer; tp < &Map[size-sizeof(GPTYPE)]; tp += dsiz)
        {
          memcpy(tmp, &tp[1], (unsigned char)(*tp));
          tmp[(unsigned char)tp[0]] = '\0';
          if (Pattern.MatchWild(tmp))
            {
              // Glob Matched..
              const GPTYPE end = GP((unsigned char *)tp, compLength+1);
              // Make sure not the first element...
              const GPTYPE start = ((long)tp - (long)Buffer) ? GP((unsigned char *)tp, -sizeof(GPTYPE)) + 1 : 0;

              int             nhits = end - start - 1;
              if (nhits <= 0)
                {
                  continue;	// Paranoia
                }
              if ((size_t)nhits > gplist_siz)
                {
                  // Reallocate space..
                  if (gplist)
                    delete[] gplist;
                  gplist_siz += nhits + 100;
                  gplist = new GPTYPE[gplist_siz];
                }
              // Read the GPs..
              num_hits = GpFread(gplist, nhits, start, fpi);	/* @@ */
              // Now sort..
              // if ((size_t)num_hits > 1) QSORT(gplist, num_hits, sizeof(GPTYPE), gpcomp);	// Speed up looking

              if (CheckField && FirstTime)
                {
                  FieldCache->SetFieldName(FieldName); // Note: Here we can adise we Disk
                  FirstTime = false;
                }

              for (INT j = 0; j < num_hits; j++)
                {

                  bool     ok = true;
                  if (CheckField)
                    ok = FieldCache->ValidateInField(gplist[j]);
                  if (!ok)
                    continue;	// Nope not in field..

                  size_t       w = Parent->GetMainMdt()->LookupByGp(gplist[j]);
                  if (w == 0)
                    continue;	// Could not find GP

                  if (DateRange.Defined())
                    {
                      if (w != old_w)
                        {
                          if (Parent->GetMainMdt()->GetEntry(w, &mdtrec))
                            old_w = w;
                          else
                            continue; // Skip
                        }
                      if (mdtrec.GetDeleted())
                        continue; // Is deleted
                      // Check date range
                      const SRCH_DATE rec_date = mdtrec.GetDate();
                      if (rec_date.Ok() && !DateRange.Contains(rec_date))
                        continue;	// Not in range
#if 1
                      iresult.SetDate(rec_date);	// Set Date
#endif
                    }		// date range check
                  if (useCase)
                    {
                      // Need to get the Source..
                      if (GetIndirectBuffer(gplist[j], (UCHR *)tmp) <= 0)
                        continue;	// Error, but be gracefull
                      tmp[(unsigned char)tp[0]] = '\0';
                      if (!Pattern.MatchWild(tmp))
                        continue;
                    }
                  // Bump up the count
                  iresult.SetMdtIndex(w);
                  // Code to mark "hit" coordinates
                  Fc.SetFieldStart(gplist[j] + ((unsigned char) *tp - TermLength));
                  Fc.SetFieldEnd(gplist[j] + ((unsigned char) *tp - 1));	// Need to handle this differently
                  iresult.SetHitTable(Fc);

                  // Add entry
#if AWWW
                  pirset->FastAddEntry(iresult);
#else
                  pirset->AddEntry(iresult, true);
#endif
                  if (ClippingThreshold > 0 && pirset->GetTotalEntries() > ClippingThreshold)
                    break;
                }			// for()
            }			// match
        }  // for()
    }				/* for */
  // Close dangling handles..
  if (fpi) ffclose(fpi);

  // Clean up
  if (gplist) delete[] gplist;	// Clean-up

#if AWWW
  pirset->MergeEntries(true);
#endif

  return pirset;
}
#endif


void INDEX::CPU_ResourcesExhausted()
{
  message_log (LOG_INFO, "Term search limit (%u ms) exceeded: Valid subset of results available",
                       (int)((MaxCPU_ticks*1000.0)/CLOCKS_PER_SEC) );
  // Resources exhausted - valid subset of results available
  Parent->SetErrorCode(33); // 

}


#if 1
PIRSET INDEX::GlobSearch(const STRING& QueryTerm, const STRING& fieldName, bool useCase)
{
  if ( QueryTerm.IsWild () == 0)
    {
      // Is not a Glob
      return TermSearch (QueryTerm, fieldName, useCase ? AlwaysMatches : Unspecified );
    }

  STRING FieldName( fieldName);

  if (DebugMode)
    message_log (LOG_DEBUG, "GlobSearch(%s, %s, %d)", QueryTerm.c_str(), fieldName.c_str(), (int)useCase);
#if 1
  // Handle wildcards in field names
  if (FieldName.GetLength())
    {
      FieldName.Replace("\\", "/");
      // Does only 1 matching field exist? If so use it.
      if (FieldName.IsWild() && Parent->GetMainDfdt()->GetFileNumber (FieldName) == 0)
        {
          STRING subtag;
          // Is the right hand side without wildcards? If so use it first..
          // Is the left hand side without wildcards? If so use it..

          if (!(subtag = FieldName.Right('/')).IsWild() || !(subtag = FieldName.Left('/')).IsWild() )
            {
              IRSET *irset = GlobSearch(QueryTerm, subtag, useCase);
              if (irset)
                {
                  irset->Within(FieldName);
                  return irset;
                }
              return NULL;
            }
          // Search the whole and do a within..
          IRSET *irset = GlobSearch(QueryTerm, NulString, useCase);
          if (irset)
            {
              irset->Within(FieldName);
              return irset;
            }
        }
      else if (!Parent->FieldExists(FieldName))
        {
          Parent->SetErrorCode(114); // "Unsupported Use attribute"
          // Field does not exist so don't bother searching
          return  new IRSET (Parent);
        }
    }
#endif

  clock_t         startClock = clock();

  int             num_hits = 0;
  PGPTYPE         gplist = NULL;
  size_t          gplist_siz = 0;
  PIRSET          pirset = new IRSET(Parent);

  INT             NumberOfIndexes = GetIndexNum();
  FILE           *fpi = NULL;

  const bool CheckField= (FieldName.GetLength() != 0);

  FC              Fc;
  IRESULT         iresult;
  MDTREC          mdtrec;
  size_t          old_w = 0;

  STRING          Pattern;

  Pattern = QueryTerm;
  Pattern.ToLower();

  iresult.SetVirtualIndex((UCHR) (Parent->GetVolume(NULL)));
  iresult.SetMdt (Parent->GetMainMdt() );
  iresult.SetHitCount (1);
  iresult.SetAuxCount (1);
  iresult.SetScore (0);

// bool      Disk = true;
  bool      FirstTime = true;

  // Loop through all sub-indexes
  MMAP MemoryMap;

//cerr << "GlobSearch..." << endl;
  for (INT jj = 1; (jj <= NumberOfIndexes) || (NumberOfIndexes == 0); jj++)
    {

      if (NumberOfIndexes == 0)
        {
          if ((fpi = ffopen(IndexFileName, "rb")) == NULL)
            continue; // Err next
          NumberOfIndexes = -1;
          MemoryMap.CreateMap(SisFileName);
        }
      else
        {
          if (fpi) ffclose(fpi);	// Close old handle
          STRING          s;
          s.form("%s.%d", IndexFileName.c_str(), jj);
          if ((fpi = ffopen(s, "rb")) == NULL)
            continue; // Err next
          s.form("%s.%d", SisFileName.c_str(), jj);
          MemoryMap.CreateMap(s);
        }

      if (!MemoryMap.Ok())
        continue;
      // Advise the memory manager that we want random access for binary search..
      MemoryMap.Advise(MapRandom); // Randon access
      const char *Map = (char *)MemoryMap.Ptr();
      const size_t size = MemoryMap.Size();

#if 1
                         // New edition indexes?
      const int    off = (( ( size % (sizeof(GPTYPE) + Map[0] + 1) ) == 2) ? 2  :
                         // old edition?
                          ( size % (sizeof(GPTYPE) + StringCompLength + 1) == 0 ? 0 : 2) );
#else /* was */
      const int  off = (size % (sizeof(GPTYPE)+StringCompLength+1) == 0 ? 0 : 2);
#endif
      const char *Buffer = Map + off;
      const size_t compLength = off ? (unsigned char)Map[0] : StringCompLength;
      const size_t dsiz = compLength + sizeof(GPTYPE)+1;

      if (off)
        {
          SetGlobalCharset( (BYTE)Map[1] );
        }

      // We now have the range...
      // Determine the size of the index
      // const off_t      Size = GetFileSize(fpi);
      // const INT    Off = Size % sizeof(GPTYPE);
      char            tmp[StringCompLength+10];

      for (const char *tp = Buffer; tp < &Map[size-sizeof(GPTYPE)]; tp += dsiz)
        {
          memcpy(tmp, &tp[1], (unsigned char)(*tp));
          tmp[(unsigned char)tp[0]] = '\0';
          if (Pattern.MatchWild(tmp))
            {
              // Glob Matched..
              const GPTYPE end = GP((unsigned char *)tp, compLength+1);
              // Make sure not the first element...
              const GPTYPE start = ((long)tp - (long)Buffer) ? GP((unsigned char *)tp, -(int)sizeof(GPTYPE)) + 1 : 0;

              int             nhits = end - start + 1;
              if (nhits <= 0)
                {
                  continue;	// Paranoia
                }
              if ((size_t)nhits > gplist_siz)
                {
                  // Reallocate space..
                  if (gplist)
                    delete[] gplist;
                  gplist_siz += nhits + 100;
                  gplist = new GPTYPE[gplist_siz];
                }
              // Read the GPs..
              num_hits = GpFread(gplist, nhits, start, fpi);	/* @@ */
              // Now sort..
              // if ((size_t)num_hits > 1) QSORT(gplist, num_hits, sizeof(GPTYPE), gpcomp);	// Speed up looking

              if (CheckField && FirstTime)
                {
                  FieldCache->SetFieldName(FieldName); // Note: Here we can adise we Disk
                  FirstTime = false;
                }

              for (INT j = 0; j < num_hits; j++)
                {
		  size_t hits = 0; // True hits

		  if (hits % 1001 == 1000 && (MaxCPU_ticks < (clock() - startClock)))
		    {
		      CPU_ResourcesExhausted();
		      break; // Timeout
		    }

                  bool     ok = true;
                  if (CheckField)
                    ok = FieldCache->ValidateInField(gplist[j]);
                  if (!ok)
                    continue;	// Nope not in field..

                  size_t          w = Parent->GetMainMdt()->LookupByGp(gplist[j]);
                  if (w == 0)
                    continue;	// Could not find GP

                  if (DateRange.Defined())
                    {
                      if (w != old_w)
                        {
                          if (Parent->GetMainMdt()->GetEntry(w, &mdtrec))
                            old_w = w;
                          else
                            continue; // Skip
                        }
                      if (mdtrec.GetDeleted())
                        continue; // Is deleted
                      // Check date range
                      const SRCH_DATE rec_date = mdtrec.GetDate();
                      if (rec_date.Ok() && !DateRange.Contains(rec_date))
                        continue;	// Not in range
#if 1
                      iresult.SetDate(rec_date);	// Set Date
#endif
                    }		// date range check
                  if (useCase)
                    {
                      // Need to get the Source..
                      if (GetIndirectBuffer(gplist[j], (UCHR *)tmp) <= 0)
                        continue;	// Error, but be gracefull
                      tmp[(unsigned char)tp[0]] = '\0';
                      if (!QueryTerm.MatchWild(tmp))
                        continue;
                    }
                  // Bump up the count
                  iresult.SetMdtIndex(w);
                  // Code to mark "hit" coordinates
                  Fc.SetFieldStart(gplist[j]);
                  Fc.SetFieldEnd(gplist[j] + ((unsigned char) tp[0] - 1));	// Need to handle this differently
                  iresult.SetHitTable(Fc);

                  // Add entry
		  hits++;
#if AWWW
                  pirset->FastAddEntry(iresult);
#else
                  pirset->AddEntry(iresult, true);
#endif
                  if (ClippingThreshold > 0 && pirset->GetTotalEntries() > ClippingThreshold)
                    break;
                }			// for()
            }			// match
        }  // for()
    }				/* for */
  // Close dangling handles..
  if (fpi) ffclose(fpi);

  // Clean up
  if (gplist) delete[] gplist;	// Clean-up

#if AWWW
  pirset->MergeEntries(true);
#endif

  return pirset;
}

#else

PIRSET INDEX::GlobSearch(const STRING& QueryTerm, const STRING& FieldName, bool useCase)
{
  int             num_hits = 0;
  PGPTYPE         gplist = NULL;
  size_t          gplist_siz = 0;
  PIRSET          pirset = new IRSET(Parent);

  INT             NumberOfIndexes = GetIndexNum();
  FILE           *fpi = NULL;

  const bool CheckField= (FieldName.GetLength() != 0);

  FC              Fc;
  IRESULT         iresult;
  MDTREC          mdtrec;
  size_t          old_w = 0;

  STRING          Pattern;

  Pattern = QueryTerm;
  Pattern.ToLower();

  iresult.SetVirtualIndex((UCHR) (Parent->GetVolume(NULL)));
  iresult.SetMdt (Parent->GetMainMdt() );
  iresult.SetHitCount (1);
  iresult.SetAuxCount (1);
  iresult.SetScore (0);

// bool      Disk = true;
  bool      FirstTime = true;

  // Loop through all sub-indexes
  MMAP MemoryMap;
  for (INT jj = 1; (jj <= NumberOfIndexes) || (NumberOfIndexes == 0); jj++)
    {

      if (NumberOfIndexes == 0)
        {
          if ((fpi = ffopen(IndexFileName, "rb")) == NULL)
            continue; // Err next
          NumberOfIndexes = -1;
          MemoryMap.CreateMap(SisFileName);
        }
      else
        {
          if (fpi) ffclose(fpi);	// Close old handle
          STRING          s;
          s.form("%s.%d", IndexFileName.c_str(), jj);
          if ((fpi = ffopen(s, "rb")) == NULL)
            continue; // Err next
          s.form("%s.%d", SisFileName.c_str(), jj);
          MemoryMap.CreateMap(s);
        }

      if (!MemoryMap.Ok())
        continue;
      // Advise the memory manager that we want random access for binary search..
      MemoryMap.Advise(MapRandom); // Randon access
      const char *Map = (char *)MemoryMap.Ptr();
      const size_t size = MemoryMap.Size();

#if 1
                         // New edition indexes?
      const int    off = (( ( size % (sizeof(GPTYPE) + Map[0] + 1) ) == 2) ? 2  :
                         // old edition?
                          ( size % (sizeof(GPTYPE) + StringCompLength + 1) == 0 ? 0 : 2) );
#else /* was */
      const int  off = (size % (sizeof(GPTYPE)+StringCompLength+1) == 0 ? 0 : 2);
#endif
      const char *Buffer = Map + off;
      const size_t compLength = off ? (unsigned char)Map[0] : StringCompLength;
      const size_t dsiz = compLength + sizeof(GPTYPE)+1;

      if (off) SetGlobalCharset( (BYTE)Map[1] );

      // We now have the range...
      // Determine the size of the index
      const off_t      Size = GetFileSize(fpi);
      // const INT    Off = Size % sizeof(GPTYPE);
      char            tmp[StringCompLength+10];

      num_hits = 0;
      for (const char *tp = Buffer; tp < &Map[size-sizeof(GPTYPE)]; tp += dsiz)
        {
          memcpy(tmp, &tp[1], (unsigned char)(*tp));
          tmp[(unsigned char)tp[0]] = '\0';
          if (Pattern.MatchWild(tmp))
            {
              // Glob Matched..
              const GPTYPE end = GP((unsigned char *)tp, compLength+1);
              // Make sure not the first element...
              const GPTYPE start = ((long)tp - (long)Buffer) ? GP((unsigned char *)tp, -sizeof(GPTYPE)) + 1 : 0;

              int             nhits = end - start + 1;
              if (nhits <= 0)
                {
                  continue;	// Paranoia
                }
              if ((size_t)(num_hits+nhits) > gplist_siz)
                {
                  // Reallocate space..
                  PGPTYPE  old_gplist = gplist;
                  gplist_siz += gplist_siz + nhits + 100;
                  gplist = new GPTYPE[gplist_siz];
                  if (old_gplist)
                    {
                      memcpy(gplist, old_gplist, sizeof(GPTYPE)*num_hits);
                      delete[] old_gplist;
                    }
                }
              // Read the GPs..
              num_hits += GpFread(gplist+num_hits, nhits, start, fpi);	/* @@ */
            }
        } /* for */
      // Now sort..
      if ((size_t)num_hits > 1) QSORT(gplist, num_hits, sizeof(GPTYPE), gpcomp);	// Speed up looking

      if (CheckField && FirstTime)
        {
          FieldCache->SetFieldName(FieldName); // Note: Here we can adise we Disk

          FirstTime = false;
        }

      for (INT j = 0; j < num_hits; j++)
        {

          bool     ok = true;
          if (CheckField)
            ok = FieldCache->ValidateInField(gplist[j]);
          if (!ok)
            continue;	// Nope not in field..

          size_t          w = Parent->GetMainMdt()->LookupByGp(gplist[j]);
          if (w == 0)
            continue;	// Could not find GP

          if (DateRange.Defined())
            {
              if (w != old_w)
                {
                  if (Parent->GetMainMdt()->GetEntry(w, &mdtrec))
                    old_w = w;
                  else
                    continue; // Skip
                }
              if (mdtrec.GetDeleted())
                continue; // Is deleted
              // Check date range
              const SRCH_DATE rec_date = mdtrec.GetDate();
              if (rec_date.Ok() && !DateRange.Contains(rec_date))
                continue;	// Not in range
#if 1
              iresult.SetDate(rec_date);	// Set Date
#endif
            }		// date range check
          if (useCase)
            {
              // Need to get the Source..
              if (GetIndirectBuffer(gplist[j], (UCHR *)tmp) <= 0)
                continue;	// Error, but be gracefull
              tmp[(unsigned char)tp[0]] = '\0';
              if (!QueryTerm.MatchWild(tmp))
                continue;
            }
          // Bump up the count
          iresult.SetMdtIndex(w);
          // Code to mark "hit" coordinates
          Fc.SetFieldStart(gplist[j]);
          Fc.SetFieldEnd(gplist[j] + ((unsigned char) tp[0]) - 1); // Need to handle this differently
          iresult.SetHitTable(Fc);

          // Add entry
          pirset->FastAddEntry(iresult);
          if (ClippingThreshold > 0 && pirset->GetTotalEntries() > ClippingThreshold)
            goto done;
        }			// for()
    }				/* for */
done:
  pirset->MergeEntries(true);
  // Close dangling handles..
  if (fpi) ffclose(fpi);

  // Clean up
  if (gplist) delete[] gplist;	// Clean-up

  return pirset;
}
#endif


// Right Truncated match
static inline INT LeftTermCompare(const UCHR *term, const UCHR *ptr, const size_t n,
                                  size_t *length = NULL)
{
  return _strncasecmp(term, ptr, n, NULL, length);
}

// Right Truncated Special match
static inline INT LeftTermCompareSpecial(const UCHR *term, const UCHR *ptr, const size_t n,
    size_t *length = NULL)
{
  int diff = StrNCaseCmp(term, ptr, n);
  if (diff == 0 && length)
    *length = n;

  return diff;
}

static inline INT TermCompareSpecial(const UCHR *term, const UCHR *ptr, const size_t n,
                                     size_t *length = NULL)
{
  INT diff = StrNCaseCmp(term, ptr, n);
  if (diff == 0 && IsTermChar(ptr[n]))
    diff = -1; // Not a match

  if (diff == 0 && length)
    *length = n - 1; // added -1  on 1 Nov 2003
  return diff;
}


// Term Match
static inline INT TermCompare(const UCHR *term, const UCHR *ptr, const size_t n,
                              size_t *length = NULL)
{
  bool x;
  INT diff = _strncasecmp(term, ptr, n, &x, length);
  if (diff == 0 && x)
    diff = -1; // Not a match
  return diff;
}


#if 0

// Threaded safe..
static int SisKeys(const void *node1, const void *node2)
{
  const unsigned char *p1 = ((const unsigned char *)node1)+2;
  const unsigned char *p2 = ((const unsigned char *)node2)+1;

  int diff = memcmp(p1, p2, *((const unsigned char *)node1+1));

  if (diff == 0)
    {
      const size_t length = (size_t)*((const unsigned char *)node1);
      if (length && !IsWordSep(p2[length]))
        diff = -p2[length];
    }
  return diff;
}
#endif

#if 0
int PATsearch(char *pat, char *index[], int n /* size of the PAT array */ )
{
  size_t          left, right;
  const size_t    m = strlen(pat);

  /* search left end */
  if (strncmp(pat, index[0], m) <= 0)
    {
      left = 0;
    }
  else if (strncmp(pat, index[n - 1], m) > 0)
    {
      left = n;
    }
  else
    {
      for (size_t i, low = 0, high = n;  i= (high + low) / 2;)
        {
          /* binary search */
          if (strncmp(pat, index[i], m) <= 0)
            high = i;
          else
            low = i;
          if (high - low <= 0)
            {
              left = high;
              break;
            }
        }
    }
  /* search right end */
  if (strncmp(pat, index[0], m) < 0)
    {
      right = -1;
    }
  else if (strncmp(pat, index[n - 1], m) >= 0)
    {
      right = n - 1;
    }
  else
    {
      for (size_t i, low = 0, high = n; i = (high + low) / 2;)
        {
          /* binary search */
          if (strncmp(pat, index[i], m) >= 0)
            low = i;
          else
            high = i;
          if (high - low <= 0)
            {
              right = low;
              break;
            }
        }
    }
}
return (right - left + 1);
}
#endif



int INDEX::findIt(MMAP *MemoryMap, const UCHR *Term, size_t TermLength, bool Truncate,
                  off_t *start, bool *overflow)
{
  int             num_hits = -1;

  if (MemoryMap && MemoryMap->Ok())
    {
      size_t          length;

      if (DebugMode)
        {
          message_log (LOG_DEBUG, "Looking for Term = '%s' (length=%d, Truncate=%s)",
                (const char *)Term, TermLength, Truncate ? "True" : "False" );
        }

      char           *Map = (char *) (MemoryMap->Ptr());

			 // New edition indexes?
      const int    off = (( ( MemoryMap->Size() % (sizeof(GPTYPE) + Map[0] + 1) ) == 2) ? 2  : 
		   	 // old edition?
			  ( MemoryMap->Size() % (sizeof(GPTYPE) + StringCompLength + 1) == 0 ? 0 : 2) );
      char           *Buffer = Map + off;
      const size_t    size = MemoryMap->Size() - off; // Remove of the offset
      const size_t    compLength = off ? (unsigned char) Map[0] : StringCompLength;

      const size_t    dsiz = compLength*sizeof(char) + sizeof(GPTYPE) + 1;

      if (off)
        {
          Charset.SetSet((BYTE)Map[1]);
          if (!Charset.Ok())
            message_log (LOG_ERROR, "Can't set Charset %d!", (int)((BYTE)Map[1]));
        }

      // Copy lower case into a BCPL-style string...
      // n[0] contains the match length
      // n[1] contains the string length
      // &n[2] contains the lower case term..
#ifdef __GNUG__
      unsigned char n[compLength + sizeof(GPTYPE)];
#else
      unsigned char  *n = (unsigned char *)alloca(compLength + sizeof(GPTYPE)); // was +3
      if (n == NULL)
        {
          message_log (LOG_PANIC|LOG_ERRNO, "findIt(): Stack allocation failed");
          Parent->SetErrorCode(2);
          return 0;
        }
#endif

//  if (off) SetGlobalCharset( (BYTE)Map[1] );
      const size_t    maxLength = TermLength < compLength ? TermLength : compLength;

      for (length = 0; length < maxLength && Term[length]; length++)
        n[length + 2] = Charset.ib_tolower(Term[length]);
      n[length + 2] = '\0';       // ASCIIZ
      n[1] = (unsigned char) length;

      // Term is longer than SIS than ALWAYS handle as truncated search
      if (maxLength < TermLength)
        {
          n[0] = '\0';
          *overflow = true;
        }
      else
        {
          n[0] = Truncate ? 0 : (unsigned char) length;
          *overflow = false;
        }

      num_hits = 0;		// Start off with No hits..
      // Find any match..

      char           *t = __priv_bsearch(n, Buffer, size/dsiz, dsiz, Charset.SisKeys());

      if (t)
        {

          if (DebugMode) message_log (LOG_DEBUG, "Found any match t='%s'(%u) [overflow=%d, *n=%d]",
		t+1, (int)((unsigned char)(*t)), *overflow, (int)(n[0]));

          // We now have ANY Match...
          char           *High = t;	// Last Element in range

          char           *Low = t;	// First Element in range

          // If Truncated search the hits can span several elements..
          if (n[0] == '\0' && !*overflow)
            {
              char           *tp;
              // Scan back to low (replace later with bsearches)
              for (tp = (char *) t; tp >= Buffer; tp -= dsiz)
                {
                  if (memcmp(&n[2], &tp[1], (unsigned char) n[1]))
                    break;
                  Low = tp;
                }
	      // Now look forward ... (replace later)
	      for (tp = (char *) t; tp < &Buffer[size]; tp += dsiz)
		{
                  if (memcmp(&n[2], &tp[1], (unsigned char) n[1]))
		    break;
		  High = tp;
		}
	      if (DebugMode)
		message_log (LOG_DEBUG, "Low:='%s' %s High:='%s'", Low+1, High==Low ? "=" : "<>", High+1);
            }

          const GPTYPE    to = GP((unsigned char *)High, compLength+1);
          // Make sure not the first element...
          const GPTYPE    from = ( ( Low - Buffer) ?
                                   GP((unsigned char *)Low, -(int)sizeof(GPTYPE)) + 1 : 0 );
          if (start)
            *start = from;
          num_hits = to - from + 1;
        }
    }
  if (DebugMode)
    message_log (LOG_DEBUG, "Term = '%s' found %d hits",
          (const char *)Term, num_hits );
  return num_hits;
}

int INDEX::findIt(bool Truncate, const STRING& Index,
                  const UCHR *Term, size_t TermLength, off_t *start, bool *overflow)
{
  MMAP MemoryMap(Index, MapRandom);

  return findIt (&MemoryMap, Term, TermLength, Truncate, start, overflow);
}

INT INDEX::find(const STRING& SisFn, const INT Slot, const STRING& Word, bool Truncate,
                off_t *start, bool *overflow)
{
  size_t TermLength = Word.GetLength();
  const UCHR *Term = (const UCHR *)Word;

  if (MemorySISCache == NULL)
    {
      IndexNum = GetIndexNum();
      if (DebugMode) message_log (LOG_DEBUG, "%d sub-indicies (%d slots)", IndexNum, IndexNum+5);
      MemorySISCache = new MMAP_TABLE (IndexNum+5); // Add 5 for extra .sis.# files
      if (MemorySISCache == NULL)
        {
          message_log (LOG_PANIC, "Could not allocate Memory Map Cache with %ld slots.", IndexNum+5);
          return -1; // Bad Error!
        }
    }
  if (MemorySISCache->CreateMap(Slot, SisFn))
    return findIt (MemorySISCache->Map(Slot), Term, TermLength, Truncate, start, overflow);
  message_log (LOG_ERRNO, "Could not create MAP in Slot %d for '%s'!", Slot, SisFn.c_str());
  Parent->SetErrorCode(2);
  return -1; // Err
}


INT INDEX::find(INT Index, const STRING& Word, bool Truncate,
                off_t *start, bool *overflow)
{
  INT Slot = 0;
  STRING SisFn (SisFileName);
  if (Index)
    {
      SisFn.Cat (".");
      SisFn.Cat ((INT)Index);
      Slot = Index - 1;
    }
  return find(SisFn, Slot, Word, Truncate, start, overflow);
}

long INDEX::TermFreq(const STRING& Word, bool Truncate)
{
  const INT       NumberOfIndexes = GetIndexNum();
  long            total = 0;
  INT             ip;
  int             err = 0;

  for (INT jj = NumberOfIndexes ? 1 : 0; jj <= NumberOfIndexes; jj++)
    {
      if ((ip = find(jj, Word, Truncate, (off_t *)NULL, (bool *)NULL)) > 0)
	{
          total += ip;
	}
      else if (ip < 0)
	{
          err++;
	}
    }
  if (total == 0 && err)
    total = -1;
  return total;
}


bool INDEX::IsSpecialTerm( const UCHR *Term) const
  {
    if (Term == NULL) return false;

    bool result = false;
    if ( _ib_isalnum(*(Term)))
      {
        // Handle things like C++
        while (*++Term)
          {
            if (_ib_isspace(*Term))
              return false;
            else if (!result)
              result = true;
          };
      }
    else
      {
        // Handle things like 1.1
        do
          {
            if (_ib_isspace(*Term))
              return false;
            if (!result && _ib_ispunct(*Term) && _ib_isalnum(*(Term+1)))
              result = true;
          }
        while (*++Term);
      }
    return result;
  }

// Private Service Class for storing hits and their lengths
class HITLIST {
  public:
    HITLIST(size_t indexes = 1, size_t mAdvice = 0)
    {
      TotalElements = 0;
      maxHits  = 0;
      GpTable  = NULL;
      LenTable = NULL;
      numberOfIndexes = indexes > 0 ? indexes : 1;
      firstAllocAdvice = mAdvice;
    }
    ~HITLIST()
    {
      if (GpTable)  delete[] GpTable;
      if (LenTable) delete[] LenTable;
    }
    bool Add (GPTYPE gp, short length)
    {
      if ( TotalElements >= maxHits )
        if (Expand() == false)
          return false;
      if (GpTable == NULL || LenTable == NULL)
        {
          message_log (LOG_PANIC, "Hitlist got zapped to NIL!!!?");
          return false;
        }
      GpTable[TotalElements] = gp;
      LenTable[TotalElements++] = length;
      return true;
    }

    bool Expand()
    {
      // Expand (4 x current + 1024)
      size_t          newMax;

      if (maxHits == 0 && firstAllocAdvice > 0) newMax = firstAllocAdvice;
      else if (maxHits < INT_MAX/5) newMax = TotalElements*4 + 4096*numberOfIndexes;
      else if (maxHits < INT_MAX/4) newMax = TotalElements*3 + 4096*numberOfIndexes;
      else if (maxHits < INT_MAX/3) newMax = TotalElements*2 + 4096*numberOfIndexes;
      else if (maxHits < INT_MAX)   newMax = TotalElements   + 4096*numberOfIndexes;
      else
        {
          newMax = maxHits + 1;
          message_log (LOG_PANIC, "Result record capacity exceeded %d>%ld!", maxHits, INT_MAX);
        }
      if (newMax <= maxHits)
        return true; // Already got enough!

      GPTYPE    *newGpTable = new GPTYPE[newMax];
      if (newGpTable == NULL)
        {
          message_log (LOG_PANIC|LOG_ERRNO, "Could not get %ld bytes for hit lists.", newMax*sizeof(GPTYPE));
          return false;
        }
      short     *newLenTable = new short[newMax];
      if (newLenTable == NULL)
        {
          delete[] newGpTable; // Don't need it since we failed!
          message_log (LOG_PANIC|LOG_ERRNO, "Could not get %ld bytes for hit lists.", newMax*sizeof(GPTYPE));
          return false;
        }
      if (GpTable)
        {
          memcpy ( newGpTable, GpTable,  TotalElements*sizeof ( GPTYPE ) );
          delete[] GpTable;
        }
      GpTable = newGpTable;
      if (LenTable)
        {
          memcpy ( newLenTable, LenTable, TotalElements*sizeof(short));
          delete[] LenTable;
        }
      LenTable = newLenTable;
      maxHits = newMax;
      return true;
    }

    GPTYPE      operator[](size_t n) const {return GpTable[n]; }
    short       operator()(size_t n) const {return LenTable[n]; }
    FC          Fc(size_t i, int Offset = 0) const
      {
        return  FC(GpTable[i]-Offset, GpTable[i] - Offset + LenTable[i]);
      }

    size_t      TotalElements;
  private:
    GPTYPE     *GpTable;
    short      *LenTable;
    size_t      maxHits;
    size_t      firstAllocAdvice, numberOfIndexes;
};


PIRSET          INDEX::TermSearch ( const STRING& QueryTerm, const STRING& fieldName, enum MATCH Typ )
{
  float           Cost = 1.0; // Cost of this caculation
  const UCHR     *FullTerm = ( const UCHR * ) QueryTerm.c_str ();
  STRING          FieldName(fieldName);

 if (DebugMode)
    message_log (LOG_DEBUG, "TermSearch(%s, %s, %d)", QueryTerm.c_str(), fieldName.c_str(), (int)Typ);

#if 1
  // Handle wildcards in field names
  if (FieldName.GetLength())
    {
      FieldName.Replace("\\", "/");
      // Does only 1 matching field exist? If so use it.
      if (FieldName.IsWild() && Parent->GetMainDfdt()->GetFileNumber (FieldName) == 0)
        {
          STRING subtag;
          // Is the right hand side without wildcards? If so use it first..
          // Is the left hand side without wildcards? If so use it..

          if (!(subtag = FieldName.Right('/')).IsWild() || !(subtag = FieldName.Left('/')).IsWild() )
            {
              IRSET *irset = TermSearch(QueryTerm, subtag, Typ);
              if (irset)
                {
                  irset->Within(FieldName);
                  return irset;
                }
              return NULL;
            }
          // Search the whole and do a within..
          IRSET *irset = TermSearch(QueryTerm, NulString, Typ);
          if (irset)
            {
              irset->Within(FieldName);
              return irset;
            }
        }
      else if (!Parent->FieldExists(FieldName))
        {
          Parent->SetErrorCode(114); // "Unsupported Use attribute"
          // Field does not exist so don't bother searching
          return  new IRSET (Parent);
        }
    }
#endif

  // Skip leading white space
  if ( FullTerm )
    {
#if 0 /* Skip stuff that's not Term Characters....  May 28 2008 */
      while (*FullTerm && !IsTermChr (FullTerm)) FullTerm++;
#else
      while ( _ib_isspace ( *FullTerm ) ) FullTerm++;
#endif
      if (DebugMode) message_log (LOG_DEBUG, "TermSearch (\"%s\", \"%s\", %s(%d)) --> %s",
                             QueryTerm.c_str(), FieldName.c_str(), MatchType(Typ), (int)Typ, (const char *)FullTerm);
    }

  // Do we have anything?
  if ( FullTerm == NULL || *FullTerm == '\0' )
    {
      Parent->SetErrorCode(125); // Malformed search term
      return new IRSET ( Parent );// NIL term
    }
  size_t          x = strlen ( (const char *) FullTerm );

  if ( Typ == Unspecified || Typ == AlwaysMatches )
    {
      INT             wild = QueryTerm.IsWild ();
      if ( wild > 0 && ( ( size_t ) wild < QueryTerm.GetLength () || QueryTerm.GetChr ( wild ) != '*' ) )
        return GlobSearch ( QueryTerm, FieldName, Typ == AlwaysMatches );
    }
  if ( Typ == Unspecified )
    {
      if ( x < 2 || FullTerm[x - 2] != '\\' )
        {
          if ( FullTerm[--x] == '*' )
            {
              Typ = LeftMatch;	// Right Truncated Search
            }
          else if ( FullTerm[x] == '~' )
            {
              Typ = Phonetic;		// Soundex Search
            }
          else if ( FullTerm[x] == '=' )
            {
              if ( Typ == Phonetic )
                Typ = PhoneticCase;
              else
                Typ = ( Typ == LeftMatch ) ? LeftAlwaysMatches : AlwaysMatches;	// Case dependent
            }
          else
            {
              Typ = Exact;
              x++;			// Go back forward...
            }
        }
      else
        Typ = Exact;
    }
  if ( _ib_isspace ( FullTerm[x] ) )
    {
      if ( Typ == LeftMatch )
        Typ = Exact;		// "This is *" --> "This is"

      else if ( Typ == LeftAlwaysMatches )
        Typ = AlwaysMatches;	// Case dependent
      // Ignore tailing while space

      while ( _ib_isspace ( FullTerm[x] ) )
        x--;
    }
  bool     myStoreHitCoordinates = storeHitCoordinates;

  if ( SetCache && (ClippingThreshold == 0 || ClippingThreshold > 1) )
    {
//cerr << "Check Cache" << endl;
      INT             in_cache =
        SetCache->Check ( SearchObj(FullTerm, Typ, FieldName, DateRange, ClippingThreshold)  );
      if ( in_cache != -1 )
        return SetCache->Fetch ( in_cache );
    }
  INT ( *Compare ) ( const UCHR *, const UCHR *, const size_t, size_t *);

  const bool Special = IsSpecialTerm ( FullTerm );
  switch ( Typ )
    {
    case FreeForm:
      myStoreHitCoordinates = false;
      // Fall into...
    case Exact:
    case ExactTerm:
    default: // Keep Compiler Happy
      Compare = (Special ? TermCompareSpecial : TermCompare);
      break;
    case LeftAlwaysMatches:
    case LeftMatch:
      Compare = (Special ? LeftTermCompareSpecial : LeftTermCompare);
      break;
    case PhoneticCase:
    case Phonetic:
      if (useSoundex)
	return SoundexSearch ( FullTerm, FieldName, Typ == PhoneticCase ? true : false );
      return MetaphoneSearch ( FullTerm, FieldName, Typ == PhoneticCase ? true : false );
    }

  clock_t         startClock = clock();

  STRING          RealTerm ( FullTerm, x );	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  STRING          SearchTerm ( TermClean ( RealTerm, true ) );

  FullTerm = (const UCHR *) SearchTerm.c_str ();

  // Is first word a stop word?
  const UCHR     *WordTerm;
  const size_t    FullTerm_length = x;

  // Skip after ' as in l'espace but not can't or computers'
  if ( IsTermChar ( FullTerm[0] ) && ( WordTerm = (UCHR *) strchr ((const char *)FullTerm, '\'' ) ) != NULL
       && IsTermChar ( WordTerm[1] ) && IsTermChar ( WordTerm[2] ) )
    {
      x -= ++WordTerm - FullTerm;
    }
  else
    WordTerm = FullTerm;

#if 1
  if ( *WordTerm == '"' || *WordTerm == '\'' )
    WordTerm++, x--;
#endif

  // Skip over single letter stuff..
  // Tue Aug 29 15:23:15 MET DST 2000
  // Bug: E. Zimmermann could not match..
  if (!Special && WordTerm[1] && !IsTermChr(WordTerm+1)) /// was !IsTermChar(WordTerm[1]) June 2008
    {
      do
        {
          WordTerm++, x--;
        }
      while (!IsTermChr(WordTerm)); // was (!IsTermChar(WordTerm[0])); June 2008
    }


  // Look for a non-stopword...
  while ( x > 0 && IsStopWord ( WordTerm, x ) )
    {
      // Skip over word
      while ( *WordTerm && IsTermChar ( *WordTerm ) && x > 0)
        WordTerm++, x--;
      // Skip while space
      while ( !IsTermChar ( *WordTerm ) && x > 0)
        WordTerm++, x--;
      // Now pointing at word after stop
    }
  if ( x == 0 )
    {
      x = FullTerm_length;
      WordTerm = FullTerm;
    }

  const UCHR     *Term = WordTerm;

  // Find length of first word (max StringCompLength)
  for ( size_t i = 0; i < x /* && i < StringCompLength */ ; i++ )
    {
      if (
#if 1
        !IsTermChr(Term+i)
#else
        !IsTermChar ( Term[i] )
        && !( IsDotInWord(Term[i]) && _ib_isalpha(Term[i+1])) // 2005 Nov for special case of domain.de etc.
#endif
      )
        {
          x = i;			// Length of first word
          break;
        }
    }
  // Empty term or a single letter?
  // added IsStopWord (to allow for single character searches) August 2007
  if (x <= 1 && IsStopWord(Term,x))
    {
      if (x == 0 || !Special)
        {
          if (DebugMode) message_log (LOG_DEBUG, "Empty term or exluded term, ErrorCode 4");
          Parent->SetErrorCode(4);
          return new IRSET ( Parent );
        }
    }

  size_t Term_length = x; // First word length
  // bool Disk = true;
  bool     FirstTime = true;

  // size_t Total = 0;
  // FC *Cache;
  const bool CheckField = ( FieldName.GetLength () != 0 );

  const INT       NumberOfIndexes = GetIndexNum ();
#define HEADROOM(_x,_y) ( ((_x)/(_y) + 1)*(_y))
  const size_t    maxTermLength = HEADROOM ( FullTerm_length, StringCompLength );

  HITLIST         HitList(NumberOfIndexes);
  int             Offset = Term - FullTerm;

  /* The Search core */
  {
    PFILE           fpi = NULL;
    // Need to allocate a bit more due to offset backwards
    const size_t    Buffer_Len = HEADROOM(maxTermLength * sizeof ( UCHR ) + StringCompLength, 1024);
#ifdef __GNUG__
    UCHR            Buffer[Buffer_Len];
#else
    PUCHR           Buffer = ( UCHR * ) alloca (Buffer_Len);
#endif
#undef HEADROOM

    STRING          SisFn;

    STRING          Word ( Term, Term_length );	// My Word

    // Save these for the loops
    const UCHR       *savedTerm = Term;
    size_t            savedTerm_length = Term_length;

    for ( INT jj = NumberOfIndexes ? 1 : 0; jj <= NumberOfIndexes; jj++ )
      {

        Term        = savedTerm;
        Term_length = savedTerm_length;

        // binary search
        if ( NumberOfIndexes == 0 )
          {
            fpi = ffopen ( IndexFileName, "rb" );
            SisFn = SisFileName;
          }
        else
          {
            if ( fpi )
              ffclose ( fpi );	// Close old stream..

            fpi = ffopen ( STRING ().form ( "%s.%d", IndexFileName.c_str (), jj ), "rb" );
            SisFn.form ( "%s.%d", SisFileName.c_str (), jj );
          }
        if ( fpi == NULL )
          continue;		// Next

        // Determine the size of the index
        const off_t      Size = GetFileSize ( fpi );
        const int       Off = Size % sizeof ( GPTYPE );

        GPTYPE          gp;
        off_t           ip;
        off_t           first, last;
        bool     overflow;

#if 1
        if (DebugMode) message_log (LOG_DEBUG, "Find '%s' in %s", Word.c_str(), SisFn.c_str());
        // New SIS code
        ip = find( SisFn, jj, Word, (Typ == LeftAlwaysMatches || Typ == LeftMatch)
                   && (FullTerm_length == Term_length) /* Added  Fri Jul 23 00:31:08 MET DST 1999 */
                   , &first, &overflow );
        if ( ip == 0 )
          {
	    if (DebugMode) message_log (LOG_DEBUG, "[%d] %s not found.", jj, Word.c_str()) ;
            continue;		// Found nothing
          }

        if ( ip > 0 )
          {
            // Lets be clever when doing literal searches...
            if ( FullTerm_length > Term_length+2 )
              {
                const UCHR     *tp = &Term[Term_length];
                int             alt_count = -1;
                bool     heuristic = true;
                unsigned        looks = 0;

                do
                  {
                    int             new_length = 0;
                    // Skip over while space
                    while ( *tp && !IsTermChr ( tp ) )
                      tp++;
                    // Find out length..
                    while ( IsTermChr ( tp+new_length ) )
                      new_length++;
                    if ( new_length > 1 && !IsStopWord ( tp, new_length ) )
                      {
                        off_t          alt_first;

                        looks++; // Looked at something
                        alt_count = findIt ( (Typ == LeftAlwaysMatches || Typ == LeftMatch)
                                             && (tp[new_length] == '\0') /* Added  Fri Jul 23 00:31:08 MET DST 1999 */
                                             , SisFn, tp, new_length, &alt_first, &overflow );
                        if ( alt_count == 0 )
                          break;		// Don't need to continue;

                        if ( alt_count < ip && alt_count != -1 )
                          {
                            // Got a magic word..
                            Offset = tp - FullTerm;
                            Term = tp;
                            Term_length = new_length;
                            ip = alt_count;
                            first = alt_first;
                          }
                      }
                    else
                      heuristic = false;
                    tp += new_length;	// Next word...

                  }
                while ( *tp );

                if (looks)
                  {
                    if ( alt_count == -1) Parent->SetErrorCode(4);

                    if (findConcatWords && jj == NumberOfIndexes && ( alt_count == 0 || forceConcatWords))
                      {
                        STRING newTerm;
                        // We only want to concat words made of letters and not numbers
                        // as they might be part of serial or parts number
                        for (;*FullTerm && !_ib_isdigit(*FullTerm);FullTerm++)
                          {
                            if (IsTermChar(*FullTerm)) newTerm.Cat(*FullTerm);
                          }
                        // Make sure we did not see a number
                        if (*FullTerm == '\0')
                          {
                            ffclose(fpi);
                            return TermSearch(newTerm, FieldName, Typ);
                          }
                      }
                    if ( alt_count <= 0 ) continue;		// No match in index...
                  }

                // Do we have a bunch of *very* common terms?
                if (ip >= PhraseWaterlimit) // Used PhraseWaterlimit instead of CommonWords.. 23 March 2004 edz@nonmonotonic.com
                  {
                    if (!heuristic)
                      {
                        message_log (LOG_INFO, "Phrase \"%s\" uses all common words (%ld > %ld : %ld) [%d]!",
                              FullTerm, (long)ip,  (long)PhraseWaterlimit, (long)CommonWordsThreshold, jj);
                      }
                    else
                      {
                        ffclose ( fpi ); // Close old handle...
                        // Alternative phrase heuristic..
                        SQUERY query;
                        if (query.PhraseToProx (FullTerm, FieldName, Typ == LeftMatch || Typ == LeftAlwaysMatches,
                                                Typ == LeftAlwaysMatches) != 0)
                          {
                            STRING S;
                            query.GetRpnTerm (&S);
                            message_log (LOG_INFO, "Phrase \"%s\" uses all common words (%ld > %ld : %ld) [%d]. \
                                  Using alternative search heuristic: %s",
                                  FullTerm, (long)ip,  (long)PhraseWaterlimit, (long)CommonWordsThreshold, jj, S.c_str());
                            return Search(query);
                          }
                        else
                          message_log (LOG_WARN, "Could not convert \"%s\" to a proximity expression.", FullTerm);
                      }
                  }
              }
            last = first + ip - 1;
          }
        else
          {
#endif
            const UCHR      first_char = _ib_tolower ( Term[0] );
            const off_t     maxip = ( ( Size - Off ) / sizeof ( GPTYPE ) ) - 1;	// Number of Entries in Index
            const off_t     minip = 0;

            off_t           high = maxip;
            off_t           low = minip;
            off_t           oip;

            if ( Term_length > StringCompLength )
              Term_length = StringCompLength;
            ip = ( high + low ) / 2;
            //----------------------------------------------------------------------------
            // find any match
            //----------------------------------------------------------------------------
            INT             z = 0;
            bool     hit = false;
            do
              {
                oip = ip;

                if ( ( gp = GpFread ( ip, fpi ) ) != (GPTYPE)-1 && GetIndirectBuffer ( gp, Buffer ) > 0)
                  {
                    // Do we match?
                    if ( ( z = first_char - ( UCHR ) _ib_tolower ( Buffer[0] ) ) == 0 &&
                         ( z = Compare ( Term, Buffer, Term_length, NULL) ) == 0 )
                      {
                        hit = true;	// Got a match

                        break;
                      }
                    if ( z < 0 )
                      high = ip;
                    else if ( z > 0 && ( ( low = ip + 1 ) > high ) )
                      low = high;	// upper-bound (Should never happen)

                    ip = ( high + low ) / 2;
                  }
                else
                  {
                    // Error
                    if ( ++ip > maxip )
                      ip = maxip;
                  }
              }
            while ( ip != oip );

            // Found anything?
            if ( !hit )
              {
                continue;
              }
            // bracket hits
            off_t             match, nomatch;

            //----------------------------------------------------------------------------
            // find first match
            //----------------------------------------------------------------------------
            low = minip;
            high = ip;
            first = ( low + high ) / 2;
            match = ip;
            nomatch = 0;
            do
              {
                if ( ( gp = GpFread ( first, fpi ) ) != (GPTYPE)-1 && GetIndirectBuffer ( gp, Buffer ) > 0)
                  {
                    // Do we match?
                    z = Compare ( Term, Buffer, Term_length, NULL );
                  }
                else
                  {
                    z = -1;		// Read error (pretend no match)
                  }
                if ( z == 0 )
                  {
                    // Got a match
                    match = high = first;
                  }
                else
                  {
                    // No match
                    nomatch = first;
                    low = first + 1;
                  }
                // Check bounds
                if ( ( first = ( low + high ) / 2 ) > ip )
                  first = ip;		// upper-bound

              }
            while ( ( match - nomatch ) > 5 );

            // Look backwards..
            first = match;
            do
              {
                if ( first > 0 )
                  first--;
                if ( ( gp = GpFread ( first, fpi ) ) != ((GPTYPE)-1) && GetIndirectBuffer ( gp, Buffer ) > 0)
                  {
                    // Do we match?
                    z = Compare ( Term, Buffer, Term_length, NULL);
                  }
              }
            while ( ( z == 0 ) && ( first > 0 ) );

            if ( ( z != 0 ) || ( first > 0 ) )
              first++;

            //----------------------------------------------------------------------------
            // find last match
            //----------------------------------------------------------------------------
            low = ip;
            high = maxip;
            last = low + ( high - low ) / 2;
            match = ip;
            nomatch = maxip;
            do
              {
                if ( ( gp = GpFread ( last, fpi ) ) != (GPTYPE)-1 && GetIndirectBuffer ( gp, Buffer ) > 0)
                  {
                    // Do we match?
                    z = Compare ( Term, Buffer, Term_length, NULL );
                  }
                else
                  {
                    // Read/Seek error (pretend no match)
                    z = -1;
                  }
                if ( z == 0 )
                  {
                    // Got a match
                    match = last;
                    low = last + 1;
                  }
                else
                  {
                    // No match
                    nomatch = high = last;
                  }
                // Check bounds
                if ( ( last = ( low + high ) / 2 ) < ip )
                  last = ip;		// Min.

                else if ( last > maxip )
                  last = maxip;	// Max.

              }
            while ( ( nomatch - match ) > 5 );

            // go forward
            last = match;
            do
              {
                if ( last < maxip )
                  last++;
                if ( ( gp = GpFread ( last, fpi ) ) != (GPTYPE)-1 && GetIndirectBuffer ( gp, Buffer ) > 0)
                  {
                    // Do we match?
                    z = Compare ( Term, Buffer, Term_length, NULL );
                  }
              }
            while ( ( z == 0 ) && ( last < maxip ) );

            if ( ( z != 0 ) || ( last < maxip ) )
              last--;

            // first++;
            // last--;

#if 1
          }
#endif

        //----------------------------------------------------------------------------
        // Build Result Set
        //----------------------------------------------------------------------------
        if (first > last)
          {
            message_log (LOG_PANIC,
#ifdef _WIN32
		"INDEX::TermSearch Last(%I64d)>first(%I64d)!" 
#else
		"INDEX::TermSearch Last(%lld)>first(%lld)!"
#endif
                  , (long long)last, (long long)first);
            ffclose(fpi);
            Parent->SetErrorCode(1);
            continue;
          }

        x = last - first + 1;

#define C_FACTOR 50
        if ((ClippingThreshold > 0) && (x > ClippingThreshold*C_FACTOR))
          {
            if (DebugMode)
              message_log (LOG_INFO, "Clipping from %d looks ('%s' frequency) down to %d (%d threshold/%d factor)",
                    x, Word.c_str(), ClippingThreshold*C_FACTOR, ClippingThreshold, C_FACTOR);
            x = ClippingThreshold*C_FACTOR;
          }

        GPTYPE *gplist = NULL;
        try
          {
            gplist = new GPTYPE[x];
          }
        catch (...)
          {
#ifdef _WIN32
	    message_log (LOG_ERRNO, "INDEX::TermSearch/%d allocation %lu failed.", jj, (unsigned long)x);
#else
            message_log (LOG_ERRNO, "INDEX::TermSearch/%d allocation %lld failed.", jj, (long long)x);
#endif
            ffclose(fpi);
            if (x < (2L<<24) )
              Parent->SetErrorCode(112);
            else
              Parent->SetErrorCode(2);
            continue;
          }

        x = GpFread ( gplist, x, ( off_t ) first, fpi );
        ffclose ( fpi );		// Was fclose but ...

        fpi = NULL;

        if ( x == 0 )
          {
            delete[] gplist;
            message_log ( LOG_ERROR, "Could not read elements %ld-%ld in .inx %d", first, last, jj );
            Parent->SetErrorCode ( 2 );
            continue;
          }
        if ( CheckField && FirstTime )
          {
            FieldCache->SetFieldName ( FieldName ); // Note: Here we can adise we Disk
            FirstTime = false;
          }
        // sort gplist
// clock_t xx = clock();
        // sort gplist
        if ( x > 1 && (Typ == LeftMatch || Typ == LeftAlwaysMatches) ) /* See XXXX Below */
          QSORT ( gplist, x, sizeof ( GPTYPE ), gpcomp ); // Speed up looking
// xx = clock() - xx;
// cerr << "Clock = " << xx << endl;

        for ( ip = 0; ip < (off_t)x ; ip++ )
          {
	    if (ip % 501 == 500)
	      {
		if (MaxCPU_ticks < (clock() - startClock))
		  {
		    CPU_ResourcesExhausted();
		    break; // Timeout
		  }
	      }
            gp = gplist[ip];
            if ( CheckField && !FieldCache->ValidateInField ( gp ) )
              {
//cerr << "Address " << ip << " not found/valid in field" << endl;
                continue; // Not in field...
              }

            size_t  myLength = FullTerm_length - 1;

            // Now check the "complete term" to see if it matches..
            if ( Typ == AlwaysMatches || Typ == LeftAlwaysMatches)
              {
                // Exact match
                Cost = 0.8;
                if (GetIndirectBuffer ( gp, Buffer, Offset, FullTerm_length ) <= 0 ||
                    RealTerm.Compare ( ( const char * ) Buffer, FullTerm_length ) != 0 )
                  {
//cerr << "GetIndirect/Compare failed: " << &Buffer[1] << endl;
                    continue;
                  }
                // Is this left?
                if ( Typ == AlwaysMatches && IsTermChar ( Buffer[FullTerm_length] ) )
                  continue;
              }
            else if (Typ == ExactTerm)
              {
                // Exact Term match
                Cost += 0.8;
                if (GetIndirectBuffer ( gp, Buffer, Offset+1, FullTerm_length+3 ) <= 0 ||
                    Compare ( ( const UCHR * ) RealTerm.c_str (), &Buffer[1], FullTerm_length, NULL ) != 0)
                  {
//cerr << "GetIndirect/Compare failed: " << &Buffer[1] << endl;
                    continue; // No match...
                  }
                if (IsTermChar(Buffer[1]) && (
                      Buffer[0] == '-' || Buffer[0] == '_' || Buffer[0] == '.' || Buffer[0] == ','))
                  continue;
                const UCHR Ch = Buffer[FullTerm_length+1];
                if ((Ch == '.' || Ch == ',') && IsTermChar(Buffer[FullTerm_length+2]))
                  continue;
                if (IsTermChar(Ch) || Ch == '-' || Ch == '_')
                  continue;
              }
            else if ( overflow || (myStoreHitCoordinates && FullTerm_length != Term_length) )
              {
                // Case independent match
                // 5 characters per word so max white space...
                // if Offset == 0 then we don't need to read backwards
                size_t      start = Offset > 0 ? (maxTermLength/5 + StringCompLength/4) : 0;

                Cost += 0.1; // Overflows can get expensive
                if (GetIndirectBuffer ( gp, Buffer, Offset + start, maxTermLength+start ) <= 0)
                  continue; // No such string

                if (start > 0)
                  {
                    if (IsTermChar(Buffer[start]))
                      {
                        while (start > 0 && IsTermChar(Buffer[start-1]))
                          start--, gp--;
                      }
                    else
                      {
                        while (start < Buffer_Len && Buffer[start] && ! IsTermChar(Buffer[start]))
			  {
                            start++, gp++;
			  }
                      }
                  }
                if (Compare ( RealTerm, Buffer+start, FullTerm_length, &myLength ) != 0 )
                  continue;
              }
            HitList.Add (gp, myLength);
          }	// Walk through
        delete[] gplist;
      }				/* for */
    if ( fpi )
      ffclose ( fpi );
  }

  // Now we add all the hits...
  bool     isDeleted = false;
  const PMDT      MainMdt = Parent->GetMainMdt ();
  size_t          old_w = 0;
  SRCH_DATE       rec_date;
  // FC           Fc;

  IRESULT         iresult;

  iresult.SetVirtualIndex ( ( UCHR ) ( Parent->GetVolume ( NULL ) ) );	// Set Volume
  iresult.SetMdt ( MainMdt );
  iresult.SetHitCount ( 1 );
  iresult.SetAuxCount (1);
  // iresult.SetScore ( 0 );


  const size_t    TotalHits =  HitList.TotalElements; 

#if 1
  const size_t    prealloc_max = 200000; // 2 sec = 150K
  PIRSET          pirset = new IRSET ( Parent, (TotalHits > prealloc_max ? prealloc_max  : TotalHits) + 1);

  pirset->SetMaxEntriesAdvice (TotalHits*3/4);
#else
  PIRSET          pirset = new IRSET ( Parent,  TotalHits + 1);
#endif

  size_t prev_w = 0;
  for ( size_t k = 0; k < TotalHits; k++ )
    {
      const GPTYPE    gp = HitList[k];
      const size_t    w = MainMdt->LookupByGp ( gp );
      size_t          running_hits = 0;

      // Did we find the gp?
      if (w == 0)
        continue; // Not found!

      if (k % 501 == 500 && MaxCPU_ticks >0 && MaxCPU_ticks <  (clock() - startClock))
	{
	  CPU_ResourcesExhausted();
	  break;
	}

      // Count the sucessive hits in a single record
      if (w == prev_w) { running_hits++; } else { prev_w = w; running_hits = 0; }


      // Check Date Range
      if ( DateRange.Defined () )
        {
          if ( w != old_w )
            {
              MDTREC          mdtrec;
              if ( MainMdt->GetEntry ( w, &mdtrec ) )
                {
                  rec_date = mdtrec.GetDate ();
                  isDeleted = mdtrec.GetDeleted ();
                }
              else
                {
                  isDeleted = true;
                }
              old_w = w;
            }
          if ( isDeleted )
            continue;		// Don't bother since its marked deleted

          if ( rec_date.Ok () )
            {
              // Check date range
              if ( !DateRange.Contains ( rec_date ) )
                {
                  continue;		// Out of range range

                }
            }
#if 1
          iresult.SetDate(rec_date);// Set Date
#endif
        }
      // Yes..
      iresult.SetMdtIndex ( w );
      if ( myStoreHitCoordinates )
        {
          const GPTYPE start = gp - Offset;
          const short  length= HitList(k);
          iresult.SetHitTable ( FC(start, start + length) );
        }
#if AWWW
      pirset->FastAddEntry ( iresult );
#else
      pirset->AddEntry ( iresult, true );
#endif
      if ( ClippingThreshold > 0 && pirset->GetTotalEntries () > ClippingThreshold )
        break;
    }				/* for() */

  // Cleanup
#if AWWW
  // pirset->SortByIndex();
  pirset->MergeEntries ( true );
#endif
  if ( SetCache )
    {
//cerr << "SetCache->Add()" << endl;
      SetCache->Add ( RCacheObj(RealTerm, Typ, FieldName, DateRange, pirset, ClippingThreshold,
	Cost*(clock() - startClock)));	// Cache set
    }

//cerr << "Time spend = " << (((clock() - startClock) *1000.0) / CLOCKS_PER_SEC ) << endl;

  return pirset;
}

void INDEX::Dump(ostream& os)
{
  Dump(0, os, false);
  return;
}

long INDEX::Dump (INT Skip, ostream& os, bool OnlyErrors)
{
  INT NumberOfIndexes = GetIndexNum();
  PFILE fpi;
  UCHR Buffer[StringCompLength + 1+64];
  UCHR Term[StringCompLength + 1+64];
  UCHR OldWord[StringCompLength + 1+64];
  STRING S;
  GPTYPE gp, old_gp = 0;
  MDTREC mdtrec;
  INT x;
  long errors = 0;
  bool haveDeleted = (Parent->GetTotalDocumentsDeleted() > 0);
  long notFound = 0;

  for (INT jj = 1; (jj <= NumberOfIndexes) || (NumberOfIndexes == 0); jj++)
    {

      if (NumberOfIndexes == 0)
        {
          NumberOfIndexes = -1;
          if ((fpi = ffopen (IndexFileName, "rb")) == NULL)
            {
              os << "( \"" << IndexFileName << "\" no index)" << endl;
              break; // Done
            }
        }
      else
        {
          S.form("%s.%d", IndexFileName.c_str(), jj);
          if ((fpi = ffopen(S, "rb")) == NULL)
            {
              os << "( \"" << S << "\" no index)" << endl;
              continue;
            }
        }

//  const int Cutoff = GetFileSize(fpi) /20;
      const int off = GetFileSize(fpi) % sizeof(GPTYPE);
      if (off)
        {
          UINT2 magic = (UINT2)getINT2(fpi);
          INT   ver = Version();
          int   bits = 8 << ( (ver % 100) + 1); // +1 !

	  if (!OnlyErrors)
	    {
	      os << endl << "Index Magic: " << magic << " ( version " << (ver > 0 ? (ver/100) : ver);
	      if (ver > 0) os << " " << bits << "-bit" ;
	      os << " )" << endl
	      << "-----------------------------------------" << endl;
	    }
        }

      if (Skip > 0 && !OnlyErrors)
        {
          os << "Skipping " << dec << Skip << " SIStrings." << endl;
        }
      if (!OnlyErrors)
	os << setw(StringCompLength + 9) << setfill(' ') << "Term" << '\t' << "Key" << endl;

      Term[0] = '\0';
      OldWord[0] = '\0';
      Buffer[StringCompLength] = '\0';

      gp = GpFread(Skip, fpi);
      do
        {
          int poff = 0;
	  if (!OnlyErrors)
	    os << "0x" << setw( sizeof(GPTYPE) /* was 6  */) << setfill('0') << hex << gp << ' ' << dec<< setfill(' ');
          if ((x = GetIndirectBuffer (gp, Buffer)) > 0)
            {
              if (x > (int)StringCompLength)
                x = StringCompLength;
              INT j = BufferClean(Buffer, x, false);
              if (x < (int)StringCompLength)
                memset(&Buffer[x], ' ', StringCompLength-x);
              memcpy (Term, Buffer, StringCompLength);
              Buffer[j] = '\0';
              x = j;

              if (StrCaseCmp(Buffer, OldWord) == 0)
                {
                  if (gp < old_gp) // Bad GP sort!
                    {
		      if (OnlyErrors)
			os << "0x" << setw( sizeof(GPTYPE) /* was     6  */  ) << setfill('0') << hex << gp << ' ' << dec<< setfill(' ');
                      os << "(*)";
                      poff = 3;
                    }
                  old_gp = gp;
                }
              else
                {
                  strcpy((char *)OldWord, (const char *)Buffer);
                  old_gp = 0;
                }
            }
          else if (x < 0)
            {
              // Not available
              sprintf((char *)Buffer, "<%s>",
                      errno ==  EIDRM ? "corrupt" : ((errno == ENOENT) ? "erased" : "not readable")
                     );
              x = strlen((const char *)Buffer);
            }
	  else
	    {
	      cerr << "READ FAILURE: " << gp << " ";
	      if (Parent->GetMainMdt ()->GetMdtRecord (gp, &mdtrec) != 0)
		{
                  const off_t pos = gp - mdtrec.GetGlobalFileStart () - (off_t)mdtrec.GetLocalRecordStart();
		  cerr << mdtrec.GetFileName() << " pos=" << pos;
		}
	      cerr << endl;
	    }
	    

          if (Parent->GetMainMdt ()->GetMdtRecord (gp, &mdtrec) != 0)
	    {
	      if (!OnlyErrors)
		os << setw(StringCompLength - poff) << (char *)Buffer << '\t' << mdtrec.GetKey();
	      if (mdtrec.GetDeleted())
		{
		  if (!OnlyErrors) os << " <deleted>";
		}
	      else if (_ib_tolower(Term[0]) - _ib_tolower(Buffer[0]) > 0)
		{
		  const off_t pos = gp - mdtrec.GetGlobalFileStart () - (off_t)mdtrec.GetLocalRecordStart();
		  os << " <ERROR Type=\"broken entry\"";
#if 1
		  Buffer[x] = ' ';
		  Buffer[StringCompLength] = '\0';
#else
		  GetIndirectBuffer (gp, Buffer);
#endif
		  os << " File=\"" << mdtrec.GetFileName() << "\" Pos=\"" << pos << "\"  Content=\"" << (char *)Buffer << "\"/>";
//cerr << "ERR: x=" << x << " " << mdtrec.GetFileName() << "(" << pos << "): \"" << (char *)Buffer << "\"" << endl;
		  errors++;

		}
	       if (!OnlyErrors) os << endl;
	     }
	  else
	    {
	       if (!haveDeleted)
		os << "Gp=" << gp << " not found" << endl;
	      notFound++;
	    }
	}
      while (!feof(fpi) && (gp = GpFread(fpi)) != (GPTYPE)-1);
      ffclose (fpi);
    }
  if (notFound)
    message_log (LOG_INFO, "INDEX: %lu addresses not found (deleted).",  notFound);
  return errors;
}

bool INDEX::CheckIntegrity() const
{
  return OK;
}


INDEX::~INDEX ()
{
  if (CommonWords)
    {
      if (Parent)
        {
          // Parent->ProfileWriteString("CommonWords", "Threshold",  CommonWordsThreshold);
          message_log (LOG_DEBUG, "Writing Common Words file");
          FILE *fp = fopen(Parent->ComposeDbFn(CommonWordsFileExtension), "w");
          if (fp)
            {
#if 1
              for (STRLIST *p = CommonWords->Next(); p != CommonWords; p = p->Next())
                fprintf(fp, "%s\n", p->Value().c_str());
#else
              REGISTRY Common("Common Words");
              // Dump Common Words
              Common.ProfileWriteString("CommonWords", "Threshold",  CommonWordsThreshold);
              Common.ProfileWriteString("CommonWords", "Words", CommonWords->Join(' '));
              Common.Write(fp);
#endif
              fclose(fp);
            }
        }
      delete CommonWords;
      CommonWords = NULL;
    }
  if (TermAliases)
    delete TermAliases;

  if (StopWords)
    {
      delete StopWords;
      StopWords = NULL;
    }
  if (SetCache)
    {
      // Save the cache between sessions?
      if (Parent->UsePersistantCache())
        DumpPersistantCache();
      delete SetCache;
      if (DebugMode) message_log (LOG_DEBUG, "Disposed of in-memory Set Cache");
      SetCache = NULL;
    }
  if (FieldCache) delete FieldCache;
  if (NumFieldCache) delete NumFieldCache;
  if (MemorySISCache)
    {
      delete MemorySISCache;
      if (DebugMode) message_log (LOG_DEBUG, "Disposed of Memory SIS Cache");
      MemorySISCache = NULL;
    }
  if (MemoryIndexCache)
    {
      delete MemoryIndexCache;
      if (DebugMode) message_log (LOG_DEBUG, "Disposed of Memory Index Cache");
      MemoryIndexCache = NULL;
    }
  message_log (LOG_DEBUG, "Disposed of INDEX instance of '%s'", IndexFileName.c_str());
}

//////////////////////////////////////////////////////////////////////////////

bool INDEX::KillAll ()
{
  long pid = 0;
  STRING s;
  const INT y = GetIndexNum();

  message_log (LOG_DEBUG, "INDEX::KillAll GetIndexNum() = %d", y);

  IndexingTotalBytesCount=0;
  IndexingTotalWordsCount=0;
  IndexingWordsTruncated = 0;
  IndexingWordsLongestLength = 0;
  if (Parent)
    {
      Parent->ProfileWriteString(DbIndexingStatisticsSection, DbTotalBytes, IndexingTotalBytesCount);
      Parent->ProfileWriteString(DbIndexingStatisticsSection, DbTotalWords, IndexingTotalWordsCount);
      Parent->ProfileWriteString(DbIndexingStatisticsSection, DbTotalWordsTruncated, IndexingWordsTruncated);
      Parent->ProfileWriteString(DbIndexingStatisticsSection, DbLongestWord, IndexingWordsLongestLength);

      Parent->ProfileWriteString(Section, FindConcatEntry, findConcatWords ? "On" : "Off");
      if (forceConcatWords)
        Parent->ProfileWriteString(FindConcatEntry, ForceEntry, "True");
    }

  if (Parent) UnlinkFile(Parent->ComposeDbFn(CommonWordsFileExtension));

  SetIndexNum(0); // Zap index count
  if (0 == UnlinkFile(IndexFileName))
    if (DebugMode) message_log(LOG_DEBUG, "Removed %s", IndexFileName.c_str());
  s = IndexFileName + _lock;
  if (Exists(s))
    {
      FILE *fp = fopen(s, _mode_rt);
      if (fp)
        {
          if (fscanf(fp, "%ld", &pid) == 0)
	    pid = 0; // Bad format
          fclose(fp);
        }
      if (FileExists(s) && -1 == UnlinkFile(s))
	{
          message_log(LOG_ERRNO, "Could not remove %s", s.c_str());
	}
    }
  for (INT i=1; i <= y; i++)
    {
      // Zap .inx.*
      s.form("%s.%d", IndexFileName.c_str(), i);
      if (DebugMode) message_log(LOG_DEBUG, "Removing %s", s.c_str());
      if (FileExists(s) && -1 == UnlinkFile(s))
	{
	  if (EraseFileContents(s) == -1)
	    message_log (LOG_ERROR|LOG_ERRNO, "Could not remove/erase contents of '%s'", s.c_str()); 
	}
      if (pid)
        {
          s.form("%s.%d.%d", IndexFileName.c_str(), i, pid);
          if (Exists(s))
            {
              if (FileExists(s) && -1 == UnlinkFile(s))
                message_log(LOG_ERRNO, "Could not remove %s", s.c_str());
            }
        }
      // Zap .sis.*
      s.form("%s.%d", SisFileName.c_str(), i);
      if (DebugMode) message_log(LOG_DEBUG, "Removing %s", s.c_str());
      if (FileExists(s) && -1 == UnlinkFile(s))
	{
	  if (EraseFileContents(s) == -1)
	    message_log (LOG_ERROR|LOG_ERRNO, "Could not remove/erase contents of '%s'", s.c_str());
	}
      if (pid)
        {
          s.form("%s.%d.%d", SisFileName.c_str(), i, pid);
          if (Exists(s))
            {
              if (DebugMode) message_log(LOG_DEBUG, "Removing %s", s.c_str());
              if (FileExists(s) && -1 == UnlinkFile(s))
                message_log(LOG_ERRNO, "Could not remove %s", s.c_str());
            }
        }
    }
  return true;
}

#if 0
size_t INDEX::SisCheck()
{
  char tmp[1024];
  int len, pos;
  int count = 0;
  FILE *inp;
  int errors = 0;


  {
    count = 0;
    if ((inp = fopen(argv[1], "rb")) == NULL)
      {
        message_log (LOG_ERRNO, "Can't open '%s'", (const char *)filename);
        continue;
      }
    last[0] = 0;
    int compLength = (unsigned char)fgetc(inp);
    int charset = (unsigned char)fgetc(inp);

    CHARSET Set(charset);
    message_log (LOG_INFO, "Character set id=%d (%s)\n", charset, (const char *)Set);
    int (* SisCompare)(const void *, const void *) = Set.SisCompare();

    while ((len = fgetc(inp)) != EOF)
      {
        count++;
        fread(tmp, 1, compLength, inp);
        tmp[len] = 0;
        if (SisCompare((void *)tmp, (void *)last) <= 0)
          {
            errors++;
            printf("Error %s followed by %s\n", last, tmp);
          }
        strcpy(last, tmp);
        fread(&pos, sizeof(GPTYPE), 1, inp); /* Position in .inx */
      }
    fclose(inp);
    message_log (LOG_INFO, "%d Terms processed", count);
  }
  return errors;
}
#endif

// Cleanup and remove deleted records
INT INDEX::Cleanup ()
{
  // First Merge
  if (MergeIndexFiles() == false)
    return 0; // Failed

  message_log (LOG_DEBUG, "INDEX::Cleanup() [Right now does nothing]");

#if 0  /* THIS NO LONGER WORKS */
  // Compute offset GP changes for each MDTREC
  size_t MdtTotalEntries = Parent->GetMainMdt ()->GetTotalEntries ();
  PGPTYPE GpList = new GPTYPE[MdtTotalEntries];
  GPTYPE Offset = 0;
  MDTREC Mdtrec;

  for (size_t x = 1; x <= MdtTotalEntries; x++)
    {
      Parent->GetMainMdt ()->GetEntry (x, &Mdtrec);
      if (Mdtrec.GetDeleted () == true)
        {
          Offset += Mdtrec.GetLocalRecordEnd () - Mdtrec.GetLocalRecordStart () + 1;
        }
      else
        {
          GpList[x - 1] = Offset;
        }
    }		// for

  if (Offset == 0)
    {
      // Nothing to do
      delete[]GpList;
      return 0;
    }

  // Remove deleted GP's from index and field files, also collapsing GP space
  const INT DfdtTotalEntries = Parent->GetMainDfdt()->GetTotalEntries ();
  DFD Dfd;
  STRING S, Fn, TempFn;

  TempFn = Parent->ComposeDbFn (DbExtTemp);

  PFILE Fpo, Fpn;

  initSISDeletes(); // TODO this function
  for (INT FileNum = 0; FileNum <= DfdtTotalEntries; FileNum++)
    {
      if (FileNum == 0)
        {
          Fn = Parent->ComposeDbFn (DbExtIndex);
          message_log (LOG_INFO, "Cleaning up index: '%s'.", Fn.c_str());
        }
      else
        {
          Parent->GetMainDfdt()->GetEntry (FileNum, &Dfd);
          Dfd.GetFieldName (&S);
          Parent->DfdtGetFileName (S, &Fn);
          message_log (LOG_INFO, "Cleaning up '%s' field data: '%s'.",
                (const char *)S, (const char *)Fn);
        }
      Fpo = fopen(Fn, "rb"); // Old Index
      Fpn = fopen(TempFn, "wb"); // New Index
      if (Fpo && Fpn)
        {
          // New version index?
          if (FileNum == 0)
            {
              const size_t count = GetFileSize(Fpo) % sizeof(GPTYPE);
              // New generation index, copy magic
              if (count)
                {
                  char buf[sizeof(GPTYPE)];
                  fread(&buf, 1, count, Fpo);
                  fwrite(&buf, 1, count, Fpn);
                }
            }

          GPTYPE Gp;
          long pos = 0;
          bool deleted;
          while (GpFread (&Gp, Fpo))
            {
              const size_t g = Parent->GetMainMdt ()->LookupByGp (Gp);
              deleted = true;
              if (g)
                {
                  Parent->GetMainMdt ()->GetEntry (g, &Mdtrec);
                  if (Mdtrec.GetDeleted () == false)
                    {
                      Gp -= GpList[g - 1];
                      GpFwrite (Gp, Fpn);
                    }
                  else
                    deleted = true;
                }
              if (FileNum == 0 && deleted)
                {
                  // Need to delete the term position from the SIS
                  // Queue up the terms to delete from the SIS
                  QueueSISDelete(pos, GetIndirectTerm(Gp)); // TODO function
                }
              pos++;
            }
          fclose (Fpn);
          fclose (Fpo);
          RenameFile ((const CHR *)TempFn, (const CHR *)Fn);
        }
      else
        {
          // Error
          if (Fpn) fclose(Fpn);
          if (Fpo) fclose(Fpo);
        }
    }		// for
  ProcessSISDeletes(); // TODO this function

  // Update GP's in MDT
  for (size_t i=1; i<=MdtTotalEntries; i++)
    {
      Parent->GetMainMdt ()->GetEntry(i, &Mdtrec);
      if (Mdtrec.GetDeleted() == false)
        {
          Mdtrec.SetGlobalFileStart(Mdtrec.GetGlobalFileStart () - GpList[i-1]);
//	    Mdtrec.SetGlobalFileEnd(Mdtrec.GetGlobalFileEnd () - GpList[i-1]);
          Parent->GetMainMdt ()->SetEntry(i, Mdtrec);
        }
    }
  delete[]GpList;
#endif
  return (Parent->GetMainMdt ()->RemoveDeleted());
}

/*
 * Qsort routine from Bentley & McIlroy's "Engineering a Sort Function".
 */

#define min(a, b)       (a) < (b) ? a : b

#define swapcode(TYPE, parmi, parmj, n) {               \
        long i = (n) / sizeof (TYPE);                   \
        register TYPE *pi = (TYPE *) (parmi);           \
        register TYPE *pj = (TYPE *) (parmj);           \
        do {                                            \
                register TYPE   t = *pi;                \
                *pi++ = *pj;                            \
                *pj++ = t;                              \
        } while (--i > 0);                              \
}

#define SWAPINIT(a) swaptype = ((char *)a - (char *)0) % sizeof(long) || \
        sizeof(GPTYPE) % sizeof(long) ? 2 : sizeof(GPTYPE) == sizeof(long)? 0 : 1;

static inline void swapfunc(void *a, void *b, int n, int swaptype)
{
  if (swaptype <= 1) swapcode(long, a, b, n)
    else              swapcode(char, a, b, n)
    }

#define swap(a, b)                                      \
        if (swaptype == 0) {                            \
                long t = *(long *)(a);                  \
                *(long *)(a) = *(long *)(b);            \
                *(long *)(b) = t;                       \
        } else                                          \
                swapfunc(a, b, sizeof(GPTYPE), swaptype)

#define vecswap(a, b, n)        if ((n) > 0) swapfunc(a, b, n, swaptype)

/* XXXX: Sort by words and if the words match sort by GP.  */
static int inline MemIndexCompare (const void *a, const void *b, const void *base)
{
#if 0
  return memcmp((char *)base+(*((PGPTYPE)(a))), (char *)base+(*((PGPTYPE)(b))), StringCompLength);
#else
  register int result;
  if ((result = strncmp((const char *)base+(*((PGPTYPE)(a))),
                        (const char *)base+(*((PGPTYPE)(b))), StringCompLength)) == 0)
    result = *((PGPTYPE)(a)) - *((PGPTYPE)(b));
  return result;
#endif
}

#define med3(a,b,c) \
  ( MemIndexCompare(a, b, base) < 0 ? (MemIndexCompare(b, c, base) < 0 ? b : \
  ( MemIndexCompare(a, c, base) < 0 ? c : a )) :(MemIndexCompare(b, c, base) > 0 ? b : \
  ( MemIndexCompare(a, c, base) < 0 ? a : c )))

void INDEX::TermSort(const void *base, void *a, size_t n) const
{
    char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
    int d, r, swaptype, swap_cnt;

loop:   SWAPINIT(a);
    swap_cnt = 0;
    if (n < 7)
      {
        for (pm = (char *)a + sizeof(GPTYPE); pm < (char *)a + n * sizeof(GPTYPE); pm += sizeof(GPTYPE))
          for (pl = pm; pl > (char *)a && MemIndexCompare(pl - sizeof(GPTYPE), pl, base) > 0; pl -= sizeof(GPTYPE))
            swap(pl, pl - sizeof(GPTYPE));
        return;
      }
    pm = (char *)a + (n / 2) * sizeof(GPTYPE);
    if (n > 7)
      {
        pl = (char *)a;
        pn = (char *)a + (n - 1) * sizeof(GPTYPE);
        if (n > 40)
          {
            d = (n / 8) * sizeof(GPTYPE);
            pl = (char *)med3(pl, pl + d, pl + 2 * d);
            pm = (char *)med3(pm - d, pm, pm + d);
            pn = (char *)med3(pn - 2 * d, pn - d, pn);
          }
        pm = (char *)med3(pl, pm, pn);
      }
    swap(a, pm);
    pa = pb = (char *)a + sizeof(GPTYPE);

    pc = pd = (char *)a + (n - 1) * sizeof(GPTYPE);
    for (;;)
      {
        while (pb <= pc && (r = MemIndexCompare(pb, a, base)) <= 0)
          {
            if (r == 0)
              {
                swap_cnt = 1;
                swap(pa, pb);
                pa += sizeof(GPTYPE);
              }
            pb += sizeof(GPTYPE);
          }
        while (pb <= pc && (r = MemIndexCompare(pc, a, base)) >= 0)
          {
            if (r == 0)
              {
                swap_cnt = 1;
                swap(pc, pd);
                pd -= sizeof(GPTYPE);
              }
            pc -= sizeof(GPTYPE);
          }
        if (pb > pc)
          break;
        swap(pb, pc);
        swap_cnt = 1;
        pb += sizeof(GPTYPE);
        pc -= sizeof(GPTYPE);
      }
    if (swap_cnt == 0)    /* Switch to insertion sort */
      {
        for (pm = (char *)a + sizeof(GPTYPE); pm < (char *)a + n * sizeof(GPTYPE); pm += sizeof(GPTYPE))
          for (pl = pm; pl > (char *)a && MemIndexCompare(pl - sizeof(GPTYPE), pl, base) > 0;
               pl -= sizeof(GPTYPE))
            swap(pl, pl - sizeof(GPTYPE));
        return;
      }

    pn = (char *)a + n * sizeof(GPTYPE);
    r = min(pa - (char *)a, pb - pa);
    vecswap(a, pb - r, r);
    r = min(pd - pc, pn - pd - (off_t)sizeof(GPTYPE));
    vecswap(pb, pn - r, r);
    if ((r = pb - pa) > (off_t)sizeof(GPTYPE))
      TermSort(base, a, r / sizeof(GPTYPE));
    if ((r = pd - pc) > (off_t)sizeof(GPTYPE))
      {
        /* Iterate rather than recurse to save stack space */
        a = pn - r;
        n = r / sizeof(GPTYPE);
        goto loop;
      }
    /*              TermSort(base, pn - r, r / sizeof(GPTYPE));*/
}



size_t INDEX::ScanSearch(SCANLIST *ListPtr, const QUERY& Query, const STRING& Fieldname, size_t MaxRecordsThreshold, bool Cat)
{
  message_log (LOG_DEBUG, "ScanSearch in Field '%s'", Fieldname.c_str());
  if (ListPtr)
    {
      SCANLIST Scanlist = ScanSearch(Query, Fieldname, MaxRecordsThreshold);

      if (Cat)
        ListPtr->AddEntry(Scanlist);
      else
        *ListPtr = Scanlist;
      return Scanlist.GetTotalEntries();
    }
  return 0;
}


#if 0
SCANLIST INDEX::ScanSearch(const SQUERY& SearchQuery, const STRING& Fieldname, size_t MaxRecordsThreshold)
{
  return ScanSearch(SearchQuery, Fieldname, NullString, MaxRecordsThreshold);

}
#endif


//Q: do we want to limit to the number of words??
//

SCANLIST INDEX::ScanSearch(const QUERY& SearchQuery, const STRING& Fieldname, /* const STRING& Pattern, */
                           size_t MaxRecordsThreshold)
{
  SCANLIST Scanlist;
  message_log (LOG_DEBUG, "ScanSearch in Field '%s'", Fieldname.c_str());

  if (Parent == NULL)
    {
      message_log (LOG_ERROR, "Orphaned INDEX::ScanSearch ?");
      return Scanlist; // Sorry need parents
    }
  const size_t      d_pos = Fieldname.SearchReverse( __AncestorDescendantSeperator );
  const bool WantNode = (d_pos != 0);
  const STRING      scanField ((WantNode ? Fieldname.Right(Fieldname.GetLength() - d_pos) : Fieldname) + "/");

  if (Fieldname.IsEmpty() || !Parent->ValidNodeName(Fieldname))
    {
      message_log (LOG_DEBUG, "25      Specified element set name not valid for specified database");
      Parent->SetErrorCode(25);
      return Scanlist;
    }

  QUERY  Query (SearchQuery);

  // If we have unlimited we don't need to have a sort
  if (Query.isUnlimited())
    {
      Query.SetNormalizationMethod ( NoNormalization); // Unnormalized search
      Query.SetSortBy(Unsorted);
    }

  IRSET *iptr ;
  size_t total;

  if ((iptr = Search (Query)) != NULL)
    {
      if ((total = iptr->GetTotalEntries ()) == 0 ||
          (total  >  MaxRecordsThreshold &&  MaxRecordsThreshold > 1))
        {
          delete iptr;
          if (total)
            {
              message_log (LOG_DEBUG, "12     Too many records retrieved");
              Parent->SetErrorCode(12);
            }
        }
      else
        {
          message_log (LOG_DEBUG, "ScanSearch (in %s) got %lu records for query", Fieldname.c_str(), (unsigned long)total);
          // Loads of looping and other fun bits....
          PRSET prset = iptr->GetRset();

          delete iptr; // don't need it anymore

          total = prset->GetTotalEntries();
          for (size_t i = 1; i <= total; i++)
            {
              STRING words;
              RESULT result;
              if (prset->GetEntry (i, &result))
                {
#if 1
		  STRLIST wordList;
		  size_t res = Parent->GetAncestorContent (result, Fieldname, &wordList);
		  if (WantNode || res)
		    {
		      for (STRLIST *tp = wordList.Next(); tp != &wordList; tp = tp->Next())
			Scanlist.AddEntry ( tp->Value().ToLower(), StopWords);
		    }
#else
                  if (WantNode)
                    {
                      // Get the Node
                      STRLIST wordList;
                      size_t res = Parent->GetAncestorContent (result, Fieldname, &wordList);
                      for (STRLIST *tp = wordList.Next(); tp != &wordList; tp = tp->Next())
			{
                          Scanlist.AddEntry ( tp->Value().ToLower(), StopWords);
			}
                    }
#endif
                  else /* Get the content in the named field */
                    {
                      Parent->Present(result, Fieldname, SutrsRecordSyntax, &words);
		      // words.Replace(",", " ");
                      Scanlist.AddEntry ( words.ToLower(), StopWords );
                    }
                }
            }
          delete prset;
        }
    } // else ERROR
  return Scanlist;
}


const char *INDEX::MatchType(enum MATCH Typ) const
{
  switch (Typ)
    {
      case Unspecified:   return "Unspecified";
      case LeftMatch:     return "LeftMatch";
      case Exact:         return "Exact";
      case ExactTerm:     return "ExactTerm";
      case ExactTermCase: return "ExactTermCase";
      case Phonetic:      return "Phonetic";
      case PhoneticCase:  return "PhoneticCase";
      case AlwaysMatches: return "AlwaysMatches";
      case LeftAlwaysMatches: return "LeftAlwaysMatches";
      case Numerical:     return "Numerical";
      case FreeForm:      return "FreeForm";
      case Phrase:        return "Phrase";
      default:            return "unknown?";
    }
}

PIRSET INDEX::DoctypeSearch (const STRING& DoctypeSpec)
{
  IRSET           *pirset = NULL;
  MDT             *MdtPtr;

  if (Parent == NULL)
    return NULL;

  if (DoctypeSpec.IsEmpty())
    {
      Parent->SetErrorCode(108); // Malformed query
      return NULL;
    }
  if ((MdtPtr    = Parent->GetMainMdt ()) == NULL)
    {
      Parent->SetErrorCode(1); // "Fatal system error";
      return NULL;
    }
  try
    {
      pirset = new IRSET(Parent);
    }
  catch (...)
    {
      Parent->SetErrorCode(2); // "Temporary system error";
      if (pirset == NULL) return NULL;
    }

  IRESULT          iresult;
  FC               Fc;
  MDTREC           mdtrec;

  iresult.SetMdt (MdtPtr);
  iresult.SetHitCount (1);
  iresult.SetAuxCount (1);
  iresult.SetScore (0);

  bool isWild = DoctypeSpec.IsWild();

  const size_t     total_records = MdtPtr->GetTotalEntries();
  for (size_t i=1; i<= total_records; i++)
    {
      if (MdtPtr->GetEntry (i, &mdtrec))
	{
	  DOCTYPE_ID doctype = mdtrec.GetDocumentType();
	  if (isWild ?  (doctype.Name.MatchWild (DoctypeSpec)) : doctype.Equals (DoctypeSpec) )
	    {
	      SRCH_DATE date (mdtrec.GetDate());
              // Have a match
              iresult.SetMdtIndex(i);
              iresult.SetDate(date);
              Fc.SetFieldStart( mdtrec.GetLocalRecordStart() );
              Fc.SetFieldEnd  ( mdtrec.GetLocalRecordEnd()   );
              iresult.SetHitTable(Fc);
              pirset->FastAddEntry (iresult);
            }
        }
    }
  return pirset;
}


// This method is also used by the unary operator WITHKEY:keyspec
PIRSET INDEX::KeySearch(const STRING& KeySpec)
{
  IRSET           *pirset = NULL;
  MDT             *MdtPtr;

  if (Parent == NULL)
    return NULL;

  if (KeySpec.IsEmpty())
    {
      Parent->SetErrorCode(108); // Malformed query
      return NULL;
    }
  if ((MdtPtr    = Parent->GetMainMdt ()) == NULL)
    {
      Parent->SetErrorCode(1); // "Fatal system error";
      return NULL;
    }
  try
    {
      pirset = new IRSET(Parent);
    }
  catch (...)
    {
      Parent->SetErrorCode(2); // "Temporary system error";
      if (pirset == NULL) return NULL;
    }

  IRESULT          iresult;
  FC               Fc;
  MDTREC           mdtrec;
  size_t           count = 0;

  iresult.SetMdt (MdtPtr);
  iresult.SetHitCount (1);
  iresult.SetAuxCount (1);
  iresult.SetScore (0);

  size_t idx = MdtPtr->LookupByKey (KeySpec);
  if (idx && MdtPtr->GetEntry (idx, &mdtrec))
    {
      // Have a match
      iresult.SetMdtIndex(idx);
      iresult.SetDate( mdtrec.GetDate() );
      Fc.SetFieldStart( mdtrec.GetLocalRecordStart() );
      Fc.SetFieldEnd  ( mdtrec.GetLocalRecordEnd()   );
      iresult.SetHitTable(Fc);
      pirset->FastAddEntry (iresult);
      count++;
    }
  else if (KeySpec.IsWild())
    {
      const size_t     total_records = MdtPtr->GetTotalEntries();
      for (size_t i=1; i<= total_records; i++)
        {
          if (MdtPtr->GetEntry (i, &mdtrec))
            {
              STRING key (mdtrec.GetKey());
              if (::Glob(KeySpec,key, false) == true)
                {
                  SRCH_DATE date (mdtrec.GetDate());
                  // Have a match
                  iresult.SetMdtIndex(i);
                  iresult.SetDate(date);
                  Fc.SetFieldStart( mdtrec.GetLocalRecordStart() );
                  Fc.SetFieldEnd  ( mdtrec.GetLocalRecordEnd()   );
                  iresult.SetHitTable(Fc);
                  pirset->FastAddEntry (iresult);
		  count++;
                }
            }
        }
    }

  message_log (LOG_DEBUG, "KeySearch(\"%s\") returns set with %lu elements", KeySpec.c_str(), count);

  return pirset;
}

#if 0

PIRSET INDEX::IndexSearch(const STRING& IndexRange)
{
  const char *ptr = IndexRange.c_str();
  const char *from;
  const char *to;

  while (isspace(*ptr)) ptr++;
  from = ptr;
  if (*ptr == '-') ptr++;
  while (isdigit(*ptr)) ptr++;
  while (*ptr && !isdigit(*ptr))
    to = ptr;

  long start = 0;
  long end   = 0;

  if (*from == '-' && *to == '\0')
    {
      start = 1;
      end   = atol(from+1);
    }
  else if (*to == '\0')
    {
      start = atol(from);
      to    = start;
    }
  else
    {
      start = atol(from);
      end   = atol(to);
    }
  if (start == 0 || end == 0 || end > start)
    {
      Parent->SetErrorCode(108); // Malformed query
      return NULL;
    }


}

#endif


PIRSET INDEX::FileSearch(const STRING& FileSpec)
{
  IRSET           *pirset = NULL;
  MDT             *MdtPtr;

  if (Parent == NULL)
    return NULL;

  if (FileSpec.IsEmpty())
    {
      Parent->SetErrorCode(108); // Malformed query
      return NULL;
    }
  if ((MdtPtr    = Parent->GetMainMdt ()) == NULL)
    {
      Parent->SetErrorCode(1); // "Fatal system error";
      return NULL;
    }
  try
    {
      pirset = new IRSET(Parent);
    }
  catch (...)
    {
      Parent->SetErrorCode(2); // "Temporary system error";
      if (pirset == NULL) return NULL;
    }

  MDTREC           mdtrec;
  IRESULT          iresult;
  FC               Fc;
  bool      isWild = FileSpec.IsWild();
  bool      noPath = !ContainsPathSep(FileSpec);

  iresult.SetMdt (MdtPtr);
  iresult.SetHitCount (1);
  iresult.SetAuxCount (1);
  iresult.SetScore (0);

  Parent->SetErrorCode(0); // OK
  pirset->SortBy(ByIndex); // Bootstrap that its sorted by index order

  PATHNAME         myPath (FileSpec);
  const size_t     total_records = MdtPtr->GetTotalEntries();

  for (size_t i=1; i<= total_records; i++)
    {
      if (MdtPtr->GetEntry (i, &mdtrec))
        {
          PATHNAME path (mdtrec.GetPathname());

          if (path.Compare(myPath) == 0 ||
              (noPath && ::FileGlob(FileSpec, path.GetFileName())) ||
              (isWild && ::FileGlob(FileSpec, path.GetFullFileName())) )
            {
              SRCH_DATE date (mdtrec.GetDate());
              // Have a match
              iresult.SetMdtIndex(i);
              iresult.SetDate(date);
              Fc.SetFieldStart( mdtrec.GetLocalRecordStart() );
              Fc.SetFieldEnd  ( mdtrec.GetLocalRecordEnd()   );
              iresult.SetHitTable(Fc);
              pirset->FastAddEntry (iresult);
            }
        }
    }
  return pirset;
}

// Import other index
size_t INDEX::ImportIndex(INDEX *IndexPtr)
{
  size_t  recordsAdded = 0;

  if (IndexPtr == NULL || IndexPtr->Parent == NULL || Parent == NULL || !OK)
    return 0; // Nope

  if (!IndexPtr->Ok())
    {
      Parent->SetErrorCode(2); // Temp System error
      message_log (LOG_ERROR, "To be imported index '%s' is not OK!",
		IndexPtr->Parent->GetDbFileStem ().c_str());
      return 0;
    }
  if (IndexPtr->ActiveIndexing || ActiveIndexing)
    {
      Parent->SetErrorCode(29); // Locked db
      if (IndexPtr->ActiveIndexing == ActiveIndexing)
	message_log (LOG_ERROR, "Can't import: Alreadying Indexing/Importing to target ('%s' importing '%s').",
		Parent->GetDbFileStem ().c_str(),  IndexPtr->Parent->GetDbFileStem ().c_str() );
      else if (ActiveIndexing)
	message_log (LOG_ERROR, "Can't import and index to same target '%s' at the same time.",
		Parent->GetDbFileStem ().c_str());
      else
	message_log (LOG_ERROR, "Can't import from an index '%s' that's being updated.",
		IndexPtr->Parent->GetDbFileStem ().c_str());
      return 0;
    }
  IndexPtr->ActiveIndexing = true;
  ActiveIndexing = true;

#if 1
  int          count = GetIndexNum();
  const int    ocount = IndexPtr->GetIndexNum();
  INT2         IndexMagic;
  int          inx = (count <= 0 ? 1 : count);
  MDT         *oMdtPtr =  IndexPtr->Parent->GetMainMdt ();
  MDT         *MdtPtr = Parent->GetMainMdt();
  const STRING import_db (IndexPtr->Parent->GetDbFileStem ());

  if (MdtPtr == NULL || oMdtPtr == NULL) return 0; // Can't

  const STRING oIndexFileName (IndexPtr->IndexFileName);
  const STRING SisFileName  (Parent->ComposeDbFn(DbExtDict));
  const STRING oSisFileName (IndexPtr->Parent->ComposeDbFn(DbExtDict));

  STRING       in    (oIndexFileName);
  STRING       out;
  STRING       sis_in (oSisFileName);
  STRING       sis_out;

  const GPTYPE  offset = MdtPtr->GetNextGlobal ();
  const size_t  oTotal = oMdtPtr->GetTotalEntries ();

  if (oTotal == 0) return 0; // Nothing to add

  size_t errors = 0;

  FILE *out_fp, *in_fp;

  MDTREC       Mdtrec;

  message_log (LOG_INFO, "Import db '%s' into %s [%lu records].", import_db.c_str(), Parent->GetDbFileStem().c_str(),
	oTotal);

  // First add the other MDTRECs
  message_log (LOG_DEBUG, "Importing records (offset=%lu)..", offset);
  for (size_t i=1; i<=oTotal; i++)
    {
      if (oMdtPtr->GetEntry (i, &Mdtrec) == true)
	{
	  STRING fn =  Mdtrec.GetFullFileName();
	  message_log (LOG_DEBUG, "Adding '%s' [%s]..", fn.c_str(), Mdtrec.GetKey().c_str());

	  Mdtrec.SetHashTable (MdtPtr->GetMDTHashTable());
          Mdtrec.SetFullFileName(fn);
	  
	  if (MdtPtr->AddEntry(Mdtrec+=offset) == 0)
	    {
	      message_log (LOG_FATAL, "ImportIndex: Could not import '%s' MDTREC#%d", import_db.c_str(), i);
	      errors++;
	    }
	  else
	    recordsAdded++;
	}
       else
	{
	  message_log (LOG_FATAL, "ImportIndex: Could not read '%s' MDTREC#%d", import_db.c_str(), i);
	  errors++;
	}
   }
  // Now write the text field data
  DFDT *oDfdtptr = IndexPtr->Parent->GetDfdt ();
  message_log (LOG_DEBUG, "Importing Fields..");
  if (oDfdtptr)
    {
      DFDT         *Dfdtptr = Parent->GetDfdt();
      const size_t fields = oDfdtptr->GetTotalEntries();
      if (fields)
	{
          DFD         Dfd;
          STRING      fieldName;
          FIELDTYPE   fieldType;
	  STRING      in_fn, out_fn;
	  bool newField;
	  GPTYPE      objects = 0;
          for (size_t x=1; x<=fields; x++)
            {
	      oDfdtptr->GetEntry(x, &Dfd);
	      fieldName = Dfd.GetFieldName();
	      fieldType = Dfd.GetFieldType();
	      if (!Dfdtptr->FieldExists(fieldName))
		{
		  Dfdtptr->AddEntry(Dfd);
		  newField = true;
		}
	       else
		newField = false;
	      // Append Text field
	      IndexPtr->Parent->DfdtGetFileName (fieldName, &in_fn); // TEXT
	      Parent->DfdtGetFileName(fieldName, &out_fn);
	      if (out_fn.IsEmpty())
		{
		  message_log (LOG_PANIC, "ImportIndex: Can't find field '%s'", fieldName.c_str());
		  continue;
		}
	      // Now copy;
	      if ((in_fp = fopen(in_fn, "rb")) != NULL)
		{
		  FC       fc;
		  size_t   elements = GetFileSize(in_fp)/sizeof(FC);
		  size_t   wrote = 0;

		  message_log (LOG_DEBUG, "%s %ld field elements of '%s' into '%s'",
			newField ? "Importing" : "Adding" , (long)elements, fieldName.c_str(), out_fn.c_str()); 
		  if ((out_fp = fopen(out_fn, "ab")) != NULL)
		    {
		      for (size_t i = 0; i < elements && fc.Read(in_fp); i++)
			{
			  fc.Write(out_fp, offset);
			  wrote++;
			}
		      if (wrote != elements) message_log (LOG_ERROR, "Wrote %ld!=%ld elements", (long)wrote, (long)elements);
		      fclose(out_fp);
		    }
		  else message_log (LOG_ERROR|LOG_ERRNO, "ImportIndex: Can't open '%s' for append", out_fn.c_str());
		  fclose(in_fp);
		}
	      else
		message_log (LOG_ERROR|LOG_ERRNO, "ImportIndex: Can't open '%s' for reading", in_fn.c_str());
	      if (fieldType.IsText())
		continue;

	      // TODO: As case above... Divided among those that use DATELISTs, NUMERICLISTS and ...

	      STRING       Fn;
	      IndexPtr->Parent->DfdtGetFileName(fieldName, fieldType, &Fn);
	      switch ((int)fieldType)
		{
		  case FIELDTYPE::ttl:
		  case FIELDTYPE::numerical:
		  case FIELDTYPE::phonhash:
		  case FIELDTYPE::phonhash2:
		  case FIELDTYPE::metaphone:
		  case FIELDTYPE::metaphone2:
		  case FIELDTYPE::hash:
		  case FIELDTYPE::casehash:
		  case FIELDTYPE::currency:
		  case FIELDTYPE::lexi:
		  case FIELDTYPE::dotnumber:
		  case FIELDTYPE::privhash:
		    // Numerical field
		    {
		      NUMERICLIST    List;
                      if (GetFileSize(Fn) <= (off_t)sizeof(GPTYPE))
			continue;
		      List.SetFileName(Fn);
		      List.LoadTable(VAL_BLOCK);
		      const size_t  count = List.GetCount();
		      if (count > 0)
			{
			  if ((out_fp = OpenForAppend(fieldName, fieldType)) != NULL)
			    {
			      for (size_t i=0; i<count; i++)
				{
				  List[i].Write(out_fp, offset);
				}
			      fclose(out_fp);
			      objects += count;
			      message_log (LOG_DEBUG, "Imported %lu %s field(s) [%s].", (long)count, fieldType.c_str(), "NUMERICLIST");
			    }
			  else message_log (LOG_ERROR|LOG_ERRNO, "Could not append to field '%s':%s [%s].",
				fieldName.c_str(), fieldType.c_str(), "NUMERICALIST");
		       }
		    }
		    break;

		  case FIELDTYPE::numericalrange:
                    {
                      INTERVALLIST List;
                      if (GetFileSize(Fn) <= (off_t)sizeof(GPTYPE))
                        continue;
                      List.SetFileName(Fn);
		      List.LoadTable(0, -1);
                      const size_t  count = List.GetCount();
                      if (count > 0)
                        {
                          if ((out_fp = OpenForAppend(fieldName, fieldType)) != NULL)
                            {
                              for (size_t i=0; i<count; i++)
                                List[i].Write(out_fp, offset);
                              fclose(out_fp);
			      objects += count;
			      message_log (LOG_DEBUG, "Imported %lu %s field(s) [%s].", (long)count, fieldType.c_str(), "INTERVALLIST");
                            }
                          else message_log (LOG_ERROR|LOG_ERRNO, "Could not append to field '%s':%s [%s].",
                                fieldName.c_str(), fieldType.c_str(), "INTERVALLIST");
                       }
                    }
		   break;

		  case FIELDTYPE::ttl_expires:
		  case FIELDTYPE::time:
		  case FIELDTYPE::date:
		    {
		      if (GetFileSize(Fn) <= (off_t)sizeof(GPTYPE))
			continue;
		      if ((out_fp = OpenForAppend(fieldName, fieldType)) != NULL)
			{
			  DATELIST List;
			  List.SetFileName(Fn);
			  List.LoadTable(0, -1, VAL_BLOCK);
			  const size_t  count = List.GetCount();
			  for (size_t i=0; i<count; i++)
			    List[i].Write(out_fp, offset);
			  fclose(out_fp);
			  objects += count;
			  message_log (LOG_DEBUG, "Imported %lu %s field(s) [%s].", (long)count, fieldType.c_str(), "DATELIST");
			}
		     else message_log (LOG_ERROR|LOG_ERRNO, "Could not append to field '%s':%s [%s].",
			fieldName.c_str(), fieldType.c_str(), "DATELIST");
		   }
		  break;
	        case FIELDTYPE::box:
#if 0
		  if (GetFileSize(Fn) <= sizeof(GPTYPE))
		    continue;
		  if ((out_fp = OpenForAppend(fieldName, fieldType)) != NULL)
		    {
		      BBOXLIST List;
		      fclose(out_fp);
		    }
		  break;
#endif
		case FIELDTYPE::daterange:

	        default: // TODO: Add type code
	        message_log (LOG_WARN, "**** Can't Import field data of type '%s' (%s) yet ****", fieldType.c_str(), fieldName.c_str());
	     }
	}
      if (objects)
	{
	  SortNumericFieldData();
	  message_log (LOG_INFO, "Updated %lu objects.", (long)objects);
	}
    } // if other has fields 
  } // if other has field DFDT ptr 

  // Now add the indexes and sis files 
  message_log (LOG_DEBUG, "Importing indexes...");
  for (int j = 0 ; j <= ocount; j++)
   {
      if (j)
	{
	  in.form("%s.%d", oIndexFileName.c_str(), j);   
          sis_in.form("%s.%d", oSisFileName.c_str(), j);
	}
      out.form("%s.%d", IndexFileName.c_str(), ++inx);
      sis_out.form("%s.%d", SisFileName.c_str(), inx);
      
      if ((in_fp = fopen(in, "rb")) != NULL)
	{
	  if ((out_fp = fopen(out, "wb")) != NULL)
	    {
	      GPTYPE Gp;
	      IndexMagic = getINT2(in_fp);
	      putINT2((INT2)IndexMagic, out_fp);
	      // Now loop, read, increment, write
	      while (GpFread(&Gp, in_fp))
		GpFwrite(Gp+offset, out_fp);
	      fclose(out_fp);
	    }
	  fclose(in_fp);
	  // Copy the .sis file
	  message_log (LOG_DEBUG, "Install %s into %s", sis_in.c_str(), sis_out.c_str());
	  if (FileLink(sis_in, sis_out) != 0)
	    {
	      message_log (LOG_FATAL|LOG_ERRNO, "ImportIndex: Could not import '%s' SIS#%d", import_db.c_str(), j);
	      errors++;
	    }
	}
   }

 if (count <= 1)
    {
      out.form("%s.1", IndexFileName.c_str());
      Parent->ffdispose(IndexFileName); // Make sure handle is not open
      if (RenameFile(IndexFileName, out) != 0)
	{
	  message_log (LOG_FATAL|LOG_ERRNO, "ImportIndex: Could not move '%s' to '%s'",
		IndexFileName.c_str(), out.c_str());
	  errors++;
	}
      out.form("%s.1", SisFileName.c_str());
      Parent->ffdispose(SisFileName); // Make sure handle is not open
      if (RenameFile(SisFileName, out) != 0)
	{
	  message_log (LOG_FATAL|LOG_ERRNO, "ImportIndex: Could not move '%s' to '%s'",
                SisFileName.c_str(), out.c_str());
	  errors++;
	}
    }
  if (count == 0)
   count = 1;
  count += (ocount ? ocount : 1);
  SetIndexNum(count);
  message_log (LOG_DEBUG, "Set %d sub-indexes", count);
  if (errors)
    message_log (LOG_FATAL, "ImportIndex: %d errors encounted.", errors);
  else if (count == 2 || MergeStatus==iMerge || MergeStatus==iOptimize)
    MergeIndexFiles();
#endif

  IndexPtr->ActiveIndexing = false;
  ActiveIndexing = false;

  return recordsAdded;
}

