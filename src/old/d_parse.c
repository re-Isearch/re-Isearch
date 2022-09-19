#pragma ident  "@(#)d_parse.c  1.4 05/24/97 12:21:31 BSN"

/* Define here the floating point format for date! */
#define DATE_FMT "%04d%02d%02d.%02d%02d%02d"

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
/* Modified by E. Zimmermann <edz@bsn.com> for use in Isearch */
/* ########################################################################

  This notice is intended as a precaution against inadvertent publication
  and does not constitute an admission or acknowledgement that publication
  has occurred or constitute a waiver of confidentiality.

  This software is the proprietary and confidential property of Basis
  Systeme netzwerk, Munich.

  Basis Systeme netzwerk, Brecherspitzstr. 8, D-81541 Munich, Germany.
  tel: +49 (89) 692 8120
  fax: +49 (89) 692 8150

   ######################################################################## */
/* Added:
   ANSI Date form: YYYYMMDD
   Obsolete ANSI Date: YYMMDD
   Euro: DD.MM.YYYY and DD.MM.YY forms
   ISO date formats:
	YYYYMMDDTHHMMSS[Z]
	YYYYMMDDTHH:MM:SS[Z]
	YYYY-MM-DDTHHMMSS[Z]
	YYYY-MM-DDTHH:MM:SS[Z] (eg.  1996-07-16T13:19:51Z)

   TODO
   ----
   Add support for:
   HH:MM ampm zone
   HH:MM:SS ampm zone
   HH:MM zone
   HH:MM:SS zone
 */

#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define SVR4
#ifdef SVR4
# ifndef SYSV
#  define SYSV
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
#define DP(str) (void) fprintf( stderr, "%s\n", str )
#else
#define DP(str) (0)
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
  int i, h, l, r;

  l = 0;
  h = n - 1;
  for (;;)
    {
      i = (h + l) / 2;
      r = strcasecmp (str, tab[i].s);
      if (r < 0)
	h = i - 1;
      else if (r > 0)
	l = i + 1;
      else
	{
	  *iP = tab[i].i;
	  return 1;
	}
      if (h < l)
	return 0;
    }
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
    {"pm", AMPM_PM},
  };
  static int sorted = 0;

  if (!sorted)
    {
      (void) qsort (
		     ampm_tab, sizeof (ampm_tab) / sizeof (struct strint),
		     sizeof (struct strint), strint_compare);
      sorted = 1;
    }
  return strint_search (
     str_ampm, ampm_tab, sizeof (ampm_tab) / sizeof (struct strint), ampmP);
}

static int scan_wday (const char *str_wday, int *tm_wdayP)
{
  static struct strint wday_tab[] =
  {
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
  };
  static int sorted = 0;

  if (!sorted)
    {
      (void) qsort (
		     wday_tab, sizeof (wday_tab) / sizeof (struct strint),
		     sizeof (struct strint), strint_compare);
      sorted = 1;
    }
  return strint_search (
  str_wday, wday_tab, sizeof (wday_tab) / sizeof (struct strint), tm_wdayP);
}

