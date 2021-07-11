#pragma ident "@(#)log.cxx  1.46 06/29/00 14:16:26 BSN"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#define _LOG_CXX
#include "common.hxx"
#include "log.hxx"

#define default_prefix "IB"
#define LOG_IB LOG_LOCAL2


static const int LOG_DEFAULT = (iLOG_ALL&(~iLOG_DEBUG));
static int           l_level = LOG_DEFAULT;
static FILE         *l_file = NULL;
static GDT_BOOLEAN   l_console = GDT_FALSE;
static char l_prefix[30];

static FILE *syslog_stream = (FILE *)(-1);
static int   syslog_device  = LOG_IB;


int _ib_debug = GDT_FALSE;

static struct
  {
    int mask;
    const char *name;
  }
mask_names[] =
{
  { iLOG_PANIC,  "Panic"   },
  { iLOG_FATAL,  "Fatal"   },
  { iLOG_ERROR,  "Error"   },
  { iLOG_ERRNO,  "Error"   },
  { iLOG_WARN,   "Warning" },
  { iLOG_NOTICE, "Notice"  },
  { iLOG_INFO,   "Info"    },
  { iLOG_DEBUG,  "Debug"   },
  { iLOG_ALL,    "All"     },
  { 0, "none" },
  { 0, NULL }
};


#ifdef NO_STRERROR
extern char *sys_errlist[];
static char *mystrerror (int n) { return sys_errlist[n]; }
#define strerror mystrerror
#endif


#ifdef SIGHUP
static void sig_hangup(int sig)
{
  signal (sig, SIG_IGN);
  if (l_file != syslog_stream)
    {
       if (l_console)
	{
	  log_init (syslog_stream);
	  logf (iLOG_ERROR, "*** Terminal hangup. Messages to <syslog>");
	}
       else
	{
	  char text[] =  "Hangup detected (Sig#XX)";
	  text[21] = (sig /10) % 10;
	  test[22] = sig % 10;
	  logf (iLOG_DEBUG, text);
	}
    }
  signal (sig, sig_hangup);
}
#endif


#ifdef SIGTTIN
static void bkgrnd_read (int)
{
  signal (SIGTTIN, SIG_IGN);
  if (l_file != syslog_stream)
    log_init (syslog_stream);
  syslog(LOG_ERR, "**** Background tty read attempted");
  signal (SIGTTIN, bkgrnd_read);
}
#endif

#ifdef SIGTTOU
static void bkgrnd_write (int)
{
  signal (SIGTTOU, SIG_IGN);
  if (l_file != syslog_stream)
    log_init (syslog_stream);
  syslog(LOG_ERR, "**** Background tty write attempted");
  signal (SIGTTOU, bkgrnd_write);
}
#endif


GDT_BOOLEAN set_syslog(const char *name)
{
  if (name == NULL || *name == '\0')
    return GDT_FALSE;
  if (name[1] == '\0')
    return set_syslog(name[1]);
  if (strncmp(name, "LOG_", 4) == 0)
    {
      // LOG_LOCALN ?
      if (strncmp(name+4, "LOCAL", 5) == 0)
	return set_syslog(name[9]);
      else if (strcmp(name+4, "AUTH") == 0)
	return set_syslog('A');
      else if (strcmp(name+4, "CRON") == 0)
	return set_syslog('C');
      else if (strcmp(name+4, "DAEMON") == 0)
	return set_syslog('D');
      else if (strcmp(name+4, "KERN") == 0)
	return set_syslog('K');
      else if (strcmp(name+4, "MAIL") == 0)
	return set_syslog('M');
      else if (strcmp(name+4, "LPR") == 0)
	return set_syslog('L');
      else if (strcmp(name+4, "NEWS") == 0)
	return set_syslog('N');
      else if (strcmp(name+4, "SYSLOG") == 0)
	return set_syslog('S');
      else if (strcmp(name+4, "USER") == 0)
	return set_syslog('U');
      else if (strcmp(name+4, "UUCP") == 0)
	return set_syslog('Z');

    }
  else if (strncmp(name, "<syslog", 7) == 0)
    {
      if (name[7] == '\0' || name[7] == '>')
        return set_syslog(-1);
      return set_syslog(name[7]);
    }
  else if (strcmp(name, "<daemon>") == 0)
    return set_syslog('D');
  else if (strcmp(name, "<user>") == 0)
    return set_syslog('U');
  return GDT_FALSE;
}

