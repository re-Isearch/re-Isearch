/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#include "protman.hxx"
#include "common.hxx"
//#include "dict.hxx"

#define NEWSTRUCT(a) (a*)malloc(sizeof(a))
#define NEWSTRING(a) (char *)malloc(a)
#define FREE(a) free(a)

/*#define USE_PROXY */

/*#define DEBUG*/

#define NEWURL 1
#define OUTOFMEMORY 2
#define DUPLICATE 3

#ifdef AWWW
#define HOSTNAME "localhost"
#else
#define HOSTNAME "doright.stsci.edu"
#endif

#define MAX_FILE_LENGTH 10000000

// Use ISO date/time format (alternative is RFCdate )
#define DATE_STRING ::ISOdate(0)


static struct strint {
  const char *s;
  int i;
} protocols[] = {
  {"gopher", 70 },
  {"whois",  43},
  {"https",  443},
  {"http",   80},
  NULL
};

/* 
   To add new supported protocols, add the protocol name as it
   appears in a url to this table and add the default port number
   for that protocol to PortTable[] below.
   */
const char *ProtTable[] = {
  "gopher",
  "whois",
  "http",
  "https",
  ""		/* code depends on this NULL being last in list! */
};
int ProtTableCount = 5;
int PortTable[] = {
  70,43,80, 4430
};


NETBOXPROFILE Net;
FILE *message_logp;

/*
   Function:	ResolveURL()
   
   Purpose:	Accepts a URL and attempts to resolve it, writing 
   the contents to a specified file stream.
   
   URL syntax:	protocol://host[:port]/access
   
   Supported 
   Protocols: 	http, gopher
   
   Example: 	"gopher://cnidr.org/welcome.txt"
   "http://cnidr.org/welcome.html"
   "gopher://cnidr.org:7070/welcome.txt"
   
   Notes:	
   An access of '*' means "walk the tree and generate URLs".
   Use with CAUTION!  For example, "gopher://kudzu.cnidr.org/(*)" 
   will walk the entire kudzu.cnidr.org gopher tree and write 
   all items as URLs out to the file specified.
   				  
   The host name MUST be as it appears from the server, i.e.
   if you enter "cnidr.org", you will only get the top level
   directory whereas if you enter "kudzu.cnidr.org", you'll
   get the entire directory.

   You can also specify a gopher subdirectory to begin walking
   from.  For example, "gopher://kudzu.cnidr.org/1multimedia/(*)"
   will start at the gopher subdirectory "multimedia" and
   work its way down from there.  Note that you MUST specify
   a trailing slash after the subdirectory name. 
*/

