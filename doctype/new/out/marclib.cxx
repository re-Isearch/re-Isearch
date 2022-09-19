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

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "gdt.h"
#include "marc.h"
#include "memcntl.h"

int rescan(int file,char *buffer,int badpos);

/***********************************************************/
/* Key for memory allocation                               */
/***********************************************************/
	extern struct MemBlock *RememberKey;
/*
	extern char *AllocSafe();
*/

/***********************************************************/
/*  Maximum number of records count - set in SeekMarc      */
/***********************************************************/

	INT4 MaxRecs = 0;


/*********************************************************************/
/* GetNum converts a specifed number of characters to a number       */
/*********************************************************************/

INT4 GetNum(char *s,int n)
{  
	char nbuf[10]; /* no more than 9 digits */

	strncpy(&nbuf[0], s, n);
	nbuf[n] = '\0';
	return(atol(nbuf));
}


/*********************************************************************/
/* SetSubF  - attaches  a list of subfield records to a field record */
/*            returns NULL if there is no more memory                */
/*********************************************************************/

int SetSubF(MARC_FIELD *f, char *dat)
{
	MARC_SUBFIELD *s;

	if (*dat != SUBFDELIM)
		return(1);
   
	while (*dat != FIELDTERM && *dat != RECTERM) {
		if (*dat == SUBFDELIM) {
			if ((s = (MARC_SUBFIELD *)AllocSafe(&RememberKey,
			  (INT4) sizeof(MARC_SUBFIELD),
			  MEMF_PUBLIC | MEMF_CLEAR, GENERALMEM)) == NULL) {
				fprintf(stderr, "couldn't allocate marc subfield structure\n");
				return(0);
			}
			f->subfcodes[0]++;
			f->subfcodes[(short)f->subfcodes[0]] = *(dat+1);
			s->code = *(dat+1);
			s->data = dat+2;
			if (f->lastsub != NULL)
				f->lastsub->next = s;
			if (f->subfield == NULL)
				f->subfield = s;
			f->lastsub = s;
			s->next = NULL;
		}
		dat++;
	}
	return(1);
}


/*********************************************************************/
/* SetField - installs a marc field into a field processing structure*/
/*            returns null if there are problems with allocation     */
/*********************************************************************/

MARC_FIELD *SetField(MARC_REC *rec, MARC_DIRENTRY_OVER *dir)
{
	MARC_FIELD *f;
	char *p;
	int i;

	if ((f = (MARC_FIELD *)AllocSafe(&RememberKey,
	  (INT4)sizeof(MARC_FIELD),MEMF_PUBLIC | MEMF_CLEAR,
	  GENERALMEM)) == NULL) {
		fprintf(stderr,"couldn't allocate marc field structure\n");
		return(NULL);
	}
	if (rec->nfields == 0)
		rec->fields = f;
	rec->nfields++;
	if (rec->lastfield != NULL)
		rec->lastfield->next = f;
	rec->lastfield = f;
	for (i=0; i<3; i++)
		f->tag[i] = dir->tag[i];
	f->tag[3] = '\0';
	f->length = GetNum(dir->flen,4);
	f->data = rec->BaseAddr + GetNum(dir->fstart,5);
	f->indicator1 = *(f->data);
	f->indicator2 = *(f->data + 1);
	f->next = NULL;
	f->subfield = NULL;
	f->lastsub = NULL;
	p = f->data + 2;
	if (SetSubF(f,p))
		return(f);
	else
		return(NULL);
}


/*********************************************************************/
/* ReadMARC - reads a marc record into a buffer and returns          */
/*            the length of the record - also appends a NULL to the  */
/*            end the buffer                                         */
/*           Returns 0 at end-of-file                                */
/*********************************************************************/

INT4 ReadMARC(int file,char *buffer, int buffsize)
{
	INT4 recoffset, lrecl;
	int i;

	if(read(file,buffer,5L) == 0L)
		return(0); /* end of file */

	/* verify that the buffer does contain digits */
	for (i=0; i < 5; i++)
		if (!isdigit(buffer[i]) &&
		  (recoffset = rescan(file,buffer,0)) == 0) {
			fprintf(stderr,"Bad record - unable to rescan\n");
			return(0);
		}
	lrecl = GetNum(buffer,5);
	if (lrecl >= buffsize) {
		fprintf(stderr,"input buffer too small: buffer %d : record %d\n",
		  buffsize, lrecl);
		return(0);
     }
	if (read(file, buffer+5, lrecl-5L) == 0)
		return(0); /* end of file */
	buffer[lrecl] = '\0';
	return(lrecl);
}    


