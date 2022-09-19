
/*
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * Author:	Ray Larson, ray@sherlock.berkeley.edu
 *		School of Library and Information Studies, UC Berkeley
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**************************************************************************/
/* DispMARC - print marc records from a file                              */
/**************************************************************************/

#define index(s,c) strchr(s,c)

#include "gdt.h"
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include "marc.h"
#include "memcntl.h"
#include "marclib.hxx"
#include "marc.hxx"

MARC::MARC(STRING & Data)
{
	c_data = Data.NewCString();
	c_len = Data.GetLength();
	if((c_rec = GetMARC(c_data,c_len,0)) == NULL) {
		cerr << "Error parsing MARC record" << endl;
		return;
	}
	c_format = 0;
	c_maxlen = 79;
}

MARC::~MARC()
{
	if(c_data)
		delete [] c_data;

	// FREE THE c_rec!!
}


#define MENUHT 3

#define   RECBUFSIZE     10000
#define   FIELDBUFSIZE   10000
#define   READONLY       O_RDONLY
#define   BADFILE        -1
#define   TRUE           1
#define   FALSE          0

#ifndef   SEEK_CUR
#define   SEEK_CUR       1
#endif


/* EXTERNAL ROUTINES -- in marclib.c and memcntl.c */

struct MemBlock *RememberKey;
 
/* EXTERNAL VARIABLES */
extern struct MemBlock *RememberKey;    /* key for memory allocation */
char recbuffer[RECBUFSIZE];
char fieldbuffer[FIELDBUFSIZE];
char linebuffer[FIELDBUFSIZE];

typedef struct {
	char *label;
	char *tags;
	char *subfields;
        char *beginpunct;
	char *subfsep;
	char *endpunct;
        int  newfield;
	int  print_all;
	int  print_indicators;
	int  print_delimiters;
        int  repeatlabel;
        int  indent;
 } DISP_FORMAT;
	
DISP_FORMAT defaultformat[] = {
/*
     {"Record #", "", "",""," ","\n",     TRUE,FALSE,FALSE,FALSE, FALSE, 0},
*/
     {"Author:" , "1xx", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Title:"  , "245", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Publisher:", "260", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Pages:"  , "300", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Series:" , "4xx", "", ""," ", "\n", TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Notes:"  , "5xx", "", "", " ","\n",   TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Subjects:","6xx", "", "", " -- ",".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Other authors:","7xx", "", "", " ",".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Call Numbers:","950", "", "", " ","\n",TRUE,FALSE,FALSE,FALSE,FALSE,15},
     {NULL,NULL,NULL,NULL,NULL,NULL,FALSE,FALSE,FALSE,FALSE,FALSE,0}
};

