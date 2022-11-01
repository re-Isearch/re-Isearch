/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)lock.cxx  1.15 08/07/01 14:01:19 BSN"



#include "defs.hxx"
#include "lock.hxx"
#include "common.hxx"

extern "C" {
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
}

#ifdef _WIN32
# define PLATFORM_WINDOWS
#endif

#ifdef PLATFORM_WINDOWS

#include <winsock2.h>
#include <sys/stat.h>

#undef getpid
#define getpid (int)GetCurrentProcessId  /* cast DWORD to int */

static const char _LOCK_TEMPLATE[] = "%s__LC%c";
static const char _PID_TEMPLATE[]  = "%s__%lu";

// void extern Sleep (DWORD dwMilliseconds); 

// Windows Sleep is ms
static inline void mySleep (int sec, int usec)
{
#ifdef _WIN32
  message_log (LOG_DEBUG, "Sleep %d seconds", sec);
#endif
  Sleep(100*sec+usec);
#ifdef _WIN32
  message_log (LOG_DEBUG, "awake..");
#endif
}

#define L_cuserid 256

#else

static const char _LOCK_TEMPLATE[] = "%s..LC%c";
static const char _PID_TEMPLATE[]  = "%s..%lu";

extern "C" {
#include <unistd.h>
#include <sys/file.h>
#include "dirent.hxx"
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
}



static inline void mySleep (int sec, int usec)
{
  if (usec >= 1000000)
    sec++, usec %= 1000000;
#if defined(SVR4)
  sigset_t oldmask, mask;
  struct timeval tv;

  tv.tv_sec = sec;
  tv.tv_usec = usec;

  sigemptyset (&mask);
  sigaddset (&mask, SIGIO);
  sigaddset (&mask, SIGALRM);
  sigprocmask (SIG_BLOCK, &mask, &oldmask);
  if ((select (0, 0, 0, 0, &tv)) == -1)
    {
      perror ("select in Sleep");
    }
  sigprocmask (SIG_SETMASK, &oldmask, (sigset_t *) NULL);
#else
  int oldmask, mask;
  struct timeval tv;

  tv.tv_sec = sec;
  tv.tv_usec = usec;

  mask = sigmask (SIGIO);
  mask |= sigmask (SIGALRM);
  oldmask = sigblock (mask);
  if ((select (0, 0, 0, 0, &tv)) == -1)
    {
      perror ("select in Sleep");
    }
  sigsetmask (oldmask);
#endif
}

#endif

static inline void rand_wait(int scale = 0)
{
  int wait, sec;
  srand((int)time(0));

  wait = (rand() & 0xFFFFF)*100;
  sec = (rand() & 3) * scale;
  mySleep(sec, wait);
}


/* 
 * uucp style locking routines
 * return: 0 - success
 *        -1 - failure
 */
static int db_lockactive (const char *lfile, int pid);
static int db_dolock (char *pid, char *tfile, char *lfile);

static char LockType (int Lock)
{
  switch (Lock&0x0F)
    {
    case L_WRITE:
      return 'K';
    case L_READ:
      return 'S';
    case L_APPEND:
      return 'A';
    default:
      return '\000';
    }
}

#ifndef MAXPIDLEN
# define MAXPIDLEN	10
#endif
#ifndef MAXNAMLEN
# define MAXNAMLEN	1024
#endif
#ifndef MAXUSERNAMLEN
# define MAXUSERNAMLEN	256
#endif
#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 256
#endif

static int db_lock (const char *dbname, int Lock)
{
  char entry[MAXPIDLEN + 5 + L_cuserid + MAXUSERNAMLEN + MAXHOSTNAMELEN];
  char whoami[L_cuserid + 2];
  char fullname[MAXUSERNAMLEN];
  char tfile[256 + MAXNAMLEN];
  char lfile[256 + MAXNAMLEN];
  char host[MAXHOSTNAMELEN];

  /* 127/edz/furball/ */
  if (!_IB_GetUserId(whoami, sizeof(whoami)-1))
    strcpy(whoami, "?");
  if (!_IB_GetUserName(fullname, sizeof(fullname)-1))
    strcpy(fullname, "?");
  /* lock file form: pid/user/host */
  if (!_IB_GetHostName (host, (int)(sizeof(host)/sizeof(char)-1)))
    strcpy(host, "localhost");
  (void) sprintf (entry, "%*lu/%s/%s/%s/\n", MAXPIDLEN, (long)getpid (), whoami, host, fullname);
  (void) sprintf (tfile, _PID_TEMPLATE, dbname, (long)getpid ());
  (void) sprintf (lfile, _LOCK_TEMPLATE, dbname, LockType (Lock));

  /*
   * If the lock fails, check to see if it is currently locked
   * by a valid process.  If not, try the lock again.
   */
  errno = 0;
#ifndef _WIN32
  rand_wait(); // Wait a random amount of time
#endif
  if (db_dolock (entry, tfile, lfile))
    {
      rand_wait();
      if (db_lockactive (lfile, 0))
	{
	  errno = 0;
	  return -1;
	}
      if (db_dolock (entry, tfile, lfile))
	{
	  return -1;
	}
    }
  return 0;
}