/*********************************************************************/
/* rescan - pick through garbage preceding a Marc record             */
/*          returns the new record offset in the file when it is     */
/*          successful, returns zero when it fails to fix things     */
/*********************************************************************/

int rescan(int file,char *buffer,int badpos)
{
	char c;
	int badcount, i;
	INT4 currentpos;

	badcount = 0;

	/* seek back in the file to the record terminator */
	currentpos = lseek(file,-6,1);
	read(file,&c,1); /*get one byte*/
	if (c == RECTERM)
		fprintf(stderr,"Rescanning - previous record OK\n");

	/* skip non-digits */
	while (isdigit(c) == 0) {
		badcount++;
		if (read(file,&c,1)) {
			fprintf(stderr,"%c",c);
		}
		else /* end of file */ {
			fprintf(stderr,"End of file in rescan - quitting\n");
			exit(0);
		}
	}
	fprintf(stderr,"%d bytes skipped at file position %d\n",
	  badcount, currentpos+1);

	/* start putting digits into the buffer */
	buffer[0] = c;
	fprintf(stderr,"new record length = %c",c);
	for (i=1; i<5; i++) {
		if (read(file,&buffer[i],1) == 0) {
			fprintf(stderr,"End of file in rescan - quitting\n");
			exit(0);
		}
		fprintf(stderr,"%c",buffer[i]);
		if (isdigit(buffer[i]) == 0) {
			fprintf(stderr,"Non-digit in suspected record length\n");
			return(0);
		}
	}
	fprintf(stderr,"\n\n");

	/* if we get to here should be a valid record (we hope) */
	return(lseek(file,0,1) - 5);
}


/*********************************************************************/
/* SeekMARC - uses offsets in an associator file to position the     */
/*            MARC file to a given logical record number -           */
/*            the associator file and MARC files must be open        */
/*            returns -1 on seek errors and the marc file position   */
/*            when successful.                                       */
/*********************************************************************/

INT4 SeekMARC(int marcfile,int assocfile,int recnumber)
{
	INT4 marcoffset;
	INT4 seekreturn;

	if (MaxRecs == 0) {
		seekreturn = lseek(assocfile,0,0);
		read(assocfile,&MaxRecs,sizeof(INT4));
	}
       

	if (recnumber > MaxRecs) {
		fprintf(stderr,"Bad marc record number in SeekMARC\n");
		return (-1);
	}

	if (recnumber != 0) { 
		seekreturn = lseek(assocfile,(recnumber * sizeof(INT4)),0);
		if (seekreturn == -1) {
			if (errno == EBADF)
				fprintf(stderr,"unable to use associator file\n");
			else if (errno == EINVAL)
				fprintf(stderr,"invalid seek value in SeekMARC\n");
			else
				fprintf(stderr,"associator file error in SeekMARC\n");
			return (-1);
		}
		else
			read(assocfile,&marcoffset,sizeof(INT4));
	}
	else
		marcoffset = 0;

	seekreturn = lseek(marcfile,marcoffset,0);
	if (seekreturn == -1) {
		if (errno == EBADF)
			fprintf(stderr,"unable to use marc file\n");
		else if (errno == EINVAL)
			fprintf(stderr,"invalid seek value in SeekMARC\n");
		else
			fprintf(stderr,"marc file error in SeekMARC\n");
		return (-1);
	}
	else
		return(seekreturn); /* return current file position */
}    


