/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)date.cxx  1.63 02/05/01 00:32:30 BSN"

/*-@@@
File:		dates.cxx
Version:	1.64
Description:	Class DATE - Isearch Date data structure class
Author:		Archie Warnock (warnock@clark.net), A/WWW Enterprises
Modifications:	Edward C. Zimmermann <edz@nonmonotonic.net>
@@@*/

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#ifdef SOLARIS
#include <tzfile.h>
#endif
#include "common.hxx"
#include "date.hxx"
#include "magic.hxx"

#ifndef TM_YEAR_BASE
#define TM_YEAR_BASE 1900
#endif

#ifdef _WIN32
# ifndef lstat
#   define lstat stat
# endif
#endif


// Infinite date := 31 Dec 9999
const INT4 INFINITE_DATE = 31129999;

const UINT4 YEAR_LOWER   =         1;
const UINT4 YEAR_UPPER   =      9999;
const UINT4 YEAR_FACTOR  =         1;

const UINT4 MONTH_LOWER  =     10000;
const UINT4 MONTH_UPPER  =    129999;
const UINT4 MONTH_FACTOR =     10000;

const UINT4 DAY_LOWER    =   1010000;
const UINT4 DAY_UPPER    =  31129999;
const UINT4 DAY_FACTOR   =   1000000;

const UINT4 DATE_ERROR   = 100000002;
const UINT4 DATE_UNKNOWN = 100000001;

const UINT4 DATE_PRESENT = 100000000;

static long _to_julian(long);
static long _to_julian(int, int, int);

#define _Year(x)	(int)(((x)/YEAR_FACTOR)%10000)
#define _Month(x)	(int)(((x)/MONTH_FACTOR)%100)
#define _Day(x) 	(int)(((x)/DAY_FACTOR)%100)
#define _Wday(x)        (int)((_to_julian (x) + 2) % 7)

#define _isLeapYear(y)	(((y) >= 1582) ? \
	(((y) % 4 == 0  &&  (y) % 100 != 0)  ||  (y) % 400 == 0 ): \
	((y) % 4 == 0))

#define _ToMonthPrecision(x)	((x) - _Day(x)*DAY_FACTOR)
#define _ToYearPrecision(x)	((x) % (YEAR_UPPER+1))

// HHMMSS
const int HOUR_FACTOR   =     1;
const int MIN_FACTOR    =     100;
const int SEC_FACTOR    =     10000;

#define _Hour(x)     (int)(((x & 0x000FFFFF)/HOUR_FACTOR)%100)
#define _Minutes(x)  (int)(((x & 0x000FFFFF)/MIN_FACTOR)%100)
#define _Seconds(x)  (int)(((x & 0x000FFFFF)/SEC_FACTOR)%100)

static inline int _TimeValue(int x) {
  return ((_Hour(x)*360)+(_Minutes(x)*60)+(_Seconds(x)));
}

static const int mntb[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };


static long long today_julian = -1;

long SRCH_DATE::DaysAgo() const
{
  return today_julian == -1 ? 0 : today_julian - _to_julian(d_date) ;
}


long SRCH_DATE::HoursAgo() const
{
  time_t now = time ((time_t *) 0); /* Get the current local time */
  const struct tm *tm_ptr = gmtime (&now);
  const int ddiff        = DaysAgo();
  int       hdiff        = tm_ptr && (d_rest & 0x000FFFFF) ? tm_ptr->tm_hour - _Hour(d_rest) : 0;


  if (hdiff > 0 && tm_ptr->tm_min < 5)
    hdiff--;
  else if (hdiff < 0 && tm_ptr->tm_min > 55)
    hdiff++;

  return ddiff*24 + hdiff;
}


long SRCH_DATE::DaysAgoHours(int Days) const
{
  const int ddiff        = DaysAgo();
  int       hdiff        = 0;

  if (ddiff <= Days)
    {
      time_t now = time ((time_t *) 0); /* Get the current local time */
      const struct tm *tm_ptr = gmtime (&now);

      if (tm_ptr && (d_rest & 0x000FFFFF))
	{
	  hdiff = tm_ptr->tm_hour - _Hour(d_rest);
	  if (hdiff > 0 && tm_ptr->tm_min < 5)
	    hdiff--;
	  else if (hdiff < 0 && tm_ptr->tm_min > 55)
	    hdiff++;
	}
    }
  return ddiff*24 + hdiff;
}





//--------------------------- Parsing Routines --------------------------------------

/*- date_parse - parse string dates into internal form
**
** Copyright (C) 1995 by Jef Poskanzer <jef@netcom.com>.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/
/* Modified heavily by E. Zimmermann <edz@nonmonotonic.net> for use in Isearch 
*/
/* ########################################################################

  This notice is intended as a precaution against inadvertent publication
  and does not constitute an admission or acknowledgement that publication
  has occurred or constitute a waiver of confidentiality.

   ######################################################################## */
/* Added:
   ANSI Date form: CCYYMMDD
   Obsolete ANSI Date: YYMMDD
   Euro: DD.MM.CCYY and DD.MM.YY forms
   ISO date formats:
	CCYYMMDDTHHMMSS[Z]
	CCYYMMDDTHH:MM:SS[Z]
	CCYY-MM-DDTHHMMSS[Z]
	CCYY-MM-DDTHH:MM:SS[Z] (eg.  1996-07-16T13:19:51Z)
        CCYY-MM-DDTHH:MM:SS+HH:SS (eg. 1969-07-20T22:56:15-04:00)

   TODO
   ----
   Add support for:
   21-JUL-1986
   Tuesday, September 29, 1998 8:38:38 PM
   HH:MM ampm zone
   HH:MM:SS ampm zone
   HH:MM zone
   HH:MM:SS zone
 */

#ifdef LINUX 
# ifndef SYSV
#  define SYSV
# endif
#endif
#ifdef SVR4
# ifndef SYSV
#  define SYSV
# endif
#endif
#ifdef BSD386
# ifndef BSD
#  define BSD
# endif
#endif

#ifdef SYSV
extern long timezone;
#endif
#define bzero(p, n) memset(p, 0, n)

#ifdef MAIN_STUB
#define DEBUG 1
#else
#undef DEBUG
#endif

#ifdef DEBUG
#define DP(str) {cerr << "DEBUG: " << str << endl;}
#else
#define DP(str) 
#endif

struct strint
  {
    const char *s;
    int i;
  };

static int strint_compare (const void *v1, const void *v2)
{
  return strcasecmp (((struct strint *) v1)->s, ((struct strint *) v2)->s);
}

static int strint_search (const char *str, struct strint *tab, int n, int *iP)
{
  int i, r;
  int l = 0;
  int h = n - 1;

  if (str == NULL || *str == '\0')
    return 0; // ERR
  while (h >= l)
    {
      if ((i = (h + l) / 2) < 0)
	i = 0;
      else if (i >= n)
	i = n-1;
      if ((r = strcasecmp (str, tab[i].s)) < 0)
	{
	  h = i - 1;
	}
      else if (r > 0)
	{
	  l = i + 1;
	}
      else // r==0
	{
	  *iP = tab[i].i;
	  return 1; // Found it
	}
    }
  return 0;
}

#define AMPM_NONE 0
#define AMPM_AM 1
#define AMPM_PM 2
static int ampm_fix (int hour, int ampm)
{
  switch (ampm)
    {
    case AMPM_NONE:
      break;
    case AMPM_AM:
      if (hour == 12)
	hour = 0;
      break;
    case AMPM_PM:
      if (hour != 12)
	hour += 12;
      break;
    }
  return hour;
}

static int scan_ampm (const char *str_ampm, int *ampmP)
{
  static struct strint ampm_tab[] =
  {
    {"am", AMPM_AM},
    {"pm", AMPM_PM}
  };
/* Already sorted
  static int sorted = 0;

  if (!sorted)
    {
      QSORT ( ampm_tab, sizeof (ampm_tab) / sizeof (struct strint),
	sizeof (struct strint), strint_compare);
      sorted = 1;
    }
*/
  return strint_search (
     str_ampm, ampm_tab, sizeof (ampm_tab) / sizeof (struct strint), ampmP);
}

static int scan_wday (const char *str_wday, int *tm_wdayP)
{
  static struct strint wday_tab[] =
  {
/* French */
    {"dimanche", 0},
    {"lundi", 1},
    {"mardi", 2},
    {"mercredi", 3},
    {"jeudi", 4},
    {"vendredi", 5},
    {"samedi", 6},
/* German */
    {"so", 0},
    {"sonntag", 0},
    {"mo", 1},
    {"montag", 1},
    {"di", 2},
    {"dienstag", 2},
    {"mi", 3},
    {"mittwoch", 3},
    {"do", 4},
    {"donnerstag", 4},
    {"fr", 5},
    {"freitag", 5},
    {"sa", 6},
    {"samstag", 6},
/* Spanish */
    {"domingo", 0},
    {"lunes", 1},
    {"martes", 2},
    {"miércoles", 3},
    {"jueves", 4},
    {"viernes", 5},
    {"sábado", 6},
/* Italian */
    {"domenica", 0},
    {"lunedì", 1},
    {"martedì", 2},
    {"mercoledì", 3},
    {"giovedì", 4},
    {"venerdì", 5},
    {"sabato", 6},
/* English */
    {"sun", 0},
    {"sunday", 0},
    {"mon", 1},
    {"monday", 1},
    {"tue", 2},
    {"tuesday", 2},
    {"wed", 3},
    {"wednesday", 3},
    {"thu", 4},
    {"thursday", 4},
    {"fri", 5},
    {"friday", 5},
    {"sat", 6},
    {"saturday", 6},
/* Polish */
    {"niedziela", 0},
    {"poniedzia³ek", 1},
    {"wtorek", 2},
    {"¦roda", 3},
    {"czwartek", 4},
    {"pi±tek", 5},
    {"sobota", 6}
  };
  static int sorted = 0;

  if (!sorted)
    {
      (void) QSORT ( wday_tab, sizeof (wday_tab) / sizeof (struct strint),
		     sizeof (struct strint), strint_compare);
      sorted = 1;
    }
  return strint_search (
  str_wday, wday_tab, sizeof (wday_tab) / sizeof (struct strint), tm_wdayP);
}

static int scan_mon (const char *str_mon, int *tm_monP)
{
  // To be Extended as needed! NOTE: Entries NEED to be UNiQUE!
  static struct strint mon_tab[] =
  {
/* French */
//  {"jan", 0},
    {"fév", 1},
    {"mars", 2},
    {"avr", 3},
    {"mai", 4},
    {"juin", 5},
    {"juil", 6},
    {"août", 7},
//  {"sep", 8},
//  {"oct", 9},
//  {"nov", 10},
    {"déc", 11},
    {"janvier", 0},
    {"février", 1},
    {"mars", 2},
    {"avril", 3},
    {"juin", 5},
    {"juillet", 6},
    {"août", 7},
    {"septembre", 8},
    {"octobre", 9},
    {"novembre", 10},
    {"décembre", 11},
/* German/Austrian */
    {"jänner", 0},
    {"januar", 0},
    {"feber", 1},
    {"februar", 1},
    {"märz", 2},
//  {"april", 3},
//  {"mai", 4},
    {"juni", 5},
    {"juli", 6},
//  {"august", 7},
//  {"September", 8},
    {"oktober", 9},
//  {"november", 10},
    {"dezember", 11},
/* Dutch */
    {"januari", 0},
    {"februari", 1},
    {"febr", 1},
    {"maart", 2},
    {"mrt", 2},
    {"mei", 4},
    {"augustus", 7},
/* English */
    {"jan", 0},
    {"january", 0},
    {"feb", 1},
    {"february", 1},
    {"mar", 2},
    {"march", 2},
    {"apr", 3},
    {"april", 3},
    {"may", 4},
    {"jun", 5},
    {"june", 5},
    {"jul", 6},
    {"july", 6},
    {"aug", 7},
    {"august", 7},
    {"sep", 8},
    {"september", 8},
    {"oct", 9},
    {"october", 9},
    {"nov", 10},
    {"november", 10},
    {"dec", 11},
    {"december", 11},
/* Polish */
    {"styczeñ", 0},
    {"luty", 1},
    {"marzec", 2},
    {"kwiecieñ", 3},
    {"maj", 4},
    {"czerwiec", 5},
    {"lipiec", 6},
    {"sierpieñ", 7},
    {"wrzesieñ", 8},
    {"pa¼dziernik", 9},
    {"listopad", 10},
    {"grudzieñ", 11}
  };
  static int sorted = 0;

  if (!sorted)
    {
      QSORT ( mon_tab, sizeof (mon_tab) / sizeof (struct strint),
	sizeof (struct strint), strint_compare);
      sorted = 1;
    }
  return strint_search (
      str_mon, mon_tab, sizeof (mon_tab) / sizeof (struct strint), tm_monP);
}

static int scan_gmtoff (const char *str_gmtoff, int *gmtoffP)
{
  static struct strint gmtoff_tab[] = {
    {"gmt", 0},
    {"utc", 0},
    {"ut", 0},
    {"0000", 0},
    {"+0000", 0},
    {"-0000", 0},
    {"0100", 3600},
    {"+0100", 3600},
    {"met", 3600},
    {"-0100", -3600},
    {"0200", 7200},
    {"+0200", 7200},
    { "eet", 7200},
    {"-0200", -7200},
    {"0300", 10800},
    {"+0300", 10800},
    { "ast",  10800},
    {"-0300", -10800},
    {"0400", 14400},
    {"+0400", 14400},
    {"-0400", -14400},
    {"0500", 18000},
    {"+0500", 18000},
    {"-0500", -18000},
    {"0600", 21600},
    {"+0600", 21600},
    {"-0600", -21600},
    {"0700", 25200},
    {"+0700", 25200},
    {"-0700", -25200},
    {"0800", 28800},
    {"+0800", 28800},
    {"-0800", -28800},
    {"0900", 32400},
    {"+0900", 32400},
    {"-0900", -32400},
    {"1000", 36000},
    {"+1000", 36000},
    {"-1000", -36000},
    {"1100", 39600},
    {"+1100", 39600},
    {"-1100", -39600},
    {"1200", 43200},
    {"+1200", 43200},
    {"-1200", -43200},
    {"jst", 7200},
    {"jdt", 10800},
    {"bst", -3600},
    {"nst", -12600},
    {"ast", -14400},
    {"edt", -10800},
    {"est", -18000},
    {"edt", -14400},
    {"cst", -21600},
    {"cdt", -18000},
    {"mst", -25200},
    {"mdt", -21600},
    {"pst", -28800},
    {"pdt", -25200},
    {"yst", -32400},
    {"ydt", -28800},
    {"hst", -36000},
    {"hdt", -32400},
    {"a", -3600},
    {"b", -7200},
    {"c", -10800},
    {"d", -14400},
    {"e", -18000},
    {"f", -21600},
    {"g", -25200},
    {"h", -28800},
    {"i", -32400},
    {"k", -36000},
    {"l", -39600},
    {"m", -43200},
    {"n", -3600},
    {"o", -7200},
    {"p", -10800},
    {"q", -14400},
    {"r", -18000},
    {"s", -21600},
    {"t", -25200},
    {"u", -28800},
    {"v", -32400},
    {"w", -36000},
    {"x", -39600},
    {"y", -43200}
  };
  static int sorted = 0;

  if (!sorted)
    {
      QSORT ( gmtoff_tab, sizeof (gmtoff_tab) / sizeof (struct strint),
	sizeof (struct strint), strint_compare);
      sorted = 1;
    }
  return strint_search (
       str_gmtoff, gmtoff_tab, sizeof (gmtoff_tab) / sizeof (struct strint),
			 gmtoffP);
}


#ifdef _WIN32
extern "C" {
 char *strptime(const char *buf, const char *format, struct tm *tm);
};
#else
# define HAVE_STRPTIME 1
#endif
#ifndef HAVE_STRPTIME
# define HAVE_STRPTIME 0
#endif
#ifdef STANDALONE
# undef HAVE_STRPTIME
# define HAVE_STRPTIME 0
#endif