static int scan_mon (const char *str_mon, int *tm_monP)
{
  static struct strint mon_tab[] =
  {
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
  };
  static int sorted = 0;

  if (!sorted)
    {
      (void) qsort (
		     mon_tab, sizeof (mon_tab) / sizeof (struct strint),
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
    {"-0200", -7200},
    {"0300", 10800},
    {"+0300", 10800},
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
    {"y", -43200},
  };
  static int sorted = 0;

  if (!sorted)
    {
      (void) qsort (
		   gmtoff_tab, sizeof (gmtoff_tab) / sizeof (struct strint),
		     sizeof (struct strint), strint_compare);
      sorted = 1;
    }
  return strint_search (
       str_gmtoff, gmtoff_tab, sizeof (gmtoff_tab) / sizeof (struct strint),
			 gmtoffP);
}

/* This is the EXPORTED FUNCTION */
/* recursive */
/* NOTE: Assumes the cannonical internal form is UTC!!! */
double date_parse (const char *str)
{
  time_t now;
  struct tm *now_tmP;
  struct tm tm;
  const char *cp;
  char str_mon[500], str_wday[500], str_gmtoff[500], str_ampm[500];
  int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday, gmtoff;
  int ampm, got_zone;
  char sc = 0;
  time_t t;

  /* Initialize tm with relevant parts of current local time. */
  now = time ((time_t *) 0);
  now_tmP = localtime (&now);

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

  /* Skip initial whitespace ourselves - sscanf is clumsy at this. */
  for (cp = str; *cp == ' ' || *cp == '\t'; ++cp)
    ;

  /* And do the sscanfs.  WARNING: you can add more formats here,
     ** but be careful!  You can easily screw up the parsing of existing
     ** formats when you add new ones.
   */

  /* N mth YYYY HH:MM:SS ampm zone */
  if (((sscanf (cp, "%d %[a-zA-Z] %d %d:%d:%d %[apmAPM] %[^: ]",
		&tm_mday, str_mon, &tm_year, &tm_hour, &tm_min, &tm_sec,
		str_ampm, str_gmtoff) == 8 &&
	scan_ampm (str_ampm, &ampm)) ||
       sscanf (cp, "%d %[a-zA-Z] %d %d:%d:%d %[^: ]",
	       &tm_mday, str_mon, &tm_year, &tm_hour, &tm_min, &tm_sec,
	       str_gmtoff) == 7) &&
      scan_mon (str_mon, &tm_mon) &&
      scan_gmtoff (str_gmtoff, &gmtoff))
    {
      DP ("N mth YYYY HH:MM:SS ampm zone");
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
      got_zone = 1;
    }
  /* N mth YYYY HH:MM ampm zone */
  else if (((sscanf (cp, "%d %[a-zA-Z] %d %d:%d %[apmAPM] %[^: ]",
		   &tm_mday, str_mon, &tm_year, &tm_hour, &tm_min, str_ampm,
		     str_gmtoff) == 7 &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%d %[a-zA-Z] %d %d:%d %[^: ]",
		    &tm_mday, str_mon, &tm_year, &tm_hour, &tm_min,
		    str_gmtoff) == 6) &&
	   scan_mon (str_mon, &tm_mon) &&
	   scan_gmtoff (str_gmtoff, &gmtoff))
    {
      DP ("N mth YYYY HH:MM ampm zone");
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = 0;
      got_zone = 1;
    }
  /* N mth YYYY HH:MM:SS ampm */
  else if (((sscanf (cp, "%d %[a-zA-Z] %d %d:%d:%d %[apmAPM]",
		     &tm_mday, str_mon, &tm_year, &tm_hour, &tm_min, &tm_sec,
		     str_ampm) == 7 &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%d %[a-zA-Z] %d %d:%d:%d",
		    &tm_mday, str_mon, &tm_year, &tm_hour, &tm_min,
		    &tm_sec) == 6) &&
	   scan_mon (str_mon, &tm_mon))
    {
      DP ("N mth YYYY HH:MM:SS ampm");
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
    }
  /* N mth YYYY HH:MM ampm */
  else if (((sscanf (cp, "%d %[a-zA-Z] %d %d:%d %[apmAPM]",
		     &tm_mday, str_mon, &tm_year, &tm_hour, &tm_min,
		     str_ampm) == 6 &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%d %[a-zA-Z] %d %d:%d",
		    &tm_mday, str_mon, &tm_year, &tm_hour, &tm_min) == 5) &&
	   scan_mon (str_mon, &tm_mon))
    {
      DP ("N mth YYYY HH:MM ampm");
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = 0;
    }
  /* HH:MM:SS ampm zone N mth YYYY */
  else if (((sscanf (cp, "%d:%d:%d %[apmAPM] %[^: ] %d %[a-zA-Z] %d",
		 &tm_hour, &tm_min, &tm_sec, str_ampm, str_gmtoff, &tm_mday,
		     str_mon, &tm_year) == 8 &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%d:%d:%d %[^: ] %d %[a-zA-Z] %d",
		  &tm_hour, &tm_min, &tm_sec, str_gmtoff, &tm_mday, str_mon,
		    &tm_year) == 7) &&
	   scan_gmtoff (str_gmtoff, &gmtoff) &&
	   scan_mon (str_mon, &tm_mon))
    {
      DP ("HH:MM:SS ampm zone N mth YYYY");
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
    }
  /* HH:MM ampm zone N mth YYYY */
  else if (((sscanf (cp, "%d:%d %[apmAPM] %[^: ] %d %[a-zA-Z] %d",
		 &tm_hour, &tm_min, str_ampm, str_gmtoff, &tm_mday, str_mon,
		     &tm_year) == 7 &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%d:%d %[^: ] %d %[a-zA-Z] %d",
		    &tm_hour, &tm_min, str_gmtoff, &tm_mday, str_mon,
		    &tm_year) == 6) &&
	   scan_gmtoff (str_gmtoff, &gmtoff) &&
	   scan_mon (str_mon, &tm_mon))
    {
      DP ("HH:MM ampm N mth YYYY");
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = 0;
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
    }
  /* HH:MM:SS ampm N mth YYYY */
  else if (((sscanf (cp, "%d:%d:%d %[apmAPM] %d %[a-zA-Z] %d",
		     &tm_hour, &tm_min, &tm_sec, str_ampm, &tm_mday, str_mon,
		     &tm_year) == 7 &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%d:%d:%d %d %[a-zA-Z] %d",
		    &tm_hour, &tm_min, &tm_sec, &tm_mday, str_mon,
		    &tm_year) == 6) &&
	   scan_mon (str_mon, &tm_mon))
    {
      DP ("HH:MM:SS ampm N mth YYYY");
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
    }
  /* HH:MM ampm N mth YYYY */
  else if (((sscanf (cp, "%d:%d %[apmAPM] %d %[a-zA-Z] %d",
		     &tm_hour, &tm_min, str_ampm, &tm_mday, str_mon,
		     &tm_year) == 6 &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%d:%d %d %[a-zA-Z] %d",
		    &tm_hour, &tm_min, &tm_mday, str_mon, &tm_year) == 5) &&
	   scan_mon (str_mon, &tm_mon))
    {
      DP ("HH:MM ampm N mth YYYY");
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = 0;
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
    }
  /* wdy, N mth YYYY HH:MM:SS ampm zone */
  else if (((sscanf (cp, "%[a-zA-Z], %d %[a-zA-Z] %d %d:%d:%d %[apmAPM] %[^: ]",
		   str_wday, &tm_mday, str_mon, &tm_year, &tm_hour, &tm_min,
		     &tm_sec, str_ampm, str_gmtoff) == 9 &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%[a-zA-Z], %d %[a-zA-Z] %d %d:%d:%d %[^: ]",
		    str_wday, &tm_mday, str_mon, &tm_year, &tm_hour, &tm_min,
		    &tm_sec, str_gmtoff) == 8) &&
	   scan_wday (str_wday, &tm_wday) &&
	   scan_mon (str_mon, &tm_mon) &&
	   scan_gmtoff (str_gmtoff, &gmtoff))
    {
      DP ("wdy, N mth YYYY HH:MM:SS ampm zone");
      tm.tm_wday = tm_wday;
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
      got_zone = 1;
    }
  /* wdy, N mth YYYY HH:MM ampm zone */
  else if (((sscanf (cp, "%[a-zA-Z], %d %[a-zA-Z] %d %d:%d %[apmAPM] %[^: ]",
		   str_wday, &tm_mday, str_mon, &tm_year, &tm_hour, &tm_min,
		     str_ampm, str_gmtoff) == 8 &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%[a-zA-Z], %d %[a-zA-Z] %d %d:%d %[^: ]",
		    str_wday, &tm_mday, str_mon, &tm_year, &tm_hour, &tm_min,
		    str_gmtoff) == 7) &&
	   scan_wday (str_wday, &tm_wday) &&
	   scan_mon (str_mon, &tm_mon) &&
	   scan_gmtoff (str_gmtoff, &gmtoff))
    {
      DP ("wdy, N mth YYYY HH:MM ampm zone");
      tm.tm_wday = tm_wday;
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = 0;
      got_zone = 1;
    }
  /* wdy, N mth YYYY HH:MM:SS ampm */
  else if (((sscanf (cp, "%[a-zA-Z], %d %[a-zA-Z] %d %d:%d:%d %[apmAPM]",
		   str_wday, &tm_mday, str_mon, &tm_year, &tm_hour, &tm_min,
		     &tm_sec, str_ampm) == 8 &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%[a-zA-Z], %d %[a-zA-Z] %d %d:%d:%d",
		    str_wday, &tm_mday, str_mon, &tm_year, &tm_hour, &tm_min,
		    &tm_sec) == 7) &&
	   scan_wday (str_wday, &tm_wday) &&
	   scan_mon (str_mon, &tm_mon))
    {
      DP ("wdy, N mth YYYY HH:MM:SS ampm");
      tm.tm_wday = tm_wday;
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
    }
  /* wdy, N mth YYYY HH:MM ampm */
  else if (((sscanf (cp, "%[a-zA-Z], %d %[a-zA-Z] %d %d:%d %[apmAPM]",
		   str_wday, &tm_mday, str_mon, &tm_year, &tm_hour, &tm_min,
		     str_ampm) == 7 &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%[a-zA-Z], %d %[a-zA-Z] %d %d:%d",
		    str_wday, &tm_mday, str_mon, &tm_year, &tm_hour,
		    &tm_min) == 6) &&
	   scan_wday (str_wday, &tm_wday) &&
	   scan_mon (str_mon, &tm_mon))
    {
      DP ("wdy, N mth YYYY HH:MM ampm");
      tm.tm_wday = tm_wday;
      tm.tm_mday = tm_mday;
      tm.tm_mon = tm_mon;
      tm.tm_year = tm_year;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = 0;
    }
  /* wdy mth N HH:MM:SS ampm zone YYYY */
  else if (((sscanf (cp, "%[a-zA-Z] %[a-zA-Z] %d %d:%d:%d %[apmAPM] %[^: ] %d",
		     str_wday, str_mon, &tm_mday, &tm_hour, &tm_min, &tm_sec,
		     str_ampm, str_gmtoff, &tm_year) == 9 &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%[a-zA-Z] %[a-zA-Z] %d %d:%d:%d %[^: ] %d",
		    str_wday, str_mon, &tm_mday, &tm_hour, &tm_min, &tm_sec,
		    str_gmtoff, &tm_year) == 8) &&
	   scan_wday (str_wday, &tm_wday) &&
	   scan_mon (str_mon, &tm_mon) &&
	   scan_gmtoff (str_gmtoff, &gmtoff))
    {
      DP ("wdy mth N HH:MM:SS ampm zone YYYY");
      tm.tm_wday = tm_wday;
      tm.tm_mon = tm_mon;
      tm.tm_mday = tm_mday;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
      got_zone = 1;
      tm.tm_year = tm_year;
    }
  /* wdy mth N HH:MM ampm zone YYYY */
  else if (((sscanf (cp, "%[a-zA-Z] %[a-zA-Z] %d %d:%d %[apmAPM] %[^: ] %d",
		     str_wday, str_mon, &tm_mday, &tm_hour, &tm_min,
		     str_ampm, str_gmtoff, &tm_year) == 8 &&
	     scan_ampm (str_ampm, &ampm)) ||
	    sscanf (cp, "%[a-zA-Z] %[a-zA-Z] %d %d:%d %[^: ] %d",
		    str_wday, str_mon, &tm_mday, &tm_hour, &tm_min,
		    str_gmtoff, &tm_year) == 7) &&
	   scan_wday (str_wday, &tm_wday) &&
	   scan_mon (str_mon, &tm_mon) &&
	   scan_gmtoff (str_gmtoff, &gmtoff))
    {
      DP ("wdy mth N HH:MM ampm zone YYYY");
      tm.tm_wday = tm_wday;
      tm.tm_mon = tm_mon;
      tm.tm_mday = tm_mday;
      tm.tm_hour = ampm_fix (tm_hour, ampm);
      tm.tm_min = tm_min;
      tm.tm_sec = 0;
      got_zone = 1;
      tm.tm_year = tm_year;
    }
 /* wdy mth N HH:MM:SS YYYY */ /* edz ADDED */
  else if ((sscanf (cp, "%[a-zA-Z] %[a-zA-Z] %d %d:%d:%d %d",
                     str_wday, str_mon, &tm_mday, &tm_hour, &tm_min,
                     &tm_sec, &tm_year) == 7) &&
           scan_wday (str_wday, &tm_wday) &&
           scan_mon (str_mon, &tm_mon) )
    {
      DP ("wdy mth N HH:MM:SS YYYY");
      tm.tm_wday = tm_wday;
      tm.tm_mon = tm_mon;
      tm.tm_mday = tm_mday;
      tm.tm_hour = tm_hour;
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
      got_zone = 0;
      tm.tm_year = tm_year;
    }

  /* wdy mth N HH:MM YYYY */ /* edz ADDED */
  else if ((sscanf (cp, "%[a-zA-Z] %[a-zA-Z] %d %d:%d %d",
		     str_wday, str_mon, &tm_mday, &tm_hour, &tm_min,
		     &tm_year) == 6) &&
           scan_wday (str_wday, &tm_wday) &&
           scan_mon (str_mon, &tm_mon) )
    {
      DP ("wdy mth N HH:MM YYYY");
      tm.tm_wday = tm_wday;
      tm.tm_mon = tm_mon;
      tm.tm_mday = tm_mday;
      tm.tm_hour = tm_hour;
      tm.tm_min = tm_min;
      tm.tm_sec = 0;
      got_zone = 0;
      tm.tm_year = tm_year;
    }

  /* N mth YYYY */
  else if (sscanf (cp, "%d %[a-zA-Z] %d",
		   &tm_mday, str_mon, &tm_year) == 3 &&
	   scan_mon (str_mon, &tm_mon))
    {
      DP ("N mth YYYY");
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
  /* ISO Date    YYYYMMDDTHH:MM:SS[Z] */
  else if ( (sscanf (cp, "%4d%2d%2dT%d:%d:%2d%c",
		&tm_year,
		&tm_mon, &tm_mday,
		&tm_hour, &tm_min, &tm_sec, &sc) > 4) ||
	  (sscanf (cp, "%4d%2d%2dT%2d%2d%2d%c",
		&tm_year,
		&tm_mon, &tm_mday,
		&tm_hour, &tm_min, &tm_sec, &sc) >= 4) )
    {
      DP("YYYYMMDDTHH:MM:SS");
      tm.tm_year = tm_year - 1900;
      tm.tm_mon = tm_mon - 1;
      tm.tm_mday = tm_mday;
      tm.tm_hour = tm_hour;
      tm.tm_min = tm_min;
      tm.tm_sec = tm_sec;
      if (sc == 'Z') {
	got_zone = 1;
	gmtoff = 0;
      }
    }
  /* ISO Date    YYYY-MM-DDTHH:MM:SSZ (eg.  1996-07-16T13:19:51Z) */
  else if (isdigit (cp[0]) && isdigit (cp[1]) && isdigit(cp[2]) && isdigit(cp[3]) &&
	cp[4] == '-' && isdigit (cp[5]) )
    {
      tm_sec  = 0;
      tm_mday = 1;
      if ((sscanf(cp, "%d-%d-%dT%d:%d:%d%c", &tm_year, &tm_mon, &tm_mday,
	&tm_hour, &tm_min, &tm_sec, &sc) > 4) ||
	  (sscanf(cp, "%d-%d-%dT%2d%2d%2d%c", &tm_year, &tm_mon, &tm_mday,
		&tm_hour, &tm_min, &tm_sec, &sc) >= 4) )
	{
	  DP("YYYY-MM-DDTHH:MM:SS");
	  tm.tm_year = tm_year - 1900;
	  tm.tm_mon = tm_mon - 1;
	  tm.tm_mday = tm_mday;
	  tm.tm_hour = tm_hour;
	  tm.tm_min = tm_min;
	  tm.tm_sec = tm_sec;
	  if (sc == 'Z') {
	    got_zone = 1;
	    gmtoff = 0;
	  }
	}
      else if (sscanf(cp, "%d-%d-%d", &tm_year, &tm_mon, &tm_mday) >= 2)
        {
	  DP("YYYY-MM-DD");
	  tm.tm_year = tm_year - 1900;
	  tm.tm_mon = tm_mon - 1;
	  tm.tm_mday = tm_mday;
	  tm.tm_hour = 0;
	  tm.tm_min = 0;
	  tm.tm_sec = 0;
	  gmtoff = 0; /* Don't adjust */
	  got_zone = 1;
        }
      else
	return -1; /* OOPS */
    }
  /* ANSI Date: YYYYMMDD or obsolete form YYMMDD and optional HH:MM ZONE */
  else if (isdigit (cp[0]) && isdigit (cp[1]) && isdigit (cp[2]) && isdigit (cp[3])
	   && isdigit (cp[4]) && isdigit (cp[5]))
    {
      int pos = 0;
      DP ("YYYYMMDD");
      if (isdigit(cp[6]) && (isdigit(cp[6]) || isdigit(cp[7])))
	{
	  tm.tm_year = 0;
	  if (isdigit(cp[7]))
	    {
	      tm.tm_year +=  (cp[pos++] - '0') * 1000;
	    }
	  tm.tm_year += (cp[pos++] - '0') * 100;
	}
      else			/* Obsolete form YYMMDD */
	{
	  tm.tm_year = 1900;
	}
      tm.tm_year += (cp[pos++] - '0') * 10;
      tm.tm_year += (cp[pos++] - '0');

      tm.tm_mon =  (cp[pos++] - '0') * 10;
      tm.tm_mon += (cp[pos++] - '0') - 1;

      tm.tm_mday =  (cp[pos++] - '0') * 10;
      tm.tm_mday += (cp[pos++] - '0');
      if (isspace (cp[pos]) || cp[pos] == '-')
	{
	  time_t rest = date_parse (cp + pos + 1);
	  if (rest != -1)
	    {
	      struct tm *rest_tm = gmtime (&rest);
	      tm.tm_hour = rest_tm->tm_hour;
	      tm.tm_min = rest_tm->tm_min;
	      tm.tm_sec = rest_tm->tm_sec;

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
  /* Stupid DD.MM.YYYY and DD.MM.YY forms */
  else if (isdigit (cp[0]) && isdigit (cp[1]) && cp[2] == '.' && isdigit (cp[3]) &&
      isdigit (cp[4]) && cp[5] == '.' && isdigit (cp[6]) && isdigit (cp[7]))
    {
      DP ("MM.DD.YYYY");
      tm.tm_mday = (cp[0] - '0') * 10 + (cp[1] - '0');
      tm.tm_mon = (cp[3] - '0') * 10 + (cp[4] - '0') - 1;
      tm.tm_year = (cp[6] - '0') * 10 + (cp[7] - '0');
      if (isdigit (cp[8]) && isdigit (cp[9]))
	tm.tm_year = tm.tm_year * 100 + (cp[8] - '0') * 10 + (cp[9] - '0');
      else
	tm.tm_year += 1900;
      /* Make sure it makes sense! */
      if (tm.tm_mon < 0 || tm.tm_mon > 12 || tm.tm_mday < 0 || tm.tm_mday > 31)
	return -1;	/* ERROR */
      gmtoff = 0; /* Don't adjust */
      got_zone = 1;
      tm.tm_hour = 0;
      tm.tm_min = 0;
      tm.tm_sec = 0;
    }
  else
    return -1;

  /* We assume that years > 100 */
  if (tm.tm_year < 100)
    {
      tm.tm_year += 1900;
    }

  /* Get offset right */
  t = (tm.tm_hour) * 60 * 60;
  t += (tm.tm_min) * 60;
  t += (tm.tm_sec);
  t -= gmtoff;
  if (tm.tm_isdst && !got_zone)
    t -= 60 * 60;
  if (t < 0)
    {
      /* push back */
      t += 24*60*60;
      /* decrement */
      if (tm.tm_mday == 1)
	{
	  static int mntb[] = {31,28,31,30,31,30,31,31,30,31,30,31};
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

  sprintf(str_mon, DATE_FMT, 
	tm.tm_year,
	tm.tm_mon+1,
	tm.tm_mday,
	(t / 3600 ) % 60,
	(t / 60) % 60,
	t % 60 );
  return atof(str_mon);
}

#ifdef MAIN_STUB
int main (int argc, char **argv)
{
  double res = date_parse (argv[1]);
  printf ("%s --> %f\n", argv[1], res);
}

#endif
