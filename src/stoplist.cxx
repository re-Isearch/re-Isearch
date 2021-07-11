#pragma ident  "@(#)stoplist.cxx  1.24 02/05/01 00:33:28 BSN"

/*
 * File:         stoplist.cxx
 * Purpose:      Manage external stoplists
 * Author:       Edward C. Zimmermann <edz@nonmonotonic.com>
 * Created:      1993
 * Updated:	 April 1995
 * Status:       Experimental
 * Distribution: unlimited
 * RCS_ID:       $Id: stoplist.cxx,v 1.2 2007/06/19 06:24:03 edz Exp $
 * Copyright:    (c) 1993, Basis Systeme netzwerk/Munich
 */
/*
Permission to use, copy, modify and distribute this software, in whole
or in part, for any purpose is hereby granted without fee.

The copyright holders disclaim all warranties with regard to this
software, including all implied warranties of merchantability and
fitness.  In no event shall the copyright holders be liable for any
special, indirect or consequential damages or any damages whatsoever
resulting from loss of use, data or profits, whether in an action of
contract, negligence or other tortuous action, arising out of or in
connection with the use or performance of this software.
*/

/* This is an experimental release, in the next version this will be
   sub-classed from a generic class to load arbitrary hierarchical lists
   from external files

   We don't use STRLIST but our own list since for the task at hand
   the STRLIST class is far too slow!

   'C' allocation (malloc) is used for the list.
 */

/*
  Format of External Stoplist files:

  # is a comment to the end of line
  Words are seperated by white space.
  That's all folks!

 */

// CHANGES:
// =======
// $Log: stoplist.cxx,v $
// Revision 1.2  2007/06/19 06:24:03  edz
// sync
//
// Revision 1.1  2007/05/15 15:47:23  edz
// Initial revision
//
// Revision 1.11  1996/05/02  19:52:37  edz
// Sync
//
// Revision 1.1  1995/11/19  02:19:09  edz
// Initial revision
//
// Revision 1.1  1995/11/19  02:19:09  edz
// Initial revision
//
// Revision 1.9  1995/07/20  22:45:45  edz
// Sync
//
// Revision 1.8  1995/06/12  08:53:10  edz
// Cosmetic changes.
//
//
//

#include <stdlib.h>
#include <string.h>
//#include <iostream.h>
#include <stdio.h>
#include <ctype.h>
//#include <malloc.h>
#include "common.hxx"
#include "stoplist.hxx"
#include "sw.hxx"


#ifndef LIBPATH
static const char *LIBPATH = NULL;   
#endif
      
#ifndef BASEPATH
static const char *BASEPATH = NULL;
#endif



//----------------------------------------------------------------------------
// Utility Functions
//----------------------------------------------------------------------------

static int _StrNCaseCmpHook(const void *s1, const void *s2, size_t len)
{
  return StrNCaseCmp((const UCHR *)s1, (const UCHR *)s2, (INT)len);
}

/*
static int _StrCaseCmpHook(const void *s1, const void *s2)
{
  return StrCaseCmp((const UCHR *)s1, (const UCHR *)s2);
}
*/


// This is the major pointer to get "the right" function
static int (*CompareNfunction)(const void *, const void *, size_t) = _StrNCaseCmpHook;
/*
static int (*Comparefunction)(const void *, const void *) = _StrCaseCmpHook;
*/

void STOPLIST::SetCharset(CHARSET& Charset)
{
  Compare = Charset.Compare();
}


// NOTE: Could also probably convert below to C++

/*
 *    %F      the pathname of the library (e.g./usr/local/lib)
 *    %l      the locale 
 *    %o      the object (eg. stoplist)
 *    %<x>    the <x> character (e.g. ``%%'' results in ``%''
 */

/* Search Strategies */
static const char *searchDirs[] =
{
  "%F/%o.%l",
  "%B/lib/%o.%l",
  "%B/conf/%o.%l",
  "%B/conf/%l/%o",
  "%B/conf/%os/%l",
  "%B/%os/%l",
  "%B/%os/%o.%l",
  "/usr/local/lib/%l.%o",
  "/usr/local/lib/%o.%l",
  NULL
};

/* Add as needed */
typedef enum { fSTOPLIST = 0, fMAX } ext_list_t;
const char *ext_list_names[] = { "stoplist" };


