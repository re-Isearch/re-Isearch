// $Id: cgi-util.cxx 178 2006-07-15 15:12:28Z warnock $
/************************************************************************
Copyright (c) A/WWW Enterprises, 2001-2002

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee.

************************************************************************/

/***********************************************************************
Copyright Notice

Copyright (c) MCNC, Clearinghouse for Networked Information Discovery
and Retrieval, 1996-2002.

Permission to use, copy, modify, distribute, and sell this software and
its documentation, in whole or in part, for any purpose is hereby
granted without fee, provided that

1. The above copyright notice and this permission notice appear in all
copies of the software and related documentation. Notices of copyright
and/or attribution which appear at the beginning of any file included in
this distribution must remain intact. 

2. Users of this software agree to make their best efforts (a) to return
to MCNC any improvements or extensions that they make, so that these may
be included in future releases; and (b) to inform MCNC/CNIDR of
noteworthy uses of this software.

3. The names of MCNC and Clearinghouse for Networked Information
Discovery and Retrieval may not be used in any advertising or publicity
relating to the software without the specific, prior written permission
of MCNC/CNIDR.

THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.

IN NO EVENT SHALL MCNC/CNIDR BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF
THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
************************************************************************/

/*@@@
File:          	cgi-util.cxx
Version:        1.04
$Revision$
Description:    CGI utilities
Authors:        Kevin Gamiel, kgamiel@cnidr.org
		Tim Gemma, stone@k12.cnidr.org
@@@*/

// change record:
// reset z and initialized entry_point to fix "GET" method    9/25/96 dtw

#include "cgi-util.hxx"
#include "string.hxx"

/*
# Class: CGIAPP
# Method: GetInput
# Comments: Reads Forms data from standard input, parses out arguments.
*/

void 
CGIAPP::GetInput(int argc, char** argv) {
  INT ContentLen=0, x, y, z, len, nn;
  CHR temp1[256], temp2[256], temp3[256];
  CHR *meth;
  CHR *p;
  CHR *query=(CHR*)NULL;
  GDT_BOOLEAN fromCGI;

#ifdef LOG
  FILE *logfile;
  logfile = fopen("/tmp/sru.log","w+");
#endif

  
  if (argc < 2) {
    if ((meth = (char *)getenv("REQUEST_METHOD"))==NULL) {
      cout << "Unable to get request_method" << endl;
      exit(1);
    } else {
      fromCGI = GDT_TRUE;
    }
  } else {
    fromCGI = GDT_FALSE;
  }

  if (fromCGI) {
#ifdef LOG
      fprintf(logfile,"Request Method: %s\n",meth);
#endif

    if (!strcmp(meth,"POST")) {
      if ((p=(char *)getenv("CONTENT_LENGTH")))
	ContentLen = atoi(p);
      Method=POST;
      
    } else if (!strcmp(meth,"GET")) {
      query = (char *)getenv("QUERY_STRING");
      Method=GET;
#ifdef LOG
      fprintf(logfile,"Query: %s\n",query);
#endif


    } else {
      cout << "This program requires a METHOD of POST or GET.\n";
//      cout << "If you don't understand this, see this ";
//      cout << "<A HREF=\"http://www.ncsa.uiuc.edu/SDG/Software/Mosaic/";
//      cout << "Docs/fill-out-forms/overview.html\">forms overview</A>" << endl;
      exit (1);
    }

  } else {
    Method = GET;
    STRING Query = argv[1];
    query = Query.NewCString();
#ifdef LOG
      fprintf(logfile,"Request Method: NoCGI\n");
      fprintf(logfile,"Query: %s\n",query);
#endif

  }

  // Build the list of cgi entries
  if (Method==POST) {
    entry_count=0;
    for (x = 0; ContentLen>0; x++) {
      cin.getline(temp1,ContentLen+1,'&');
      entry_count++;
      len=strlen(temp1);
      ContentLen=ContentLen-(len+1);
      for (y=0;temp1[y]!='=' && y<len;y++) {
        temp2[y]=temp1[y];
      }
      temp2[y]='\0';
      y++;
      for (z=0;y<len;y++) {
        temp3[z]=temp1[y];
        z++;
      }
      temp3[z]='\0';
      plustospace(temp3);
      unescape_url(temp3);
      name[x]=new CHR[strlen(temp2)+1];
      strcpy(name[x],temp2);

      // Trim trailing blanks off the string before we stick it into Value
      for (nn=strlen(temp3)-1;nn==0;nn--) {
	if(temp3[nn] == ' ')
	  temp3[nn]='\0';
	else
	  break;
      }

      value[x]=new CHR[strlen(temp3)+1];
      strcpy(value[x],temp3);
    }
  } else {  // Get
    entry_count=0;
    y=0;
    z=0;
    plustospace(query);
    unescape_url(query);
    len=strlen(query);
    for (x=0;y<len;x++) {
      while ((query[y]!='=') 
	&& (query[y]!='&') 
	&& (query[y]!='?') 
	&& (y<len)) {
        temp1[z]=query[y];
        z++;
        y++;
      }
      temp1[z]='\0';
      z=0;
      if (query[y]=='=') {
        y++;
        while ((query[y]!='&') && (y<len)) {
          temp2[z]=query[y];
          z++;
          y++;
        }
      }
      y++;
      temp2[z]='\0';
      z = 0;  // dtw patch for GET ?
      if (temp2[0]=='\0') {
        name[x]=new CHR[1];
        strcpy(name[x],"");

	// Trim trailing blanks off the string before we stick it into Value
	for (nn=strlen(temp1)-1;nn==0;nn--) {
	  if(temp1[nn] == ' ')
	    temp1[nn]='\0';
	  else
	    break;
	}

        value[x]=new CHR[strlen(temp1)+1];
        strcpy(value[x],temp1);
      } else {
        name[x]=new CHR[strlen(temp1)+1];
        strcpy(name[x],temp1);

	// Trim trailing blanks off the string before we stick it into Value
	for (nn=strlen(temp2)-1;nn==0;nn--) {
	  if(temp2[nn] == ' ')
	    temp2[nn]='\0';
	  else
	    break;
	}

        value[x]=new CHR[strlen(temp2)+1];
        strcpy(value[x],temp2);
      }
      entry_count++;
      z=0;
    }
    entry_count = x;  // dtw patch for GET ?
  }  // Get
#ifdef LOG
  fclose(logfile);
#endif

// For some odd reason, with a GET, Apache 2.0.40 (at least) seems to
// stick a '\' character on the end of each parameter, perhaps to quote
// the '&' character.  Apache 2.0.50 does not seem to do the same this.
// In any event, I'm trimming off any trailing '\' characters from the
// ends of the parameter value.
  STRING Value;
  CHR *v;
  INT last;

  for (x=0;x<entry_count;x++) {
    Value = value[x];
    v = Value.NewCString();
    last = strlen(v) - 1;
    if ((last>0) && (v[last] == '\\')) {
      v[last] = (CHR)NULL;
      delete [] value[x];
      value[x]=new CHR[strlen(v)+1];
      strcpy(value[x],v);
    }
  }
}