#if  HAVE_STRPTIME
class DateFormats
{
public:
  DateFormats() { loaded = false; }

  STRING& Item(size_t i) const { return Formats.Item(i);         }
  size_t  Count()              { Init(); return Formats.Count(); }
  ~DateFormats() { }

private:
  void    Init() {
    if (loaded) return;
    const STRING path = ResolveConfigPath("datemsk"); //// MS-DOS/WINDOWS !!!?? 
    if (FileExists(path)) {
      FILE *fp = fopen(path, "r");
      if (fp) {
	char buf[BUFSIZ+1];
	STRING entry;
	while (fgets(buf, BUFSIZ, fp) != NULL) {
	  if (*buf)
	    {
	      char *tcp = strchr(buf, '#');
	      if (tcp) *tcp = '\0';
	      if (*buf)
		{
		  entry = STRING(buf).Strip(STRING::both);
		  if (!entry.IsEmpty())
		    Formats.Add(entry);
		}
	    }
        }
	fclose(fp);
      }
    }
    loaded = true; 
  }
  bool loaded;
  ArraySTRING Formats; 
};


static DateFormats Formats;

#endif

/* 
  RR format:

    Current Year        Two Digit Year     Year RR Format
    Last Two Digits       Specified            Returns
    ---------------     --------------     ----------------
         0-49                0-49          Current Century
        50-99                0-49          One Century after current
         0-49               50-99          One Century before current
        50-99               50-99          Current Century

   Unix Model: Year starts at 70 for 1970
       00-68                2000-2068
       69-99                1969-1999

*/
// Since year >= 2000
static int Normalize_year(const int Year, const int y_cut)
{
  int year = Year;
  if (year < 100)
    year += (year >= 0 && year <= y_cut) ? 2000 : 1900;
  // Handle also 102 as 2002 etc.
  else if (year < TM_YEAR_BASE)
    year += TM_YEAR_BASE;
  return year;
}

// 29 --> 2029 but 45 --> 1945
static int Normalize_RR_year(int year)
{
  return Normalize_year(year, 45);
}

// 29 --> 2029 but 68 --> 1968
static int Normalize_tm_year(int tm_year)
{
  return Normalize_year(tm_year, 68);
}

bool SRCH_DATE::getdate(const char *string, struct tm *tm)
{
#if HAVE_STRPTIME
  const size_t count = Formats.Count();
  for (size_t i=0; i < count; i++)
    { 
      if (strptime(string, Formats.Item(i).c_str(), tm))
	{
	  tm->tm_year += TM_YEAR_BASE; 
	  return true;
	}
    }
#endif
  return false;
}

/* recursive */
/* NOTE: Assumes the cannonical internal form is UTC!!! */


// Convert 20 to 2020, 0020 to 20, 69 to 1969, 0069 to 69... 
static int scan_year(const char *str, int *year)
{
  int i, yr = 0;
  for (i=0; str[i]; i++)
    {
      const register int k = str[i] - '0';
      if (k < 0 || k > 9)
	return 0; // ERROR
      yr *= 10;
      yr += k;
    }
  if (i <= 2 || (i==3 && yr>99 && yr<200)) // handle 100 as 2000
    *year = Normalize_tm_year(yr);
  else
    *year = yr;
  return 1;
}

#define STRSIZ 128 /* was 500 */

