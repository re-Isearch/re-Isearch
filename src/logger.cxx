#pragma ident "%Z%%Y%%M%  %I% %G% %U% BSN"
#define _GLOBAL_MESSAGE_LOGGER_INTERNALS 1

/*
Compile:

g++ -c  -I. -I../src logger.cxx
*/

#include "platform.h"

// use vasprintf instead!!!

#ifndef HAVE_SAFESPRINTF
# if defined(_WIN32) || defined(BSD) || defined(LINUX)
#  define HAVE_SAFESPRINTF 1
# else
#  define HAVE_SAFESPRINTF 0
# endif
#endif

#ifndef HAVE_SYSLOG
# ifndef _WIN32
#  define HAVE_SYSLOG 1
# endif
#endif

#include "common.hxx"

#include <stdarg.h>
#include <errno.h>
#if HAVE_SYSLOG
# include <syslog.h>
#endif

#include "logger.hxx"

#ifdef _WIN32
# define vsnprintf _vsnprintf
#endif

#if HAVE_SAFESPRINTF
# define MAX_MESSAGE_LENGTH 8192
#else
# define MAX_MESSAGE_LENGTH 65535 
#endif

static const char STANDARD_IBLOG[] =
#if defined(_MSDOS) || defined(_WIN32)
  "/tmp/iblog";
#else
  "/var/log/iblog";
#endif
#if defined(_MSDOS) || defined(_WIN32)

static char *getlogin()
{
  static char tmp[127];
  if (tmp[0] == '\0')
    {
      if (_IB_GetUserId(tmp, sizeof(tmp)-1) == GDT_FALSE)
        strcpy(tmp, "anonymous");
    }
  return tmp;
};


#ifndef __GNUG__
# define getpid() GetCurrentProcessId()  //cast DWORD to long
#endif
#endif

#define default_prefix "IB"
#ifndef LOG_IB
# ifdef LOG_LOCAL2
#  define LOG_IB LOG_LOCAL2
# else
#  define LOG_IB -1
# endif
#endif

GDT_BOOLEAN _ib_debug = GDT_FALSE;
static const int LOG_DEFAULT = (iLOG_ALL&(~iLOG_DEBUG));


MessageLogger::MessageLogger()
{
  l_level = LOG_DEFAULT;
  l_file = NULL;
  l_console = GDT_FALSE;
  l_prefix[0] = '\0';
  syslog_device  = LOG_IB;
  syslog_stream = (FILE *)(-2); // was -1
  OutputFunc = 0;

  MaxMessageLength = MAX_MESSAGE_LENGTH;

  last_message = (char *)calloc(MaxMessageLength+1, sizeof(char));
  curr_message = (char *)calloc(MaxMessageLength+1, sizeof(char));
  if (last_message == NULL || last_message == NULL)
    {
      perror("Out of memory: Can't allocate error message buffer");
      abort();
    }

#ifdef SOLARIS
  // Threading
  pthread_mutex_init(&m_mutex, NULL);
#endif
}


MessageLogger::~MessageLogger()
{
  if (l_file != NULL && l_file != stderr && l_file != stdout && l_file != syslog_stream)
    fclose(l_file);
#ifdef SOLARIS
  pthread_mutex_destroy(&m_mutex);
#endif
  if (last_message) free(last_message);
  if (curr_message) free(curr_message);
}

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


GDT_BOOLEAN  MessageLogger::set_syslog(const char *name)
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

GDT_BOOLEAN  MessageLogger::set_syslog(const int Ch)
{
  switch (Ch)
    {
#ifdef LOG_LOCAL0
	case 0: case '0': syslog_device = LOG_LOCAL0; break;
#endif
#ifdef LOG_LOCAL1
	case 1: case '1': syslog_device = LOG_LOCAL1; break;
#endif
#ifdef LOG_LOCAL2
	case 2: case '2': syslog_device = LOG_LOCAL2; break;
#endif
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
#ifdef LOG_AUTH
	case 'a': case 'A': syslog_device = LOG_AUTH; break;
#endif
#ifdef LOG_CRON
	case 'c': case 'C': syslog_device = LOG_CRON; break;
#endif
#ifdef LOG_DAEMON
	case 'd': case 'D': syslog_device = LOG_DAEMON; break;
#endif
#ifdef LOG_KERN
	case 'k': case 'K': syslog_device = LOG_KERN; break;
#endif
#ifdef LOG_LPR
	case 'l': case 'L': syslog_device = LOG_LPR; break;
#endif
#ifdef LOG_MAIL
        case 'm': case 'M': syslog_device = LOG_MAIL; break;
#endif
#ifdef LOG_NEWS
	case 'n': case 'N': syslog_device = LOG_NEWS; break;
#endif
#ifdef LOG_SYSLOG
	case 's': case 'S': syslog_device = LOG_SYSLOG; break;
#endif
#ifdef LOG_USER
	case 'u': case 'U': syslog_device = LOG_USER; break;
#endif
#ifdef LOG_UUCP
	case 'z': case 'Z': syslog_device = LOG_UUCP; break;
#endif
	case -1: default:  syslog_device = LOG_IB; break;
    }
  return GDT_TRUE;
}