int ResolveURL(const STRING& URL, FILE *fp, size_t *Len) 
{

cerr << "RESOLVE " << URL << endl;
  char Host[64], Protocol[64];
  char *P, *Q, *S;
  int ProtTabEntry, Port,err;
  //  char Command[64];
  char Command[256];
#ifdef DEBUG
  message_logp=fopen("protman.log","a");
#endif
  if((S = (char *)calloc(1,URL.GetLength()+1)) == NULL) {
    message_log(LOG_ERROR, "out of memory!");
    return(-1);
  }
  strcpy(S, URL.c_str());

  /* Error check a bit */
  if((Q = strtok(S, "://")) == NULL) {
    message_log(LOG_ERROR, "Invalid URL format: --> %s\n",P);
    return(-1);
  }
	
  strcpy(Protocol, Q);
  strcpy(S, URL.c_str());
  /* Which protocols are we supporting? */
  for(ProtTabEntry=0;ProtTabEntry<ProtTableCount;ProtTabEntry++) {
    if(ProtTable[ProtTabEntry][0] == '\0') {
      message_log (LOG_ERROR, "Invalid URL format: Unsupported protocol.");
      return(-1);
    }
    if(!strcasecmp(Protocol, ProtTable[ProtTabEntry])) {
      break;
    }
  } 
	
	
  P = strstr(S, "://");
  if(P == NULL) {
    message_log (LOG_ERROR, "PROTMAN: Invalid URL format: --> %s\n",URL.c_str());
    return(-1);
  }
  P = strtok(P+3, "/");
  if(P == NULL) {
    message_log (LOG_ERROR, "PROTMAN: Invalid URL format: --> %s\n",URL.c_str());
    return(-1);
  }
  strcpy(S,P);	
  if((P = strchr(S,':')) != NULL) {
    /* Alternative port specified */
    Port = atoi(P+1);
    P = strtok(S,":");
    strcpy(Host, P);
  } else {
    strcpy(Host, S);
    Port = PortTable[ProtTabEntry];	
  }
  strcpy(S, URL.c_str());
  if((P = strstr(S, "://"))==NULL) {
    message_log (LOG_ERROR, "PROTMAN: Invalid URL format.  Expected '://' --> %s\n",URL.c_str());
    return(-1);
  }
  if((P = strstr(P+3, "/"))==NULL) {
    message_log (LOG_ERROR, "PROTMAN: Invalid URL format.  Expected ':' --> %s\n",URL.c_str());
    return(-1);
  }
  strcpy(Command, P+1);
  Command[strlen(Command)] = '\0';

cerr << "PROTOCOL: " << Protocol << endl;

  if(!strcmp(Protocol, "gopher")) {
    if(Command[strlen(Command)-1] == '*') {
     // Walk the tree
      LPBSTNODE BSTree;
      int Count=0l;
      Command[strlen(Command)-1]='\0';
      /* Walk the gopher tree */
      BSTree=bst_Create();
#ifdef DEBUG
      fprintf(message_logp,"*************************************************\n");
      fprintf(message_logp,"[%s] Walk: <%s>\n", DATE_STRING, URL.c_str());
#endif
      err= GopherWalk(Host, Port, Command, fp, BSTree);
      bst_Print(BSTree, fp);
#ifdef DEBUG
      bst_Count(BSTree,&Count);
      fprintf(message_logp,"[%s] Read %ld URLs\n", DATE_STRING, Count);
#endif
      // printf("End Gopher Walk: %s\n",Time);	
      return err;
    } else {
      // Get a file
#ifdef DEBUG
      fprintf(message_logp,"*************************************************\n");
      fprintf(message_logp,"[%s] Retrieve <%s>\n", DATE_STRING,URL.c_str());
#endif
      err = ProcessGopherRequest(Host,Port,Command,fp,Len);
#ifdef DEBUG
      fprintf(message_logp,"[%s] Read %ld bytes\n", DATE_STRING,*Len);
#endif
      return err;
    }
  }
  else if(!strcmp(Protocol, "http")) {
    return(ProcessHTTPRequest(Host, Port, Command,fp,Len));
  }
  else  if(!strcmp(Protocol, "ipfs")) {
    // IPFS
    return(ProcessIPFSRequest(Host, Port, Command,fp,Len));
  }
  return(1);
}


int 
ProcessGopherRequest(char *Host, int Port, char *Request,FILE *fp,size_t *Len)
{
  char *NewRequest =(char *) calloc(1,strlen(Request)+2);
  int MaxLen = MAX_FILE_LENGTH;
  char *Buf = (char *)calloc(1,MaxLen+1);
  char Temp[256];
  int err;
  int Binary=0,i;

  *Len=0l;
  sprintf(NewRequest,"%s\r\n",Request);

  Net.SetHostName(Host);
  Net.SetPort(Port);
  Net.SetTimeOutSec(60l);

  if(Net.Open()!=1) {
    message_log (LOG_ERROR, "Failed to open connection: %s[%d]",Host,Port);
    FREE(NewRequest);
    return -1;
  }
	
  if(Net.SendBuffer(NewRequest,strlen(NewRequest))<=0) {
    message_log (LOG_ERROR, "Failed to send data: %s[%d]",Host,Port);
    FREE(NewRequest);
    return -1;
  }

  if(Net.WaitForData()!=1) {
    message_log (LOG_ERROR, "Failed to send data: %s[%d]",Host,Port);
    FREE(NewRequest);
    return -1;
  }

  do{
    Binary = 0;
    err = Net.GetBuffer(Buf, MaxLen);
    if(err>=0) {
      if(err==0)
	break;
      /* kludge */
      if(strstr(Buf,"0Server error:")) {
	/* specified URL doesn't exist*/
	Net.Close();
	FREE(NewRequest);
	return URL_NOT_AVAILABLE;
      }
      *Len+=err;
      fwrite(Buf,1,err,fp);
    } else {
      message_log (LOG_ERROR, "Failed to read data: %s[%i]",Host,Port);
      Net.Close();
      FREE(NewRequest);
      return -1;
    }
  } while(err>0);

  FREE(NewRequest);
  Net.Close();
  return *Len;
}