bool SRCH_DATE::date_parse (const char *str)
{
  time_t now = time ((time_t *) 0); /* Get the current local time */
  const struct tm *now_tmP = localtime (&now);
  struct tm tm;
  const char *cp;
  char str_mon[STRSIZ], str_wday[STRSIZ], str_year[STRSIZ];
  char str_gmtoff[STRSIZ], str_ampm[STRSIZ];
  int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_week, tm_wday, gmtoff;
  int ampm, got_zone;
  char ch, ch2;
  char sc = 0;
  long t;

  d_date = 0;
  d_rest = 0;

  // # means date
  if (str && *str == '#')
    return date_parse(str+1);

  /* Set internal Now (local!) */
  today_julian = _to_julian (now_tmP->tm_mday, now_tmP->tm_mon + 1,
	now_tmP->tm_year < 2000 ?  now_tmP->tm_year+1900 :  now_tmP->tm_year);

  /* Special cases first */
  if (str == NULL || *str == '\0' || strcasecmp(str, "Now") == 0 ||
		strcasecmp(str, "Present") == 0)
    {

      // Now is current date/time UTC!
      struct tm *tm_ptr = gmtime (&now);
      d_date = (tm_ptr->tm_mday)*DAY_FACTOR + (tm_ptr->tm_mon + 1)*MONTH_FACTOR +
	Normalize_tm_year(tm_ptr->tm_year)*YEAR_FACTOR;
      // Now includes the hour, min and sec..
      if (str && *str)
	{
	  d_rest = (tm_ptr->tm_hour)*HOUR_FACTOR + (tm_ptr->tm_min)*MIN_FACTOR + (tm_ptr->tm_sec)*SEC_FACTOR;
	}
      SetPrecision( DAY_PREC );
      return true;
    }
  else if (strncasecmp(str, "Unknown", 7) == 0)
    {
      d_date = DATE_UNKNOWN;
      SetPrecision (UNKNOWN_DATE);
      return true; // Its parsed but unknown
    }
  else if (str[0] == 'D' && str[1] == ':' && strlen(str) >= 10)
    {
      // Format used by Adobe
      // D:19980305090013 --> 19980305T090013Z
      strncpy(str_mon, str+2, 8);
      str_mon[8] = '\0'; 
      if (isdigit(str[10]))
	{
	  strcat(str_mon, "T");
	  strcat(str_mon, str+10);
	  if (isdigit(str_mon[14]) && !isalnum(str_mon[15]))
	    {
	      str_mon[15] = 'Z';
	      str_mon[16] = '\0';
	    }
	}
      // Try to parse as an ISO date/time
      DP(str_mon);
      return date_parse(str_mon);
    }
  else if (strcasecmp(str, "Today") == 0)
    {
      // Today is LOCAL date without time..
      d_date =  ( now_tmP->tm_mday)*DAY_FACTOR + ( now_tmP->tm_mon + 1)*MONTH_FACTOR +
	Normalize_tm_year(now_tmP->tm_year)*YEAR_FACTOR;
      SetPrecision ();

      return true;
   }
  else if (*str == '/' && SetTimeOfFile(str))
   {
      return true;
   }
  else if (*str == '(')
   {
      // Handle (SCHEME=ISO8601)
      do { str++; } while (isspace(*str)); // Skip potential blanks
      if (strncasecmp(str, "scheme", 6) == 0)
	{
	  enum { usa, europe, iso8601, guess} paradigm = guess;
	  //scheme="USA" implies "MM-DD-YYYY"
	  //scheme="Europe" implies "DD-MM-YYYY"
	  str += 6;
	  while (isspace(*str)) str++;

	  if (*str == '=')
	    {
	      // Now skip potential spaces
	      do { str++; } while (isspace(*str));
	      if (*str == '"' || *str == '\'')
		str++;

	      if (strncasecmp(str, "USA", 3) == 0)
		paradigm = usa; // US Scheme of things
	      else if (strncasecmp(str, "Eur", 3) == 0)
		paradigm = europe; // European scheme of things
	      else if (strncasecmp(str, "iso", 3) == 0)
		paradigm = iso8601;
	    }
	  // Assume ISO or that we can parse it correctly
	  while (*str && *str != ')')
	    str++;
	  if (*str == ')') str++;
	  while (isspace(*str)) str++;

	  switch (paradigm)
	    {
	      case usa:
		// MM-DD-YYYY
		DP("MM-DD-YY?");
		if (sscanf(str, "%d-%d-%d", &tm_mon , &tm_mday, &tm_year) != 3)
		  tm_mday = -1; // Mark BAD
		break;
	      case europe:
		// DD-MM-YYYY
		DP("DD-MM-YYYY ?");
		if (sscanf(str, "%d-%d-%d", &tm_mday, &tm_mon, &tm_year) != 3)
		  tm_mday = -1; // Mark BAD
		break;
	      case iso8601:
	      default: return date_parse (str);
	    }
	  if (tm_mday < 1 || tm_mday > 31 || tm_mon < 1 || tm_mon > 12)
	    {
	      // BAD values
	      d_date = DATE_ERROR;
	      SetPrecision (BAD_DATE);
	      return false;
	    }
	  // Now er have a good date
	  d_date =  (tm_mday)*DAY_FACTOR + (tm_mon)*MONTH_FACTOR +
		Normalize_tm_year(tm_year)*YEAR_FACTOR;
	  SetPrecision ();
	  return true;
	}
   }

  // Set empty
  str_mon[0] = str_wday[0] = str_gmtoff[0] = str_ampm[0] = '\0';

  /* Initialize tm with relevant parts of current local time. */
  bzero ((char *) &tm, sizeof (struct tm));
  tm.tm_sec = now_tmP->tm_sec;
  tm.tm_min = now_tmP->tm_min;
  tm.tm_hour = now_tmP->tm_hour;
  tm.tm_mday = now_tmP->tm_mday;
  tm.tm_mon = now_tmP->tm_mon;
  tm.tm_year = now_tmP->tm_year;
  ampm = AMPM_NONE;
  got_zone = 0;

  /* Find local zone offset.  This is the only real area of
     ** non-portability, and it's only used for local times that don't
     ** specify a zone - those don't occur in email and netnews.
   */
#ifdef _WIN32
  tzset ();
  gmtoff = 0;
#else
#ifdef SYSV
  tzset ();
  gmtoff = -timezone;
#else /* SYSV */
#ifdef BSD
  gmtoff = now_tmP->tm_gmtoff;
#else /* BSD */
  /* You have to fill this in yourself. */
  gmtoff = !!!;
#endif /* BSD */
#endif /* SYSV */
#endif /* WIN */

  /* Skip initial whitespace ourselves - sscanf is clumsy at this. */
  for (cp = str; *cp == ' ' || *cp == '\t'; ++cp)
    ;

  /* Handle NN Days/Months/Years */
  if (sscanf(cp, "%d %s", &tm_mday, str_mon) == 2)
    {
      if (strncasecmp(str_mon, "Day", 3) == 0) {
      DP("+XX Days");
      // First set Today
      d_date =  ( now_tmP->tm_mday)*DAY_FACTOR + ( now_tmP->tm_mon + 1)*MONTH_FACTOR +
        Normalize_tm_year(now_tmP->tm_year)*YEAR_FACTOR;
      d_rest = 0;
      SetPrecision ();
      // Now Increment
      PlusNdays(tm_mday);
      return true;
      } else if (strncasecmp(str_mon, "Month", 5) == 0) {
      DP("+XX Months");
      // First set Today
      d_date =  ( now_tmP->tm_mday)*DAY_FACTOR + ( now_tmP->tm_mon + 1)*MONTH_FACTOR +
        Normalize_tm_year(now_tmP->tm_year)*YEAR_FACTOR;
      d_rest = 0;
      SetPrecision ();
      // Now Increment
      PlusNmonths(tm_mday);
      return true;
      } else if (strncasecmp(str_mon, "Year", 4) == 0) {
      DP("+XX Years");
      // First set Today
      d_date =  ( now_tmP->tm_mday)*DAY_FACTOR + ( now_tmP->tm_mon + 1)*MONTH_FACTOR +
        Normalize_tm_year(now_tmP->tm_year)*YEAR_FACTOR;
      d_rest = 0;
      SetPrecision ();
      // Now Increment
      PlusNyears(tm_mday);
      return true;
      }

    }

  /* And do the sscanfs.  WARNING: you can add more formats here,
     ** but be careful!  You can easily screw up the parsing of existing
     ** formats when you add new ones.
   */
  if (getdate(cp, &tm) == true)
    {
      got_zone = 1;
    }
  /* 1997-W01-2 */
  else if ((sscanf(cp, "%[0-9]-W%d-%d%s", str_year, &tm_week, &tm_mday, str_mon) > 1 ||
	   sscanf(cp, "%[0-9]W%02d%d%s", str_year, &tm_week, &tm_mday, str_mon) > 1) &&
	   scan_year (str_year, &tm_year) )
    {
      DP ("CCYY-WXX-DD");
      if (tm_mday == 0)
	tm_mday = 1;
      if (SetWeekOfYear(tm_year, tm_week, tm_mday))
	{
	  if (str_mon[0] == 'T')
	    {
	      DP ("Optional TXX:XX:XX");
	      tm_sec = 0;
	      if (sscanf(&str_mon[1], "%d:%d:%d", &tm_hour, &tm_min, &tm_sec) > 1 ||
		   sscanf(&str_mon[1], "%02d%02d%02d", &tm_hour, &tm_min, &tm_sec))
		{
		  SetTime(tm_hour, tm_min, tm_sec);
		}
	    }
	  return true;
	}
      return false;
    }
  /* N mth CCYY HH:MM:SS ampm zone */
  else if (((sscanf (cp, "%d %[a-zA-Z] %[0-9] %d:%d:%d %[apmAPM] %[^: ]",
		&tm_mday, str_mon, str_year, &tm_hour, &tm_min, &tm_sec,
		str_ampm, str_gmtoff) == 8 &&
	scan_year (str_year, &tm_year) &&
	scan_ampm (str_ampm, &ampm)) ||
       sscanf (cp, "%d %[a-zA-Z] %[0-9] %d:%d:%d %[^: ]",
	       &tm_mday, str_mon, str_year, &tm_hour, &tm_min, &tm_sec,
	       str_gmtoff) == 7) &&
	scan_year(str_year, &tm_year) &&
	scan_mon (str_mon, &tm_mon) &&
	scan_gmtoff (str_gmtoff, &gmtoff))
    {
      DP ("N mth CCYY HH:MM:SS ampm zone");
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year =  tm_year;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
      got_zone = 1;
    }
  /* N mth CCYY HH:MM ampm zone */
  else if (((sscanf (cp, "%d %[a-zA-Z] %[0-9] %d:%d %[apmAPM] %[^: ]",
		   &tm_mday, str_mon, str_year, &tm_hour, &tm_min, str_ampm,
		     str_gmtoff) == 7 &&
	     scan_year (str_year, &tm_year) &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%d %[a-zA-Z] %[0-9] %d:%d %[^: ]",
		    &tm_mday, str_mon, str_year, &tm_hour, &tm_min,
		    str_gmtoff) == 6) &&
	     scan_year( str_year, &tm_year) &&
	     scan_mon (str_mon, &tm_mon) &&
	     scan_gmtoff (str_gmtoff, &gmtoff))
    {
      DP ("N mth CCYY HH:MM ampm zone");
      tm.tm_mday = tm_mday;
      tm.tm_mon  = tm_mon;
      tm.tm_year = tm_year;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min  = tm_min;
      tm.tm_sec  = 0;
      got_zone   = 1;
    }
  /* N mth CCYY HH:MM:SS ampm */
  else if (((sscanf (cp, "%d %[a-zA-Z] %[0-9] %d:%d:%d %[apmAPM]",
		     &tm_mday, str_mon, str_year, &tm_hour, &tm_min, &tm_sec,
		     str_ampm) == 7 &&
	     scan_year (str_year, &tm_year) &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%d %[a-zA-Z] %[0=9] %d:%d:%d",
		    &tm_mday, str_mon, str_year, &tm_hour, &tm_min,
		    &tm_sec) == 6) &&
	     scan_year (str_year, &tm_year) &&
	     scan_mon (str_mon, &tm_mon))
    {
      DP ("N mth CCYY HH:MM:SS ampm");
      tm.tm_mday = tm_mday;
      tm.tm_mon  = tm_mon;
      tm.tm_year = tm_year;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min  = tm_min;
      tm.tm_sec  = tm_sec;
    }
  /* N mth CCYY HH:MM ampm */
  else if (((sscanf (cp, "%d %[a-zA-Z] %[0-9] %d:%d %[apmAPM]",
		     &tm_mday, str_mon, str_year, &tm_hour, &tm_min,
		     str_ampm) == 6 &&
	     scan_year (str_year, &tm_year) &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%d %[a-zA-Z] %[0-9] %d:%d",
		    &tm_mday, str_mon, str_year, &tm_hour, &tm_min) == 5) &&
	   scan_year (str_year, &tm_year) &&
	   scan_mon (str_mon, &tm_mon))
    {
      DP ("N mth CCYY HH:MM ampm");
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = 0;
    }
  /* HH:MM:SS ampm zone N mth CCYY */
  else if (((sscanf (cp, "%d:%d:%d %[apmAPM] %[^: ] %d %[a-zA-Z] %[0-9]",
		 &tm_hour, &tm_min, &tm_sec, str_ampm, str_gmtoff, &tm_mday,
		     str_mon, str_year) == 8 &&
	     scan_year (str_year, &tm_year) &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%d:%d:%d %[^: ] %d %[a-zA-Z] %[0-9]",
		  &tm_hour, &tm_min, &tm_sec, str_gmtoff, &tm_mday, str_mon,
		    str_year) == 7) &&
	   scan_gmtoff (str_gmtoff, &gmtoff) &&
	   scan_year (str_year, &tm_year) &&
	   scan_mon (str_mon, &tm_mon))
    {
      DP ("HH:MM:SS ampm zone N mth CCYY");
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
    }
  /* HH:MM ampm zone N mth CCYY */
  else if (((sscanf (cp, "%d:%d %[apmAPM] %[^: ] %d %[a-zA-Z] %[0-9]",
		 &tm_hour, &tm_min, str_ampm, str_gmtoff, &tm_mday, str_mon,
		     str_year) == 7 &&
	     scan_year (str_year, &tm_year) &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%d:%d %[^: ] %d %[a-zA-Z] %[0-9]",
		    &tm_hour, &tm_min, str_gmtoff, &tm_mday, str_mon,
		    str_year) == 6) &&
	   scan_gmtoff (str_gmtoff, &gmtoff) &&
	   scan_year (str_year, &tm_year) &&
	   scan_mon (str_mon, &tm_mon))
    {
      DP ("HH:MM ampm N mth CCYY");
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = 0;
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
    }
  /* HH:MM:SS ampm N mth CCYY */
  else if (((sscanf (cp, "%d:%d:%d %[apmAPM] %d %[a-zA-Z] %[0-9]",
		     &tm_hour, &tm_min, &tm_sec, str_ampm, &tm_mday, str_mon,
		     str_year) == 7 &&
	     scan_year (str_year, &tm_year) &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%d:%d:%d %d %[a-zA-Z] %[0-9]",
		    &tm_hour, &tm_min, &tm_sec, &tm_mday, str_mon,
		    str_year) == 6) &&
	   scan_year (str_year, &tm_year) &&
	   scan_mon (str_mon, &tm_mon))
    {
      DP ("HH:MM:SS ampm N mth CCYY");
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
    }
  /* HH:MM ampm N mth CCYY */
  else if (((sscanf (cp, "%d:%d %[apmAPM] %d %[a-zA-Z] %[0-9]",
		     &tm_hour, &tm_min, str_ampm, &tm_mday, str_mon,
		     str_year) == 6 &&
	     scan_year (str_year, &tm_year) &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%d:%d %d %[a-zA-Z] %[0-9]",
		    &tm_hour, &tm_min, &tm_mday, str_mon, str_year) == 5) &&
	   scan_year (str_year, &tm_year) &&
	   scan_mon (str_mon, &tm_mon))
    {
      DP ("HH:MM ampm N mth CCYY");
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = 0;
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;

    }
  /* wdy, N mth CCYY HH:MM:SS ampm zone */
  else if (((sscanf (cp, "%[a-zA-Z], %d %[a-zA-Z] %[0-9] %d:%d:%d %[apmAPM] %[^: ]",
		   str_wday, &tm_mday, str_mon, str_year, &tm_hour, &tm_min,
		     &tm_sec, str_ampm, str_gmtoff) == 9 &&
	     scan_year (str_year, &tm_year) &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%[a-zA-Z], %d %[a-zA-Z] %[0-9] %d:%d:%d %[^: ]",
		    str_wday, &tm_mday, str_mon, str_year, &tm_hour, &tm_min,
		    &tm_sec, str_gmtoff) == 8) &&
	   scan_wday (str_wday, &tm_wday) &&
	   scan_year (str_year, &tm_year) &&
	   scan_mon (str_mon, &tm_mon) &&
	   scan_gmtoff (str_gmtoff, &gmtoff))
    {
      DP ("wdy, N mth CCYY HH:MM:SS ampm zone");

      tm.tm_wday = tm_wday;
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year =  tm_year;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
      got_zone = 1;
    }
#if 1
 /* wdy, mth N, CCYY HH:MM:SS ampm zone */
// Tuesday, September 29, 1998 8:38:38 PM
  else if ((sscanf (cp, "%[a-zA-Z], %[a-zA-Z] %d, %[0-9] %d:%d:%d %[apmAPM] %[^: ]",
                   str_wday, str_mon, &tm_mday, str_year, &tm_hour, &tm_min,
                     &tm_sec, str_ampm, str_gmtoff) >=8 &&
	   (tm_mday > 0 && tm_mday < 32) &&
           scan_ampm (str_ampm, &ampm)) &&
           scan_wday (str_wday, &tm_wday) &&
	   scan_year (str_year, &tm_year) &&
           scan_mon (str_mon, &tm_mon) )
    {
      DP ("wdy, mth N, CCYY HH:MM:SS ampm zone");
      tm.tm_wday = tm_wday;
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
#if DEBUG
    cerr << "Time = " << tm.tm_hour << ":" << tm.tm_min << ":" << tm.tm_sec << endl;
#endif
      got_zone = scan_gmtoff (str_gmtoff, &gmtoff);
#if DEBUG
cerr << "GM offset = " << gmtoff << endl;
#endif
    }
#endif
  /* wdy, N mth CCYY HH:MM ampm zone */
  else if (((sscanf (cp, "%[a-zA-Z], %d %[a-zA-Z] %d %d:%d %[apmAPM] %[^: ]",
		   str_wday, &tm_mday, str_mon, &tm_year, &tm_hour, &tm_min,
		     str_ampm, str_gmtoff) == 8 &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%[a-zA-Z], %d %[a-zA-Z] %[0-9] %d:%d %[^: ]",
		    str_wday, &tm_mday, str_mon, str_year, &tm_hour, &tm_min,
		    str_gmtoff) == 7) &&
	   scan_wday (str_wday, &tm_wday) &&
	   scan_year (str_year, &tm_year) &&
	   scan_mon (str_mon, &tm_mon) &&
	   scan_gmtoff (str_gmtoff, &gmtoff))
    {
      DP ("wdy, N mth CCYY HH:MM ampm zone");
      tm.tm_wday = tm_wday;
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = 0;
      got_zone = 1;
    }
  /* wdy, N mth CCYY HH:MM:SS ampm */
  else if (((sscanf (cp, "%[a-zA-Z], %d %[a-zA-Z] %[0-9] %d:%d:%d %[apmAPM]",
		   str_wday, &tm_mday, str_mon, str_year, &tm_hour, &tm_min,
		     &tm_sec, str_ampm) == 8 &&
	     scan_year (str_year, &tm_year) &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%[a-zA-Z], %d %[a-zA-Z] %[0-9] %d:%d:%d",
		    str_wday, &tm_mday, str_mon, str_year, &tm_hour, &tm_min,
		    &tm_sec) == 7) &&
	   scan_wday (str_wday, &tm_wday) &&
	   scan_year (str_year, &tm_year) &&
	   scan_mon (str_mon, &tm_mon))
    {
      DP ("wdy, N mth CCYY HH:MM:SS ampm");
      tm.tm_wday = tm_wday;
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
    }
  /* wdy, N mth CCYY HH:MM ampm */
  else if (((sscanf (cp, "%[a-zA-Z], %d %[a-zA-Z] %[0-9] %d:%d %[apmAPM]",
		   str_wday, &tm_mday, str_mon, str_year, &tm_hour, &tm_min,
		     str_ampm) == 7 &&
	     scan_year (str_year, &tm_year) &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%[a-zA-Z], %d %[a-zA-Z] %[0-9] %d:%d",
		    str_wday, &tm_mday, str_mon, str_year, &tm_hour,
		    &tm_min) == 6) &&
	   scan_wday (str_wday, &tm_wday) &&
	   scan_year (str_year, &tm_year) &&
	   scan_mon (str_mon, &tm_mon))
    {
      DP ("wdy, N mth CCYY HH:MM ampm");
      tm.tm_wday = tm_wday;
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = 0;
    }
  /* wdy mth N HH:MM:SS ampm zone CCYY */
  else if (((sscanf (cp, "%[a-zA-Z] %[a-zA-Z] %d %d:%d:%d %[apmAPM] %[^: ] %[0-9]",
		     str_wday, str_mon, &tm_mday, &tm_hour, &tm_min, &tm_sec,
		     str_ampm, str_gmtoff, str_year) == 9 &&
	     scan_year (str_year, &tm_year) &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%[a-zA-Z] %[a-zA-Z] %d %d:%d:%d %[^: ] %[0-9]",
		    str_wday, str_mon, &tm_mday, &tm_hour, &tm_min, &tm_sec,
		    str_gmtoff, str_year) == 8) &&
	   scan_wday (str_wday, &tm_wday) &&
	   scan_mon (str_mon, &tm_mon) &&
	   scan_year (str_year, &tm_year) &&
	   scan_gmtoff (str_gmtoff, &gmtoff))
    {
      DP ("wdy mth N HH:MM:SS ampm zone CCYY");
      tm.tm_wday = tm_wday;
      tm.tm_mon = tm_mon;
      tm.tm_mday = tm_mday;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
      got_zone = 1;
      tm.tm_year = tm_year;
    }
  /* wdy mth N HH:MM ampm zone CCYY */
  else if (((sscanf (cp, "%[a-zA-Z] %[a-zA-Z] %d %d:%d %[apmAPM] %[^: ] %[0-9]",
		     str_wday, str_mon, &tm_mday, &tm_hour, &tm_min,
		     str_ampm, str_gmtoff, str_year) == 8 &&
	     scan_year (str_year, &tm_year) &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%[a-zA-Z] %[a-zA-Z] %d %d:%d %[^: ] %[0-9]",
		    str_wday, str_mon, &tm_mday, &tm_hour, &tm_min,
		    str_gmtoff, str_year) == 7) &&
	   scan_wday (str_wday, &tm_wday) &&
	   scan_mon (str_mon, &tm_mon) &&
	   scan_year (str_year, &tm_year) &&
	   scan_gmtoff (str_gmtoff, &gmtoff))
    {
      DP ("wdy mth N HH:MM ampm zone CCYY");
      tm.tm_wday = tm_wday;
      tm.tm_mon = tm_mon;
      tm.tm_mday = tm_mday;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = 0;
      got_zone = 1;
      tm.tm_year = tm_year;
    }
 /* wdy mth N HH:MM:SS CCYY */ /* edz ADDED */
  else if ((sscanf (cp, "%[a-zA-Z] %[a-zA-Z] %d %d:%d:%d %[0-9]",
                     str_wday, str_mon, &tm_mday, &tm_hour, &tm_min,
                     &tm_sec, str_year) == 7) &&
           scan_wday (str_wday, &tm_wday) &&
	   scan_year (str_year, &tm_year) &&
           scan_mon (str_mon, &tm_mon) )
    {
      DP ("wdy mth N HH:MM:SS CCYY");
      tm.tm_wday = tm_wday;
      tm.tm_mon = tm_mon;
      tm.tm_mday = tm_mday;
      tm.tm_hour = tm_hour;
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
      got_zone = 0;
      tm.tm_year = tm_year;
    }

  /* wdy mth N HH:MM CCYY */ /* edz ADDED */
  else if ((sscanf (cp, "%[a-zA-Z] %[a-zA-Z] %d %d:%d %[0-9]",
		     str_wday, str_mon, &tm_mday, &tm_hour, &tm_min,
		     str_year) == 6) &&
           scan_wday (str_wday, &tm_wday) &&
	   scan_year (str_year, &tm_year) &&
           scan_mon (str_mon, &tm_mon) )
    {
      DP ("wdy mth N HH:MM CCYY");
      tm.tm_wday = tm_wday;
      tm.tm_mon = tm_mon;
      tm.tm_mday = tm_mday;
      tm.tm_hour = tm_hour;
      tm.tm_min = tm_min;
      tm.tm_sec = 0;
      got_zone = 0;
      tm.tm_year = tm_year;
    }
 /* N mth CCYY */
  else if ((sscanf (cp, "%[0-9]%c%[a-zA-Z]%c%[0-9]", str_wday, &ch, str_mon, &ch2, str_year) == 5 &&
	    (ch == ch2) &&
            (ch == ' ' || ispunct(ch)) ) &&
            scan_mon (str_mon, &tm_mon) &&
            ((tm_mday = atoi(str_wday)) > 31 || (tm_year = atoi(str_year)) > 31 ||
             (str_year[0] == '0' && str_year[1] == '0') || (str_wday[0] == '0' && str_wday[1] == '0')))
    {
      if (tm_year > 31 || (str_year[0] == '0' && str_year[1] == '0'))
	{
	  DP ("N mth CCYY");
	  tm.tm_mday = tm_mday;
	  tm.tm_mon = tm_mon;
	  scan_year(str_year, &tm_year);
	  tm.tm_year = tm_year;
	}
      else
	{
	  DP("CCYY mth N");
	  tm.tm_mday = atoi(str_year);
	  tm.tm_mon = tm_mon;
	  scan_year(str_wday, &tm_year);
	  tm.tm_year = tm_year;
	}
      tm.tm_hour = 0;
      tm.tm_min = 0;
      tm.tm_sec = 0;
      gmtoff = 0; /* Don't adjust */
      got_zone = 1;
    }
 /* Mth CCYY */
 else if ( strlen(cp) <= 9 &&
        sscanf(cp,  "%[a-zA-Z]%c%[0-9]", str_mon, &ch, str_year) == 2 &&
	(ch == ' ' || ispunct(ch)) &&
	scan_mon(str_mon, &tm_mon) &&
	isdigit(str_year[0]) && isdigit(str_year[1]) && isdigit(str_year[2]) && isdigit(str_year[3]) &&
	scan_year(str_year, &tm_year) )
    {
       DP("mth CCYY");
      tm.tm_mday = 1;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
      tm.tm_hour = 0;
      tm.tm_min = 0;
      tm.tm_sec = 0;
      gmtoff = 0; /* Don't adjust */
      got_zone = 1;
    }
  // mth N CCYY */
  else if ((sscanf (cp, "%[a-zA-Z] %d %[0-9]", str_mon, &tm_mday, str_year) == 3) &&
            scan_year(str_year, &tm_year) &&
            tm_mday < 32 && scan_mon (str_mon, &tm_mon))
    {
      DP ("mth N CCYY");
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
      tm.tm_hour = 0;
      tm.tm_min = 0;
      tm.tm_sec = 0;
      gmtoff = 0; /* Don't adjust */
      got_zone = 1;
    }
  /* N, mth CCYY */
  else if ((sscanf (cp, "%d, %[a-zA-Z] %[0-9]", &tm_mday, str_mon, str_year) == 3) &&
	    scan_year(str_year, &tm_year) &&
            tm_mday < 32 && scan_mon (str_mon, &tm_mon))
    {
      DP ("N, mth CCYY");
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
      tm.tm_hour = 0;
      tm.tm_min = 0;
      tm.tm_sec = 0;
      gmtoff = 0; /* Don't adjust */
      got_zone = 1;
    }
  /* HH:MM:SS ampm */
  else if ((sscanf (cp, "%d:%d:%d %[apmAPM]",
		    &tm_hour, &tm_min, &tm_sec, str_ampm) == 4 &&
	    scan_ampm (str_ampm, &ampm)) ||
	   sscanf (cp, "%d:%d:%d", &tm_hour, &tm_min, &tm_sec) == 3)
    {
      DP ("HH:MM:SS ampm");
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
    }
  /* HH:MM ampm */
  else if ((sscanf (cp, "%d:%d %[apmAPM]", &tm_hour, &tm_min,
		    str_ampm) == 3 &&
	    scan_ampm (str_ampm, &ampm)) ||
	   sscanf (cp, "%d:%d", &tm_hour, &tm_min) == 2)
    {
      DP ("HH:MM");
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = 0;
    }
  /* ISO CCYY-DayOfYear */
  else if (isdigit (cp[0]) && isdigit (cp[1]) && isdigit (cp[2]) && isdigit (cp[3]) &&
        cp[4] == '-' && isdigit(cp[5]) && isdigit(cp[6]) && isdigit(cp[7]) &&
	(cp[8] == '\0' || cp[8] == 'T'))
    {
      DP ("ISO CCYY-DDD");
      if (SetDayOfYear(atoi(cp), atoi(cp+5)))
 	{
	  tm_sec = 0;
	  if ((cp[8] == 'T' &&  sscanf (cp+9, "%d:%d:%d", &tm_hour, &tm_min, &tm_sec) > 1) ||
		sscanf(cp+9, "%02d%02d%02d", &tm_hour, &tm_min, &tm_sec))
	    SetTime(tm_hour, tm_min, tm_sec);
	  return true;
	}
      return false;
    }
  /* ISO Date    CCYYMMDDTHH:MM:SS[Z] */
  else if ( (sscanf (cp, "%4d%2d%2dT%d:%d:%2d%c%s",
		&tm_year, &tm_mon, &tm_mday, &tm_hour, &tm_min, &tm_sec, &sc, str_gmtoff) > 4) ||
	  (sscanf (cp, "%4d%2d%2dT%2d%2d%2d%c%s",
		&tm_year, &tm_mon, &tm_mday, &tm_hour, &tm_min, &tm_sec, &sc, str_gmtoff) >= 4) )
    {
      DP("CCYYMMDDTHH:MM:SS");
      tm.tm_year = tm_year;
      tm.tm_mon = tm_mon - 1;
      tm.tm_mday = tm_mday;
      tm.tm_hour = tm_hour;
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
      if (sc == 'Z') {
	DP("HAVE GMT ZONE");
	got_zone = 1;
	gmtoff = 0;
      } else if (sc == '+' || sc == '-') {
zone_it:
	DP("+- ISO ZONE SPECIFIED");
	// Timezone difference specified
	if (isdigit(str_gmtoff[0])) {
	  int   gmtoff_hour = 0;
	  int   gmtoff_min  = 0;
	  if (sscanf (str_gmtoff, "%d:%d", &gmtoff_hour, &gmtoff_min) != 2 ) {
	    sscanf(str_gmtoff, "%2d%2d", &gmtoff_hour, &gmtoff_min);
	  }
#if DEBUG
	cerr << "Hour offset = " << gmtoff_hour << endl;
	cerr << "Min offset  = " << gmtoff_min << endl;
#endif
	  gmtoff = (60*gmtoff_hour + gmtoff_min) * (sc == '-' ? -60 : 60);
#if DEBUG
	cerr << "Gmtoff = " << gmtoff << endl;
#endif
	}
      }
    }
  /* ISO Date    CCYY-MM-DDTHH:MM:SSZ (eg.  1996-07-16T13:19:51Z) */
  else if (isdigit (cp[0]) && isdigit (cp[1]) && isdigit(cp[2]) && isdigit(cp[3]) &&
	cp[4] == '-' && isdigit (cp[5]) )
    {
      tm_sec  = 0;
      tm_mday = 1;
      if ((sscanf(cp, "%d-%d-%dT%d:%d:%d%c%s",
		&tm_year, &tm_mon, &tm_mday, &tm_hour, &tm_min, &tm_sec, &sc, str_gmtoff) > 4) ||
	  (sscanf(cp, "%d-%d-%dT%2d%2d%2d%c%s",
		&tm_year, &tm_mon, &tm_mday, &tm_hour, &tm_min, &tm_sec, &sc, str_gmtoff) >= 4) )
	{
	  DP("CCYY-MM-DDTHH:MM:SS");
	  tm.tm_year = tm_year;
	  tm.tm_mon = tm_mon - 1;
	  tm.tm_mday = tm_mday;
	  tm.tm_hour = tm_hour;
	  tm.tm_min = tm_min;
	  tm.tm_sec = tm_sec;
	  if (sc == 'Z') {
	    got_zone = 1;
	    gmtoff = 0;
	  } else if (sc == '+' || sc == '-') {
	    goto zone_it;
	  }
	}
      else if (sscanf(cp, "%d-%d-%d", &tm_year, &tm_mon, &tm_mday) >= 2)
        {
	  DP("CCYY-MM-DD");
	  tm.tm_year = tm_year;
	  tm.tm_mon = tm_mon - 1;
	  tm.tm_mday = tm_mday;
	  tm.tm_hour = 0;
	  tm.tm_min = 0;
	  tm.tm_sec = 0;
	  gmtoff = 0; /* Don't adjust */
	  got_zone = 1;
        }
      else
	return false; /* OOPS */
    }
  /* ANSI Date: CCYYMMDD or obsolete form YYMMDD and optional HH:MM ZONE */
  else if (isdigit (cp[0]) && isdigit (cp[1]) && isdigit (cp[2]) && isdigit (cp[3])
	   && isdigit (cp[4]) && isdigit (cp[5]))
    {
      int pos = 0;
      int normalize = 0;

      // 19xx or 20xx or yyNMzz where NM > 12 or 0
      int test = (cp[2] - '0')*10 + cp[3] - '0';
      if (( (test == 0) || (test > 12) ||
	(cp[0] == '1' && cp[1] == '9') || (cp[0] == '2' && cp[1] == '0') ) 
	&& strlen(cp) == 6)
	{
	  const int month  = (cp[4]-'0')*10 + cp[5]-'0';

	  if (month > 0 && month < 13)
	    {
	      DP("CCYYMM");
	      const int year = (cp[0]-'0')*1000 + (cp[1]-'0')*100 + (cp[2]-'0')*10 + cp[3]-'0';

	      d_date = year*YEAR_FACTOR + month*MONTH_FACTOR;
	      d_rest = 0;
	      SetPrecision( MONTH_PREC );
	      return true;
	    }
	}

      DP ("CCYYMMDD or YYMMDD");
      tm.tm_year = 0;
      if (isdigit(cp[6]) && (cp[7] == '\0' || isdigit(cp[7])))
	{
	  // CCYYMMDD
	  tm.tm_year = 0;
	  if (isdigit(cp[7]))
	    {
	      tm.tm_year +=  (cp[pos++] - '0') * 1000;
	    }
	  // else CYYMMDD
	  tm.tm_year += (cp[pos++] - '0') * 100;
	}
      else			/* Obsolete form YYMMDD */
	{
	  normalize = 1;
	}
      tm.tm_year += (cp[pos++] - '0') * 10;
      tm.tm_year += (cp[pos++] - '0');

      if (normalize)
	tm.tm_year = Normalize_tm_year(tm.tm_year);


      tm.tm_mon =  (cp[pos++] - '0') * 10;
      tm.tm_mon += (cp[pos++] - '0') - 1;

      tm.tm_mday =  (cp[pos++] - '0') * 10;
      tm.tm_mday += (cp[pos++] - '0');
      if (isspace (cp[pos]) || cp[pos] == '-')
	{
	  if (date_parse (cp + pos + 1))
	    {
	      long rest  = GetTime();
	      tm.tm_hour = _Hour(rest);
	      tm.tm_min  = _Minutes(rest);
	      tm.tm_sec  = _Seconds(rest);

	      gmtoff = 0;
	      cp += (pos+1);
	      while (*cp && *cp != ' ') cp++;
	      while (*cp == ' ') cp++;
	      scan_gmtoff (cp, &gmtoff);
	      got_zone = 1;
	    }
	}
      if (!got_zone)
	{
	  tm.tm_hour = 0;
	  tm.tm_min = 0;
	  tm.tm_sec = 0;
	  gmtoff = 0; /* Don't adjust */
	  got_zone = 1;
	}
    }
  /* CCYY Year format */
  else if (isdigit(cp[0]) && isdigit(cp[1]) && isdigit(cp[2]) && isdigit(cp[3]) && cp[4] == '\0')
    {
      int year_test = (cp[0] - '0')*10 + cp[1] - '0';
      int month_test = (cp[2] - '0')*10 + cp[3] - '0';

      if (((year_test < 10) || (year_test > 30)) && (month_test > 0 && month_test < 13))
	{
	  DP("YYMM");
	  d_date = (year_test+2000)*YEAR_FACTOR + month_test*MONTH_FACTOR;
	  SetPrecision( MONTH_PREC );
	  return true;
	}

      DP("CCYY");
      d_date = (year_test*100 + month_test)*YEAR_FACTOR;
      d_rest = 0;
      SetPrecision( YEAR_PREC );
      return true;
    }
  /* YY Year format */
  else if (isdigit(cp[0]) && isdigit(cp[1]) && cp[2] == '\0')
    {
      DP("YY");
      d_date = Normalize_tm_year(atol(cp))*YEAR_FACTOR;
      d_rest = 0;
      SetPrecision( YEAR_PREC );
      return true;
    }
  /* Stupid CCYY.MM.DD form */
  else if (isdigit (cp[0]) && isdigit (cp[1]) && isdigit(cp[2]) && isdigit(cp[3]) &&
	ispunct(cp[4]) && isdigit (cp[5]) && isdigit (cp[6]))
    {
      DP ("CCYY.MM[.DD]");
      tm.tm_year = 1000*(cp[0] - '0') + 100*(cp[1] - '0') + 10*(cp[2] - '0') + (cp[3] - '0');
      tm.tm_mon = (cp[5] - '0') * 10 + (cp[6] - '0') - 1;
      if (ispunct(cp[7]) && isdigit(cp[8]) && isdigit(cp[9]))
	{
	  tm.tm_mday = (cp[8] - '0') * 10 + (cp[9] - '0');
	}
      else if (cp[7] && !isspace(cp[7]))
	{
	  return false; // ERROR
	}
      else
	tm.tm_mday = 0;
      gmtoff = 0; /* Don't adjust */
      if (tm.tm_mon < 0 || tm.tm_mon > 12 || tm.tm_mday < 0 || tm.tm_mday > 31)
	{
	  return false; /* ERROR */
	}
      tm.tm_hour = 0;
      tm.tm_min = 0;
      tm.tm_sec = 0;
    }
  /* Stupid DD.MM.CCYY and DD.MM.YY forms */
  else if (isdigit (cp[0]) && isdigit (cp[1]) && cp[2] == '.' && isdigit (cp[3]) &&
      isdigit (cp[4]) && cp[5] == '.' && isdigit (cp[6]) && isdigit (cp[7]))
    {
      DP ("MM.DD.CCYY");
      tm.tm_mday = (cp[0] - '0') * 10 + (cp[1] - '0');
      tm.tm_mon = (cp[3] - '0') * 10 + (cp[4] - '0') - 1;
      tm.tm_year = (cp[6] - '0') * 10 + (cp[7] - '0');
      if (isdigit (cp[8]) && isdigit (cp[9]))
	tm.tm_year = tm.tm_year * 100 + (cp[8] - '0') * 10 + (cp[9] - '0');
      else
	tm.tm_year = Normalize_tm_year(tm.tm_year);
      /* Make sure it makes sense! */
      if (tm.tm_mon < 0 || tm.tm_mon > 12 || tm.tm_mday < 0 || tm.tm_mday > 31)
	return false;	/* ERROR */
      gmtoff = 0; /* Don't adjust */
      got_zone = 1;
      tm.tm_hour = 0;
      tm.tm_min = 0;
      tm.tm_sec = 0;
    }
  else
    return false;

  d_date = (tm.tm_year)*YEAR_FACTOR + (tm.tm_mon+1)*MONTH_FACTOR + (tm.tm_mday)*DAY_FACTOR;

  /* Get offset right */
  if (tm.tm_hour || tm.tm_min || tm.tm_sec || got_zone)
    {
      t = (tm.tm_hour - ((tm.tm_isdst && !got_zone) ? 1 : 0)) * 3600;
      t += (tm.tm_min) * 60;
      t += (tm.tm_sec);
      t -= gmtoff;
      if (t < 0) {
	/* push back */
	t += 24*60*60; // Add 1 day of seconds..
	/* decrement */
	if (tm.tm_mday == 1) {
	  if (tm.tm_mon <= 0)
	    {
	      tm.tm_mon = 11;
	      tm.tm_year--;
	    }
	  else tm.tm_mon--;
	  /* Get days in month for year */
	  tm.tm_mday = mntb[tm.tm_mon] +
		(((tm.tm_year)%400)?
		(((tm.tm_year)%100)?
		((tm.tm_year)%4?0:1):0):1); 
	}
	else tm.tm_mday--;
      }
      // if (t == 0) t = 1; // We don't want 0:0:0
      d_rest = ((t / 3600 ) % 24 )*HOUR_FACTOR + ((t / 60) % 60 )*MIN_FACTOR + ( t % 60 )*SEC_FACTOR;
    }
  SetPrecision( DAY_PREC );
  if (IsBogusDate())
    return false;
  return true;
}

