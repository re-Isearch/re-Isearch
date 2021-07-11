/*@@@
File:		unisock.cxx
Version:	1.02
Description:	Multiplatform TCP/IP Socket code.
Author:		Kevin Gamiel, Kevin.Gamiel@cnidr.org
	        Edward C. Zimmermann <edz@nonmonotonic.com>
@@@*/

#include "unisock.hxx"


#ifdef HAVE_SIGNAL
static jmp_buf sig_buf;

#if defined(CYGWIN) || defined(SOLARIS) || defined(_BSD)
static int sigsave;
#else
static __sighandler_t sigsave;
#endif

static struct itimerval itimer;
static int timer_timeouts;
struct sigaction newaction, oldaction;
 
static void alarm_handler (int x)
{
  timer_timeouts += 1;
  //  fprintf (stdout,"Timeout number %d\n", timer_timeouts);
  //  fflush (stdout);
  itimer.it_interval.tv_sec = 0;
  itimer.it_interval.tv_usec = 0;
  itimer.it_value.tv_sec = 0;
  itimer.it_value.tv_usec = 0;
  if (sigsave) {

#if defined(CYGWIN) || defined(SOLARIS) || defined(_BSD)
    signal (SIGALRM, (void (*)(int))sigsave);
#else
    signal (SIGALRM, sigsave);
#endif

    sigsave = 0;
    longjmp (sig_buf, 1);
  }
  abort ();
}
#endif // HAVE_SIGNAL

#ifdef UNISOCK_WINSOCK
#define EADDRNOTAVAIL WSAEADDRNOTAVAIL
#define EAFNOSUPPORT WSAEAFNOSUPPORT

#define errno WSAGetLastError()
#define close(a) closesocket(a)

#endif


/*
** These 2 definitions check to see if the socket is in an unconnected
** state and if so, sets an error code and returns.  As with all the
** unisock methods, the caller should always call UNISOCK::LastError()
** after each unisock call to determine the error state.
*/
#define RET_IF_SOCK_CLOSED if(c_state == UNI_UNCONNECTED) {\
	c_error = c_state;\
	return;\
	} else c_error = UNI_OK;

#define RET_VAL_IF_SOCK_CLOSED if(c_state == UNI_UNCONNECTED) {\
	c_error = c_state;\
	return 0;\
	} else c_error = UNI_OK;

/*
Desc:	Constructor for socket
Pre:	family = One of the supported address families (usually PF_INET)
		BSD - See /usr/include/sys/socket.h for AF_ values
		Winsock - Only supports PF_INET 

	comm_type - SOCK_STREAM | SOCK_DGRAM
		BSD - Supports SOCK_RAW

	protocol - 0 for no protocol (typical) or other value if you
		know what you are doing
Post:	Check UNISOCK::LastError() for error state.
	If error state == UNI_OK then a socket has been allocated
*/
UNISOCK::UNISOCK(INT family, INT comm_type, INT protocol = 0)
{
  c_family = family;
  c_comm_type = comm_type;
  c_protocol = protocol;
  c_error = UNI_OK;
  c_state = UNI_UNCONNECTED;
  c_reverse_name_lookup = 1;
//  c_hostname = NulString;
//  c_ipaddress = NulString;
  c_inetd_not_set_yet = GDT_TRUE;
  c_inetd = GDT_FALSE;
#ifdef UNISOCK_WINSOCK
  WORD VersionReqd;
  WSADATA WSAData;
	
  VersionReqd = 0x0101;
  WSAStartup(VersionReqd, &WSAData);
#endif
}


/*
Desc:	Constructor for socket, with timeout
Pre:	family = One of the supported address families (usually PF_INET)
		BSD - See /usr/include/sys/socket.h for AF_ values
		Winsock - Only supports PF_INET 

	comm_type - SOCK_STREAM | SOCK_DGRAM
		BSD - Supports SOCK_RAW

	protocol - 0 for no protocol (typical) or other value if you
		know what you are doing
Post:	Check UNISOCK::LastError() for error state.
	If error state == UNI_OK then a socket has been allocated
*/
UNISOCK::UNISOCK(INT family, INT comm_type, INT protocol = 0, INT timeout = 3600)
{
  c_family = family;
  c_comm_type = comm_type;
  c_protocol = protocol;
  c_error = UNI_OK;
  c_state = UNI_UNCONNECTED;
  c_reverse_name_lookup = 1;
//  c_hostname = NulString;
//  c_ipaddress = NulString;
  c_inetd_not_set_yet = GDT_TRUE;
  c_inetd = GDT_FALSE;
  c_timeout = timeout;

#ifdef UNISOCK_WINSOCK
  WORD VersionReqd;
  WSADATA WSAData;
	
  VersionReqd = 0x0101;
  WSAStartup(VersionReqd, &WSAData);
#endif
}