void PrintOtherHosts(char *InBuf) { printf("%s\n",InBuf); }


int ProcessIPFSRequest(char *Host, int Port, char *Request,FILE *fp,size_t *Len)
{
  // Rewrite ipfs://hash into https://gateway/hash
  // https://ipfs.io/api/v0/dag/get?arg=bafyreiah7uhzdxbuik6sexirej22iyi5nau3d4nnfhv6ux33ogtdpeznpm
  // bafyreiah7 uhzdxbuik6 sexirej22i yi5nau3d4 nnfhv6ux3 3ogtdpeznp m // 61 byte hash
  return 0;
}


int ProcessHTTPRequest(char *Host, int Port, char *Request,FILE *fp,size_t *Len)
{
  char *NewRequest =(char *) calloc(1,strlen(Request)+strlen(Host)+100);
  int MaxLen = 10000;
  char *Buf = (char *)calloc(1,MaxLen+1);
  char Temp[256];
  int err;
  int Binary=0,i;

  cerr << "HTTP REquest" << endl;

  //  sprintf(NewRequest, "GET /%s\r\n", Request);
  sprintf(NewRequest, "GET /%s HTTP/1.0\r\n\r\n", Request);
  *Len=0l;

#ifdef USE_PROXY	
  sprintf(NewRequest, "GET http://%s:%i/%s\r\n",Host,Port,Request);
  Net.SetHostName(HOSTNAME);
  Net.SetPort(3128);
  Net.SetTimeOutSec(60l);
#else
  Net.SetHostName(Host);
  Net.SetPort(Port);
  Net.SetTimeOutSec(60l);
#endif /* USE_PROXY */

  if(Net.Open()!=1) {
    message_log (LOG_ERROR, "Failed to open HTTP connection: %s[%i]",Host,Port);
    FREE(NewRequest);
    return -1;
  }

  if(Net.SendBuffer(NewRequest,strlen(NewRequest))<=0) {
    message_log (LOG_ERROR, "Failed to send HTTP request: %s[%i]",Host,Port);
    FREE(NewRequest);
    return -1;
  }
  if(Net.WaitForData()!=1) {
    message_log (LOG_ERROR, "Failed to send HTTP data: %s[%i]",Host,Port);
    FREE(NewRequest);
    return -1;
  }

  do{
    Binary = 0;
    err = Net.GetBuffer(Buf, MaxLen);
    if(err>=0) {
      if(err==0)
	break;
      /* Will this always work? */
      if(strstr(Buf,"404 Not Found")) {
	/* specified URL doesn't exist*/
	Net.Close();
	FREE(NewRequest);
	return URL_NOT_AVAILABLE;
      }
      *Len+=err;
      fwrite(Buf,1,err,fp);
    } else {
      message_log (LOG_ERROR, "Failed to read HTTP data: %s[%i]",Host,Port);
      Net.Close();
      FREE(NewRequest);
      return -1;
    }
  } while(err>0);

  FREE(NewRequest);
  Net.Close();
  return *Len;
}