//-------------------------- C++ Interface ------------------------------------------------

// Constructors
SRCH_DATE::SRCH_DATE() 
{
  d_date = d_rest = 0;
//SetPrecision(UNKNOWN_DATE);
}

SRCH_DATE::SRCH_DATE(const SRCH_DATE& OtherDate)
{
  Set(OtherDate);
}


SRCH_DATE::SRCH_DATE(const DOUBLE FloatVal)
{
  Set(FloatVal);
}


SRCH_DATE::SRCH_DATE(const STRING& StringVal)
{
  Set(StringVal);
}


SRCH_DATE::SRCH_DATE(const CHR* CStringVal)
{
  Set(CStringVal);
}


SRCH_DATE::SRCH_DATE(const long LongVal)
{
  Set(LongVal);
}


SRCH_DATE::SRCH_DATE(const int IntVal)
{
  Set((long)IntVal);
}

SRCH_DATE::SRCH_DATE(struct tm *time_str)
{
  Set(time_str);
}

SRCH_DATE::SRCH_DATE(const time_t *Time)
{
  Set(Time);
}

SRCH_DATE::SRCH_DATE(FILE *Fp)
{
  Set(Fp);
}

enum Date_Precision SRCH_DATE::GetPrecision() const
{
  return (enum Date_Precision)((d_rest & 0x0FF00000) >> 20);
}

UINT4 SRCH_DATE::GetTime() const
{
  return (d_rest & 0x000FFFFF);
}

void SRCH_DATE::SetTime(UINT4 Time)
{
  d_rest = (d_rest & 0xFFF00000) | (Time & 0x000FFFFF); 
}


void SRCH_DATE::SetPrecision(enum Date_Precision prec)
{
  d_rest = (d_rest & 0x000FFFFF) | ((UINT4)(prec & 0xFF) << 20);
}


// Write the date as 8 bytes.. 
void SRCH_DATE::Write(PFILE Fp) const
{
//  putObjID(objDATE, Fp);
  ::Write((INT4)d_date, Fp); 
  ::Write((INT4)d_rest, Fp);
}

bool SRCH_DATE::Read(PFILE Fp)
{
//  obj_t obj = getObjID(Fp);
//  if (obj != objDATE)
//    {
//      PushBackObjID(obj, Fp);
//    }
//  else
//    {
      INT4 date, rest;
      ::Read(&date, Fp);
      ::Read(&rest, Fp);
      d_date = date;
      d_rest = rest;
return true;
 //   }
//  return obj == objDATE;
}

SRCH_DATE::~SRCH_DATE() 
{
}


// See "Astronomical Formulae For Calculators" by Jean Meeus,
//
// or
// Fliegel, H. F., and Van Flandern, T. C., "A Machine Algorithm for
//   Processing Calendar Dates," Communications of the Association of
//   Computing Machines, vol. 11 (1968), p. 657.