/*
** Desc: Destructor for socket
*/
UNISOCK::~UNISOCK()
{
  if(c_state == UNI_CONNECTED)
    UNISOCK::Close();

#ifdef UNISOCK_WINSOCK
  WSACleanup();
#endif
}


/*
Desc:	Associates local address with remote address
Pre:	name = valid socket address for address family and protocol in use
Post:	Check UNISOCK::LastError() for error state.
	If error state == UNI_OK then connection was successful
*/
void UNISOCK::Connect(SOCKADDR *name)
{
  c_error = UNI_OK;

  if((c_socket = socket(c_family, c_comm_type, c_protocol)) == 
     INVALID_SOCKET) {
    c_error = errno;
    return;
  }

  // Connect to remote host

#ifdef HAVE_SIGNAL
  // Set up an alarm clock timer in case the connect fails.
  newaction.sa_handler = alarm_handler;
  sigaction (SIGALRM, &newaction, &oldaction);

#if defined(CYGWIN) || defined(SOLARIS) || defined(_BSD)
  signal (SIGALRM, (void (*)(int))sigsave);
#else
  signal (SIGALRM, sigsave);
#endif

  getitimer (ITIMER_REAL, &itimer);
  itimer.it_interval.tv_sec = c_timeout;
  itimer.it_interval.tv_usec = 0;
  itimer.it_value.tv_sec = c_timeout;
  itimer.it_value.tv_usec = 0;
  setitimer (ITIMER_REAL, &itimer, 0);

#if defined(CYGWIN) || defined(SOLARIS) || defined(_BSD)
  sigsave = (int)alarm_handler;
#else
  sigsave = alarm_handler;
#endif

  if (setjmp(sig_buf)) {
    errno = ETIMEDOUT;
    return;
  }
#endif // HAVE_SIGNAL
	
  if(connect(c_socket, name, sizeof(*name)) == SOCKET_ERROR) {
    // Set error flag
    c_error = errno;
    return;
  }

#ifdef HAVE_SIGNAL
  // Stop the alarm clock.
  itimer.it_interval.tv_sec = 0;
  itimer.it_interval.tv_usec = 0;
  itimer.it_value.tv_sec = 0;
  itimer.it_value.tv_usec = 0;
  sigaction (SIGALRM, &oldaction, 0);
  signal (SIGALRM, SIG_DFL);
#endif // HAVE_SIGNAL

  c_state = UNI_CONNECTED;
}

/*
Desc:	Send data to remote site
Pre:	Socket is connected
Post:	Returns SOCKET_ERROR on failure.  Call UNISOCK::LastError() for code.
	Otherwise, returns the actual number of bytes sent (may be less 
		than len)
*/
INT UNISOCK::Send(CHR *buf, INT len)
{
  RET_VAL_IF_SOCK_CLOSED

    INT err;
  err = send(c_socket, buf, len, 0);
  if(err == SOCKET_ERROR)
    c_error = errno;
	
  return err;
}

/*
Desc:	Reads up to len bytes from socket subsystem into buf
Pre:	Socket is connected
Post:	Returns 0 if remote side closed gracefully
	Returns SOCKET_ERROR on failure.  Call UNISOCK::LastError() for code.
	Otherwise returns the number of bytes copied into buf
*/
INT UNISOCK::Recv(CHR *buf, INT len)
{
  RET_VAL_IF_SOCK_CLOSED

  INT err;
  if ((err = recv(c_socket, buf, len, 0)) == SOCKET_ERROR)
    c_error = errno;
  return err;
}


void UNISOCK::Close()
{
  RET_IF_SOCK_CLOSED

    if(close(c_socket) == -1)
      c_error = errno;

  c_socket = (TSOCKET)-1;

  c_state = UNI_UNCONNECTED;
}


