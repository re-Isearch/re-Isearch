#ifndef _PROTMAN_HXX
#define _PROTMAN_HXX

#include "defs.hxx"
#include "netbox.hxx"
#include "strbox.hxx"
#include "treebox.hxx"

int ResolveURL(const STRING& URL, FILE *fp, size_t *Len);
int ProcessGopherRequest(char *Host, int Port, char *Request, FILE *fp,
			 size_t *Len);
int ProcessHTTPRequest(char *Host, int Port, char *Request, FILE *fp,
		       size_t *Len);
void StripURLsToStream(char *InBuf, FILE *Stream, char *Separator);
void PrintOtherHosts(char *InBuf);
int GopherWalk(char *Host, int Port, char *Selector, FILE *fp, LPBSTNODE Tree);
int gsscanf(char *src, char *type, char *title, char *sel, char *host, 
	int *port, char *plus);

#define URL_NOT_AVAILABLE -1 

#endif /* _PROTMAN_HXX */