static long _to_julian(int day, int month, int year)
{
  long b = 0;
  if (month <= 2)
    {
      year--;
      month +=12;
    }
  // deal with Gregorian calendar
  if (year*10000. + month*100. + day >= 15821015L)
    {
      int a = year/100;
      b += 2 - a + (int)(a/4);
    }
  // Julian date..
  return (long) (365.25*year) + (long) (30.6001 * (month+1)) + day + 1720994L + b;
}

static long _to_julian (long time)
{
  return _to_julian(_Day(time), _Month(time), _Year(time));
}

long SRCH_DATE::GetJulianDate() const
{
  return _to_julian(d_date);
}


static long _julian_to_date(long julian)
{
  long a;
  const long z = julian+1;

  // dealing with Gregorian calendar reform
  if (z < 2299161L)
    a = z;
  else
    {
      const long alpha = (long) ((z-1867216.25) / 36524.25);
      a = z + 1 + alpha - alpha/4;
    }
  long b = ( a > 1721423 ? a + 1524 : a + 1158 );
  long c = (long) ((b - 122.1) / 365.25);
  long d = (long) (365.25 * c);
  long e = (long) ((b - d) / 30.6001);

  int day = (int)(b - d - (long)(30.6001 * e));
  int month = (int)((e < 13.5) ? e - 1 : e - 13);
  int year = (int)((month > 2.5 ) ? (c - 4716) : c - 4715);

  return year*YEAR_FACTOR + month*MONTH_FACTOR + day*DAY_FACTOR + julian - z + 1;
}



long SRCH_DATE::MinutesDiff(SRCH_DATE Other) const
{
  const long long julian         = _to_julian( d_date );
  const int  res_min             = _Hour(d_rest)*60 + _Minutes(d_rest);
  const long long other_julian   = _to_julian( Other.d_date );
  const int  other_res_min       = _Hour(Other.d_rest)*60 + _Minutes(Other.d_rest);

  return julian*60*24 + res_min - (other_julian*60*24 + other_res_min);
}



SRCH_DATE& SRCH_DATE::Plus(const SRCH_DATE& OtherDate)
{
  if (IsValidDate() && OtherDate.IsValidDate())
    {
      const long date = OtherDate.d_date;
      const long rest = OtherDate.GetTime();

      const int year       = _Year(date);
      const int mon        = _Month(date);
      const int mday       = _Day(date);

      if (rest)
	{
	  const int hour  = _Hour(rest);
	  const int min   = _Minutes(rest);
	  const int sec   = _Seconds(rest);

	  PlusNseconds(sec);
	  PlusNminutes(min);
	  PlusNhours(hour);
	}
      PlusNdays(mday);
      PlusNmonths(mon);
      PlusNyears(year);
    }
  return *this;
}

SRCH_DATE& SRCH_DATE::Minus(const SRCH_DATE& OtherDate)
{
  if (IsValidDate() && OtherDate.IsValidDate())
    {
      const long date = OtherDate.d_date;
      const long rest = OtherDate.GetTime();

      const int year       = _Year(date);
      const int mon        = _Month(date);
      const int mday       = _Day(date);

      if (rest)
	{
	  const int hour  = _Hour(rest);
	  const int min   = _Minutes(rest);
	  const int sec   = _Seconds(rest);

	  MinusNseconds(sec);
	  MinusNminutes(min);
	  MinusNhours(hour);
	}
      MinusNdays(mday);
      MinusNmonths(mon);
      MinusNyears(year);
    }
  return *this;
}



SRCH_DATE& SRCH_DATE::PlusNseconds(int seconds)
{
#ifdef DEBUG
  cerr << "PlusNseconds(" << seconds << ")" << endl;
  cerr << "PRECISON = " << GetPrecision() <<  " DAY_PREC=" << (int)DAY_PREC << endl;
  cerr << "ValidDate = " << (int)IsValidDate() << endl;
#endif
  if (GetPrecision() < DAY_PREC && (abs(seconds) < 60*60*24))
    return *this;
  if (IsValidDate() && seconds)
    {
      const long rest = GetTime();

      int hour  = _Hour(rest);
      int min   = _Minutes(rest); 
      int sec   = _Seconds(rest); 
      int add_days    = 0;
#ifdef DEBUG
      cerr << "time = " << hour << ":" << min << ":" << sec << endl;
#endif
      sec += seconds; // Add the seconds
      if (sec < 0)
	{
	  int sec_offset  = -seconds;
	  int min_offset  = sec_offset/60; // 60 secs to 1 min
	  int hour_offset = min_offset/60; // 60 min to 1 hour
	  int day_offset  = hour_offset/24; // 24 hours to 1 day

	  sec -= seconds; // Restore

	  sec_offset  %= 60;
	  min_offset  %= 60;
	  hour_offset %= 24;

	  if ((sec -= sec_offset) < 0)
	    {
	      sec+= 60;
	      if(++min_offset >= 60)
		{
		  min_offset -= 60;
		  if (++hour_offset >= 24)
		    {
		      hour_offset -= 24;
		      day_offset++;
		    }
		}
	    }
	  if ((min -= min_offset) < 0)
	    {
	      min += 60;
	      if (++hour_offset >= 24)
		day_offset++;
	    }
	  if ((hour -= hour_offset) < 0)
	    {
	      hour += 24;
	      day_offset++;
	    }
	  add_days = - day_offset;
	}
      else if (sec >= 60)
	{
	  // plus
	  min += sec/60;
	  sec %= 60;
	  
	  if (min > 59)
	    {
	      hour += min/60;
	      min  %= 60;
	      if (hour >= 24)
		{
		  add_days = hour/24;
		  hour %= 24;
		}
	    }
	}
      d_rest = hour*HOUR_FACTOR + min*MIN_FACTOR + sec*SEC_FACTOR;
      SetPrecision();
      if (add_days)
	PlusNdays(add_days);
    }
  return *this;
}

SRCH_DATE& SRCH_DATE::PlusNminutes(int minutes)
{
  return PlusNseconds(60*minutes);
}

SRCH_DATE& SRCH_DATE::PlusNhours(int hours)
{
  return PlusNminutes(60*hours);
}


SRCH_DATE& SRCH_DATE::PlusNdays(int days)
{
  if (IsValidDate() && days)
    {
      enum Date_Precision d_prec = GetPrecision();
      d_date = _julian_to_date ( _to_julian (d_date) + days);
      SetPrecision(d_prec); // Maintain precision
    }
  return *this;
}

SRCH_DATE& SRCH_DATE::PlusNmonths(int months)
{
  if (IsValidDate() && months)
    {
      int year      = _Year(d_date);
      int month     = _Month(d_date) + months - 1;
      int day       = _Day(d_date);

      enum Date_Precision d_prec = GetPrecision();
      if (month < 0)
	{
	  year -= 1 + month/12;
	  month += 12;
	  month %= 12;
	}
      else if (month > 0)
	{
	  year += month/12;
	  month %= 12;
	}
      d_date = year*YEAR_FACTOR + (month+1)*MONTH_FACTOR + day*DAY_FACTOR;
      SetPrecision(d_prec); // Maintain precision
    }
  return *this;
}

SRCH_DATE& SRCH_DATE::PlusNyears(int years)
{
  if (IsValidDate())
    d_date += years*YEAR_FACTOR;
  return *this;
}

// Operators
SRCH_DATE& SRCH_DATE::operator +=(const SRCH_DATE& OtherDate)
{
  return Plus(OtherDate); 
}

SRCH_DATE& SRCH_DATE::operator -=(const SRCH_DATE& OtherDate)
{
  return Minus(OtherDate);
}

SRCH_DATE& SRCH_DATE::operator +=(INT add)
{
  SRCH_DATE OtherDate(add);
  if (IsValidDate() && OtherDate.IsValidDate())
    {
      Plus(OtherDate);
    }
  return *this;
}

SRCH_DATE& SRCH_DATE::operator -=(INT minus)
{
  SRCH_DATE OtherDate(minus);
  if (IsValidDate() && OtherDate.IsValidDate())
    {
      Minus(OtherDate);
    }
  return *this;
}

SRCH_DATE& SRCH_DATE::operator +=(DOUBLE add)
{
  SRCH_DATE OtherDate(add);
  if (IsValidDate() && OtherDate.IsValidDate())
    {
      Plus(OtherDate);
    }
  return *this;
}

SRCH_DATE& SRCH_DATE::operator -=(DOUBLE minus)
{
  SRCH_DATE OtherDate(minus);
  if (IsValidDate() && OtherDate.IsValidDate())
    {
      Minus(OtherDate);
    }
  return *this;
}


SRCH_DATE::operator DOUBLE () const
{
  return GetValue();
}

DOUBLE SRCH_DATE::GetValue() const
{
  enum Date_Precision d_prec = GetPrecision();
  if (d_prec == UNKNOWN_DATE)
    return DATE_UNKNOWN;
  else if (d_prec == BAD_DATE)
    return DATE_ERROR;
  else if (d_prec == CURRENT_DATE)
    return DATE_PRESENT;

  // Write as CCYYMMDD . Fraction of Day
  return (10000L*_Year(d_date)) + (100L*_Month(d_date)) + _Day(d_date) +
	(double)(GetTimeSeconds())/ (double)(24*60*60L);
}


DOUBLE SRCH_DATE::GetWholeDayValue() const
{

  enum Date_Precision d_prec = GetPrecision();
  if (d_prec == UNKNOWN_DATE)
    return DATE_UNKNOWN;
  else if (d_prec == BAD_DATE)
    return DATE_ERROR;
  else if (d_prec == CURRENT_DATE)
    {
      SRCH_DATE date;
      date.SetNow();
      return date.GetWholeDayValue();
    }

  // Write as CCYYMMDD . Fraction of Day
  return (10000L*_Year(d_date)) + (100L*_Month(d_date)) + _Day(d_date);
}

SRCH_DATE::operator long () const
{
  enum Date_Precision d_prec = GetPrecision();
  if (d_prec == UNKNOWN_DATE)
    return DATE_UNKNOWN;
  if (d_prec == BAD_DATE)
    return DATE_ERROR;
  if (d_prec == CURRENT_DATE)
    return DATE_PRESENT;
  return d_date;
}

SRCH_DATE::operator STRING () const
{
  return STRING(ISOdate());
}

// Year of date CCYY
int SRCH_DATE::Year() const
{
  return _Year(d_date);
}

// Month of date MM (1-12)
int SRCH_DATE::Month() const
{
  return _Month(d_date);
}

int SRCH_DATE::DayOfWeek() const
{
  return _Wday(d_date) + 1;
}

int SRCH_DATE::DayOfYear() const
{
  SRCH_DATE temp;
  // 1 Jan of the same year...
  temp.SetDate(_Year(d_date), 1, 1);

  return GetJulianDate() - temp.GetJulianDate()+1;
}

bool SRCH_DATE::SetDayOfYear(int Year, int Day)
{
  SRCH_DATE temp;
  temp.SetDate(Year, 1, 1);
  const long julian = Day + temp.GetJulianDate() - 1;
  d_date = _julian_to_date( julian);
  SetPrecision();
  return true;
}


bool SRCH_DATE::SetDayOfYear(int Day)
{
  return SetDayOfYear(_Year(d_date), Day);
}

int SRCH_DATE::GetFirstDayOfMonth() const
{
  SRCH_DATE temp;
  // 1st of the current month and year
  temp.SetDate(_Year(d_date), _Month(d_date), 1);
  return temp.DayOfWeek();
}

int SRCH_DATE::GetWeekOfMonth() const
{
  // Abs day includes the days from previous month that fills up
  // the begin. of the week.
  int nAbsDay = _Day(d_date) + GetFirstDayOfMonth()-1;
  return (nAbsDay-DayOfWeek())/7 + 1;
}

int SRCH_DATE::WeekOfYear() const
{
  return (DayOfYear()/7) + 1;
}

bool SRCH_DATE::SetWeekOfYear(int Week, int Day)
{
  return SetWeekOfYear(_Year(d_date), Week, Day);
}

bool SRCH_DATE::SetWeekOfYear(int Year, int Week, int Day)
{
  if (Week > 53 || Week < 1)
    return false;
  const int day = (Week - 1)*7;
  SRCH_DATE date;
  date.SetDayOfYear(Year, day);
  int diff = 1 - date.DayOfWeek() + (Day%8);
  SetDayOfYear(Year, day + diff);
  return true;
}


// Day of date YY (1-31)
int SRCH_DATE::Day() const
{
  return _Day(d_date);
}

// Seconds of Time since Midnight
long SRCH_DATE::GetTimeSeconds() const
{
#if 0
  long rest = GetTime();
  // HHMMSSX  where X is have time or not..
  const int hour  = (int)(rest/10000L) % 100;
  const int min   = (int)(rest/100L) % 100;
  const int sec   = (int)(rest % 100);

  return hour*3600L + min*60L + sec;
#else
  return _Hour(d_rest)*3600L + _Minutes(d_rest)*60L + _Seconds(d_rest);
#endif
}


SRCH_DATE& SRCH_DATE::operator=(const SRCH_DATE& OtherDate)
{
  Set(OtherDate);
  return *this;
}

long SRCH_DATE::DaysDifference(const SRCH_DATE& Other) const
{
  const long long julian         = _to_julian( d_date );
  const long long other_julian   = _to_julian( Other.d_date );
  return julian-other_julian ;
}


bool SRCH_DATE::Set(const SRCH_DATE& OtherDate)
{
  d_date = OtherDate.d_date;
  d_rest = OtherDate.d_rest;
  return IsValidDate();
}


bool SRCH_DATE::Set(const long LongVal)
{
  if (LongVal == -1)
    {
      d_date = 0;
      SetPrecision(UNKNOWN_DATE); // Infinite Date
      return false;;
    }
  d_date = LongVal;
  SetPrecision();
  return true;
}

bool SRCH_DATE::Set(const DOUBLE FloatVal)
{
  if (FloatVal == DATE_PRESENT)
    {
      SetNow();
      return true;
    }
  else if (FloatVal == DATE_ERROR || FloatVal == UNKNOWN_DATE)
    {
      d_date = (UINT4)FloatVal;
      SetPrecision();
      return false;
    }

  // Passed as CCYYMMDD
  const int year  = (int)(FloatVal/10000);
  const int month = ((int)(FloatVal/100)) % 100;
  const int day   = ((int)FloatVal) % 100;

  d_date = year*YEAR_FACTOR + month*MONTH_FACTOR + day*DAY_FACTOR;
  d_rest = SetTime((long)FloatVal - FloatVal);
  SetPrecision();
  return IsValidDate();
}

bool SRCH_DATE::Set(const CHR *CStringVal)
{
  bool res;
  if ((res = date_parse(CStringVal)) == false)
    {
      d_date = 0;
      d_rest = 0;
      SetPrecision(UNKNOWN_DATE);
    }
  return res;
}

bool SRCH_DATE::Set(const STRING& str)
{
  return Set((const char *)str);
}

#ifdef WIN32
bool SRCH_DATE::Set(const SYSTEMTIME *sTime)
{
/*
{
  WORD wYear;
  WORD wMonth;
  WORD wDayOfWeek;
  WORD wDay;
  WORD wHour;
  WORD wMinute;
  WORD wSecond;
  WORD wMilliseconds;
} SYSTEMTIME, 
*/
  if (sTime == NULL)
    {
      SetNow ();
      return true;
    }
  return Set(sTime->wYear, sTime->wMonth, sTime->wDay, sTime->wHour, sTime->wMinute, sTime->wSecond);
}
#endif

bool SRCH_DATE::SetYear(int nYear)
{
  int month= _Month(d_date);
  int day  = _Day(d_date);
        
  d_date = nYear*YEAR_FACTOR + month*MONTH_FACTOR + day*DAY_FACTOR;
  return true;
}

bool SRCH_DATE::SetMonth(int nMonth)
{
  if (nMonth < 13)
    {
      int year = _Year(d_date);
      int day  = _Day(d_date);
     
      d_date = year*YEAR_FACTOR + nMonth*MONTH_FACTOR + day*DAY_FACTOR;
      if (day)         SetPrecision(DAY_PREC);
      else if (nMonth) SetPrecision(MONTH_PREC);
      else             SetPrecision(YEAR_PREC);
      return true;
    }
  return false;
}