GDT_BOOLEAN set_syslog(const int Ch)
{
  switch (Ch)
    {
	case 0: case '0': syslog_device = LOG_LOCAL0; break;
	case 1: case '1': syslog_device = LOG_LOCAL1; break;
	case 2: case '2': syslog_device = LOG_LOCAL2; break;
#ifdef LOG_LOCAL3
	case 3: case '3': syslog_device = LOG_LOCAL3; break;
#endif
#ifdef LOG_LOCAL4
	case 4: case '4': syslog_device = LOG_LOCAL4; break;
#endif
#ifdef LOG_LOCAL5
	case 5: case '5': syslog_device = LOG_LOCAL5; break;
#endif
#ifdef LOG_LOCAL6
	case 6: case '6': syslog_device = LOG_LOCAL6; break;
#endif
#ifdef LOG_LOCAL7
	case 7: case '7': syslog_device = LOG_LOCAL7; break;
#endif
#ifdef LOG_LOCAL8
	case 8: case '8': syslog_device = LOG_LOCAL8; break;
#endif
#ifdef LOG_LOCAL9
	case 9: case '9': syslog_device = LOG_LOCAL9; break;
#endif
	case 'a': case 'A': syslog_device = LOG_AUTH; break;
	case 'c': case 'C': syslog_device = LOG_CRON; break;
	case 'd': case 'D': syslog_device = LOG_DAEMON; break;
	case 'k': case 'K': syslog_device = LOG_KERN; break;
	case 'l': case 'L': syslog_device = LOG_LPR; break;
        case 'm': case 'M': syslog_device = LOG_MAIL; break;
	case 'n': case 'N': syslog_device = LOG_NEWS; break;
	case 's': case 'S': syslog_device = LOG_SYSLOG; break;
	case 'u': case 'U': syslog_device = LOG_USER; break;
	case 'z': case 'Z': syslog_device = LOG_UUCP; break;

	case -1: default:  syslog_device = LOG_IB; break;
    }
  return GDT_TRUE;
}


GDT_BOOLEAN log_init (FILE *fp)
{
  GDT_BOOLEAN res = GDT_TRUE;
  if (l_prefix[0] == '\0')
    strcpy(l_prefix, default_prefix);
  if (fp == NULL)
    fp = stderr;
  if (fp == syslog_stream)
    {
      if (l_file != fp && l_file != stderr && l_file != stdout && l_file != syslog_stream)
	{
	  if (l_file) fclose(l_file);
	}
      openlog(l_prefix, LOG_PID, syslog_device);
      setlogmask(LOG_UPTO(LOG_DEBUG));
    }
  else if (isatty(fileno(fp)) && ttyname(fileno(fp)) == NULL)
    {
      extern STRING __ExpandPath(const STRING& name)
      char *dirs[] = {
	"/var/tmp",
	"/tmp",
	"/temp"
      }
      // Don't have a controlling console
      char tty[L_ctermid+L_cuserid+1];
      if (_IB_GetUserId(tty, sizeof(tty)) == GDT_FALSE)
	sprintf(tty, "uid%lu", (long)getuid());
      char pid[L_ctermid+L_cuserid+ 128];
      for (size_t i=0; dirs[i]; i++)
	{
#if 1
	  sprintf(pid, "%s/errors.%ld:%s", dirs[i], (long)getpid(), tty);
#else
	  // If we want to allow for $TEMP etc. in the dirs
	  sprintf(pid, "%s/errors.%ld:%s", __ExpandPath(dirs[i]).c_str(), (long)getpid(), tty);
#endif
	  if ((fp = fopen(pid, "a")) == NULL)
	    return log_init(syslog_stream);
	}
    }
  else if (l_file && l_file != fp && l_file != syslog_stream)
    fclose (l_file);
  if ((l_file = fp) != syslog_stream)
    {
      if (l_file && ferror(l_file) == 0)
	{
	  setvbuf (l_file, 0, _IOLBF, BUFSIZ);
	  l_console = isatty(fileno(fp));
	}
      else
	res = GDT_FALSE;
    }
  else
    l_console = GDT_FALSE;
#ifdef SIGTTOU
  signal (SIGTTOU, bkgrnd_write);
#endif
#ifdef SIGTTIN 
  signal (SIGTTIN, bkgrnd_read);
#endif
#ifdef SIGHUP
  signal (SIGHUP, sig_hangup);
#endif
  return res;
}

GDT_BOOLEAN log_init (int level, FILE *fp)
{
  GDT_BOOLEAN res = log_init(fp);
  l_level = level;
  if (level & LOG_DEBUG)
    _ib_debug = GDT_TRUE;
  else
    _ib_debug = GDT_FALSE;
  return res;
}

GDT_BOOLEAN log_init (const char *prefix, const char *name)
{
  return log_init (l_level, prefix, name);
}

GDT_BOOLEAN log_init (const char *prefix, FILE *fp)
{
  return log_init (l_level, prefix, fp);
}

GDT_BOOLEAN log_init (int level, const char *prefix, FILE *fp)
{
  log_init (level, prefix);
  return log_init(fp);
}