static int db_lockactive (const char *lfile, int zap)
{
  int         lf;
  char        pid_str[MAXPIDLEN + 2];
  int         pid, r;
  time_t      difftime = 0;
  struct stat sb;

  /*
   * Make sure the file is readable and contains a valid PID.
   * If not, and we can unlink it, then return success.
   */
  lf = open (lfile, O_RDONLY, 0);
  if (lf < 0)
    {
      return (errno == ENOENT) ? 0 : unlink (lfile);
    }

  if ((r = read (lf, pid_str, MAXPIDLEN + 1)) > 0)
    pid_str[r-1] = '\0';
  else
    pid_str[0] = 0;

  //time_t mtime = 0;

  if (fstat (lf, &sb) >= 0)
    difftime = (time((time_t *)0) - sb.st_mtime)/60;

  (void) close (lf);

  if (r != MAXPIDLEN + 1)
    {
      if (unlink(lfile) == -1)
	message_log (LOG_ERRNO, "Can't remove bogus lockfile '%s", lfile);
      return 0;
    }
  if ((pid = atoi (pid_str)) == getpid())
    return 0; // my lock

  if (pid <= 0 || pid == zap)
    {
      // Bad lock
      if (unlink(lfile) == -1)
	message_log (LOG_ERRNO, "Can't remove old lock '%s'", lfile);
      return 0;
    }
  /*
   * We have a found a seemingly valid pid.
   * Make sure the process is still around.
   */
  if (!IsRunningProcess(pid))
    {
      // Stale lock
      if (unlink (lfile) == -1)
	message_log (LOG_ERRNO, "Can't remove stale lock '%s'", lfile);
      return 0;
    }
  if (difftime > 61)
    message_log (LOG_WARN, "Lock '%s' is active for over %ld hour(s) (pid=%ld). Zombie?", lfile, difftime/60, (long)pid);
  else
    message_log (LOG_INFO, "Lock '%s' is active %ld minutes (pid=%ld)", lfile, difftime, (long)pid);
  return -1; // Have an active lock
}

static int db_dolock (char *entry, char *tfile, char *lfile)
{
  errno = 0;
  int tf = tf = open (tfile, O_WRONLY | O_CREAT | O_TRUNC, 0444);
  if (tf >= 0)
    {
      write (tf, entry, strlen(entry));
#ifndef _WIN32
      fchmod (tf, 0444);
#endif
      close (tf);

      if (db_lockactive (lfile, 0) == 0)
	{
#ifdef _WIN32
	  rename(tfile, lfile);
#else
	  if (link(tfile, lfile) < 0)
	    {
	      // Did some process beat us?
	      if (db_lockactive(lfile, 0) == -1)
		{
		  unlink (tfile);
		  return -1;
		}
	      // hmm...
	    }
	  unlink (tfile);
#endif
	  return 0;
	}
    }
  else if (errno == EACCES)
    {
      message_log (LOG_ERRNO, "Don't have permission to set lockfile '%s", lfile);
//    return 0;
    }
  unlink (tfile);
  return -1;
}

