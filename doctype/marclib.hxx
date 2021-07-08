
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
#ifndef _marclib_
#define _marclib_
/*
#ifdef __cplusplus
extern "C" {
#endif
*/
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>

#include "marc.h"
#include "memcntl.h"

INT4 GetNum(char *s,int n);
int SetSubF(MARC_FIELD *f, char *dat);
MARC_FIELD *SetField(MARC_REC *rec, MARC_DIRENTRY_OVER *dir);
INT4 ReadMARC(int file,char *buffer, int buffsize);
INT4 SeekMARC(int marcfile,int assocfile,int recnumber);
MARC_REC *GetMARC(char *buffer,INT4 lrecl,int copy);
int fieldcopy(char *To, char *From);
void codeconvert(char *string);
char charconvert(char c);
int subfcopy(char *To, char *From,int flag);
int tagcmp(char *pattag, char *comptag);
MARC_FIELD *GetField(MARC_REC *rec,MARC_FIELD *startf,char *buffer,char *tag);
MARC_SUBFIELD *GetSubf(MARC_FIELD *f, char *buffer, char code);
char *normalize(char *in, char *out);
/*
#ifdef __cplusplus
}
#endif
*/
#endif