/*
Desc:	Polls socket for readability
Pre:	Socket is connected
	SecondsToWait - number of seconds to block (see Note)
Post:	Returns 1 if data available for guaranteed reading.
	Returns 0 if no data available.
	Returns SOCKET_ERROR if error.  Call UNISOCK::GetLastError() for 
		error code.
Note:	SecondsToWait of 0 will poll the socket and return instantly.
	SecondsToWait of -1 will block indefinitely
	Any other value of SecondsToWait will block only for that period of time
*/
INT UNISOCK::DataReady(INT4 SecondsToWait)
{
  if(c_state == UNI_UNCONNECTED) {
    c_error = c_state;
    return SOCKET_ERROR;
  } else c_error = UNI_OK;

  // Initialize the set of sockets to check for readability
  fd_set ReadFDS;
  FD_ZERO(&ReadFDS);
  FD_SET(c_socket, &ReadFDS);
  // Set select time 
  struct timeval *TimeVal;
  if(SecondsToWait < 0)
    // Block indefinitely
    TimeVal = (struct timeval *)NULL;
  else {
    TimeVal = new timeval;
    TimeVal->tv_sec = SecondsToWait;
    TimeVal->tv_usec = 0;
  }
	
  // Check the socket for readability.
  INT err;
#ifdef _HP_
  err = select(c_socket + 1, (int *)&ReadFDS, (int *)NULL, (int *)NULL, TimeVal);
#else
  err = select(c_socket + 1, (fd_set *)&ReadFDS, (fd_set *)NULL, (fd_set *)NULL, TimeVal);
#endif
  if(TimeVal)
    delete TimeVal;

  switch(err) {
  case SOCKET_ERROR:
    // Peer aborted connection
    c_error = errno;
    break;
  case 0:
    // Timed out.  No data ready for reading
    return 0;
  default:
    // _Some_ socket has data to be read 
    if(FD_ISSET(c_socket, &ReadFDS) == 0)
      // It wasn't our socket
      return 0;

    // Our socket has some data to read!
    return 1;
  }

  return SOCKET_ERROR;
}


/*
Desc:	Determines the number of bytes that can be read immediately with 
		UNISOCK::Recv()
Pre:	Socket is connected
Post:	Returns >= 0 on success, indicating the number of bytes ready for
		reading with UNISOCK::Recv()
	Returns SOCKET_ERROR on error.  Call UNISOCK::GetLastError() for 
		error code
*/
INT4 UNISOCK::DataReadyCount()
{
  RET_VAL_IF_SOCK_CLOSED

    INT Err;
#ifdef UNISOCK_WINSOCK
  u_long BytesAvail;
#else
  UINT BytesAvail;
#endif

#ifdef UNISOCK_WINSOCK
  Err = ioctlsocket(c_socket, FIONREAD, &BytesAvail);
#else
  Err = ioctl(c_socket, FIONREAD, (CHR*)((UINT *)&BytesAvail));
#endif

  if(Err == SOCKET_ERROR) {
    c_error = errno;
    return Err;
  }

  return BytesAvail;
}


void UNISOCK::ErrorMessage(CHR *msg, INT maxlen)
{
  ErrorMessage(c_error, msg, maxlen);
}


void UNISOCK::ErrorMessage(INT err, CHR *msg, INT maxlen)
{
  char buf[512];

  // err is either one we define in unisock.hxx or errno
  switch(err) {
  case UNI_INVALID_HOST:
    sprintf(buf, "Hostname lookup failed [%s]", c_addinfo.c_str());
    strncpy(msg, buf, maxlen);
    break;
  case UNI_UNCONNECTED:
    strncpy(msg, "Socket is closed", maxlen);
    break;
  default: {
      CHR *p = strerror(err);
      if(!p)
	strncpy(msg, "Unknown Error", maxlen);
      else 
	strncpy(msg, p, maxlen);
    }
  }	
  // Append the error value to the error message
  sprintf(buf, "[%i]", err);
  strncat(msg, errstr,  strlen(buf));
}