GDT_BOOLEAN MessageLogger::Init (FILE *fp)
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
#if HAVE_SYSLOG
      openlog(l_prefix, LOG_PID, syslog_device);
      setlogmask(LOG_UPTO(LOG_DEBUG));
#endif
    }
#ifndef _WIN32
  else if (isatty(fileno(fp)) && ttyname(fileno(fp)) == NULL)
    {
      // Don't have a controlling console
      // Append to a standard log?
      if ((fp = fopen(STANDARD_IBLOG, "a")) == NULL)
	{
         STRING S = GetTempFilename("iblog");
	  if ((fp = S.fopen("a")) == NULL)
	    return Init(syslog_stream);
	}
    }
#endif
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
  return res;
}

GDT_BOOLEAN MessageLogger::Init (int level, FILE *fp)
{
  GDT_BOOLEAN res = Init(fp);
  l_level = level;
  if (level & iLOG_DEBUG)
    _ib_debug = GDT_TRUE;
  else
    _ib_debug = GDT_FALSE;
  return res;
}

GDT_BOOLEAN MessageLogger::Init (const char *prefix, const char *name)
{
  return Init (l_level, prefix, name);
}

GDT_BOOLEAN MessageLogger::Init (const char *prefix, FILE *fp)
{
  return Init (l_level, prefix, fp);
}

GDT_BOOLEAN MessageLogger::Init (int level, const char *prefix, FILE *fp)
{
  Init (level, prefix);
  return Init(fp);
}

GDT_BOOLEAN MessageLogger::Init (int level, const char *prefix, const char *name)
{
  GDT_BOOLEAN res = GDT_TRUE;
//  if (level != 0) 
    l_level = level;

  if (prefix && *prefix)
    {
      STRING appname = RemovePath(prefix);
      strncpy (l_prefix, appname.c_str(), sizeof(l_prefix));
      l_prefix[sizeof(l_prefix)-1] = '\0';
    }
  else if (l_prefix[0] == '\0')
    strcpy(l_prefix, default_prefix);
  if (name && *name)
    {
      if (strcmp(name, "<stderr>") == 0 || strcmp(name, "-") == 0)
	Init(stderr);
      else if (strcmp(name, "<stdout>") == 0)
	Init(stdout);
      else if (strcmp(name, "<stdin>") == 0)
	Init(stdin);
      else if (strncmp(name, "<syslog", 7) == 0)
	{
          res = set_syslog(name[7]);
	  res |= Init(syslog_stream);
	}
      else
	res = Init(fopen(name, "a"));
    }
  if (level & iLOG_DEBUG)
    _ib_debug = GDT_TRUE;
  else
    _ib_debug = GDT_FALSE;
  return res;
}

void MessageLogger::log_message(int level, const char *string)
{
  MessageLogger::log (level, "%s", string);
}