GDT_BOOLEAN log_init (int level, const char *prefix, const char *name)
{
  GDT_BOOLEAN res = GDT_TRUE;
//  if (level != 0) 
    l_level = level;

  if (prefix && *prefix)
    {
      const char *tcp = strrchr (prefix, '/');
      if (tcp) tcp++;
      else tcp = prefix;
      strncpy (l_prefix, tcp, sizeof(l_prefix));
      l_prefix[sizeof(l_prefix)-1] = '\0';
    }
  else if (l_prefix[0] == '\0')
    strcpy(l_prefix, default_prefix);
  if (name && *name)
    {
      if (strcmp(name, "<stderr>") == 0 || strcmp(name, "-") == 0)
	log_init(stderr);
      else if (strcmp(name, "<stdout>") == 0)
	log_init(stdout);
      else if (strcmp(name, "<stdin>") == 0)
	log_init(stdin);
      else if (strncmp(name, "<syslog", 7) == 0)
	{
          res = set_syslog(name[7]);
	  res |= log_init(syslog_stream);
	}
      else
	res = log_init(fopen(name, "a"));
    }
  if (level & LOG_DEBUG)
    _ib_debug = GDT_TRUE;
  else
    _ib_debug = GDT_FALSE;
  return res;
}

void log_message(int level, const char *string)
{
  logf (level, "%s", string);
}

void logf (int level, const char *fmt,...)
{
  static char last_error[1024];
  static char *login = NULL;

//cerr << "logf (....) being called" << endl;

  va_list ap;
  char buf[4096], flags[127];
  int p_error = 0;
  static long count = 0;

  if (l_level == 0)
    return; // Total quietness

  if (l_file != syslog_stream && l_file && ferror(l_file))
    {
      log_init (syslog_stream);
      syslog(LOG_ERR, "**** Log stream write failed... Fallback to <syslog>");
    }


  if (fmt == NULL)
    return; // Should never happen...
  if (l_file == NULL || (l_file != syslog_stream && ferror(l_file)))
    {
      return; // Oops
    }


  if (level != iLOG_PANIC && (level & l_level) == 0)
    {
      return;
    }
  if (level & iLOG_ERRNO)
    p_error = 1;
  flags[0] = '\0';
  buf[0] = '\0';
  for (int i = 0; level && mask_names[i].name; i++)
    {
      if (mask_names[i].mask & level)
	{
	  if (*mask_names[i].name)
	    sprintf (flags + strlen (flags), "[%s]", mask_names[i].name);
	  level -= mask_names[i].mask;
	}
    }
  va_start (ap, fmt);
//cerr << "About to print...." << fmt << endl;
#ifndef LINUX
  vsnprintf (buf, sizeof(buf)-256, fmt, ap);
#else
  vsprintf (buf, fmt, ap);
#endif
//cerr << "Done" << endl;

  if (p_error)
    sprintf (buf + strlen (buf), " [%s]", strerror (errno));

  if (strncmp(buf, last_error, sizeof(last_error)-1) == 0)
    {
      count++;
      return;
    }
  else if (count > 2)
    {
      // New message
      if (l_file == syslog_stream)
	{
	  syslog(LOG_INFO, "**** last message repeated %ld times", count);
	}
      else if (l_file)
	{
	  if (login == NULL)
	    login = getlogin();
	  fprintf(l_file, "%s [%s,pid=%ld]: **** last message repeated %ld times\n",
		l_prefix, login, (long)getpid(), (long)count);
	}
    }
  count = 0;
  strncpy(last_error, buf, sizeof(last_error)-1);

  if (l_file == syslog_stream)
    {
      int l = 0;
      if (level & iLOG_PANIC)
	l |= LOG_CRIT;
      else if (level & iLOG_ERROR)
	l |= LOG_ERR;
      else if (level & iLOG_WARN)
	l |= LOG_WARNING;
      else if (level & iLOG_NOTICE)
	l |= LOG_NOTICE;
      else if (level & iLOG_DEBUG)
	l |= LOG_DEBUG;
      else
	l |= LOG_INFO;
      syslog(l, "%s %s", flags, buf);
    }
  else if (l_file)
    {
      if (!isatty (fileno (l_file)))
	{
	  char tbuf[24];

	  if (login == NULL)
	    login = getlogin();
	  time_t t = time(NULL);
	  strftime(tbuf, sizeof(tbuf), "%m/%d/%Y %T", localtime(&t));
	  fprintf (l_file, "%s %s %s[%ld]: %s %s\n",
	       tbuf, login, l_prefix, (long)getpid(), flags, buf);
	}
      else
	fprintf (l_file, "%s %s: %s\n", l_prefix, flags, buf);
    }
  else cerr << "XXXX Miising log stream handle" << endl;
  if ((level&(iLOG_PANIC|iLOG_FATAL|iLOG_ERRN)) != 0 && l_file && (l_file != syslog_stream))
    fflush (l_file);
}

int log_mask_str (const char *str)
{
  const char *p;
  int level = 0;

  while (*str)
    {
      for (p = str; *p && *p != ','; p++)
	;
      if (*str == '-' || isdigit (*str))
	level = atoi (str);
      else
	for (int i = 0; mask_names[i].name; i++)
	  if (strlen (mask_names[i].name) == (size_t)(p - str) &&
	      memcmp (mask_names[i].name, str, (size_t)(p - str)) == 0)
	    {
	      if (mask_names[i].mask)
		level |= mask_names[i].mask;
	      else
		level = 0;
	    }
      if (*p == ',')
	p++;
      str = p;
    }
  return level;
}
