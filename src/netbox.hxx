/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
/*@@@
File:           netbox.hxx
Version:        1.01.01
Description:    Socket-interfacing network functions
Author:         Nassib Nassar; Kevin Gamiel
@@@*/

#ifndef NETBOX_HXX
#define NETBOX_HXX

#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef PLATFORM_WINDOWS
#include <sys\types.h>
#include "winsock.h"
#define MAXINT 32767
#else
#include <unistd.h>
#include <sys/types.h>
#include <arpa/nameser.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#endif
#ifndef BSD
#include <malloc.h>
#endif
#include "defs.hxx"
//#include "platform.h"

/* Global definitions */

#if !defined(MAXINT)
#define MAXINT 2147483647

#define SOCKET int
#define SOCKADDR_IN struct sockaddr_in
#define LPHOSTENT struct hostent*
#define LPSERVENT struct servent*
#define INVALID_SOCKET (SOCKET)(-1)
#define SOCKET_ERROR (-1)
#define closesocket(s) close(s)
#endif

#ifndef FAR
#define FAR
#endif

/* Public typedefs & vars */
#define NB_INETD 0
#define NB_COMMANDLINE	1

class NETBOXPROFILE {
private:
  SOCKET Socket;	/* Resulting value */
  long   TimeOutSec;	/* Must be set by user */
  long   TimeOutUSec;	/* Must be set by user */
  char   HostName[256];	/* Must be set by user */
  int    ServerOwner;	/* Resulting value. 0=INETD, 1=Command Line */
  char   PeerName[256];	/* Resulting value */
  int    Port;		/* Must be set by user */

public:
  NETBOXPROFILE();
  ~NETBOXPROFILE();
  NETBOXPROFILE* Create();
  void Destroy();
  char FAR *SetHostName(char FAR* Name);
  int SetPort(INT Port);
  char FAR *GetHostName();
  int GetPort();
  long SetTimeOutSec(off_t Seconds);
  long SetTimeOutUSec(off_t USeconds);
  INT WSAStartup(void);
  void WSACleanup(void);
  INT SetBufferSize(INT BufferSize);
  INT Open();
  INT Close();
  INT SendBuffer(char FAR* Buffer, INT BufferSize);
  INT iGetBuffer(char FAR* Buffer, INT BufferSize);
  off_t GetBuffer(char FAR* Buffer, off_t BufferSize);
  INT WaitForData();
  INT StartServer();
  NETBOXPROFILE* AcceptClient();
  INT CurrentStatus();
  INT GetLine(char *Buffer, INT MaxLen);
};

typedef NETBOXPROFILE *PNETBOXPROFILE;

#endif
