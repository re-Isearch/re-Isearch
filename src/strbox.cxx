#include "strbox.hxx"

#ifdef NO_STRDUP
*CHR 
strdup(CHR *S) {
  CHR *T;
  T = new CHR[strlen(S)+1];
  if (T != NULL) {
    strcpy(T, S);
  }
  return T;
}
#endif

#ifdef PLATFORM_MSC
#define strdup() _strdup()
#endif

/*
FUNCTION:	strbox_StringInSet()													      
PURPOSE:	Looks for a specified string within a specified
		set of strings.										      
PRE:		s = string for which to look.
		stopList = Array of null-terminated strings containing
		string set.                   
POST:		0 if s is found within stopList.
		1 if s not found within stopList.
*/
INT 
strbox_StringInSet(CHR* s, CHR* *stopList) {
  INT i=0;

  while(stopList[i] != NULL) {
    if(!strcmp(s,stopList[i]))
      return 0;
    i++;
  }
  return 1;
}


void 
strbox_Upper(CHR* s) {
  size_t i;

  for(i=0;i<strlen(s);i++) {
    if(islower(s[i]))
      s[i]=s[i]-0x20;
  }
}


/*
FUNCTION:	strbox_TheTime()
PURPOSE:	Copies the current time as a string into specified
buffer.
PRE:		buf = a pointer to an allocated char buffer.
POST:		buf = NULL-terminated string of the form:
		"Thu Feb 10 13:00:27 1994"
CALLS:		time(), sprintf(), ctime(),strlen()
*/
void 
strbox_TheTime(CHR* buf)
{
  time_t lTimePtr;
  size_t i;

  time(&lTimePtr);
  sprintf(buf,"%s",ctime(&lTimePtr));
  /* strip off the trailing newline */
  for(i=0;i<strlen(buf);i++) {
    if(buf[i] == '\r' || buf[i] == '\n') {
      buf[i]='\0';
      return;
    }
  }
}


INT 
strbox_itemCountd(CHR* S, CHR Delim) {
  CHR* NS;
  INT c;
  if (S == NULL)
    return 0;
  NS = new CHR[strlen(S)+1];
  strcpy(NS, S);
  if (NS == NULL)
    return 0;
  strbox_Cleanup(NS);
  if (NS[0] == '\0')
    return 0;
  c = 1;
  for (size_t x = 0; x < strlen(NS); x++)
    c += (NS[x] == Delim);
  return c;
}


INT 
strbox_itemCount(CHR* S) {
  CHR* NS;
  INT c;
  if (S == NULL)
    return 0;
  NS = new CHR[strlen(S)+1]; 
  strcpy(NS, S);
  if (NS == NULL)
    return 0;
  strbox_Cleanup(NS);
  if (NS[0] == '\0')
    return 0;
  c = 1;
  for (size_t x = 0; x < strlen(NS); x++)
    c += (NS[x] == ',');
  return c;
}


CHR* 
strbox_itemd(CHR* S, INT X, CHR* Item, CHR Delim) {
  CHR* NS;
  CHR* p;
  CHR* NSp;
  INT c;
  if (Item == NULL)
    return NULL;
  Item[0] = '\0';
  if (S == NULL)
    return Item;
  if (X < 1)
    return Item;
  NS = new CHR [strlen(S)+1];
  strcpy(NS,S);
  if (NS == NULL)
    return Item;
  strbox_Cleanup(NS);
  if (NS[0] == '\0')
    return Item;
  c = X - 1;
  NSp = NS;
  while ( (c > 0) && ((p=strchr(NSp, Delim)) != NULL) ) {
    NSp = p + 1;
    c--;
  }
  if (c > 0)
    return Item;
  if ((p=strchr(NSp, Delim)) != NULL)
    *p = '\0';
  strcpy(Item, NSp);
  delete [] NS;
  return Item;
}


CHR* 
strbox_item(CHR* S, INT X, CHR* Item) {
  CHR* NS;
  CHR* p;
  CHR* NSp;
  INT c;
  if (Item == NULL)
    return NULL;
  Item[0] = '\0';
  if (S == NULL)
    return Item;
  if (X < 1)
    return Item;
  NS = new CHR [strlen(S)+1];
  strcpy(NS, S);
  if (NS == NULL)
    return Item;
  strbox_Cleanup(NS);
  if (NS[0] == '\0')
    return Item;
  c = X - 1;
  NSp = NS;
  while ( (c > 0) && ((p=strchr(NSp, ',')) != NULL) ) {
    NSp = p + 1;
    c--;
  }
  if (c > 0)
    return Item;
  if ((p=strchr(NSp, ',')) != NULL)
    *p = '\0';
  strcpy(Item, NSp);
  delete [] NS;
  return Item;
}


void 
strbox_Cleanup(CHR* s) {
  CHR* p = s;
  while (*p == ' ')
    p++;
  strcpy(s, p);
  p = s + strlen(s) - 1;
  while ( (p >= s) && (*p == ' ') )
    *p-- = '\0';
}


/*
Returns number of characters read up to and including CR or CRLF and
writes them into Dest.
However, Dest does not contain CR or CRLF.
*/
INT 
strbox_SGetS(CHR* Dest, CHR* Src)
{
  size_t i;
 
  if(Src==NULL || Src[0]=='\0' || Dest==NULL)
    return -1;
 
  for(i=0;i<strlen(Src);i++) {
    if(Src[i]==0x0a && Src[i+1]!=0x0d) {
      Dest[i]='\0';
      return(strlen(Dest)+1);
    } else
      if(Src[i]==0x0d) {
	Dest[i]='\0';
	return(strlen(Dest)+2);
      }
    Dest[i]=Src[i];
  }
  Dest[i]='\0';
 
  return(strlen(Dest));
}


/* 
Converts all spaces in a string to the character sequence "%22".

You must enocode spaces in URLs in this manner.  Make sure that your
buffer is large enough to handle the extra space!
*/
void 
strbox_EscapeSpaces(CHR* S)
{
  INT j=0;
  CHR* T;

  T = new CHR[strlen(S)+100];
  T[0]='\0';
	
  for(size_t i=0;i<strlen(S);i++) {
    if(S[i]==' ') {
      strcat(T,"%22");
      j+=3;
    } else {
      T[j++]=S[i];
      T[j]='\0';
    }
  }
  strcpy(S,T);
  delete [] T;

}
