/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#include "platform.h"

#ifdef _WIN32

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>

#if 0
# define MAX_PROCESS_TIME 60000 /*let it run for max. one minute*/
#else
# define MAX_PROCESS_TIME INFINITE /* let it run until finished */
#endif

int _IB_system (const char* strCommand, int Async)
{
  STARTUPINFO StartupInfo;
  PROCESS_INFORMATION ProcessInfo;
  char Args[4096];
  char *pEnvCMD = NULL;
  char *pDefaultCMD = "CMD.EXE";
  ULONG rc;

  memset(&StartupInfo, 0, sizeof(StartupInfo));
  StartupInfo.cb = sizeof(STARTUPINFO);
  StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
  StartupInfo.wShowWindow = SW_HIDE;

  Args[0] = 0;

  pEnvCMD = getenv("COMSPEC");

  if (pEnvCMD)
    {
      strcpy(Args, pEnvCMD);
    }
  else
    {
      strcpy(Args, pDefaultCMD);
    }

  /* "/c" option - Do the command then terminate the command window */
  strcat(Args, " /c ");
  /*the application you would like to run from the command window */
  strcat(Args, strCommand);

  if (!CreateProcess( NULL, Args, NULL, NULL, FALSE,
                      CREATE_NEW_CONSOLE,
                      NULL,
                      NULL,
                      &StartupInfo,
                      &ProcessInfo))
    {
      return GetLastError();
    }

  WaitForSingleObject(ProcessInfo.hProcess, MAX_PROCESS_TIME);
  if (!GetExitCodeProcess(ProcessInfo.hProcess, &rc))
    rc = 0;

  CloseHandle(ProcessInfo.hThread);
  CloseHandle(ProcessInfo.hProcess);

  return rc;
}


int _IB_system (const char * const *argv, int Async)
{
  char    cmd[4096];
  size_t  maxLen   = 4096;
  size_t  totalLen = 0;
  cmd[0] = '\0';
  for (size_t i = 0; i < 1024 && argv[i]; i++)
    {
      size_t len = strlen(argv[i]);

      totalLen += len + 3;
      if (totalLen > maxLen) break; // Too long

      if (i) strcat(cmd, " \"");
      strcat(cmd, argv[i]);
      if (i) strcat(cmd, "\"");
    }
  return _IB_system(cmd, Async);
}


#else


#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

#ifndef __GNUG__
# ifndef BSD
#  ifndef NO_ALLOCA_H
#   include <alloca.h>
#  endif
# endif
#endif

#ifndef BUFSIZ
# define BUFSIZ 1024*4
#endif

#if defined(sun) || defined(__ultrix) || defined(__bsdi__) || defined (__linux__) || defined(linux) || defined(BSD)
# define FORK vfork
#else
# define FORK fork
#endif


//#if !defined(SVR4) && !defined(sun) && !defined(__sgi) && !defined(__hpux) && !defined(BSD)
//# define WAIT(_x)   wait ((union wait *)(_x))
//#else
# define WAIT(_x)   wait(_x)
//#endif


extern void _IB_ErrnoMessage(const char *);
extern void _IB_WarningMessage(const char *);
extern void _IB_PanicMessage(const char *);

//extern char  *Copystring (const void *s);

#include "common.hxx"
#include <alloca.h>


/* Process */
int _IB_system (const char * const *argv, int Async)
{
  if (*argv == NULL)
    return 0; // Nothing???
  // Run a program the recomended way under X (XView)

  int to_subprocess[2], from_subprocess[2];

  pipe(to_subprocess); pipe(from_subprocess);

  /* fork the process */
  pid_t pid = FORK ();
  if (pid == -1)
    {
      _IB_ErrnoMessage("fork failed");
      return -1;
    }
  else if (pid == 0)
    {
      // Copy pipe descriptors to stdin and stdout
      dup2(to_subprocess[0], fileno(stdin));
      dup2(from_subprocess[1], fileno(stdout));

      // Close unwanted descriptors
      close(to_subprocess[0]);   close(to_subprocess[1]);
      close(from_subprocess[0]); close(from_subprocess[1]);

      /* child */
      execvp (*argv, (char* const*)argv); // Run the command
      if (errno == ENOENT)
        {
          char tmp[BUFSIZ];
          sprintf(tmp, "%s: command not found", *argv);
          _IB_WarningMessage(tmp);
        }
      else
        {
          char tmp[BUFSIZ];
          sprintf(tmp, "could not execute '%s'", *argv);
          _IB_ErrnoMessage( tmp );
        }
      _exit(2); //  _exit (-1);
    }

  // Code below is NOT really acceptable!
  // One should NEVER use wait under X
  // Ideas? A Sleep idle callback?

  // WARNING: WARNING: WARNING: WARNING:
  // The CODE BELOW IS BAD BAD BAD BAD!
  if (Async)
    {
      int status;
      while (WAIT(&status) != pid)
        sleep(1);
    }

  return 0;
}

int _IB_system (const char *command, int Async)
{
  if (command == NULL || *command == '\0')
    return 0; // Nothing to do

  if (strchr(command, '<') || /* piping in */
      strchr(command, '>') || /* piping out */
      strchr(command, '|') /* bipipe */)
    return system(command); // Needs to run under a shell

  unsigned argc = 0;
  char *argv[1024];
  const char *IFS = " \t\n";

#ifdef __GNUG__
  size_t      len = strlen(command)+1;
  char        tmp[len+1];
  memcpy(tmp, command, len);

#else
#ifdef NO_ALLOCA
// Build argument vector
char *tmp = Copystring((void *)command);
#else
size_t      len = strlen(command)+1;
char       *tmp = (char *)alloca(len);

if (tmp == NULL)
  {
    _IB_ErrnoMessage("_IB_system could not allocate bytes from stack!");
    return 0;
  }

memcpy(tmp, command, len);
#endif
#endif

  char *tcp = tmp;
  int   quote = 0;

  do
    {
      argv[argc++] = tcp;

      if (argc >= sizeof(argv)/sizeof(argv[0])-1)
        {
          _IB_WarningMessage("_IB_system: Too many args");
          break;
        }

      for (;*++tcp;)
        {
          if (quote)
            {
              if (*tcp == quote)
                quote = 0;
            }
          else if (*tcp == '"' || *tcp == '\'')
            quote = *tcp;
          else if (isspace(*tcp))
            {
              do {*tcp++ = '\0'; }
              while (isspace(*tcp));
              break; // Break out
            }
        } // for(;;)
    }
  while (*tcp);
  argv[argc] = NULL;


  int res = _IB_system(argv, Async);

#ifndef __GNUG__
# ifdef NO_ALLOCA
  delete[] tmp;
# endif
#endif
  return res;
}
#endif

#ifdef TEST

#include <iostream.h>


int main(int argc, char **argv)
{
  char *arg = argv[1];

  if (arg)
    {
      cout << argv[0] << ": Run " << arg << "... ";
      if (_IB_system(arg, 1) == 0)
        cout << "OK";
      else
        cout << "FAILED";
      cout << endl;
    }
  return 0;
}


#endif