void UNISOCK::Listen(UINT port, INT backlog, const CHR *hostname)
{
  INT val = 1;
  struct hostent *hp;
  struct sockaddr_in name_in;

  if((c_socket = socket(c_family, c_comm_type, c_protocol)) == 
     INVALID_SOCKET) {
    c_error = errno;
    return;
  }

  // Bind to the port, then listen

  memset(&name_in, 0, sizeof(name_in));
 
  name_in.sin_family = AF_INET;
  name_in.sin_addr.s_addr = htonl(INADDR_ANY);
  name_in.sin_port = htons(port);

// start - from J. Wehle
       if (hostname && hostname[0])
               if (isalpha (hostname[0])) {
                       if (! (hp = gethostbyname (hostname)) ) {
                               c_error = EADDRNOTAVAIL;
                               return;
                       }
                       if (hp->h_addrtype != AF_INET) {
                               c_error = EAFNOSUPPORT;
                               return;
                       }
                       memcpy (&name_in.sin_addr, hp->h_addr,
                               sizeof (name_in.sin_addr));
               }
               else {
                       name_in.sin_addr.s_addr = inet_addr (hostname);
                       if ((long)name_in.sin_addr.s_addr == -1) {
                               c_error = EADDRNOTAVAIL;
                               return;
                       }
               }
// end - from J. Wehle
	
  setsockopt(c_socket, SOL_SOCKET, SO_REUSEADDR, (CHR*)&val, sizeof(INT));

  if(bind(c_socket, (SOCKADDR *)&name_in, sizeof(name_in)) == 
     SOCKET_ERROR ) {
    c_error = errno;
    return;
  }

  if (listen(c_socket, backlog) == SOCKET_ERROR) {
    c_error = errno;
    return;
  }
}

//
// 1 on success, -1 on failure
//
INT UNISOCK::Accept(UNISOCK *NewSocket)
{
  struct sockaddr_in name_in;
  INT name_in_size;
  TSOCKET s;

  name_in_size = sizeof(name_in);

  c_error = UNI_OK;
  for ( ; ; ) {
    if ((s = accept(c_socket, (SOCKADDR *)&name_in, 
		   (SOCKLEN_T*)&name_in_size)) != INVALID_SOCKET)
      break;
	    
    if(errno != EINTR) {
      c_error = errno;
      return -1;
    }
  }
  //	do {
  //		if((s = accept(c_socket, (SOCKADDR *)&name_in, 
  //			&name_in_size)) == INVALID_SOCKET) {
  //			if(errno != EINTR) {
  //				c_error = errno;
  //				return -1;
  //			}
  //               }
  //        }/ while (errno == EINTR);

  NewSocket->SetIPAddress(inet_ntoa(name_in.sin_addr));
  NewSocket->SetSocket(s);

  if(c_reverse_name_lookup) {
    // Determine the hostname.
    struct hostent *peer;
    peer = gethostbyaddr((char *)&name_in.sin_addr, 
			 sizeof(name_in.sin_addr), AF_INET);
    if(peer)
      NewSocket->SetHostname((CHR *)peer->h_name);
    else
      NewSocket->SetHostname(inet_ntoa(name_in.sin_addr));
  }

  return 1;
}


void UNISOCK::SetSocket(TSOCKET socket, INT State)
{
  c_socket = socket;
  c_state = State;
}


void UNISOCK::BlockingMode(INT f)
{
#ifdef UNISOCK_WINSOCK
  u_long flag;
#else
  UINT flag;
#endif
  INT err;

  c_error = UNI_OK;

  if(f == 0)
    flag = 0;
  else
    flag = 1;

  //	if((err = ioctl(c_socket, FIONBIO, &flag)) == SOCKET_ERROR)
#ifdef UNISOCK_WINSOCK
  err = ioctlsocket(c_socket, FIONBIO, &flag);
#else
  err = ioctl(c_socket, FIONBIO, &flag);
#endif
  
  if(err == SOCKET_ERROR)
    c_error = errno;

}


UNISOCK& UNISOCK::operator=(const UNISOCK & Other)
{
  c_family = Other.c_family;
  c_comm_type = Other.c_comm_type;
  c_protocol = Other.c_protocol;
  c_error = Other.c_error;
  c_state = Other.c_state;
  c_reverse_name_lookup = Other.c_reverse_name_lookup;
  c_hostname = Other.c_hostname;
  c_ipaddress = Other.c_ipaddress;
  c_socket = Other.c_socket;

  return *this;
}


GDT_BOOLEAN UNISOCK::StartedByInetd()
{
  if (c_inetd_not_set_yet)
    c_inetd = StartedByInetd();
  else 
    c_inetd_not_set_yet = GDT_TRUE;
  return c_inetd;
}


GDT_BOOLEAN 
StartedByInetd()
{
  struct sockaddr_in Name;
  INT NameLen;
  NameLen = sizeof(Name);
  if (getpeername(fileno(stdout), (SOCKADDR *)&Name, 
		  (SOCKLEN_T *)&NameLen) == -1) {
    return GDT_FALSE;
  } else {
    return GDT_TRUE;
  }
}