CGIAPP::CGIAPP(int argc, char** argv) {
  GetInput(argc, argv);
}


void
HTMLcleanup(CHR* in, CHR* out) {
  INT n,m;
  CHR *p;
  n=0;
  m=0;

  while(in[n] != '\0') {
    if (in[n] == '&') {
      p=&out[m];
      memcpy((void*)p,"&amp;",strlen("&amp;"));
      m += strlen("&amp;");
    } else {
      out[m] = in[n];
    }
    n++;
    m++;
  }
  out[m]='\0';
}

void 
CGIAPP::DisplayAsHTML() {
  INT x;
  INT len;
  CHR *HTMLout;
  for (x=0;x<entry_count;x++) {
    len = strlen(value[x]);
    HTMLout = new CHR[2*len];
    HTMLcleanup(value[x],HTMLout);
    cout << name[x] << " = " << HTMLout << "<br>\n";
    delete [] HTMLout;
  }
}


void 
CGIAPP::Display() {
  INT x;
  for (x=0;x<entry_count;x++) {
    if (strlen(name[x]) > 0)
	cout << name[x] << "=" << value[x] << endl;
    else
	cout << "Found nameless parameter: " << value[x] << endl;
  }
}


void 
CGIAPP::Dump(FILE *fp) {
  INT x;
  for (x=0;x<entry_count;x++) {
    if (x == 0)
      fprintf(fp,"%s=%s", name[x],value[x]);
    else
      fprintf(fp,"&%s=%s", name[x],value[x]);
  }
}


void 
CGIAPP::DumpToHTML(FILE *fp) {
  INT x;
  for (x=0;x<entry_count;x++) {
    if (x == 0)
      fprintf(fp,"%s=%s", name[x],value[x]);
    else
      fprintf(fp,"&amp;%s=%s", name[x],value[x]);
  }
}


CHR*
CGIAPP::GetValueByName(const CHR *field) {
  if ((field==NULL) || (field[0]=='\0'))
    return NULL;
  INT i;
  for (i=0;i<entry_count;i++) {
    if ((name[i]==NULL)||(value[i]==NULL))
      return NULL;
    if (!strcmp(name[i], field)) {
      if (value[i] != NULL) {
        return value[i];
      }
      return NULL;
    }
  }
  return NULL;

}

CGIAPP::~CGIAPP() {
  INT i;
  //  for (i=0;i<entry_count;i++) {
  for (i=entry_count-1;i>=0;i--) {
    if (name[i])
      delete [] name[i];
    if (value[i])
      delete [] value[i];
  }
}


CHR 
x2c(CHR *what) {
  CHR digit;
  digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A')+10 : (what[0] - '0'));
  digit *= 16;
  digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A')+10 : (what[1] - '0'));
  return(digit);
}


void 
unescape_url(CHR *url) {
  INT x,y;
  for(x=0,y=0;url[y];++x,++y) {
    if((url[x] = url[y]) == '%') {
      url[x] = x2c(&url[y+1]);
      y+=2;
    }
  }
  url[x] = '\0';
}


void 
plustospace(CHR *str) {
  INT x;
  for(x=0;str[x];x++)
    if(str[x] == '+')
      str[x] = ' ';
}


void 
spacetoplus(CHR *str) {
  INT x;
  for(x=0;str[x];x++)
    if(str[x] == ' ')
      str[x] = '+';
}


CHR* 
c2x(CHR what) {
  CHR *out=new CHR[4];
  sprintf(out, "%%%2x", what);
  return out;
}


void 
escape_url(CHR *url, CHR *out) {
  out[0] = '\0';
  INT x, y;
  for(x=0,y=0;url[x];++x,++y) {
    if (isalnum(url[x]) || (url[x] == ' ')) {
      out[y] = url[x];
    } else {
      CHR *esc = c2x(url[x]);
      INT plus = strlen(esc);
      out[y] = '\0';
      strcat(out, esc);
      y += (plus - 1);
      delete esc;
    }
  }
  out[y] = '\0';
  spacetoplus(out);
}
