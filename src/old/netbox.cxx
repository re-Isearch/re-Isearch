/*@@@
File:           netbox.cxx
Version:        1.01.01
Description:    Socket-interfacing network functions
Author:         Nassib Nassar; Kevin Gamiel
@@@*/

#include "netbox.hxx"

#define  NETBOX_SOCKADDR_CAST 1 

/*
Returns pointer to NETBOXPROFILE structure allocated using imem on success.
Returns NULL on failure.
*/
NETBOXPROFILE::NETBOXPROFILE() {
}


NETBOXPROFILE::~NETBOXPROFILE() {
}


PNETBOXPROFILE 
NETBOXPROFILE::Create()
{
  PNETBOXPROFILE NewOne = new NETBOXPROFILE;;
  return (NewOne);
}


void 
NETBOXPROFILE::Destroy()
{
  //	*this;
}


char FAR *
NETBOXPROFILE::SetHostName(char FAR *Name)
{
  if(strlen(Name)>255)
    return NULL;

  strcpy(HostName, Name);
	
  return HostName;
}


int 
NETBOXPROFILE::SetPort(int OtherPort)
{
  if(OtherPort<0)
    return -1;

  Port = OtherPort;
  return Port;
}


long 
NETBOXPROFILE::SetTimeOutSec(off_t T)
{
  if(T<0)
    return -1;
  TimeOutSec = T;

  return TimeOutSec;
}


long 
NETBOXPROFILE::SetTimeOutUSec(off_t T)
{
  if(T<0)
    return -1;
  TimeOutUSec = T;

  return TimeOutUSec;
}


int 
NETBOXPROFILE::GetPort()
{
  return Port;
}


/*
Returns a pointer to the host name.
*/
char FAR *
NETBOXPROFILE::GetHostName()
{
  return HostName;
}


int NETBOXPROFILE::WSAStartup(void)
{
#ifdef PLATFORM_WINDOWS
  WSADATA WsaData;
  if ( WSAStartup(0x0101, &WsaData) != 0 )
    return -1;
#endif
  return 0;
}


void 
NETBOXPROFILE::WSACleanup(void) {
#ifdef PLATFORM_WINDOWS
  WSACleanup();
#endif
}


int 
NETBOXPROFILE::SetBufferSize(int BufferSize) {
  int BufSize = BufferSize;
  return setsockopt(Socket, SOL_SOCKET, SO_RCVBUF,
		    (char FAR*)&BufSize, sizeof(BufSize));
}