DISP_FORMAT titleformat[] =  {
     {"Title:"  , "245", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {NULL,NULL,NULL,NULL,NULL,NULL,FALSE,FALSE,FALSE,FALSE,FALSE,0}
};


DISP_FORMAT shortformat[] =  {
/*
     {"Record #", "", "",""," ","\n",     TRUE,FALSE,FALSE,FALSE, FALSE, 0},
*/
     {"Author:" , "1xx", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Title:"  , "245", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {NULL,NULL,NULL,NULL,NULL,NULL,FALSE,FALSE,FALSE,FALSE,FALSE,0}
};

 DISP_FORMAT marcformat[] =  {
     {"Record ID: ", "", "",""," ","\n", TRUE,FALSE,FALSE,FALSE,FALSE, 0},
     {"" , "xxx", "", "","", "\n",  TRUE,TRUE,TRUE,TRUE,FALSE,0},
     {NULL,NULL,NULL,NULL,NULL,NULL,FALSE,FALSE,FALSE,FALSE,FALSE,0}
};

DISP_FORMAT evaluationformat[] = {
/*
     {"Record #", "", "",""," ","\n",     TRUE,FALSE,FALSE,FALSE, FALSE, 0},
*/
     {" ", "", "",""," "," ",     TRUE,FALSE,FALSE,FALSE, FALSE, 0},
     {" "  , "245", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
/*
     {"Title:"  , "245", "", ""," ", ".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Subjects:","6xx", "", "", " -- ",".\n",  TRUE,FALSE,FALSE,FALSE,FALSE, 15},
     {"Call Numbers:","950", "", "", " ","\n",TRUE,FALSE,FALSE,FALSE,FALSE,15},
*/
     {NULL,NULL,NULL,NULL,NULL,NULL,FALSE,FALSE,FALSE,FALSE,FALSE,0}
};

/* local prototypes */
char *format_field(MARC_FIELD *mf,DISP_FORMAT *format,char *buff,int repeat);
void outputline(void *(outfunc)(),char *line, int maxlen, int indent,
	FILE *fp);

void MARC::Print(FILE *fp)
{
	INT4 format = c_format;
	INT4 displaynum = -1;
	INT4 maxlen = c_maxlen;
	INT4 recordID = 1;

	MARC_FIELD *fld;
	//MARC_REC *rec;
	//INT4 lrecl;
	int  repeat = FALSE;
        char *line;
        DISP_FORMAT *formatcontrol, *f;
        
        switch (format) {
		case 1: formatcontrol = &shortformat[0];
			break;
		case 2: formatcontrol = &marcformat[0];
        		break;
		case 3: formatcontrol = &evaluationformat[0];
			break;
		case 4: formatcontrol = &titleformat[0];
			break;
        	default: formatcontrol = &defaultformat[0];
        		break;
        }

		if (displaynum == -1) displaynum = recordID;

                for (f = formatcontrol; f->label; f++) {
                	/* get the first field in the format */
			fld = GetField(c_rec, NULL, fieldbuffer, f->tags);

			/* if no field found, check for number format */
			if (fld == NULL && *f->tags == '\0') {
				/* a null tag means output the supplied */
				/* record number			*/
				sprintf(linebuffer,"%s%s%d%s", 
					f->label, f->beginpunct,displaynum, 
					f->endpunct);
				/* assume it won't be INT4er than maxlen*/
				outputline (NULL,linebuffer, maxlen, f->indent, fp);
			}	
			repeat = FALSE;
			
			while (fld) {
				if (f->print_all) {
					codeconvert(fieldbuffer);
					if (*f->label == '\0')
						sprintf(linebuffer,"%s %s%s%s", 
							fld->tag, f->beginpunct,fieldbuffer, f->endpunct);
					else
						sprintf(linebuffer,"%s %s%s%s", 
							f->label, f->beginpunct,fieldbuffer, f->endpunct);
					outputline (NULL, linebuffer, maxlen, f->indent, fp);
				}
				else  {/* more selective printing */
					line = format_field(fld,f,linebuffer,repeat);
					if (line) outputline (NULL, line, maxlen, f->indent, fp);
				}
				/* more of the same tag set? */
				fld = GetField(NULL,fld->next,fieldbuffer,f->tags);
				if (fld) repeat = TRUE;
			}
		}
	        return;
}


/***********************************************************************/
/* format_field - given a marc field struct and a format item, build   */
/*                a line in a buffer according to the format.          */
/***********************************************************************/
char *format_field(MARC_FIELD *mf,DISP_FORMAT *format,char *buff,int repeat)
{
	MARC_SUBFIELD *subf;
	register char *linend, *c;
        int pos, count, ok;
	
	linend = buff;
	*linend = '\0';
	pos = 0;

	if (repeat && (format->repeatlabel == FALSE)) ;  /* skip it */
	else /* add the label */
		for(c = format->label; *c ; *linend++ = *c++) pos++;

        /* indentation */
        for (; pos < format->indent; pos++) *linend++ = ' ';
        /* initial 'punctuation' */		
	for(c = format->beginpunct; *c ; *linend++ = *c++);
	/* subfields */
	for (subf = mf->subfield; subf; subf = subf->next) {
		if ((*format->subfields == '\0') || 
			(index(format->subfields, subf->code))) {	
			/* this one should be copied */
			for(c = format->beginpunct; *c ; *linend++ = *c++);
			count = subfcopy(linend,subf->data,1) - 1;
			linend += count;
			for(c = format->subfsep; *c ; *linend++ = *c++);
			ok = TRUE;
		}
	}
	
	if (ok) {
		/* backtrack over the last subfield separator */
		linend -= strlen(format->subfsep);
		/* add end of field punctuation */
		if (*format->endpunct) {
			/* kill the existing punctuation and trailing blanks */
			if (ispunct(*(linend-1)) && ispunct(*format->endpunct) &&
			   *(linend-1) != ')' && *(linend-1) != ']')
				linend--;
			while(*(linend-1) == ' ') linend--;
	 		for(c = format->endpunct; *c ; *linend++ = *c++) pos++;
 		}
		*linend = '\0'; /* Null terminate the line */
		return(buff);	
	}
	else return(NULL); /* no subfields copied */
}



/* break an output line into segments to fit on the screen and call the */
/* output function                                                      */
/*
KAG - Hacking this to ignore outfunc b/c I figure out how to get the C++
compiler to quit complaining about passing the wrong number of args
to outfunc():-(
*/
void outputline(void *(outfunc)(),char *line, int maxlen, int indent,
	FILE *fp)
{
  int linelen, i;
  char indentstr[80];
  char *nextpart, *c;

  /* if the line will fit output it now */
  if ((linelen = strlen(line)) <= maxlen) {
/*	(*outfunc)(line);*/
	fwrite(line, 1, strlen(line), fp);
	return;
  }
  else { /* put out first part, no indentation */
	for (c = &line[maxlen - 1]; *c != ' '; c--); /* find word break */
	*c = '\0';
	nextpart = c+1;
	fwrite(line, 1, strlen(line), fp);
	/*(*outfunc)(line);*/
	fprintf(fp, "\n");
	/*(*outfunc)("\n");*/
  }
  /* set up indent string - add 3 spaces to regular indent for wrapped lines */
  indent += 3;
  for (i=0;i<indent; i++) indentstr[i] = ' ';
  indentstr[i] = '\0';

  /* loop to output rest of line */
  while ((linelen = strlen(nextpart)) > (maxlen - indent)) {
        for (c = &nextpart[maxlen - indent]; *c != ' '; c--); 
             /* find word break */ 
        *c = '\0'; 
        /*(*outfunc)(indentstr); 
        (*outfunc)(nextpart); 
        (*outfunc)("\n");*/
	fwrite(indentstr, 1, strlen(indentstr), fp);
	fwrite(nextpart, 1, strlen(nextpart), fp);
	fprintf(fp, "\n");
        nextpart = c+1;  
  }
/*
  (*outfunc)(indentstr); 
  (*outfunc)(nextpart); */
fwrite(indentstr, 1, strlen(indentstr), fp);
fwrite(nextpart, 1, strlen(nextpart), fp);
  return;
}


