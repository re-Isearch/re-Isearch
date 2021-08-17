/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#ifndef _LOG_HXX
#define _LOG_HXX

#ifndef GDT_SYS_H
# include "gdt-sys.h"
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

#ifndef _LOG_CXX
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

GDT_BOOLEAN set_syslog(const char *name);
GDT_BOOLEAN set_syslog(const int Ch);

GDT_BOOLEAN  log_init (FILE *fp);
GDT_BOOLEAN  log_init (const char *prefix, const char *name=0);
GDT_BOOLEAN  log_init (const char *prefix, FILE *fp);
GDT_BOOLEAN  log_init (int level, FILE *fp);
GDT_BOOLEAN  log_init (int level, const char *prefix, FILE *fp);
GDT_BOOLEAN  log_init (int level, const char *prefix=0, const char *name=0);

void  message_log (int level, const char *fmt, ...);
void  log_message(int level, const char *string);

#ifdef __STRING_HXX__
inline void  panic_message(const STRING& String) { log_message(iLOG_PANIC, String.c_str());}
inline void  error_message(const STRING& String) { log_message(iLOG_ERROR, String.c_str());}
inline void  fatal_message(const STRING& String) { log_message(iLOG_FATAL, String.c_str());}
#endif

int   log_mask_str (const char *str);

#define _IB_DEBUG  if (_ib_debug) message_log

extern GDT_BOOLEAN _ib_debug;

#endif		/* _LOG_HXX */