int NETBOXPROFILE::Open() {
  SOCKET s;
  char HostNameBuf[256];
  LPHOSTENT HostEnt;
  SOCKADDR_IN addr_in;
  int rc;

  memset((char*)&addr_in, 0, sizeof(addr_in));
  addr_in.sin_addr.s_addr = inet_addr(HostName);
  if (addr_in.sin_addr.s_addr != -1) {
    addr_in.sin_family = AF_INET;
    strcpy(HostNameBuf, HostName);
  } else {
    HostEnt = gethostbyname(HostName);
    if ( NULL == HostEnt ) {
      return 0;
    }
    addr_in.sin_family = HostEnt->h_addrtype;
#ifdef h_addr
    strncpy((char*)&addr_in.sin_addr, HostEnt->h_addr_list[0],
	    HostEnt->h_length);
#endif
    strcpy(HostNameBuf, HostEnt->h_name);
  }
  addr_in.sin_port = htons(Port);
  if ( (s=socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET )
    return 0;
  rc = connect(s, (struct sockaddr FAR*)&addr_in, sizeof(addr_in));
  if (rc == -1) {
    logf (LOG_ERRNO, "Error connecting to %s (%d)", HostName, Port);
    return 0;
  }
  Socket = s;
  return 1;
}


int 
NETBOXPROFILE::Close() {
  if ( closesocket(Socket) == 0 ) {
    return 1;
  } else {
    return 0;
  }
}


int 
NETBOXPROFILE::SendBuffer(char FAR* Buffer, int BufferSize) {

  if((!this) || (!Buffer) || (BufferSize <= 0))
    return -2;
  /* select() checking should be inserted here */
#ifdef PLATFORM_WINDOWS
  if ( send(Socket, Buffer, BufferSize, 0) == SOCKET_ERROR )
    return -1;
#else
  /*
    if(profile->ServerOwner != NB_COMMANDLINE)
    fwrite(Buffer, 1, BufferSize, stdout);
    else	
  */
  if ( write(Socket, Buffer, BufferSize) == -1 )
    return -1;
#endif
  return BufferSize;
}


int 
NETBOXPROFILE::iGetBuffer(char FAR* Buffer, int BufferSize) {
  int x;
  int TotalRead = 0;
  int Left;
  char* p;
  p = Buffer;
  Left = BufferSize;
  do {
#ifdef PLATFORM_WINDOWS
	  
    x = recv(Socket, p, Left, 0);
#else
    //    if(this.WaitForData()==0)
    if (WaitForData()==0)
      return -2;
    x = read(Socket, p, Left);

#endif
    if ( (x != SOCKET_ERROR) && (x != 0) ) {
      p += x;
      Left -= x;
      TotalRead += x;
    }
  } while ( (x != SOCKET_ERROR) && (x != 0) &&
	    (TotalRead < BufferSize) );
  if (x == SOCKET_ERROR)
    return -1;
  else
    return TotalRead;
}


off_t 
NETBOXPROFILE::GetBuffer(char FAR* Buffer, off_t BufferSize) {
  int x = 1;
  int y;
  char* p = Buffer;
  long Left = BufferSize;
  long TotalRead = 0;
  while ( (x != 0) && (TotalRead < BufferSize) ) {
    if (Left > MAXINT)
      y = MAXINT;
    else
      y = (int)Left;
    x = read(Socket, p, y);
    return(x);
    /*x = netbox_iGetBuffer(profile, p, y);*/
    if (x == -1)
      return -1;
    if (x != 0) {
      TotalRead += x;
      Left -= x;
      p += x;
    }
  }
  return TotalRead;
}


int 
NETBOXPROFILE::WaitForData() {
  fd_set ReadFDS;
  struct timeval TimeVal;
  TimeVal.tv_sec = TimeOutSec;
  TimeVal.tv_usec = TimeOutUSec;
  FD_ZERO(&ReadFDS);
  FD_SET(Socket, &ReadFDS);
  /* Check the socket for readability. */
  if ( (select((Socket)+1, &ReadFDS, 0, 0, &TimeVal)) <= 0 ) {
    /* Timed out or SOCKET_ERROR! Not ready to write. */
    return 0;
  }
  return 1;
}


int 
NETBOXPROFILE::StartServer() {
  SOCKET s;
  SOCKADDR_IN addr_in;
  
  if(ServerOwner == 0) {
    /* Server already started by INETD */
    return(0);
  }

  memset(&addr_in, 0, sizeof(addr_in));
  if ( (s=socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET )
    return -1;
  addr_in.sin_family = AF_INET;
  addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
  //  addr_in.sin_port = (unsigned short int)htons(Profile->Port);
  addr_in.sin_port = (unsigned short int)htons(Port);
  if ( bind(s, (struct sockaddr*)&addr_in, sizeof(addr_in)) ==
       SOCKET_ERROR ) {
#ifdef PLATFORM_WINDOWS
    //    if (WSAGetLastError() == WSAEADDRINUSE) {
    //  if ( connect(s, (const struct sockaddr FAR*)&addr_in,
    //	   sizeof(addr_in)) == 0 ) {
    //netbox_WSACleanup();
#else
    if (errno == EADDRINUSE) {
      if ( connect(s, (struct sockaddr*)&addr_in,
		   sizeof(addr_in)) == 0 ) {
#endif
	close(s);
	return -1;
      }
    } else {
      int test = 1;
#ifdef PLATFORM_WINDOWS
      //  WSACleanup();
#endif
      close(s);
      if ( (s=socket(AF_INET, SOCK_STREAM, 0)) ==
	   INVALID_SOCKET )
	return -1;
#ifdef SOLARIS
      setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&test,
		 sizeof(test));
#else
      setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &test,
		 sizeof(test));
#endif
      addr_in.sin_family = AF_INET;
      addr_in.sin_addr.s_addr = INADDR_ANY;
      addr_in.sin_port = (unsigned short int)htons(Port);
      if ( bind(s, (struct sockaddr*)&addr_in, sizeof(addr_in)) ==
	   SOCKET_ERROR )
	return -1;
    }
  }
  if ( listen(s, 8) == -1 ) {
#ifdef PLATFORM_WINDOWS
    netbox_WSACleanup();
#endif
    close(s);
    return -1;
  }
  //  Profile->Socket = s;
  Socket = s;
  return(0);

}


