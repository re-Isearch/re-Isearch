/*@@@
File:		unisock.hxx
Version:	1.02
Description:	Multiplatform TCP/IP socket code.
Author:		Kevin Gamiel, kevin.gamiel@cnidr.org
	        Edward C. Zimmermann <edz@bsn.com>
Note:		Currently supports BSD and Winsock
@@@*/

#ifndef _UNISOCK_
#define _UNISOCK_

#include <ctype.h>
#include <iostream.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef SGI_CC
#include <bstring.h> // Apparently only needed for SGI's CC compiler
#endif


/*
Porting Information

You should define the appropriate value depending on your platform.
You're choices are:

#define UNISOCK_WINSOCK
#define UNISOCK_BSD
#define UNISOCK_MACTCP

I'll take a stab at figuring out what platform you are compiling on first
and if I fail, the compiler should bail out.
*/
// MS VC++
#if (defined _WINDOWS) | (defined _Windows) | (defined _MSDOS) | (defined _WIN32)
#define UNISOCK_WINSOCK
#endif

#if (defined UNIX) | (defined unix)
#define UNISOCK_BSD
#define BSD_COMP
#endif

#if (defined THINKC)
#define UNISOCK_MACTCP
#endif

#if (defined __AIX)
#define UNISOCK_AIX
#endif

// Timeout alarm clock timer for connect.
/*
#include <unistd.h>
#include <sys/time.h>
*/

#ifndef __OBJECTCENTER__
#if (!defined UNISOCK_WINSOCK) & (!defined UNISOCK_BSD) & (!defined UNISOCK_MACTCP) & (!defined UNISOCK_AIX)
#error YOU MUST DEFINE A UNISOCK PLATFORM - See unisock.hxx
#endif
#endif

#ifdef UNISOCK_BSD
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
extern "C" {
#include <netdb.h>
}
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/uio.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/time.h>
#define TSOCKET INT
#define SOCKADDR struct sockaddr

#if defined (socklen_t)
#define SOCKLEN_T socklen_t
#else
#define SOCKLEN_T UINT
#endif

#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (TSOCKET)(~0)
#endif /* UNISOCK_BSD */

#ifdef UNISOCK_AIX
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
extern "C" {
#include <netdb.h>
}
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/time.h>
#define TSOCKET INT
#define SOCKADDR struct sockaddr

#if defined (socklen_t)
#define SOCKLEN_T socklen_t
#else
#define SOCKLEN_T long unsigned int
#endif

#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (TSOCKET)(~0)
#endif /* UNISOCK_AIX */

#ifdef UNISOCK_WINSOCK
#include "windows.h"
#include "winsock.h"
#define TSOCKET SOCKET

#if defined (socklen_t)
#define SOCKLEN_T socklen_t
#else
#define SOCKLEN_T int
#endif

#endif /* UNISOCK_WINSOCK */

#include "defs.hxx"
#include "string.hxx"

// These values MUST NOT collide with errno values
#define UNI_BASE 2500
#define UNI_OK 0
#define UNI_UNCONNECTED UNI_BASE + 1
#define UNI_CONNECTED UNI_BASE + 2
#define UNI_PEERABORT UNI_BASE + 3
#define UNI_INVALID_HOST UNI_BASE + 4
#define UNI_NO_HOSTLOOKUP UNI_BASE + 5

class UNISOCK {
protected:
  TSOCKET c_socket;
  INT 	c_error,
        c_state,
	c_family,
	c_comm_type,
	c_protocol,
	c_reverse_name_lookup,
        c_timeout;
  STRING	c_hostname,
		c_ipaddress,
		c_addinfo;
  GDT_BOOLEAN	c_inetd,
		c_inetd_not_set_yet;
public:
  UNISOCK(INT family, INT comm_type, INT protocol);
  UNISOCK(INT family, INT comm_type, INT protocol, INT timeout);
  ~UNISOCK();

  UNISOCK & operator=(const UNISOCK & Other);
  
  void SetHostname(CHR *Hostname) { c_hostname = Hostname; }
  void GetHostname(STRING *Hostname) { *Hostname = c_hostname; }
  void SetIPAddress(CHR *Address) { c_ipaddress = Address; }
  void GetIPAddress(STRING *Address) { *Address = c_ipaddress; }

  // For TCP, connects to host.  For UDP, stores address for
  // subsequent sending of data.	
  void Connect(SOCKADDR *name);
  
//  void Listen(UINT Port, INT backlog);
  void Listen(UINT Port, INT backlog, const CHR *hostname = NULL);
  INT Accept(UNISOCK *NewSocket);
  INT IsConnected() { return c_state == UNI_CONNECTED; }
  void Close();

  INT Send(CHR *buf, INT len);
  INT Recv(CHR *buf, INT len);

  // Determine if and how much data is ready.
  INT DataReady(INT4 SecondsToWait = 0);
  INT4 DataReadyCount();
	
  // Simple error handling	
  INT LastError() { return c_error; }

  void ErrorMessage(CHR *msg, INT maxlen);
  void ErrorMessage(INT err, CHR *msg, INT maxlen);
	
  TSOCKET Socket() const { return c_socket; }
  void SetSocket(TSOCKET NewSocket, INT State=UNI_CONNECTED);
  //	void SetSocket(INT NewSocket, INT State=UNI_CONNECTED);
  
  void BlockingMode(INT f);
  void BlockingModeON() { BlockingMode(0); }
  void BlockingModeOFF() { BlockingMode(1); }

  GDT_BOOLEAN StartedByInetd();
};

GDT_BOOLEAN StartedByInetd();

// Well defined port numbers

// UDP Ports
#define PORT_ECHO       7
#define PORT_DISCARD    9
#define PORT_USERS      11
#define PORT_DAYTIME    13
#define PORT_NETSTAT    15
#define PORT_QUOTE      17
#define PORT_CHARGEN    19
#define PORT_TIME       37
#define PORT_NAMESERVER 42
#define PORT_NICNAME    43
#define PORT_DOMAIN     53
#define PORT_BOOTPS     67
#define PORT_BOOTPC     68
#define PORT_TFTP       69
#define PORT_HTTP       80
#define PORT_SUNRPC     111
#define PORT_NTP        123
#define PORT_SNMP       161
#define PORT_SNMP_TRAP  162
#define PORT_Z3950      210
#define PORT_BIFF       512
#define PORT_WHO        513
#define PORT_SYSLOG     514
#define PORT_TIMED      525

#endif