// This is where the log is written. A global instance is what the vararg call logf(..) calls
void MessageLogger::log (int Mask, const char *fmt,...)
{
  static char *login = NULL;
  static long  count = 0;
  char         flags[127];
  int          level = Mask;
  int          p_error = 0;
  va_list      ap;

  if (l_level == 0)
    return; // Total quietness

  flags[0] = '\0'; 

  if (fmt == NULL || *fmt == '\0')
    fmt = "<ERROR: Message Logger called with Empty format!>";

  if ((l_file != syslog_stream && l_file && ferror(l_file)) || l_file == NULL)
    {
#if HAVE_SYSLOG
      Init (syslog_stream);
      syslog(LOG_ERR, "**** Log stream write failed... Fallback to <syslog>");
#endif
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

  for (int i = 0; level && mask_names[i].name && strlen(flags)<=119; i++)
    {
      if (mask_names[i].mask & level)
	{
	  if (*mask_names[i].name)
	    sprintf (flags + strlen (flags), "[%s]", mask_names[i].name);
	  level -= mask_names[i].mask;
	}
    }

  va_start (ap, fmt);
#if HAVE_SAFESPRINTF
  const int    max_chars = MaxMessageLength - 512;
  int          num_chars = vsnprintf (curr_message, max_chars + 256, fmt, ap);

  if (num_chars >= max_chars 
#ifdef _WIN32
      // vsnprintf, _vsnprintf, and _vsnwprintf return the number of characters
      // written if the number of characters to write is less than or equal to count;
      // if the number of characters to write is greater than count, these functions
      // return -1 indicating that output has been truncated. The return value does
      // not include the terminating null, if one is written.
	|| num_chars == -1
#endif
	)
     strcpy(curr_message+max_chars, "...");
#else
  int          num_chars = vsprintf (curr_message, fmt, ap);
#endif

  if (p_error)
    {
#ifdef _WIN32
      STRING error (" [");
      LPVOID lpMsgBuf;
      if (errno)
	error << strerror(errno) <<  " : ";
      FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),0, (LPTSTR) &lpMsgBuf, 0, NULL);
      error.Cat ((const char *)lpMsgBuf);
      LocalFree(lpMsgBuf); 
      if (error.GetLength() > 500)
	{
	  error.EraseAfter(497);
	  error.Cat("...");
	}
      error.Cat("]");
      strcat(curr_message, error.c_str());
#else
      if (errno)
	{
	  size_t len = strlen(curr_message);
	  sprintf (&curr_message[len], " [%s]", strerror (errno));
	}
#endif
    }

  if (strncmp(curr_message, last_message, MaxMessageLength) == 0)
    {
      count++;
      return;
    }
  else if (count > 2)
    {
      // New message
      if (OutputFunc)
	{
	  char tmp[BUFSIZ];
	  sprintf(tmp, "**** last message repeated %ld times", count);
	  OutputFunc(Mask, tmp);
	}
      else if (l_file == syslog_stream)
	{
#if HAVE_SYSLOG
	  syslog(LOG_INFO, "**** last message repeated %ld times", count);
#endif
	}
      else if (l_file)
	{
#ifdef USE_pThreadLocker
	  pThreadLocker lock(&m_mutex);
#endif
	  if (login == NULL)
	    login = getlogin();
	  fprintf(l_file, "%s [%s,pid=%ld]: **** last message repeated %ld times\n",
		l_prefix, login, (long)getpid(), (long)count);
	}
    }
  count = 0;
  strcpy(last_message, curr_message);

// cerr << "l_file0" << l_file << " and syslog_stream=" << syslog_stream << endl;

  if (OutputFunc)
    {
      OutputFunc(Mask, curr_message);
    }
  else if (l_file == syslog_stream)
    {
#if HAVE_SYSLOG
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
      syslog(l, "%s %s", flags, curr_message);
#endif
    }
  else if (l_file)
    {
      if (!isatty (fileno (l_file)))
	{
	  char tbuf[24];

#ifdef USE_pThreadLocker
	  pThreadLocker lock(&m_mutex);
#endif
	  if (login == NULL)
	    login = getlogin();
	  time_t t = time(NULL);
	  strftime(tbuf, sizeof(tbuf), "%m/%d/%Y %T", localtime(&t));
	  fprintf (l_file, "%s %s %s[%ld]: %s %s\n",
	       tbuf, login, l_prefix, (long)getpid(), flags, curr_message);
	}
      else
	fprintf (l_file, "%s %s: %s\n", l_prefix, flags, curr_message);
    if ((level&(iLOG_PANIC|iLOG_FATAL)) != 0 && l_file && (l_file != syslog_stream))
      fflush (l_file);
   }

}

int MessageLogger::log_mask_str (const char *str)
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

#ifdef PURE_STANDALONE
MessageLogger _globalMessageLogger;
#endif


#if 0

static void bad(const char *fmt)
{
  fprintf("bad fmt in Sprintf, starting with \"%s\"\n", fmt);
  abort();
}
#define put(x) outbuf.Cat (x)

STRING Sprintf(const char *fmt, ...)
{
 	STRING outbuf;

        char *s;
	char buf[32];
	va_list ap;
	long i, j;

	va_start(ap, fmt);
	for(;;) {
		for(;;) {
			switch(i = *fmt++) {
				case 0:
					goto done;
				case '%':
					break;
				default:
					put(i);
					continue;
				}
			break;
			}
		switch(*fmt++) {
			case 'c':
				i = va_arg(ap, int);
				put(i);
				continue;
			case 'l':
				if (*fmt != 'd')
					bad(fmt);
				fmt++;
				i = va_arg(ap, long);
				goto have_i;
			case 'd':
				i = va_arg(ap, int);
 have_i:
				if (i < 0) {
					put('-');
					i = -i;
					}
				s = buf;
				do {
					j = i / 10;
					*s++ = i - 10*j + '0';
					}
					while(i = j);
				do {
					i = *--s;
					put(i);
					}
					while(s > buf);
				continue;
			case 's':
				s = va_arg(ap, char*);
				while(i = *s++)
					{ put(i); }
				continue;
			default:
				bad(fmt);
			}
		}
 done:
  return outbuf;
}


#endif