STRING STOPLIST::Description() const
{
  STRING message;
  message << "STOPLIST can be passed a path or format (see below) or with a language\n\
searches the formats: \n";
  for (int i=0; searchDirs[i]; i++)
    message << "\t" << searchDirs[i] << "\n";
  message << "Where:\n\
\t%B      the pathname of the base of the package (e.g. " << BASEPATH << ")\n\
\t%F      the pathname of the library (e.g." << LIBPATH << ")\n\
\t%l      the locale (eg. de.iso-8859-1)\n\
\t%o      the object (eg.";
  for (int j=0; j < fMAX; j++)
    message << (j > 0 ? ", " : "" ) <<  ext_list_names[j]; 
 message << ")\n\
\t%<x>    the <x> character (e.g. ``%%'' results in ``%''\n";

 return message;
}


/* Generic Search Fmt */
static char *sPathFmt (char *buf, ext_list_t w, const char *fmt, const char *l)
{
  char *ptr = buf;

  if (fmt && *fmt)
    do
      {
	if (*fmt == '%')
	  {
	    const char *add = NULL;
	    switch (*++fmt)
	      {
	      case '\0':
		*ptr++ = '%';
		continue;
	      case 'l':
		add = l;
		break;
	      case 'B':
		add = BASEPATH;
		break;
	      case 'F':
		add = LIBPATH;
		break;
	      case 'o':
		add = (w < fMAX  ? ext_list_names[w] : "???");
		break;
	      }
	    if (add)
	      {
		strcpy (ptr, add);
		ptr += strlen (ptr);
	      }
	  }
	else
	  *ptr++ = *fmt;
      }
    while (*++fmt);
  *ptr = '\0';			/* ASCIIZ */
  return buf;
}

// Our private collection authority
static void lfree (UCHR **list)
{
  /* Destroy old lists */
  if (list)
    {
      UCHR **ptr = list;
      do {
	if (*ptr) delete (*ptr);
      } while (*++ptr);
      free (list);
    }
}


//----------------------------------------------------------------------------
// The StopList Class
//----------------------------------------------------------------------------
const int CHUNK=512; // Memory allocation chunks 

/* case INDEPENDENT string Compare */
static inline int PchrCompare (const void *str1, const void *str2, size_t CompLength)
{
  const void *s1 = *((const UCHR **)str1);
  const void *s2 = *((const UCHR **)str2);
  return CompareNfunction (s1, s2, CompLength);
}

static inline int PchrCompare (const void *str1, const void *str2)
{
  return PchrCompare (str1, str2, StringCompLength);
}

static const STRING NullStop ("<NULL>");

STOPLIST::STOPLIST ()
{
  if (LIBPATH == NULL)  LIBPATH  = _IB_LIBPATH();   
  if (BASEPATH == NULL) BASEPATH = _IB_BASEPATH();

#ifndef _USE_ArraySTRING
  list = NULL;
  list_len = 0;
#endif
  deallocate = GDT_FALSE;
  currentLanguage =  NullStop;
}

STOPLIST::STOPLIST (const CHR* language)
{
  currentLanguage =  NullStop;
#ifndef _USE_ArraySTRING

  list = NULL;
  list_len = 0;
#endif
  deallocate = GDT_FALSE;
  Load(language);
}

STOPLIST::STOPLIST (const STRING& language)
{
  currentLanguage =  NullStop;
#ifndef  _USE_ArraySTRING
  list = NULL;
  list_len = 0;
#endif
  deallocate = GDT_FALSE;
  Load(language);
}

STOPLIST::STOPLIST (const PPCHR words, INT len, const STRING& language)
{
#ifndef  _USE_ArraySTRING
  list = NULL;
  list_len = 0;
#endif
  deallocate = GDT_FALSE;
  Load (words, len, language);
}

STOPLIST::STOPLIST (const STRLIST& wordList, const STRING& language)
{
#ifndef  _USE_ArraySTRING
  list = NULL;
  list_len = 0;
#endif
  deallocate = GDT_FALSE;
  Load (wordList, language);
}


GDT_BOOLEAN STOPLIST::LoadInternal(const STRING& Lang)
{
  // Zap en-gb to en etc..
  STRING language(Lang);
  STRINGINDEX pos = language.Search('-');
  if (pos) language.EraseAfter(pos-1);

  for (size_t i=0; i < sizeof(internal_stoplists)/sizeof(internal_stoplists[0]); i++)
    {
      if (language ^= internal_stoplists[i].language)
	{
	  return Load ((PPCHR)internal_stoplists[i].words, (size_t)internal_stoplists[i].size, Lang);
        }
    }
  return GDT_FALSE;
}

GDT_BOOLEAN STOPLIST::Load (const PPCHR words, size_t len, const STRING& language)
{
#ifndef  _USE_ArraySTRING
  if (deallocate && list_len)
    lfree(list); // Collect old trash
#else
  Words.Clear();
#endif
  deallocate = GDT_FALSE;
  list = (UCHR **)words;
  list_len = len; 
  QSORT (list, list_len, sizeof(UCHR *), PchrCompare);
  currentLanguage = language;
  return GDT_TRUE;
}