PNETBOXPROFILE NETBOXPROFILE::AcceptClient()
{
  SOCKADDR_IN addr_in;
  PNETBOXPROFILE NewSocket;

  NewSocket = new NETBOXPROFILE;
  if(!NewSocket)
    return NULL;

  strcpy(NewSocket->HostName, HostName);
  NewSocket->Port = Port;
  NewSocket->ServerOwner = ServerOwner;
  NewSocket->TimeOutSec = TimeOutSec;
  NewSocket->TimeOutUSec = TimeOutUSec;

  if(ServerOwner == 0) {
    /* Server already started by INETD */
    CurrentStatus();
    NewSocket->CurrentStatus();
    return NewSocket;               
    /*		return(0);From wahl@sunl.cr.usgs.gov */
  }

  socklen_t addrSize = sizeof(addr_in);
 
  do {
#ifdef NETBOX_SOCKADDR_CAST
    if ( (NewSocket->Socket 
	  = accept(Socket, (struct sockaddr *)&addr_in, &addrSize)) 
	 == INVALID_SOCKET) {
#else
    if ( (NewSocket->Socket = accept(Socket,&addr_in, &addrSize)) 
	 == INVALID_SOCKET) {
#endif

	   switch(errno) {
	   case EBADF:
	     printf("ebadf\n");
	     break;
	   case ENOTSOCK:
	     printf("enotsock\n");
	     break;
	   case EOPNOTSUPP:
	     printf("eopnotsupp\n");
	     break;
	   case EFAULT:
	     printf("efault\n");
	     break;
	   case EWOULDBLOCK:
	     printf("ewouldblock\n");
	     break;
	   default:
	     printf("dunno\n");

	   }
	   return NULL;
	 }
#ifdef PLATFORM_WINDOWS
	 } while (WSAGetLastError() == WSAEINTR);
#else
  } while (errno == EINTR);
#endif
  //  CurrentStatus(Profile);
  CurrentStatus();
  //  NewSocket->CurrentStatus(NewSocket);
  NewSocket->CurrentStatus();
  return NewSocket;		
}


int NETBOXPROFILE::CurrentStatus()
{
  SOCKADDR_IN     Name;
  socklen_t       NameLen;
  struct hostent *Peer;

  NameLen = sizeof(SOCKADDR_IN);
#ifdef NETBOX_SOCKADDR_CAST
  if(!getpeername(fileno(stdout),(struct sockaddr *)&Name,&NameLen)) 
    {
    ServerOwner = 0; 
  } else
    ServerOwner = 1; 
#else
  if(!getpeername(fileno(stdout),&Name,&NameLen)) {
    ServerOwner = 0; 
  } else
    ServerOwner = 1; 
#endif
  
#ifdef NETBOX_SOCKADDR_CAST
  getpeername(Socket,(struct sockaddr *)&Name,&NameLen);
#else
  getpeername(Socket,&Name,&NameLen);
#endif
  Peer = gethostbyaddr(inet_ntoa(Name.sin_addr), 4, AF_INET);
  if(Peer != NULL) {
    sprintf(PeerName,"%s [%s]",Peer ? Peer->h_name:"?",
	    inet_ntoa(Name.sin_addr));
  } else {
    sprintf(PeerName,"[%s]", inet_ntoa(Name.sin_addr));
  }
  return(0);
}


/* 
PRE:	NetProfile is connected.
	0 < sizeof(Buffer) < MaxLen
POST:	Buffer contains a NULL-terminated string from the net on success.
	On failure, returns <= 0.  See netbox_iGetBuffer for error
	codes.
NOTE:	Copies a single character from the net into Buffer until either
	CR or CRLF or MaxLen bytes.  CR or CRLF are not stored in
	Buffer.
*/
int 
NETBOXPROFILE::GetLine(char *Buffer, int MaxLen)
{
  int Index=0, len, Continue=1;
  do{
    //    if((len=NetProfile->iGetBuffer(Buffer+Index,1))<=0) {
    if((len=iGetBuffer(Buffer+Index,1))<=0) {
      return len;
    } else {
      switch(Buffer[Index]) {
      case 0x0a:
	/* Stop reading */
	if(Buffer[Index-1]==0x0d)
	  Buffer[Index-1]='\0';
	else
	  Buffer[Index]='\0';
	Continue=0;
	break;	
      }
      Index += len;
    }
  } while(Continue==1 && Index < MaxLen);
  return strlen(Buffer);
}