int 
GopherWalk(char *Host, int Port, char *Selector, FILE *fp, LPBSTNODE Tree)
{
  int MaxLen = 1000000;
  char Temp[1301];
  int err;
  char tHost[256],tSelector[1024], tTitle[1024],tPlus[256];
  char tType;
  int tPort, Total=0;
  char *NewRequest, P;
  char *Buf;
  LPBSTNODE Dictionary;	

  Dictionary=bst_Create();

  if((Buf = NEWSTRING(MaxLen+1)) == NULL) {
    message_log (LOG_ERROR, "GopherWalk:Out of memory");
    return -1;
  }

  if((NewRequest=NEWSTRING(2048))==NULL) {
    message_log (LOG_ERROR, "GopherWalk:Out of memory");
    FREE(Buf);
    return -1;
  }

  sprintf(NewRequest,"%s\r\n",Selector);

  Net.SetHostName(Host);
  Net.SetPort(Port);
  Net.SetTimeOutSec(60l);

  if(Net.Open()!=1) {
    message_log (LOG_ERROR, "Failed to open Gopher connection: %s[%d]",Host,Port);
    FREE(NewRequest);
    FREE(Buf);
    return -1;
  }

  if(Net.SendBuffer(NewRequest,strlen(NewRequest))<=0) {
    message_log (LOG_ERROR, "Failed to send data: %s[%d]",Host,Port);
    FREE(NewRequest);
    FREE(Buf);
    return -1;
  }

  if(Net.WaitForData()!=1) {
    message_log (LOG_ERROR,"Failed to send data: %s[%d]",Host,Port);
    FREE(NewRequest);
    return -1;
  }

  do{
    err = Net.GetBuffer(Buf+Total, MaxLen);
    if(err>=0) {
      if(err==0)
	break;
      Total+=err;
    } else {
      message_log (LOG_ERROR, "Failed to read data: %s[%d]",Host,Port);
      Net.Close();
      FREE(NewRequest);
      FREE(Buf);
      return -1;
    }
  } while(err>0);
  Total=0;
  Net.Close();

  while((err=strbox_SGetS(Temp,Buf+Total))!=-1) {
    Total+=err;
    gsscanf(Temp,&tType, tTitle, tSelector, tHost, &tPort, tPlus);
    sprintf(Temp,"gopher://%s:%i/%s", tHost, tPort, tSelector);
    if((err = bst_Insert(Tree, Temp))==1) {
      fprintf(fp,"%s\n",Temp);
      /* Not a duplicate */
      if(tType=='1') {
	if(!strcmp(Host,tHost) && tPort==Port) {
	  if(GopherWalk(Host,Port,tSelector,fp,Tree)==-1) {
	    FREE(NewRequest);
	    FREE(Buf);
	    return -1;
	  }
	}
      }
      if(tType=='0') {
	FILE  *statsFp=fopen("dictstats.txt","a");
	FILE  *tfp;
	size_t tlen=0;
	int    tCount=0;
	tfp=fopen("temp001","w");
	/* Text file (in theory) */
	ProcessGopherRequest(Host,Port,tSelector,tfp,&tlen);
	fclose(tfp);
	tfp=fopen("temp001","r");
	//	dict_BuildDict(tfp,Dictionary);
	bst_Count(Dictionary, &tCount);
	if(statsFp){
	  fprintf(statsFp,"UniqueWords=%d\n",tCount);
	  fclose(statsFp);
	}
	printf("Dictionary now contains %d unique words...\n",tCount);
	fclose(tfp);
      }
    } else {
      if(err == -1) {
	/* out of memory */
	FREE(NewRequest);
	FREE(Buf);
	return -1;
      }
      /* Duplicate found */
#ifdef DEBUG
      fprintf(message_logp,"Duplicate URL=%s\n",Temp);
#endif
    }
  }
  FREE(NewRequest);
  FREE(Buf);
  return 1;
}


int 
gsscanf(char *src, char *type, char *title, char *sel, char *host, 
	int *port, char *plus)
{
  typedef enum _state {Type,Title,Selector,Host,Port,Plus,Done} STATE;
  STATE State = Type;
  int j=0;
  size_t i;
  char tport[16];

  if(src[0]=='.')
    return -1;

  for(i=0;i<strlen(src);i++) {
    switch(State) {
    case Type:
      *type=src[i];
      State=Title;
      break;
    case Title:	
      if(src[i]=='\t') {
	title[j]='\0';
	j=0;
	State = Selector;
      } else
	title[j++]=src[i];
      break;
    case Selector:
      if(src[i]=='\t') {
	sel[j]='\0';
	j=0;
	State = Host;
      } else
	sel[j++]=src[i];
      break;	
    case Host:
      if(src[i]=='\t') {
	host[j]='\0';
	j=0;
	State = Port;
      } else
	host[j++]=src[i];
      break;	
    case Port:
      tport[j++]=src[i];
      if(src[i+1]=='\t'||src[i+1]=='\0') {
	if(src[i+1]=='\t')
	  State = Plus;
	else
	  State = Done;
	tport[j]='\0';
	j=0;
	*port = atoi(tport);
      }
      break;	
    case Plus:
      if(src[i]=='\t')
	break;
      plus[j++]=src[i];
      if(src[i+1]=='\0')
	plus[j]='\0';
      break;
    case Done:
      break;
    } /* switch() */

    if(State == Done)
      break;
  } /* for() */
  return 1;
}