bool SRCH_DATE::SetDay(int nDay)
{
  if (nDay < 32)
    {
      int year   = _Year(d_date);
      int month  = _Day(d_date);
        
      d_date = year*YEAR_FACTOR + month*MONTH_FACTOR + nDay*DAY_FACTOR;
      if (nDay)       SetPrecision(DAY_PREC);
      else if (month) SetPrecision(MONTH_PREC);
      else            SetPrecision(YEAR_PREC);
      return true;
    }
  return false;
}


bool SRCH_DATE::Set(int year, int mon, int day, int hour, int min, int sec)
{
  bool result = SetDate(year, mon, day);

  SetTime(hour, min, sec);
  return result;
}

bool SRCH_DATE::SetTime(int hour, int min, int sec)
{
  if (hour >= 0 && hour < 24 && min >=0 && min < 60 && sec >=0 && sec < 60)
    {
      SetTime( (UINT4)(hour*HOUR_FACTOR + min*MIN_FACTOR + sec*SEC_FACTOR));
      return true;
    }
  return false;
}

bool SRCH_DATE::SetTime(DOUBLE Fraction)
{
  const long t = (long)(24*60*60*((long)Fraction-Fraction)+0.5);

  SetTime( (UINT4)( ((t / 3600 ) % 24 )*HOUR_FACTOR + ((t / 60) % 60 )*MIN_FACTOR +
	( t % 60 )*SEC_FACTOR) );
  return true;
}

bool SRCH_DATE::SetDate(int year, int month, int day)
{
  if (month < 13 && day < 32)
    {
      d_date = year*YEAR_FACTOR + month*MONTH_FACTOR + day*DAY_FACTOR;
      if (day)         SetPrecision(DAY_PREC);
      else if (month)  SetPrecision(MONTH_PREC);
      else if (year)   SetPrecision(YEAR_PREC);
      return true;
    }
  return false;
}

/*
 * tm2sec converts a GMT tm structure into the number of seconds since
 * 1st January 1970 UT.  Note that we ignore tm_wday, tm_yday, and tm_dst.
 * 
 * The return value is always a valid time_t value -- (time_t)0 is returned
 * if the input date is outside that capable of being represented by time(),
 * i.e., before Thu, 01 Jan 1970 00:00:00 for all systems and 
 * beyond 2038 for 32bit systems.
 *
 * This routine is intended to be very fast, much faster than mktime().
 */
static time_t tm2sec(const struct tm * t)
{
    int year;
    time_t days;
    static const int dayoffset[12] =
    {306, 337, 0, 31, 61, 92, 122, 153, 184, 214, 245, 275};

    year = t->tm_year;

    if (year < 70 || ((sizeof(time_t) <= 4) && (year >= 138)))
	return BAD_DATE;

    /* shift new year to 1st March in order to make leap year calc easy */

    if (t->tm_mon < 2)
	year--;

    /* Find number of days since 1st March 1900 (in the Gregorian calendar). */

    days = year * 365 + year / 4 - year / 100 + (year / 100 + 3) / 4;
    days += dayoffset[t->tm_mon] + t->tm_mday - 1;
    days -= 25508;		/* 1 jan 1970 is 25508 days since 1 mar 1900 */

    days = ((days * 24 + t->tm_hour) * 60 + t->tm_min) * 60 + t->tm_sec;

    if (days < 0)
	return BAD_DATE;	/* must have overflowed */
    else
	return days;		/* must be a valid time */
}



bool SRCH_DATE::Set(struct tm *time_str)
{
  if (time_str)
    {
      if (mktime(time_str) != -1)
	{
	  d_date = Normalize_tm_year(time_str->tm_year)*YEAR_FACTOR +
		(time_str->tm_mon+1)*MONTH_FACTOR +
		(time_str->tm_mday)*DAY_FACTOR;
	  d_rest = (time_str->tm_hour)*HOUR_FACTOR +
		(time_str->tm_min)*MIN_FACTOR +
		(time_str->tm_sec)*SEC_FACTOR;
	  SetPrecision();
	  return true;
	}
    }
  return false;
}

bool SRCH_DATE::SetTimeOfFile(int fd)
{
  struct stat stbuf;

  if (fstat(fd, &stbuf) != -1)
    {
      time_t mtime  = stbuf.st_mtime;
      return Set(&mtime);
    }
  return false;
}

bool SRCH_DATE::SetTimeOfFile(FILE *fp)
{
  return fp ? SetTimeOfFile(fileno(fp)) : false;
}

bool SRCH_DATE::SetTimeOfFile(const STRING& Pathname)
{
  struct stat stbuf;

  if (lstat(Pathname.c_str(), &stbuf) != -1)
    {
      time_t mtime  = stbuf.st_mtime;
#ifndef _WIN32
      if (stbuf.st_nlink > 1)
	{
	  // File is multiple linked. Need to see which is newer.
	  if (stat(Pathname.c_str(), &stbuf) != -1)
	    {
	      if (mtime < stbuf.st_mtime)
		mtime = stbuf.st_mtime;
	    }
	}
#endif
      return Set(&mtime);
    }
  return false;
}


bool SRCH_DATE::SetTimeOfFileCreation(int fd)
{
  struct stat stbuf;
  if (fstat(fd, &stbuf) != -1)
    return Set(&stbuf.st_ctime);
  return false;
}

bool SRCH_DATE::SetTimeOfFileCreation(FILE *fp)
{
  return fp ? SetTimeOfFileCreation(fileno(fp)) : false;
}

bool SRCH_DATE::SetTimeOfFileCreation(const STRING& Pathname)
{
  struct stat stbuf;

  if (lstat(Pathname.c_str(), &stbuf) != -1)
    {
      time_t ctime  = stbuf.st_ctime;
#ifndef _WIN32
      if (stbuf.st_nlink > 1) 
        { 
          // File is multiple linked. Need to see which is older.
          if (stat(Pathname.c_str(), &stbuf) != -1)
            {
              if (ctime > stbuf.st_mtime)
                ctime = stbuf.st_mtime;
            }
        }
#endif
       Set(&ctime);
    }

  return false;
}


bool SRCH_DATE::Set(const time_t *time)
{
  d_date = DATE_UNKNOWN;

  SetPrecision(UNKNOWN_DATE);

  if (time == NULL)
    {
      SetNow();
    }
  else if (*time != -1)
    {
      struct tm *tm = localtime (time);
      return Set(tm);
    }
  return true;
}

time_t SRCH_DATE::MkTime() const
{
  struct tm tm;
  return MkTime(&tm, *this);
}

time_t SRCH_DATE::MkTime(const SRCH_DATE& time) const
{
  struct tm tm;
  return MkTime(&tm, time);
}

time_t SRCH_DATE::MkTime(struct tm *time_str, const SRCH_DATE& time) const
{
  if (time_str == NULL)
    return MkTime(time);
  if (ParseTm(time_str, time))
    return tm2sec(time_str);
  return (time_t)-1;
}

bool SRCH_DATE::ParseTm(struct tm *time_str) const
{
  return ParseTm(time_str, *this, false);
}

bool SRCH_DATE::ParseTm(struct tm *time_str, const SRCH_DATE& time,
	bool Validate) const
{
  if (!time.Ok())
    return false; // ERROR

  int Year  = _Year( time.d_date );
  int Month = _Month( time.d_date );
  int Day   = _Day( time.d_date );
  int Wday  = _Wday(time.d_date );

  // No month? Then Jan
  if (Month <= 0)
    {
      Month = 1;
      // No Day set as well? Then 1 Jan
      if (Day == 0)
	Day = 1;
    }
  // No Day or out of range? Last day of month
  if ((Day <= 0) || (Day > (mntb[Month-1] + ( ((Month==2) && _isLeapYear(Year)) ? 1 : 0))))
    {
      if (Validate)
	return false;
      Day = (mntb[Month-1] + (Month==2 && _isLeapYear(Year))); 
    }
  if (time_str)
    {
      time_str->tm_year       = Year - TM_YEAR_BASE; // Years since 1900
      time_str->tm_mon        = (Month - 1) % 12;
      time_str->tm_mday       = Day; 
      time_str->tm_wday       = Wday;

      // Handle Time
      const long rest = time.GetTime();
      time_str->tm_hour       = _Hour(rest);
      time_str->tm_min        = _Minutes(rest);
      time_str->tm_sec        = _Seconds(rest);
      time_str->tm_isdst      = -1;
    }
  return true;
}

time_t SRCH_DATE::MkTime(struct tm *time_str) const
{
  if (time_str == NULL)
    return MkTime();
  return MkTime(time_str, *this);
}


// ISO date format
char * SRCH_DATE::ISOdate(char *buf, size_t maxsize) const
{
  char tmp[25];

  // CCYYMMDD.HHMMSS
  // CCYY-MM-DDTHH:MM:SS[Z] (eg.  1996-07-16T13:19:51Z)

  if (GetPrecision() == BAD_DATE)
    {
      tmp[0] = '\0';
    }
  else
    {
      const long rest  = GetTime();
      const int  year  = _Year(d_date);
      const int  mon   = _Month(d_date);
      const int  mday  = _Day(d_date);

      if (mday == 0)
	{
	  if (mon == 0)
	    sprintf(tmp, "%04d", year);
	  else
	    sprintf(tmp, "%04d%02d", year, mon);
	}
      else if (rest == 0)
	{
	  sprintf(tmp, "%04d%02d%02d", year, mon, mday);
	}
      else
	{
	  int hour = _Hour(rest);
	  int min  = _Minutes(rest);
	  int sec  = _Seconds(rest); 
	  sprintf(tmp, "%04d%02d%02dT%02d:%02d:%02dZ",
		year, mon, mday, hour, min, sec);
	}
    }

  if (buf == NULL)
    {
      buf = Copystring(tmp);
    }
  else
    {
      strncpy(buf, tmp, maxsize);
      buf[maxsize] = '\0';
    }
  return buf;
}

STRING SRCH_DATE::ISOdate() const
{
  char buf[25];
  return STRING (ISOdate(buf, sizeof(buf)-1));
}

bool SRCH_DATE::ISO(PSTRING StringBuffer) const
{
  *StringBuffer = ISOdate();
  return StringBuffer->IsEmpty();
}

char * SRCH_DATE::ANSIdate(char *buf, size_t maxlen) const
{
  char tmp[27];
  struct tm tm;

  if (ParseTm(&tm))
    {
      // CCYYMMDD HH:MM GMT
      sprintf(tmp, "%04d%02d%02d",
	tm.tm_year + TM_YEAR_BASE, tm.tm_mon + 1, tm.tm_mday);
      if (tm.tm_hour || tm.tm_min || tm.tm_sec)
	{
	  sprintf(tmp+8, " %02d:%02d", tm.tm_hour, tm.tm_min);
	  if (tm.tm_sec)
	    sprintf(tmp+14, "%02d", tm.tm_sec);
	  strcat(tmp, " GMT");
	}
    }
  else
    tmp[0] = '\0';

  if (buf == NULL)
    {
      buf = Copystring(tmp);
    }
  else
    {
      strncpy(buf, tmp, maxlen);
      buf[maxlen] = '\0';
    }
  return buf;
}

STRING SRCH_DATE::ANSIdate() const
{
  char buf[27];

  return STRING(ANSIdate(buf, sizeof(buf)-1));
}

bool SRCH_DATE::ANSI(PSTRING StringBuffer) const
{
  *StringBuffer = ANSIdate();
  return !StringBuffer->IsEmpty();
}

// Locale date format
char *SRCH_DATE::LCdate(char *buf, size_t maxsize) const
{
  struct tm tm;
  char tmp[60];

  if (ParseTm(&tm))
    {
      // 31. Oktober 1997, 18:56:47 Uhr GMT
      if (GetTime() == 0)
	::strftime (tmp, maxsize, "%d %h %Y",  &tm);
      else
	::strftime (tmp, maxsize, "%d %h %Y %X UTC", &tm);
    }
  else
    tmp[0] = '\0';

  if (buf == NULL)
    {
      buf = Copystring(tmp);
    }
  else
    {
      strncpy(buf, tmp, maxsize);
      buf[maxsize] = '\0';
    }

  return buf;
}

STRING SRCH_DATE::LCdate() const
{
  char buf[60];

  return STRING(LCdate(buf, sizeof(buf)-1));
}

// Locale date format
bool SRCH_DATE::Locale(PSTRING StringBuffer) const
{
  *StringBuffer = LCdate();
  return !StringBuffer->IsEmpty();
}


