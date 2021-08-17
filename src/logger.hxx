/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#ifndef _LOG_HXX
#define _LOG_HXX

#ifdef USE_pThreadLocker
# include <pthread.h>
#endif

const int iLOG_PANIC=  (1 << 0); /* system is unusable */
const int iLOG_FATAL=  (1 << 1); /* fatal error conditions */ 
const int iLOG_ERROR=  (1 << 2); /* error conditions */ 
const int iLOG_ERRNO=  (1 << 3); /* error conditions (system) */
const int iLOG_WARN=   (1 << 4); /* warning conditions */
const int iLOG_NOTICE= (1 << 5); /* normal but signification condition */
const int iLOG_INFO=   (1 << 6); /* informational */
const int iLOG_DEBUG=  (1 << 7); /* debug info */
const int iLOG_ALL  =0xffff;

#ifndef _GLOBAL_MESSAGE_LOGGER_INTERNALS
# define LOG_PANIC	iLOG_PANIC
# define LOG_FATAL	iLOG_FATAL
# define LOG_ERROR	iLOG_ERROR
# define LOG_ERRNO	iLOG_ERRNO
# define LOG_WARN	iLOG_WARN
# define LOG_NOTICE	iLOG_NOTICE
# define LOG_INFO	iLOG_INFO
# define LOG_DEBUG	iLOG_DEBUG
# define LOG_ALL	iLOG_ALL
# define LOG_ANY        (iLOG_ALL & (~ iLOG_ERRNO))
#endif

typedef int (*_MessageFunc_t)(int, const char *);

class MessageLogger {
public:
 MessageLogger();
 ~MessageLogger();

  GDT_BOOLEAN Init (FILE *fp);
  GDT_BOOLEAN Init (const char *prefix, const char *name=0);
  GDT_BOOLEAN Init (const char *prefix, FILE *fp);
  GDT_BOOLEAN Init (int level, FILE *fp);
  GDT_BOOLEAN Init (int level, const char *prefix, FILE *fp);
  GDT_BOOLEAN Init (int level, const char *prefix=0, const char *name=0);

  GDT_BOOLEAN Init (int level, const char *prefix, _MessageFunc_t newFunc) {
    if (Init(level, prefix)) return Init(newFunc);
    return GDT_FALSE;
  }
  GDT_BOOLEAN Init (int level,_MessageFunc_t newFunc) {
    if (Init(level)) return Init(newFunc);
    return GDT_FALSE;
  }
  GDT_BOOLEAN  Init(_MessageFunc_t newOutputFunc) {
     return ((OutputFunc = newOutputFunc) != 0);
  }
  
  GDT_BOOLEAN use_syslog() { return Init((FILE *)syslog_stream); }
  GDT_BOOLEAN set_syslog(const char *name);
  GDT_BOOLEAN set_syslog(const int Ch);


  void        log (int level, const char *fmt, ...); // __attribute__((fmt(printf, 2, 3))) ; 
  void        log_message(int level, const char *string) ;

  void        info_message (const char *str) { log_message(iLOG_INFO, str);}
  void        panic_message(const char *str) { log_message(iLOG_PANIC, str);}
  void        error_message(const char *str) { log_message(iLOG_ERROR, str);}
  void        errno_message(const char *str) { log_message(iLOG_ERRNO, str);}
  void        fatal_message(const char *str) { log_message(iLOG_FATAL, str);}

/*
#ifdef __STRING_HXX__
  void        panic_message(const STRING& String) { log_message(iLOG_PANIC, String.c_str());}
  void        error_message(const STRING& String) { log_message(iLOG_ERROR, String.c_str());}
  void        fatal_message(const STRING& String) { log_message(iLOG_FATAL, String.c_str());}
#endif
*/
  GDT_BOOLEAN to_syslog() const  { return l_file == syslog_stream; }
  GDT_BOOLEAN to_console() const { return l_console;               }
protected:
  int         log_mask_str (const char *str);
#ifdef USE_pThreadLocker
  operator    pthread_mutex_t *() const { return (pthread_mutex_t *)&m_mutex; } 
#endif
  operator    FILE *() const            { return l_file;   } 
private:
#ifdef USE_pThreadLocker
  pthread_mutex_t   m_mutex;
#endif
  int               l_level;
  GDT_BOOLEAN       l_console;
  char              l_prefix[30];
  FILE             *l_file;
  _MessageFunc_t    OutputFunc;
  int               syslog_device;
  FILE             *syslog_stream; 
  char             *last_message;
  char             *curr_message;
  size_t            MaxMessageLength;
};

#endif		/* _LOG_HXX */