// Load words from a STRLIST method
GDT_BOOLEAN STOPLIST::Load(const STRLIST& wordList, const STRING& language)
{
  if (deallocate && list_len) {
    lfree(list);
    deallocate = GDT_FALSE;
  }

  size_t TotalEntries = wordList.GetTotalEntries();

  STRING value;
  UCHR **tp = (UCHR **)malloc((TotalEntries + 1)*sizeof(UCHR *));

  if (tp == NULL)
   {
     logf (LOG_ERRNO , "Memory allocation to load stoplist failed.");
     return GDT_FALSE;
   }

  size_t x;
  for (x=0; x < TotalEntries; x++)
    {
      wordList.GetEntry(x+1, &value);
      tp[x] = value.NewUCString();
    }
  tp[x] = NULL; // End-of-list
  QSORT (tp, x, sizeof (UCHR *), PchrCompare); // Sort

  // Install the words
  list_len = x;
  list = tp;
  deallocate = GDT_TRUE;
  currentLanguage = language;
  return GDT_TRUE;
}

GDT_BOOLEAN STOPLIST::Load(const STRING& language)
{
  return LoadList(language);
}

// Load a data file whose name is a 'C' string method
GDT_BOOLEAN STOPLIST::Load(const CHR* language)
{
  if (language == NULL || *language == '\0')
    return LoadList(NulString);
  return LoadList(language);
}

GDT_BOOLEAN STOPLIST::LoadList(const STRING& language)
{
//cerr << "LoadList(" << language << ")" << endl;
  if (currentLanguage ^= language)
    return GDT_TRUE; // Cached!
  if (deallocate && list_len)
    {
      // Collect old trash
      lfree(list);
      deallocate = GDT_FALSE;
    }
  // "C" language? Use default "internal" list
  if (!language.IsEmpty())
    {
      logf (LOG_DEBUG, "Load stoplist %s", language.c_str());
      currentLanguage = language;
      // <NULL> means NO stoplist
      if (currentLanguage ==  NullStop)
	{
	  list = NULL;
	  list_len = 0;
	  return GDT_TRUE;
	}

      static const char *IFS = " ,\t\n\r\013\f";
      char word[BUFSIZ];
      FILE *fp = NULL;

      if (currentLanguage.Search('%') != 0)
	{
	  STRING s;
	  ::GetGlobalCharset(&s);
	  sPathFmt (word, fSTOPLIST, currentLanguage, GlobalLocale.LocaleName());
	  fp = fopen (word, "r");
	  if (fp == NULL)
	     logf (LOG_ERRNO, "Can't open '%s'", word);
	}
      // Absolute Path?
      else if (IsAbsoluteFilePath(currentLanguage))
	{
	  fp = fopen(currentLanguage, "r");
	}
      else
        for (size_t i = 0; searchDirs[i]; i++) // Search list
	  {
	    sPathFmt (word, fSTOPLIST, searchDirs[i], currentLanguage.c_str());
	    if ((fp = fopen (word, "r")) != NULL)
	      break;
	  };
      if (fp == NULL) {
	logf (LOG_WARN, "No external stoplist found for \"%s\"!", currentLanguage.c_str());
	if (LoadInternal(currentLanguage))
	  {
	    logf (LOG_WARN, "Using internal '%s' stoplist.", currentLanguage.c_str());
	    return GDT_TRUE;
	  }
	return GDT_TRUE;
      }

      // An external stopword file is a list of words
      // seperated by white space. The '#' symbol is used
      // to start a comment to end-of-line 
      size_t count = 0;
      size_t chunk = CHUNK;
      UCHR **words = (UCHR **) malloc (chunk * sizeof (UCHR *));

      while (words && fgets (word, sizeof (word) / sizeof (char) - 1, fp))
	{
	  char *cp;

	  /* Strip comment */
	  if ((cp = strchr (word, '#')) != NULL)
	    *cp = '\0';

	  if (word[0] == '\0')
	    continue;

	  /* Strip trailing white space */
	  for (cp = &word[strlen (word) - 1]; cp > word; *cp-- = '\0')
	    {
	      if (strchr(IFS, *cp) == NULL)
		break;
	    }
	  /* skip over leading space */
	  for (cp = word; strchr(IFS, *cp); cp++)
	    /* loop */;

	  if (*cp == '\0')
	    continue;		/* blank line */

	  char *ctxt; // strtok_r context 
	  cp = strtok_r (cp, IFS, &ctxt);
	  do
	    {
	      if (*cp == '\0')
		continue;

	      words[count++] = (UCHR *)Copystring (cp); // Copy string
	      if (count >= chunk)
		{
		  chunk += CHUNK;
		  words = (UCHR **) realloc (words, chunk * sizeof (UCHR *) +1);
		  if (words == NULL) break; // ERROR: No more core
		  
		}
	    }
	  while ((cp = strtok_r (NULL, IFS, &ctxt)) != NULL);
	}			/* while() */
      fclose (fp);

      if (words) {
	/* sort */
	QSORT (words, count, sizeof(char *), PchrCompare);
	words[++count] = NULL;
	/* Contract */
	if (count  != chunk)
	  words = (UCHR **) realloc (words, count * sizeof (UCHR *));
      }
      if (words == NULL) {
	// We ran out of memory
	logf (LOG_ERRNO, "Not enough core to load '%s' stoplist.", currentLanguage.c_str());
	return GDT_FALSE;
      }
      list = words;
      list_len = count-1;
      deallocate = GDT_TRUE;
    }
  else
    {
      /* Use builtin english list */
      return LoadInternal("en");
    }

  // Load OK
  return GDT_TRUE;
}