/*********************************************************************/
/* GetMARC - Moves a marc record into the marc processing structure  */
/*           and returns a pointer to the structure                  */
/*           if the copy flag is not zero a new record buffer is     */
/*           allocated and the input buffer copied to it             */
/*********************************************************************/
MARC_REC *GetMARC(char *buffer,INT4 lrecl,int copy)
{
	MARC_DIRENTRY_OVER *dir;
	MARC_REC *m;
	char *record;

	if (copy) {
		if ((record = AllocSafe(&RememberKey,lrecl+1,
		  MEMF_PUBLIC | MEMF_CLEAR,GENERALMEM)) == NULL) {
			fprintf(stderr,"couldn't allocate record\n");
			return(NULL);
		}
		strcpy(record,buffer);
	}
	else
		record = buffer;

	if ((m = (MARC_REC *)AllocSafe(&RememberKey,
	  (INT4)sizeof(MARC_REC), MEMF_PUBLIC | MEMF_CLEAR,
	  GENERALMEM)) == NULL) {
		fprintf(stderr,"couldn't allocate marc processing structure\n");
		return(NULL);
	}
	m->length = lrecl;
	m->record = record;
	m->leader = (MARC_LEADER_OVER *)record;
	m->BaseAddr = record + GetNum(m->leader->BaseAddr,5);
	dir = (MARC_DIRENTRY_OVER *)(record + sizeof(MARC_LEADER_OVER));
	for (; isdigit(dir->tag[0]); dir++)
		if ( SetField(m,dir) == NULL) {
			FreeSafe(&RememberKey,(char *)m,0);
			fprintf(stderr,"could not setfield in getmarc\n");
			return(NULL);
		}
	return (m);
}     
 

/*********************************************************************/
/* fieldcopy  -  copy string From to string To stopping when either  */
/*            terminal null character, or a field terminator         */
/*            is encountered in the From string.                     */
/*            returns number of chars copied                         */
/*********************************************************************/

int fieldcopy(char *To, char *From)
{
	register char *t, *f;
	register int count;

	t = To;
	f = From;
	count = 0;

	while (*f && *f != FIELDTERM && *f != RECTERM) {
		*t++ = *f++;
		count++;
	}
	*t = '\0';
	return(count+1);
}


/*********************************************************************/
/* codeconvert  - convert subfield codes and other non-print chars   */
/*                in a null terminated string                        */
/*********************************************************************/
void codeconvert(char *string)
{
	while (*string) {
        if (*string == SUBFDELIM) *string = '$';
        if (*string == FIELDTERM) *string = '+';
        if (*string == RECTERM) *string = '|';
        if (*string && iscntrl(*string)) *string = '?';
        string++;
	}
}


/*********************************************************************/
/* charconvert  - convert subfield codes and other non-print chars   */
/*                normal characters are returned unchanged           */
/*********************************************************************/

char charconvert(char c)
{
	switch (c) {
		case  SUBFDELIM :  return( '$');
		case  FIELDTERM :  return('+');
		case  RECTERM :  return('|');
		case  '\0': return(c);
		default:  return (iscntrl(c) ? '?' : c);
	}
}


/*********************************************************************/
/* subfcopy  -  copy string From to string To stopping when either   */
/*            terminal null character, subfield delim or a field     */
/*            terminator is encountered in the From string.          */
/*            returns number of chars copied                         */
/*            if flag is 1, non-ascii characters are removed         */
/*********************************************************************/

int subfcopy(char *To, char *From,int flag)
{
	register char *t, *f;
	register int count;

	t = To;
	f = From;
	count = 0;

	while(*f && *f != FIELDTERM && *f != RECTERM && *f != SUBFDELIM) {
		if (flag) {
			if (*f > 0) {
				*t++ = *f++;
				count++;
			}
			else f++;
		}
		else {
			*t++ = *f++;
			count++;
		}
	}
	*t = '\0';
	return(count+1);
}


/********************************************************************/
/* tagcmp - compare two marc tags with X or x as wildcards.         */
/*          returns -1 for no match and 0 for match                 */
/********************************************************************/

int tagcmp(char *pattag, char *comptag)
{
	int i;
	for (i = 0; i < 3; i++) {
		if (pattag[i] == 'x' || pattag[i] == 'X')
			continue;
		if (isdigit(pattag[i]) && pattag[i] == comptag[i])
			continue;
		else
			return(-1);
	}
	/* must match */
	return(0);
} 


