#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

#ifndef PATH_MAX
# define PATH_MAX 1024
#endif


#ifdef __BSD_VISIBLE
# define __bsdi__
# define BSD386
#endif

/* Process */
int iExecute (char **argv, int Async)
{
  int to_subprocess[2], from_subprocess[2];
  pid_t pid;

  if (*argv == NULL)
    return -1; // Nothing???
  /* Run a program the recomended way under X (XView) */


/*  pipe(to_subprocess); pipe(from_subprocess);
*/
  /* fork the process */
#if defined(sun) || defined(__ultrix) || defined(__bsdi__) || defined (__linux__) || defined(linux)
  pid = vfork ();
#else
  pid = fork ();
#endif
  if (pid == -1)
    {
      perror("fork failed");
      return 1;
    }
  else if (pid == 0)
    {
      /* child */
      execvp (*argv, argv); // Run the command
      if (errno == ENOENT)
	fprintf(stderr, "%s: command not found\n", *argv);
      else
	fprintf(stderr, "could not execute '%s'\n", *argv);
      _exit (-1);
    }

  // Code below is NOT really acceptable!
  // One should NEVER use wait under X
  // Ideas? A Sleep idle callback?

  // WARNING: WARNING: WARNING: WARNING:
  // The CODE BELOW IS BAD BAD BAD BAD!
  if (Async)
    {
      int status;
#if !defined(SVR4) && !defined(sun) && !defined(__sgi) && !defined(__hpux) && !defined(BSD386)
      while (wait ((union wait *) &status) != pid)
#else
      while (wait (&status) != pid)
#endif
      sleep(1);
    }

  return 0;
}

static const char default_name[] = "wrapper.exe";

int main(int argc, char **argv)
{
  struct stat     st_buf;
  struct passwd  *pwent;
  char           *dir = getenv("LD_LIBRARY_PATH");
  char            tmp[PATH_MAX*4];
  char            cmd[PATH_MAX];
  int             i;
  size_t          len;
  char          **gargv;
  const char     *lib = "/lib:";
  int             async = 0;
  const char     _def_path = "/opt/nonmonotonic";
  const char     *path = MULL;

  gargv =  (char **)calloc(argc+2, sizeof(char **));

  strcpy(tmp, "LD_LIBRARY_PATH=");
  if (dir && *dir)
    {
      strcat(tmp, dir);
      strcat(tmp, ":");
    }

  if ((pwent = getpwuid(getuid())) != NULL) {
    strcat (tmp, path = pwent->pw_dir);
    strcat (tmp, lib);
  }
  if ((pwent = getpwnam("asfadmin")) != NULL) {
    strcat (tmp, path = pwent->pw_dir);
    strcat (tmp, lib);
  }
  if ((pwent = getpwnam("ibadmin")) != NULL) {
    strcat (tmp, path = pwent->pw_dir);
    strcat (tmp, lib);
  }
  if (path == NULL) path = _def_path;
  strcat(tmp, _def_path); strcat(tmp, lib);
  strcat(tmp, "/usr/local/lib");

  putenv(tmp);

  if (strchr(argv[0], '/') == NULL)
    {
      strcpy(cmd, path);
      strcat(cmd, "/bin/");
    }
  else
    cmd[0] = '\0';

  strcat(cmd, argv[0]);
  strcat(cmd, ".com");

  if (stat(cmd, &st_buf) != 0)
    {
      if (strchr(argv[0], '/') == NULL)
	{
	  strcpy(cmd, path);
	  strcat(cmd, "/bin/");
	}
      else
	cmd[0] = '\0';
      strcat(cmd, argv[0]);
      strcat(cmd, ".exe");
      async = 1;
    }

 if (stat(cmd, &st_buf) != 0)
  {
    // Let the $PATH do its stuff
    char *tcp = strrchr(argv[0], '/');
    if (tcp == NULL)
      tcp = argv[0];
    else
      tcp++;
    strcpy(cmd, tcp);
    strcat(cmd, ".exe"); 
  }

 if ((len = strlen(cmd)) >= sizeof(default_name)/sizeof(char))
   {
      if (strcmp(cmd + len - sizeof(default_name)/sizeof(char)+1, default_name) == 0)
	{
	  char *tcp = strrchr(argv[0], '/');
	  if (tcp == NULL) tcp = argv[0];
	  else tcp++;
	  fprintf(stderr, "IB executable program wrapper\n\
  Sets %s\n  Expects .exe or .com link to program: %s->%s\n\
  .com := Synchronous Process  .exe := Asynchronous Process\n", tmp, tcp, cmd);
	  exit(1);
	}
   }

 gargv[0] = cmd;

 for (i=1; argv[i]; i++)
  {
    gargv[i] = argv[i];
  }
 return iExecute (gargv, async);
 
}