// RFC date format
char * SRCH_DATE::RFCdate(char *buf, size_t maxlen) const
{
  // Need US names */
  const char *days[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  const char *months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  struct tm tm;
  char tmp[60];

  if (ParseTm(&tm))
    {
      if ( tm.tm_hour || tm.tm_min || tm.tm_sec )
	{
	  sprintf(tmp, "%s, %02d %s %04d %02d:%02d:%02d GMT",
		days[tm.tm_wday],
		tm.tm_mday,
		months[tm.tm_mon],
		tm.tm_year + TM_YEAR_BASE,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	}
      else
	{
	  sprintf(tmp, "%s, %02d %s %04d",
		days[tm.tm_wday],
		tm.tm_mday,
		months[tm.tm_mon],
		tm.tm_year + TM_YEAR_BASE);
	}
    }
   else
    tmp[0] = '\0'; // Error
  if (buf == NULL)
    {
      if (*tmp)
	buf = Copystring(tmp); 
    }
  else
    {
      strncpy(buf, tmp, maxlen);
      buf[maxlen] = '\0';
    }
  return buf;
}

STRING SRCH_DATE::RFCdate() const
{
  char buf[60];
  return STRING(RFCdate(buf, sizeof(buf)-1));
}

bool SRCH_DATE::RFC(PSTRING StringBuffer) const
{
  *StringBuffer = RFCdate();
  return !StringBuffer->IsEmpty();
}


bool SRCH_DATE::Strftime(const char *format, PSTRING StringBuffer) const
{
  char buf[BUFSIZ];

  struct tm tm;
  if (ParseTm(&tm) == false)
    {
      *StringBuffer = "";
      return false;
    }
  ::strftime (buf, sizeof (buf), format,  &tm);
  *StringBuffer = buf;
  return true;
}

ostream &operator << (ostream &os, const SRCH_DATE &dt)
{
  char buf[25];

  return os << dt.ISOdate(buf, sizeof(buf)-1);
}

STRING &operator << (STRING &String, const SRCH_DATE &dt)
{
  char buf[25];

  return String << dt.ISOdate(buf, sizeof(buf)-1);
}


// Precision conversion routines
bool SRCH_DATE::TrimToMonth()
{
  bool status=true;

  switch (GetPrecision()) {

  case YEAR_PREC:
    // Can't convert from lower precision
    status = false;
    break;

  case MONTH_PREC:
    // Already have the right precision
    break;

  case DAY_PREC:
    // Lop off the last two digits & reset precision
    d_date = _ToMonthPrecision(d_date);
    d_rest = 0;
    SetPrecision(MONTH_PREC);
    break;

  default:
    // Precision was probably unknown
    status = false;
    break;
  }

  return(status);
}


bool SRCH_DATE::TrimToYear()
{
  bool status=true;

  switch (GetPrecision()) {

  case YEAR_PREC:
    // Already have the right precision
    break;

  case DAY_PREC:
  case MONTH_PREC:
    d_date = _ToYearPrecision(d_date);
    d_rest = 0;
    SetPrecision(YEAR_PREC);
    break;

  default:
    // Precision was probably unknown
    status = false;
    break;
  }

  return(status);
}

bool SRCH_DATE::SetToYearStart()
{
  // To 1 Jan
  const int  year  = _Year(d_date);
  d_date = 1*DAY_FACTOR + 1*MONTH_FACTOR + year*YEAR_FACTOR;
  SetPrecision(  DAY_PREC );
  return true;
}

bool SRCH_DATE::SetToYearEnd()
{
  // To 31 Dec
  const int  year  = _Year(d_date);

  d_date = 31*DAY_FACTOR + 12*MONTH_FACTOR + year*YEAR_FACTOR;
  SetPrecision(  DAY_PREC );
  return true;
}

bool SRCH_DATE::SetToMonthStart()
{
  // To 1 XXX
  const int  year  = _Year(d_date);
  const int  mon   = _Month(d_date);

  if (mon == 0)
    return false;
  d_date = 1*DAY_FACTOR + mon*MONTH_FACTOR + year*YEAR_FACTOR;
  SetPrecision(  DAY_PREC );
  return true;
}

bool SRCH_DATE::SetToMonthEnd()
{
  // To ? XXX (depends on month)
  const int  year  = _Year(d_date);
  const int  mon   = _Month(d_date);

  if (mon == 0)
    return false;
  d_date = GetDaysInMonth()*DAY_FACTOR + mon*MONTH_FACTOR + year*YEAR_FACTOR;
  SetPrecision(  DAY_PREC );
  return true;
}

bool SRCH_DATE::SetToDayStart()
{
  // To 00:00:00
  const int  day = _Day(d_date);
  if (day == 0)
    return false;
  SetTime(0,0,0);
  return true;
}

bool SRCH_DATE::SetToDayEnd()
{
  // To 23:59:59
  const int  day = _Day(d_date);
  if (day == 0)
    return false;
  SetTime(23, 25, 59);
  return true;
}

bool SRCH_DATE::PromoteToMonthStart()
{
  bool status=true;

  switch (GetPrecision()) {

  case YEAR_PREC:
    // Convert CCYY to 01CCYY & reset precision
    d_date += 1*MONTH_FACTOR;
    SetPrecision( MONTH_PREC );
    break;

  case MONTH_PREC:
    // Already have the right precision
    break;

  case DAY_PREC:
    // Can't promote from higher precision
    status = false;
    break;

  default:
    status = false;
    break;
  }

  return(status);
}

bool SRCH_DATE::IsLeapYear() const
{
  const int year = _Year(d_date);
  return  ( (year >= 1582) ?
	((year % 4 == 0  &&  year % 100 != 0)  ||  year % 400 == 0 ):
	(year % 4 == 0) );
}

int SRCH_DATE::GetDaysInMonth() const
{
  int month = _Month(d_date);
  return month > 0 ?  mntb[month-1] + (month==2 && IsLeapYear()) : 0;
}

bool SRCH_DATE::PromoteToMonthEnd()
{
  bool status=true;

  switch ( GetPrecision() ) {

  case YEAR_PREC:
    // Convert CCYY to 12CCYY & reset precision
    d_date += 12*MONTH_FACTOR;
    SetPrecision( MONTH_PREC );
    break;

  case MONTH_PREC:
    // Already have the right precision
    break;

  case DAY_PREC:
    // Can't promote from higher precision
    status = false;
    break;

  default:
    status = false;
    break;
  }

  return(status);
}


bool SRCH_DATE::PromoteToDayStart()
{
  bool status=true;

  switch ( GetPrecision() ) {

  case YEAR_PREC:
    // Convert CCYY to 0101CCYY & reset precision
    d_date += 1*MONTH_FACTOR + 1*DAY_FACTOR; 
    SetPrecision( DAY_PREC );
    break;

  case MONTH_PREC:
    // Convert MMCCYY to 01MMCCYY & reset precision
    d_date += 1*DAY_FACTOR;
    SetPrecision( DAY_PREC );
    break;

  case DAY_PREC:
    // Already have the right precision
    break;

  default:
    status = false;
    break;
  }

  return(status);
}


bool SRCH_DATE::PromoteToDayEnd()
{
  bool status=true;

  switch ( GetPrecision() ) {

  case YEAR_PREC:
    // Convert CCYY to 3112CCYY & reset precision
    d_date += 12*MONTH_FACTOR + 31*DAY_FACTOR;
    SetPrecision( DAY_PREC );
    break;

  case MONTH_PREC:
    // Convert MMCCYY to DDMMCCYY & reset precision
    d_date += GetDaysInMonth()*DAY_FACTOR; // days in month
    SetPrecision( DAY_PREC );
    break;

  case DAY_PREC:
    // Already have the right precision
    break;

  default:
    status = false;
    break;
  }

  return(status);
}


// Boolean precision routines
bool SRCH_DATE::IsYearDate() const
{
  if (GetPrecision() >= YEAR_PREC)
    return(true);
  return(false);
}


bool SRCH_DATE::IsMonthDate() const
{
  if (GetPrecision() >= MONTH_PREC)
    return(true);
  return(false);
}


bool SRCH_DATE::IsDayDate() const
{
  if (GetPrecision() >= DAY_PREC)
    return(true);
  return(false);
}

bool SRCH_DATE::IsValidDate() const
{
  int d_prec = GetPrecision();
  if (d_prec == BAD_DATE || d_prec == UNKNOWN_DATE)
    return(false);
  return(true);
}

bool SRCH_DATE::IsBogusDate() const
{
  if (GetPrecision() == BAD_DATE)
    return true;
  if (ParseTm(NULL, *this, true))
    return false;
  return true;
}

void SRCH_DATE::SetNow()
{
  time_t today = time((time_t *)NULL);
  struct tm *tm = gmtime (&today);

  d_date = (tm->tm_mday)*DAY_FACTOR + (tm->tm_mon + 1)*MONTH_FACTOR
	+ Normalize_tm_year(tm->tm_year)*YEAR_FACTOR;
  d_rest = (tm->tm_hour)*HOUR_FACTOR
	+ (tm->tm_min)*MIN_FACTOR
	+ (tm->tm_sec)*SEC_FACTOR;
  if (d_rest == 0) d_rest = 1*SEC_FACTOR; // We don't want 00:00:00
  SetPrecision( DAY_PREC );
}


//const size_t MAX_DATESTR_LEN = 9;
/// Fills this object with the current date
void SRCH_DATE::GetTodaysDate()
{
  time_t today = time((time_t *)NULL);
  struct tm *tm = localtime (&today);

  d_date = (tm->tm_mday)*DAY_FACTOR
        + (tm->tm_mon + 1)*MONTH_FACTOR
        + Normalize_tm_year(tm->tm_year)*YEAR_FACTOR;
  SetPrecision(DAY_PREC);
}

bool SRCH_DATE::IsBefore(const SRCH_DATE& OtherDate) const
{
  return (DateCompare(OtherDate) == BEFORE);
}


bool SRCH_DATE::Equals(const SRCH_DATE& OtherDate) const
{
  return (DateCompare(OtherDate) == DURING_EQUALS);
}


// During the same day..
bool SRCH_DATE::IsDuring(const SRCH_DATE& OtherDate) const
{
  SRCH_DATE date1 (*this);
  SRCH_DATE date2 (OtherDate);

  date1.SetTime((UINT4)0);
  date2.SetTime((UINT4)0);

  return (date1.DateCompare(date2) == DURING_EQUALS);
}


bool SRCH_DATE::IsAfter(const SRCH_DATE& OtherDate) const
{
  return (DateCompare(OtherDate) == AFTER);
}


// private
void SRCH_DATE::SetPrecision()
{
//cerr << "SetPrecision, d_date = " << d_date << endl;
  if (d_date == DATE_ERROR)
    SetPrecision( BAD_DATE );
  else if (d_date == DATE_UNKNOWN)
    {
      SetPrecision( UNKNOWN_DATE );
    }
  else if ((d_date > YEAR_LOWER) && (d_date < YEAR_UPPER))
    {
      SetPrecision( YEAR_PREC );
    }
  else if ((d_date > MONTH_LOWER) && (d_date < MONTH_UPPER))
    {
      SetPrecision( MONTH_PREC );
    }
  else if ((d_date > DAY_LOWER) && (d_date < DAY_UPPER))
    SetPrecision( DAY_PREC );
  else
    SetPrecision( BAD_DATE );
  return;
}

INT Compare(const SRCH_DATE& Date1, const SRCH_DATE& Date2)
{
  switch (Date1.DateCompare(Date2))
    {
      case BEFORE:
      case BEFORE_DURING:
        return -1;
      case DURING_EQUALS:
	return 0;
      case DURING_AFTER:
      case AFTER:
        return 1;
      case MATCH_ERROR:
	return 0; 
    }
  // NOT REACHED
  DOUBLE diff = Date1.GetValue() - Date2.GetValue();
  if (diff > 0)
    return 1;
  else if (diff < 0)
    return -1;
  return 0;
}


Date_Match SRCH_DATE::DateCompare(const SRCH_DATE& OtherDate) const
{
  if (d_date == OtherDate.d_date && d_rest == OtherDate.d_rest)
    return DURING_EQUALS;

  Date_Precision p1 = GetPrecision();
  Date_Precision p2 = OtherDate.GetPrecision();

  if ((p1 == BAD_DATE) || (p2 == BAD_DATE))
    return MATCH_ERROR;

  SRCH_DATE D1 (*this);
  SRCH_DATE D2 (OtherDate);

  if (p1 > p2) {         // Trim OtherDate to match this object
    switch(p2)
      {
	case YEAR_PREC:
	  DP("Trim to Year");
	  D1.TrimToYear();
	  break;
	case MONTH_PREC:
	  DP("Trim to month");
	  D1.TrimToMonth();
	  break;
	default:
	  DP("Precision was " << p2);
	  break;
      }
  } else if (p1 < p2) {    // Trim this object to match OtherDate
    switch(p1)
      {
	case YEAR_PREC:
	  DP("Trim to year");
	  D2.TrimToYear();  break;
	case MONTH_PREC:
	  DP("Trim to month");
	  D2.TrimToMonth(); break;
	default: break;
      }
  }

  long rest1, rest2;

  // Trim time
  if ((rest1 = D1.GetTime()) == 0)
    {
      D2.SetTime(0,0,0);
    }
  else if ((rest2 = D2.GetTime()) == 0)
    {
      D1.SetTime(0,0,0);
    }
  else
    {
      // Set Hour/Min/Sec prevision
      int hour1  = _Hour(rest1);
      int min1   = _Minutes(rest1);
      int sec1   = _Seconds(rest1);
      int hour2  = _Hour(rest2);
      int min2   = _Minutes(rest2);
      int sec2   = _Seconds(rest2);

      if (sec1 == 0 || sec2 == 0) sec1=sec2=0;
      if (min1 == 0 || min2 == 0) min1=min2=0;
      if (hour1 == 0 || hour2 == 0) hour1 = hour2 = 0;

      if (hour1 == 0 || min1 == 0 || sec1 == 0)
	D1.SetTime(hour1, min1, sec1);
      if (hour2 == 0 || min2 == 0 || sec2 == 0)
	D2.SetTime(hour2, min2, sec2);
    }

  const DOUBLE diff = D1.GetValue() - D2.GetValue();

  DP("XXXX DIFF= " << diff);

#if DEBUG
cerr << "DateCompare " << D1.RFCdate() << " and " << D2.RFCdate() << "  = " << diff;
if (diff > 0.0) cerr << " AFTER" << endl;
else if (diff < 0.0) cerr << "BEFORE" << endl;
else cerr << " EQUALS" << endl;
#endif

  // Same precision - we can just compare
  if      (diff > 0.0 ) return AFTER;
  else if (diff < 0.0 ) return BEFORE;
  return DURING_EQUALS;
}

int operator <  (const SRCH_DATE &dt1, const SRCH_DATE &dt2)
{
  switch (dt1.DateCompare(dt2))
    {
      case BEFORE:
      case BEFORE_DURING:
	return 1; // true;
      case DURING_EQUALS:
      case DURING_AFTER:
      case AFTER:
      default:
	return 0; // false;
    }
  return dt1.d_date < dt2.d_date; // NOT REACHED! 
}

int operator <= (const SRCH_DATE &dt1, const SRCH_DATE &dt2)
{
  return (dt1.DateCompare(dt2) != AFTER);
}

int operator >  (const SRCH_DATE &dt1, const SRCH_DATE &dt2)
{
  switch (dt1.DateCompare(dt2))
    {
      case BEFORE:
      case BEFORE_DURING:
      case DURING_EQUALS:
      default:
	return 0; // false;
      case DURING_AFTER:
      case AFTER:
	return 1; // false;
    }
  // NOT REACHED
}

int operator >= (const SRCH_DATE &dt1, const SRCH_DATE &dt2)
{
  return (dt1.DateCompare(dt2) != BEFORE);
}

int operator == (const SRCH_DATE &dt1, const SRCH_DATE &dt2)
{
  return dt1.Equals(dt2); 
}

int operator != (const SRCH_DATE &dt1, const SRCH_DATE &dt2)
{
  return !dt1.Equals(dt2);
}


////////////////////// Date Range Class Methods ////////////////
DATERANGE::DATERANGE()
{
}


DATERANGE::DATERANGE(const SRCH_DATE& NewStart, const SRCH_DATE& NewEnd)
{
  d_start = NewStart;
  d_end   = NewEnd;
}


DATERANGE::DATERANGE(const SRCH_DATE& NewDate)
{
  // Put same date in start and end
  d_start = NewDate;
  d_end   = NewDate;
}

// The one exception to the rule..
DATERANGE::DATERANGE(time_t *NewStart, time_t *NewEnd)
{
  d_start.Set(NewStart);
  d_end.Set(NewEnd);
}

static const char iso8601_range_sep[] = "/";


DATERANGE::DATERANGE(const STRING& DateString)
{
  // String should be "CCYY[MM[DD]]/CCYY[MM[DD]]"
  STRING Single (DateString);
  STRINGINDEX seperator;

#ifdef DEBUG
  cerr << "DATERANGE(" << DateString << ")" << endl;
#endif

  // Look at the last character to see if s as in 1950's or 1950s
  if (DateString.Last() == 's' || DateString.Search(' ') == 0)
    {
       // OK looks good
       STRING s (DateString);
       s.EraseAfter(s.Len() - 1);
       if (s.Last() == '\'') s.EraseAfter(s.Len() - 1);
       if (s.IsNumber())
	{
	  char tmp[30];
	  int value = (int)s;
	  int end   = value;
	  if (value >= 100 && (value == 100*(value/100)))
	    {
	       // We have Century
	       end = value + 99;
	    }
	  else if (value >= 0 && (value == 10*(value/10)))
	    {
	       // We have a Decade
	       if (value <= 90) value += 1900;
	       end = value + 9;
	    }
	  // else,, an error 1911's or 1911s but we'll accept it means during..
	  sprintf(tmp, "%d/%d", value, end);
	  *this = DATERANGE(tmp);
	  return;
	}
    }
  if (isdigit(DateString.GetChr(1)) && (
	// Century or cent. or ...
	(seperator = DateString.SearchAny("cent")) > 2 ||
	(seperator = DateString.SearchAny("jahrhundred")) > 2 ||
	(seperator = DateString.SearchAny("jh.")) > 2 ||
	// 19ème Siècle 
	((seperator = DateString.SearchAny("Si")) > 2 && DateString.GetChr(seperator+3) == 'c') ) )
    {
      // TODO: Finish support of B.C. dates
      const int bc = ((DateString.Len() > (seperator+8) && DateString.GetChr(seperator+8) == 'b') ? -100 : 100 );
      // 19th century
      // 12th century b.c.
      const int century = (atoi(DateString.c_str()) - 1)* bc;
      const int end = (century > 0) ? century + 99 : century - 99;
      if (end >= century)
	{
	   d_start.d_date = century*YEAR_FACTOR;
	   d_end.d_date   = end*YEAR_FACTOR;
	}
      else
	{
	  d_start.d_date = end*YEAR_FACTOR;
	  d_end.d_date   = century*YEAR_FACTOR;
	}
      d_start.d_rest = 0;
      d_end.d_rest   = 0;
      d_start.SetPrecision(YEAR_PREC);
      d_end.SetPrecision(YEAR_PREC);
      return;
    }
  // YYYY
  if (DateString.GetLength() <= 4 && DateString.IsNumber())
    {
      SRCH_DATE start (DateString);
      SRCH_DATE end   (start);

      start.SetToYearStart();
      end.SetToYearEnd();
      *this = DATERANGE(start,end);
      return;
    }

  if (DateString.GetLength() < 24 && DateString.Search(' ') > 0) {
    // We have something space.. maybe 10 days? 
    int         count;
    char        token[25]; 

    if (sscanf(DateString.c_str(), "%d %20s", &count, token) == 2 && count != 0)
      {
	if (strcmp(token, "to") == 0 || strcmp(token, "through") == 0 || strcmp(token, "bis") == 0)
	  {
	    int y1 = 0;
	    int y2 = 0;
	    // YEAR to YEAR   YEAR through YEAR or YEAR bis YEAR
	    if (sscanf(DateString.c_str(), "%d %*s %d", &y1, &y2) == 2)
	      {
		char tmp[64];
		sprintf(tmp, "%d/%d", y1, y2);
		DATERANGE d = DATERANGE(tmp);
		if (d.Ok())
		  {
		    *this = d;
		    return; // Done
		  }
	      }
	  }

	SRCH_DATE date("Now"); // Set the date now
	SRCH_DATE date2 (date); // same date/time

	// Seconds, Minutes, Hours, Days, Months, Years
	if (strncasecmp(token, "Sec", 3) == 0)
	  date.PlusNseconds(count);
	else if (strncasecmp(token, "Min", 3) == 0)
	  date.PlusNminutes(count);
	else if (strncasecmp(token, "Hou", 3) == 0)
	  date.PlusNhours(count);
        else if (strncasecmp(token, "Day", 3) == 0)
	  date.PlusNdays(count);
	else if (strncasecmp(token, "Mon", 3) == 0)
	  date.PlusNmonths(count);
	else if (strncasecmp(token, "Yea", 3) == 0)
	  date.PlusNyears(count);
	if (date > date2)
	  {
	    d_end = date;
	    d_start = date2;
	    return;
	  }
	if (date < date2)
	  {
	    d_end = date2;
	    d_start = date;
	    return;
	  }
      }
  }

  if ((seperator = Single.Search(*iso8601_range_sep)) == 0)
    {
#ifdef DEBUG
      cerr << "ISO Sep not seen" << endl;
#endif
      bool irange = false; // Normally not
      CHR         ch     = Single.GetChr(1);
      if (ch == '[' || ch == '{')
	{
	  // Ranges as {x,y} or [x,y]
	  if (Single.Last() == ( ch == '[' ? ']' : '}'))
	    {
	      irange = true; 
	      Single.EraseBefore(2);
	      Single.EraseAfter(Single.GetLength() - 1);
	    }
	}
      // Space is NOT a valid range in ISO8601 but we will accept it
      if ((seperator = Single.Search(' ')) == 0)
	{
	  // , ranges
	  // - is a special character not for ranges but we will
	  // accept it as well...
	  if (irange==false || ((seperator = Single.Search(',')) == 0))
	    if ((seperator = Single.Search('-')) == 0 &&
		((irange=false) || (seperator = Single.Search(',')) == 0))
	    {
#ifdef DEBUG
	      cerr << "No seps so error" << endl;
#endif
	      return; // Error
	    }
	  // TODO: Handle Implicit ranges as defined in ISO 8601 
	}
#ifdef DEBUG
      cerr << "End = " << Single << " + " << seperator << endl;
#endif
      d_end = Single.c_str() + seperator;
      Single.EraseAfter(seperator-1);
#ifdef DEBUG
      cerr << "Start = " << Single << endl;
#endif
      d_start = Single;
      return;
    }

#ifdef DEBUG
  cerr << "Single = " << Single << "    Sep=" << seperator << endl;
#endif

  // ISO 8601   20010101/15 -> 20010101-20010115
  //  YYYY[MM[DD]]/[[YYYY]MM]DD
  STRING end_range ( Single.c_str() + seperator);

#ifdef DEBUG
  cerr << "End Range = " << end_range << endl;
#endif

  Single.EraseAfter(seperator-1);
  d_start = Single;
  switch (end_range.GetLength())
    {
      int value;

      case 1:
      case 2:
	// DD
	d_end = d_start;
	d_end.SetDay( end_range.GetInt() );
	break;
      case 3:
	// MDD
	if (end_range.GetChr(2) == '-')
	  {
	    //M-D
	    d_end = d_start;
	    d_end.SetDay  ( end_range.GetChr(3) - '0'); 
	    d_end.SetMonth( end_range.GetChr(1) - '0');
	    break;
	  }
	value = end_range.GetInt();
	d_end = d_start;
	d_end.SetDay  ( value % 100 );
	d_end.SetMonth( value/100  );
	break;
      case 4:
	if (end_range.GetChr(2) == '-')
	  {
	    // M-DD
	    int day = (end_range.GetChr(3) - '0')*10 +
		end_range.GetChr(4) - '0';
	    int month = end_range.GetChr(1) - '0';
	    d_end = d_start;
	    d_end.SetDay  ( day );
	    d_end.SetMonth( month );
	    break;
	  }
        if (end_range.GetChr(3) == '-')
          {
	    // MM-D
	    int month = (end_range.GetChr(1) - '0')*10 +
                end_range.GetChr(2) - '0';
	    int day   = end_range.GetChr(4) - '0';
	    d_end = d_start;
	    d_end.SetDay  ( day );
	    d_end.SetMonth( month );
	    break;
	  }
	value = end_range.GetInt();
	// numbers <=1232 are MMDD
	if (value <= 1231)
	  {
	    d_end = d_start;
	    d_end.SetDay( value % 100 );
	    d_end.SetMonth( value/100 );
	    break;
	  }
	d_end = end_range; 
	d_end.SetToYearEnd();
	break;
      case 5:
	if (end_range.GetChr(3) == '-')
          {
            // MM-DD
            int month = (end_range.GetChr(1) - '0')*10 +
                end_range.GetChr(2) - '0';
            int day   = (end_range.GetChr(4) - '0')*10 +
		end_range.GetChr(5) - '0';
            d_end = d_start;
            d_end.SetDay  ( day );
            d_end.SetMonth( month );
            break;
          }
	// Note the Year forms like YYYY-MM-DD are handled by
	// the normal date parser. The above are the special
	// case of ISO 8601 where the range is twiddled from
	// smallest element to large..
      default: d_end = end_range;
    }
}


DATERANGE::DATERANGE(const CHR* Cstring)
{
  *this = DATERANGE(STRING(Cstring));
}

DATERANGE::operator  STRING () const
{
  STRING range;

  range << d_start << iso8601_range_sep << d_end;
  return range;
}

ostream &operator << (ostream &os, const DATERANGE &dt)
{
  return os << dt.GetStart() << iso8601_range_sep << dt.GetEnd();
}

STRING  &operator << (STRING &String, const DATERANGE &dt)
{
  return String << dt.d_start << iso8601_range_sep << dt.d_end;
}


DATERANGE&  DATERANGE::operator  =(const DATERANGE& OtherRange)
{
  d_start = OtherRange.d_start;
  d_end   = OtherRange.d_end;
  return *this;
}

DATERANGE& DATERANGE::operator+=(const INT Days)
{
  d_start += Days;
  d_end   += Days;
  return *this;
}

DATERANGE& DATERANGE::operator+=(const DOUBLE Offset)
{
  d_start += Offset;
  d_end   += Offset;
  return *this;
}

DATERANGE& DATERANGE::operator+=(const DATERANGE& DateRange)
{
  d_start += DateRange.d_start;
  d_end   += DateRange.d_end;
  return *this;
}

DATERANGE& DATERANGE::operator-=(const INT Days)
{
  d_start -= Days;
  d_end   -= Days;
  return *this;
}

DATERANGE& DATERANGE::operator-=(const DOUBLE Offset)
{
  d_start -= Offset;
  d_end   -= Offset;
  return *this;
}

DATERANGE& DATERANGE::operator-=(const DATERANGE& DateRange)
{
  d_start += DateRange.d_start;
  d_end   += DateRange.d_end;
  return *this;
}

  // Comparison operators
bool DATERANGE::operator ==(const DATERANGE& Other) const
{
  return (d_start == Other.d_start) && (d_end == Other.d_end);
}

bool DATERANGE::operator !=(const DATERANGE& Other) const
{
  return (d_start != Other.d_start) && (d_end != Other.d_end);
}

bool DATERANGE::operator  >(const DATERANGE& Other) const
{
  return (d_start > Other.d_start) && (d_end > Other.d_end);
}

bool DATERANGE::operator  <(const DATERANGE& Other) const
{
  return (d_start < Other.d_start) && (d_end < Other.d_end);
}

bool DATERANGE::operator >=(const DATERANGE& Other) const
{
  return (d_start >= Other.d_start) && (d_end >= Other.d_end);
}

bool DATERANGE::operator <=(const DATERANGE& Other) const
{
  return (d_start <= Other.d_start) && (d_end <= Other.d_end);
}


bool DATERANGE::Ok() const
{
  return d_start.IsValidDate() && d_end.IsValidDate();
}

bool DATERANGE::Defined() const
{
  return (d_start.Ok() || d_end.Ok());
}

bool DATERANGE::Contains(const SRCH_DATE& TestDate) const
{
  bool result;
  if      (!Defined())     result = true; // Underdefined Ranges contain everything
  else if (! d_start.Ok()) result = (TestDate  <= d_end);
  else if (! d_end.Ok())   result = (TestDate  >= d_start);
  else                     result = ((TestDate >= d_start) && (TestDate <= d_end));
  return result;
}

bool DATERANGE::Contains(const DATERANGE& OtherRange) const
{
  return (Contains(OtherRange.d_start) && Contains(OtherRange.d_end));
}


void DATERANGE::Write(PFILE Fp) const
{
  putObjID(objDATERANGE, Fp);
  d_start.Write(Fp);
  d_end.Write(Fp);
}

bool DATERANGE::Read(PFILE Fp)
{
  obj_t obj = getObjID(Fp);
  if (obj != objDATERANGE)
    {
      PushBackObjID(obj, Fp);
    }
  else
    {
      d_start.Read(Fp);
      d_end.Read(Fp);
    }
  return obj == objDATERANGE;
}

bool DATERANGE::ISO(STRING *String) const
{
  STRING start, end;
  bool res1 = d_start.ISO(&start);
  bool res2 = d_end.ISO(&end);
  if (String)
    {
      String->Clear();
      if (res1)
	String->Cat(start);
      String->Cat("-");
      if (res2)
	String->Cat(end);
    }
  return res1 && res2;
}

bool DATERANGE::ISO(STRING *From, STRING *To) const
{
  bool res = true;
  if (From) res = d_start.ISO(From);
  if (To) res = res && d_end.ISO(To);
  return res;
}

bool DATERANGE::RFC(STRING *From, STRING *To) const
{
  bool res = true;
  if (From) res = d_start.RFC(From);
  if (To) res = res && d_end.RFC(To);
  return res;
}

bool DATERANGE::Locale(STRING *From, STRING *To) const
{
  bool res = true;
  if (From) res = d_start.Locale(From);
  if (To) res = res && d_end.Locale(To);
  return res;
}

bool DATERANGE::Strftime(const char *fmt, STRING *From, STRING *To) const
{
  bool res = true;
  if (From) res = d_start.Strftime(fmt, From);
  if (To) res = res && d_end.Strftime(fmt, To);
  return res;
}


DATERANGE::~DATERANGE()
{
}

#ifdef MAIN_STUB

MessageLogger _globalMessageLogger;

int main(int argc, char **argv)
{

  if (argc < 3) {
    cerr << "Usage: date1 date2" << endl;
    return -1;
  }
  DATERANGE Range (argv[1]);
  STRING s;

  if (Range.Ok())
    cerr << "Range defined: " << Range << endl;
  else cerr << "Not a RANGE: " << argv[1] << endl;


  SRCH_DATE Date (argv[1]);
  SRCH_DATE Date2 (argv[2]);


  int diff;
  const SRCH_DATE Today ("Today");

  cerr << "Today = " << Today.LCdate() << endl;

  cerr << "Day Difference between " << Date << " and " << Date2 << " is " <<
	Date.DaysDifference(Date2) << endl;

  cerr << "Minutes Difference between " << Date << " and " << Date2 << " is " <<
        Date.MinutesDiff(Date2) << endl;


  cerr << Date << " is Hours Ago = " << Date.HoursAgo() << endl;
  cerr << Date2 << " is Hours Ago = " << Date2.HoursAgo() << endl;
  cerr << "Today is " << Today.HoursAgo() << " hours ago" << endl;
  cerr << "Now is " << SRCH_DATE("Now").HoursAgo() << " hours ago" << endl;


  if (Date == Today || Date2 == Today)
    {
      if (Date == Today)
	cerr << "[1] Today is " << Date.LCdate() << endl;
      if (Date2 == Today)
	cerr << "[2] Today is " << Date2.LCdate() << endl;
      if ((diff = Compare(Date2, Date)) == 0)
	cerr << Date2.LCdate()  <<" = " << Date.LCdate() << endl;
      else if (diff < 0)
	cerr << Date2.LCdate()  <<" < " << Date.LCdate() << endl;
      else
	cerr << Date2.LCdate()  <<" > " << Date.LCdate() << endl;
    }

  if (Date.Ok())
    cerr << argv[1] << " is GOOD DATE" << endl;
  else
    cerr << argv[1] << " is BAD DATE" << endl;

  long julian = _to_julian ((long)Date);
  long d  = _julian_to_date (julian);

  cerr << "Input =\"" << argv[1] << "\" --> DATE = " << Date << " --> " << julian << "(julian)" <<
	(long)julian << endl;
  cerr << "---> to date " << d << "(" << (long)d << ") : " << (SRCH_DATE)d <<  endl;
  cerr << "LC = " << Date.LCdate() << endl;

  cerr << "RANGE(" << (Range.Ok() ? "Good" : "Bad") << ")[1] = " << Range << endl;


  if (Date.ISO(&s))
   cerr << "Date (" << argv[1] << ") = " << s << endl;
  if (Date.RFC(&s))
   cerr << "Date (RFC Format) = " << s << endl;
  cerr << "ISO Format = " << Date.ISOdate() << endl;
  if (argv[2] && Date2.Ok()) {
   cerr << "Second date = " << Date2 << endl;
   if (Date2 < Date) cerr << "Second date is before" << endl;
   if (Date2 > Date) cerr << "Second date is after" << endl;
   if (Date2 >= Date) cerr << "Second Date (" << Date2.ISOdate() << ") is after or equal " << Date.ISOdate() << endl;
   if (Date2 <= Date) cerr << "Second Date (" << Date2.ISOdate() << ") is before or equal " << Date.ISOdate() << endl;
   if (Date.Equals(Date2)) cerr << "Equals" << endl;
  } else
   cerr << "No second date specified" << endl;

  if (argv[2])
    {
      if (argv[3]) Date.SetWeekOfYear(atoi(argv[3]));
      if (Date.RFC(&s))
	cerr << "Date (RFC Format) = " << s << endl;
    }

  cerr << "TEST(2):" << endl;
  Date.SetNow();
  cerr << " SetNow -> " << Date.RFCdate() << endl;
  Date.Yesterday();
  cerr << " Yesterday = " << Date.RFCdate() << endl;
  Date.GetTodaysDate();
  cerr << " Today = " << Date.RFCdate() << endl;

  STRING S = "11 Dec 2006 00:12:00 CET";
  cerr << "TEST(3):" << endl;
  Date.Set(S);
  cerr << " Set " << S << " -> " << Date.RFCdate() << endl;
  Date.Yesterday();
  cerr << " Yesterday = " << Date.RFCdate() << endl;
  Date.PlusNhours(1);
  cerr << " +1 Hour = " << Date.RFCdate() << endl;
  int xsecs = 26012;
  Date.MinusNseconds(xsecs);
  cerr << " -" << xsecs << " seconds = " << Date.RFCdate() << endl;

  xsecs = 360;
  Date.MinusNseconds(xsecs);
  cerr << " -" << xsecs << " seconds = " << Date.RFCdate() << endl;

  return 0;

}


#endif