/*********************************************************************/
/* GetField - extract a field with a given tag from a marc           */
/*            processing structure. Returns a pointer to the         */
/*            field and copies the field to the buffer(if provided)  */
/*            successful. Returns NULL if it fails to find the       */
/*            field. Startf lets it start from later in a field list.*/
/*            Permits "wildcard" comparisons using tagcmp            */
/*********************************************************************/
MARC_FIELD *GetField(MARC_REC *rec,MARC_FIELD *startf,char *buffer,char *tag)
{
	MARC_FIELD *f;
   
	if (rec == NULL && startf == NULL)
		return (NULL);
	if (buffer)
		buffer[0] = '\0';
	if (startf)
		f = startf;
	else
		f = rec->fields;
   
	while     (f && tagcmp(tag,f->tag))
		f = f->next;
	if (f && buffer)
		fieldcopy(buffer,f->data);
	return(f);
}
     

/*********************************************************************/
/* GetSubf - get a named subfield from a marc processing structure   */
/*           for a given field. returns a pointer to the subfield    */
/*           and copies it to a buffer (if provided) when successful */
/*           Returns NULL if it fails to find the subfield           */
/*********************************************************************/
MARC_SUBFIELD *GetSubf(MARC_FIELD *f, char *buffer, char code)
{
  MARC_SUBFIELD *s;
  char *c;

  if (f == NULL) return(NULL);
  c = &f->subfcodes[1];
  while(*c && *c != code) c++;
  if(*c) /* go for the data */ 
    { s = f->subfield;
      while(s)  
         { if (s->code == code)
              { if (buffer) subfcopy(buffer,s->data,0);
                return(s);
              }
           s = s->next;
         }
     }
   return(NULL); /* didn't find it */
}

/************************************************************************/
/* NORMALIZE -- convert an LC call number to a standardized format      */
/*              returns NULL if call number is abnormal, out otherwise  */
/************************************************************************/
char *normalize(char *in, char *out)
{
  char mainclass[4], mainsub[7];
  char decimal[7], subcutter[10];
  int  valmainsub, ofs,decflag;
  char sep;

  for (ofs = 0; ofs < sizeof mainclass - 1 && isalpha(*in); ofs++, in++)
	*(mainclass+ofs) = islower(*in) ? toupper(*in):*in;
  if (ofs && ofs < sizeof mainclass)
	*(mainclass+ofs) = '\0';
  else
	return(NULL);

  while (*in && isspace(*in))
	in++;                    /* skip blanks */

  for (ofs = 0; ofs < sizeof mainsub - 1 && isdigit(*in); ofs++, in++)
	*(mainsub+ofs) = *in;
  if (ofs && ofs < sizeof mainsub)
	*(mainsub+ofs) = '\0';
  else
	return(NULL);
  valmainsub = atoi(mainsub);

  if (*in == '\0') {
	decflag = 0;
	decimal[0] = '\0';
	subcutter[0] = '\0';
  }
  else {                      /* still more of the call number */
	while (*in && isspace(*in))
		in++;               /* skip blanks */
	if (*in == '.' && isdigit(*(in+1)))
		decflag = 1;
	else
		decflag = 0;
	in++;
	decimal[0] = '\0';
	if (decflag) {
		while(isspace(*in))
			in++;          /* skip blanks */
		for (ofs = 0; ofs < sizeof decimal - 1 && isdigit(*in); ofs++, in++)
			*(decimal+ofs) = *in;
		if (ofs < sizeof decimal)
			*(decimal+ofs) = '\0';
		else
			return(NULL);
	}

	 /* this could be changed to separate successive cutters */
	 while (*in && isspace(*in))
		in++;               /* skip blanks */
	 if (*in == '.' && isalnum(*(in+1)))
	 	in++;
	 for (ofs = 0; ofs < sizeof subcutter - 1 && isalnum(*in); ofs++, in++)
		*(subcutter+ofs) = islower(*in) ? toupper(*in):*in;
	 if (ofs < sizeof subcutter)
		*(subcutter+ofs) = '\0';
	 else
		return(NULL);
  }

  decflag ? (sep = '.') : (sep = ' ');
  sprintf(out, "%-3s%05d%c%-5s %-10s", mainclass,
    valmainsub, sep, decimal, subcutter);
  return(out);
}