GDT_BOOLEAN STOPLIST::InList (const STRING& Word) const
{
  STRINGINDEX Len;

  if ((Len = Word.GetLength()) > StringCompLength)
    Len = StringCompLength;
  return InList(Word.c_str(), Len);
}


/* 
   Determine if the word starting at WordStart is in the
   stop list.
*/

GDT_BOOLEAN STOPLIST::InList (const UCHR* WordStart, const STRINGINDEX Len) const
{
// cerr << "Looking at \"" << WordStart << "\"  len=" << Len;
  const size_t WordLength = (Len == 0 && WordStart) ? strlen((const char *)WordStart) : Len;

  int z = list_len ? 0 : -1;
  // Search Ordered List for Word
  if (WordLength <= 1 || !_ib_isalnum(WordStart[1]))
    {
      z = 0; // Single letter words are taboo
    }
  else if (list_len)
    {
      size_t high = list_len;
      size_t low = 0;
      size_t ip  = 0;
      size_t old_ip;

      for (;;) {
	old_ip = ip;
	ip = (high + low)/2;
	if (old_ip == ip)
	  break;

	if ((z = PchrCompare (&WordStart, &list[ip], WordLength)) == 0)
	  {
#if 1 /* Temp code as long as !isalnum in index.cxx is cut point */
	    if (_ib_isalnum(list[ip][WordLength])) z = -1;
#else
	    z = -list[ip][WordLength]; 
#endif
	  }
	if (z == 0) break;     // Had a match
	else if (z < 0)        high = ip;
	else if (((low = ip + 1) > high))
	  low = high;	// upper-bound (Should never happen)
      };
    }
//  cerr << " -----> " << ((z == 0) ? "True" : "False") << endl;
  return (z == 0);
}


GDT_BOOLEAN STOPLIST::AddWord(const CHR *NewWord)
{
  if (NewWord == NULL || *NewWord == '\0')
    return GDT_TRUE; // Nothing to add

  size_t  count = list_len + 1;
  UCHR **words = (UCHR **) malloc ((count+1)*sizeof (UCHR *));

  if (words == NULL)
    return GDT_FALSE; // NO MORE CORE

  // Add Word
  int installed = 0;
  size_t i;
  for (i=0; i <= count; i++)
    {
      if (installed == 0 && PchrCompare(NewWord, list[i]) >= 0)
	{
	  words[i] = (UCHR *)Copystring(NewWord);
	  installed = 1;
	}
      words[i+installed] = list[i];
    }
  if (installed == 0)
    words[i] = (UCHR *)Copystring(NewWord);

  // Free Old
  if (deallocate) lfree(list);

  // Install
  list = words;
  list_len += installed;
  deallocate = GDT_TRUE;
  return GDT_TRUE;
}


size_t STOPLIST::GetTotalEntries () const
{
  return list_len;
}

PUCHR STOPLIST::Nth (size_t idx) const
{
  return (( idx<= list_len && idx > 0) ?  list[idx-1] : NULL);
}

GDT_BOOLEAN STOPLIST::GetEntry (const size_t Index, PSTRING StringEntry) const
{
  UCHR *val = Nth(Index);
  if (val == NULL)
    {
      if (StringEntry) StringEntry->Clear();
      return GDT_FALSE;
    }
  if (StringEntry) *StringEntry = val; 
  return GDT_TRUE;
}

STOPLIST::~STOPLIST ()
{
  if (list_len && deallocate)
    lfree (list);
  list_len = 0;
  list = NULL;
}