// Check if we have the lock, if so remove it
static int db_unlock (const char *dbname, int Lock)
{
  int lf;
  char pid_str[MAXPIDLEN + 2];
  int pid, r;
  char lfile[256 + MAXNAMLEN];

//  if (Lock == L_APPEND) db_unlock(dbname, L_WRITE);

  (void) sprintf (lfile, _LOCK_TEMPLATE, dbname, LockType (Lock));

  if ((lf = open (lfile, O_RDONLY, 0)) < 0)
    return (errno == ENOENT) ? 0 : unlink (lfile);
  if ((r = read (lf, pid_str, MAXPIDLEN + 1)) > 0)
   pid_str[r-1] = '\0';
  else
   pid_str[0] = 0;

  (void) close (lf);

  if (r != MAXPIDLEN + 1)
    return unlink (lfile);
  pid = atoi (pid_str);
  /* My lock?  */
//cerr << "Pid = " << pid << " mypid = " << getpid() << endl;
  if (pid == getpid () )
    return unlink (lfile);

//cerr << "NOT MY LOCK" << endl;
  return -1; // Nope, not my lock
}

static bool SetLock (const char *dbFileStem, int Lock)
{
  int i = 0;
  int nap = Lock != L_READ ? 5 : 2;
  const int timeout = 30;
  errno = 0;
  while (-1 == db_lock (dbFileStem, Lock))
    {
      if (++i > timeout)
	{
	  return false;	// Timeout;
	}
      rand_wait (nap);
    }
  return true;
}

INT Lock (const char *dbFileStem, int Flags)
{
  int stat = 0x00;
  errno = 0;
  if (Flags & L_WRITE)
    {
      if (Flags & L_CHECK)
	{
	  if ((LockWait (dbFileStem, L_WRITE, 0) & L_WRITE) == L_WRITE)
	    stat |= L_WRITE;
	}
      else
	{
	  if (SetLock (dbFileStem, L_WRITE))
	    stat |= L_WRITE;
	}
    }
  if (Flags & L_READ)
    {
      if (Flags & L_CHECK)
	{
	  if ((LockWait (dbFileStem, L_READ, 0) & L_READ) == L_READ)
	    stat |= L_READ;
	}
      else
	{
	  if (SetLock (dbFileStem, L_READ))
	    stat |= L_READ;
	}
    }
  if (Flags & L_APPEND)
    {
      if (Flags & L_CHECK)
	{
	  if ((LockWait (dbFileStem, L_APPEND) & L_APPEND) == L_APPEND)
	    stat |= L_APPEND;
	}
      else
	{
	  if (SetLock (dbFileStem, L_APPEND))
	    stat |= L_APPEND;
	}
    }

  if (Flags == 0x00)
    {
      if ((LockWait (dbFileStem, L_WRITE, 0) & L_WRITE) == L_WRITE)
	stat |= L_WRITE;
      if ((LockWait (dbFileStem, L_READ, 0) & L_READ) == L_READ)
	stat |= L_READ;
      if ((LockWait (dbFileStem, L_APPEND, 0) & L_APPEND) == L_APPEND)
	stat |= L_APPEND;
    }

  return stat;
}

INT UnLock (const char *dbFileStem, int Flags)
{
  int res = 0x00;
  if ((Flags & L_READ) && db_unlock (dbFileStem, L_READ) == 0)
    res |= L_READ;
  if ((Flags & L_WRITE) && db_unlock (dbFileStem, L_WRITE) == 0)
    res |= L_WRITE;
  if ((Flags & L_APPEND) && db_unlock (dbFileStem, L_APPEND) == 0)
    res |= L_APPEND;
  return res;
}

static bool waitlock (const char *lfile, int secs)
{
  int i = secs ? (secs / 10 + 1) : 0;
  bool res = true;
  while (db_lockactive (lfile, 0))
    {
      if (i-- == 0)
	{
	  res = false;
	  break;
	}
      rand_wait (5);
    }
  return res;
}

INT LockWait (const char *DbFileStem, const int Flags, const int secs)
{
  char lfile[256 + MAXNAMLEN];
  int res = Flags & 0x0F;

  if (Flags & L_READ)
    {
      sprintf (lfile, _LOCK_TEMPLATE, DbFileStem, LockType (L_READ));
      if (waitlock (lfile, secs))
	{
	  res ^= L_READ;
	}
    }
  if (Flags & L_WRITE)
    {
      sprintf (lfile, _LOCK_TEMPLATE, DbFileStem, LockType (L_WRITE));
      if (waitlock (lfile, secs))
	{
	  res ^= L_WRITE;
	}
    }
  if (Flags & L_APPEND)
    {
      sprintf (lfile, _LOCK_TEMPLATE, DbFileStem, LockType (L_APPEND));
      if (waitlock (lfile, secs))
	{
	  res ^= L_APPEND;
	}
    }
  return res;
}
