#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>

#define SVR4 1

void Sleep (int sec, int usec)
{
#if defined(SVR4)
  sigset_t oldmask, mask;
  struct timeval tv;
#else
  int oldmask, mask;
  struct timeval tv;
#endif
  if (usec >= 1000000) {
    sec++, usec %= 1000000;
  }
#if defined(SVR4)
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


int main(int argc, char **argv)
{
  struct stat st_buf;
  time_t last = -1;
  char **gargv = argv + 1;
  int tick = 5*60;
  int pause = 10;
  char *cwd = ".";

  if (argc == 1)
    {
      printf("Iwatch: Run a program when a file is added or deleted from dir.\n");
error:
      printf("Usage: %s [-s sec] [-d dir] command args ...\n", argv[0]);
      printf(" [-s sec]\tNumber of seconds between checks (%d)\n", tick);
      printf(" [-d dir]\tDirectory to watch ('%s')\n", cwd ? cwd : ".");
      printf(" [-p sec]\tNumber of seconds to not look after command (%d)\n", pause);
      exit(1);
    }
  while (gargv[0] && gargv[0][0] == '-')
    {
      if (gargv[0][1] == 's')
	{
	  if ((tick = atoi(gargv[1])) <= 0)
	    goto error;
	}
      else if (gargv[0][1] == 'p')
	{
	  if ((pause = atoi(gargv[1])) <= 0)
	    goto error;
	}
      else if (gargv[0][1] == 'd')
	{
	  cwd = gargv[1];
	  if (cwd == NULL || *cwd == '\0')
	    goto error;
	}
      gargv += 2, argc -=2;
    }
  if (gargv[0] == NULL) goto error;
  printf("Watching '%s' %d seconds to run %s\n", cwd, tick, gargv[0]);

  if (stat(cwd, &st_buf) != 0)
    {
      perror("stat");
      exit(1);
    }
  last = st_buf.st_mtime; 

  for (;;)
    {
      Sleep(tick, 0);
      if (stat(cwd, &st_buf) == 0)
	{
	  if (st_buf.st_mtime != last)
	    {
	      pid_t pid, w;
	      last = st_buf.st_mtime;
	      switch( pid = fork() )
		{
		  case 0:
		    execvp(*gargv, gargv);
		    perror("exec");
		    _exit(1);
		  case -1: printf("%s: Can't fork\n", argv[0]);
		  default:
		      do {
			int status;
			w = wait(&status);
		      } while (w != pid && w != -1);
		}
	      if (pause) Sleep(pause, 0);
	      stat(cwd, &st_buf);
	      last = st_buf.st_mtime;
	    }
	}
      else perror("stat");
    }
  return 0;
}
