/* Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE */
/************************************************************************
#define STANDALONE if not part of the lib
************************************************************************/
/*@@@
File:		defs.hxx
Description:	General definitions
@@@*/

#ifndef PLATFORM_INCLUDED
# include "platform.h"
#endif

#ifdef PURE_STANDALONE
# ifndef STANDALONE
#   define STANDALONE 1
# endif
#endif

#ifndef STANDALONE
# include "config.hxx"
#endif

#ifndef DEFS_HXX
#define DEFS_HXX

#ifdef WIN32
# ifndef _WIN32
#   define _WIN32 1
#   define lstat stat
# endif
#endif

#ifdef IB_USE_NAMESPACES
# ifdef __STD
#  define IB_STD __STD
# else
#  define IB_STD std
# endif
#else
# define IB_STD
#endif

#include "gdt.h"
#include "string.hxx"

#ifdef STANDALONE
const size_t StringCompLength = 0x7fffffff;
#else
# include "ib_defs.hxx"
#endif /* !STANDALONE */

#define BSN_EXTENSIONS	2

#ifdef PLATFORM_MSVC
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif

#define COUT cout
#define CERR cerr

#define QSORT _IB_Qsort /* BentleyQsort */
extern void BentleyQsort(void *, size_t, size_t, int (*)(const void *, const void *));
extern void SedgewickQsort(void *, size_t, size_t, int (*)(const void *, const void *));
extern void DualPivotQsort( void *, size_t, size_t, int (*)(const void *, const void *));

extern void (*_IB_Qsort)(void *, size_t, size_t, int (*)(const void *, const void *));

#if !defined(_WIN32) // && !defined(__APPLE__)
#define USE_pThreadLocker 1
#endif


/////////////////////////////////////////////////////////////////////////////////////
/////////////////// Message Logger Subsystem ////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

#include "logger.hxx"
extern MessageLogger _globalMessageLogger;
inline bool set_syslog(const char *name) {
  return _globalMessageLogger.set_syslog(name);
}
inline bool set_syslog(const int Ch) {
  return _globalMessageLogger.set_syslog(Ch);
}
inline bool  log_init (FILE *fp) {
  return _globalMessageLogger.Init(fp);
}
inline bool  log_init (int level, const char *prefix, _MessageFunc_t fn) {
  return _globalMessageLogger.Init(level, prefix, fn);
}
inline bool  log_init (int level, _MessageFunc_t fn) {
  return _globalMessageLogger.Init(level, fn);
}
inline bool  log_init (_MessageFunc_t fn) {
  return _globalMessageLogger.Init(fn);
}
inline bool  log_init (const char *prefix, const char *name=0) {
  return _globalMessageLogger.Init(prefix, name);
}
inline bool  log_init (const char *prefix, FILE *fp) {
  return _globalMessageLogger.Init(prefix, fp);
}
inline bool  log_init (int level, FILE *fp) {
  return _globalMessageLogger.Init(level, fp);
}
inline bool  log_init (int level, const char *prefix, FILE *fp) {
  return _globalMessageLogger.Init(level, prefix, fp);
}
inline bool  log_init (int level, const char *prefix=0, const char *name=0) {
  return _globalMessageLogger.Init(level, prefix, name);
}
inline void         log_message(int level, const char *string) {
//cerr << "STRING=" << string << endl;
  _globalMessageLogger.log_message(level, string);
//cerr << "Return" << endl;
}

#if 0 /* This is just to check to make sure the messages are OK! */
extern void message_log(int, const char *format, ...) __attribute__((format(printf,2,3)));
#else
#define message_log _globalMessageLogger.log
#endif

inline void  info_message (const char *str) {  _globalMessageLogger.info_message(str);}
inline void  panic_message(const char *str) {  _globalMessageLogger.panic_message(str);}
inline void  error_message(const char *str) {  _globalMessageLogger.error_message(str);}
inline void  errno_message(const char *str) {  _globalMessageLogger.errno_message(str);}
inline void  fatal_message(const char *str) {  _globalMessageLogger.fatal_message(str);}
/*
#ifdef __STRING_HXX__
inline void  panic_message(const STRING& String) { log_message(iLOG_PANIC, String.c_str());}
inline void  error_message(const STRING& String) { log_message(iLOG_ERROR, String.c_str());}
inline void  errno_message(const STRING& String) { log_message(iLOG_ERRNO, String.c_str());}
inline void  fatal_message(const STRING& String) { log_message(iLOG_FATAL, String.c_str());}
#endif
*/

#define _IB_DEBUG  if (_ib_debug) message_log
extern bool _ib_debug;


////////////////////////////////// Threads ///////////////////////////////////////
//////
#ifdef  USE_pThreadLocker
class pThreadLocker {
public:
  pThreadLocker(pthread_mutex_t * mutex, const STRING& caller = NulString) {
    m_mutex = mutex;
    what = caller;
    if ((errno = pthread_mutex_lock(mutex)) != 0)
      {
        locked = false;
        // _globalMessageLogger.use_syslog();
        _globalMessageLogger.log_message(iLOG_ERRNO, "Error locking thread mutex");
      }
     else
      locked = true;
  }
  ~pThreadLocker() {
    if (locked) pthread_mutex_unlock(m_mutex);
  }
  bool Ok() const { return locked; }

private:
  pthread_mutex_t* m_mutex;
  bool             locked;
  STRING           what;
};
#else
#define pThreadLocker(_a, _b) /**/
#endif


#endif
