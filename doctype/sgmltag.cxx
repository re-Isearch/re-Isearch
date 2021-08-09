#pragma ident  "@(#)sgmltag.cxx	1.10 05/09/01 13:48:03 BSN"
/************************************************************************
************************************************************************/

/*-@@@
File:		sgmltag.cxx
Version:	1.03
Description:	Class SGMLTAG - SGML-like Text
Author:		Kevin Gamiel, Kevin.Gamiel@cnidr.org
Changes:	1.02
			Thanks to Jae Chang (jae+@CMU.EDU) for these fixes:
			- RecBuffer was being overwritten
			- OrigRecBuffer was being overwritten
			- Use all new and deletes instead of mixing with
				mallocs and frees.
			- Free memory if parse fails
		1.03
			- Misc Error checking
		1.04	- E. Zimmermann (edz@nonmonotonic.net) added
			  SourceMIMEContent()
@@@*/

//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <sys/time.h>
//#include <errno.h>
#include "common.hxx"
#include "sgmltag.hxx"

// Local prototypes
const char *find_end_tag(char **t, char *tag);

SGMLTAG::SGMLTAG(PIDBOBJ DbParent, const STRING& Name) :
	DOCTYPE(DbParent, Name)
{
}

const char *SGMLTAG::Description(PSTRLIST List) const
{
  List->AddEntry ("SGMLTAG");
  DOCTYPE::Description(List);
  return "SGML-like tagged text";
}



void SGMLTAG::SourceMIMEContent(PSTRING StringPtr) const
{
  *StringPtr =  "Application/X-SGMLTAG";
}

// Produce MIME content on the basis of the upper level
// typical examples: Gils, USMarc, WAIS, ...
void SGMLTAG::SourceMIMEContent(const RESULT& ResultRecord,
	PSTRING StringPtr) const
{
  STRING FieldName;
  STRING Key;
  DFDT Dfdt;
  ResultRecord.GetKey(&Key);
  if (Db->GetRecordDfdt (Key, &Dfdt))
    {
      DFD Dfd;
      Dfdt.GetEntry(1, &Dfd);
      Dfd.GetFieldName (&FieldName);
    }

  STRING MIMEContent;
  SourceMIMEContent(&MIMEContent);
  if (FieldName.GetLength())
    {
      MIMEContent << "-" << FieldName;
    }

  *StringPtr = MIMEContent;
}

void SGMLTAG::BeforeRset (const STRING& RecordSyntax)
{
  DOCTYPE::BeforeRset(RecordSyntax);
}

void SGMLTAG::AfterIndexing()
{
  recBuffer.Free(Doctype.c_str(), "recBuffer");
  tagsBuffer.Free(Doctype.c_str(), "tagsBuffer");
  DOCTYPE::AfterIndexing();
}

/*
- Open the file.
- Read two copies of the file into memory (implementation feature;-)
- Build an index into the SGML-like tag pairs.  An SGML-like tag pair is one
	that begins and ends with precisely the same text, excluding the
	closing tag's slash.  For example:

		<title> </title>	- sgml-like
		<dog> </dog>		- sgml-like
		<!-- test>		- NOT sgml-like
		<a href=> </a>		- NOT sgml-like
- For each valid tag pair, hence field, add the field to the Isearch record
	structure.
- Cleanup
*/
void SGMLTAG::ParseFields(PRECORD NewRecord) {
	PFILE 	fp;
	STRING 	fn;
	GPTYPE 	RecStart, 
		RecEnd, 
		RecLength, 
		ActualLength;
	PCHR 	RecBuffer; 
	PCHR 	file;

	// Open the file
	NewRecord->GetFullFileName(&fn);
	file = fn.NewCString();
	fp = Db->ffopen(fn, "rb");
	if (!fp) {
		return;
	}

	// Determine the start and size of the record
	RecStart = NewRecord->GetRecordStart();
	RecEnd = NewRecord->GetRecordEnd();
	if (RecEnd == 0) {
		if(fseek(fp, 0, 2) == -1) {
			 logf (LOG_ERRNO, "SGMLTAG::ParseRecords(): Seek failed - '%s'",
				(const char *)fn);
			Db->ffclose(fp);
			return;	
		}
		RecStart = 0;
		RecEnd = ftell(fp);
		if(RecEnd == 0) {
			Db->ffclose(fp);
			STRING Message;
			Message << "zero-length record - '" << fn << "'";
			if (RecEnd || RecStart)  
				Message << "[" << (INT)RecStart << "-" << (INT)RecEnd << "]"; 
			Message << "...";
			logf (LOG_WARN, Message);
			logf (LOG_NOTICE, "SGMLTAG skipping record");
			return;
		}
		//RecEnd -= 1;
	}

	// Make two copies of the record in memory
	if(fseek(fp, RecStart, 0) == -1) {
		logf (LOG_ERRNO, "SGMLTAG::ParseRecords(): Seek failed - '%s'",
			(const char *)fn);
		Db->ffclose(fp);
		return;	
	}
	RecLength = RecEnd - RecStart + 1;
	
	RecBuffer = (PCHR)recBuffer.Want(RecLength + 1);
	if(!RecBuffer) {
		Db->ffclose(fp);
		return;
	}
	ActualLength = (GPTYPE)fread(RecBuffer, 1, RecLength, fp);
	if(ActualLength == 0) {
		logf (LOG_ERRNO, "SGMLTAG::ParseRecords(): Failed to fread");
		Db->ffclose(fp);
		return;
	}
	Db->ffclose(fp);
	if(ActualLength != RecLength) {
		logf (LOG_ERROR, "SGMLTAG::ParseRecords(): fread less bytes");
		return;
	}

	// Parse the record and add fields to record structure
	STRING FieldName;
	FC fc;
	DF df;
	PDFT pdft;
	PCHR *tags;
	PCHR *tags_ptr;
	INT val_start;
	INT val_len;
	DFD dfd;

	pdft = new DFT();
	if(!pdft) {
		logf (LOG_ERRNO, "SGMLTAG::ParseRecords(): Failed to allocate DFT");
		return;
	}
	tags = parse_tags(RecBuffer, RecLength);
	if(tags == NULL) {
		logf (LOG_ERROR, "Unable to parse SGML file %s", (const char *)fn);
		delete pdft;
		return;
	}
	tags_ptr = tags;	
	while(*tags_ptr) {
		const char *p = find_end_tag(tags_ptr, *tags_ptr);
		if(p && *p) {
			// We have a tag pair
			val_start = (*tags_ptr + strlen(*tags_ptr) + 1) - 
				RecBuffer;
			val_len = (p - *tags_ptr) - strlen(*tags_ptr) - 2;
			FieldName = *tags_ptr;
			dfd.SetFieldName(FieldName);
			Db->DfdtAddEntry(dfd);
			fc.SetFieldStart(val_start);
			fc.SetFieldEnd(val_start + val_len -1);
			df.SetFct(fc);
			df.SetFieldName(FieldName);
			pdft->AddEntry(df);
		} else if (p == NULL)
		  logf (LOG_WARN, "No End-tag for <%s>", *tags_ptr);
		tags_ptr++;
	}

	NewRecord->SetDft(*pdft);
	delete pdft;
}

// edz@nonmonotonic.net: Added (Use second-level tag)
void SGMLTAG::
Present (const RESULT& ResultRecord, const STRING& ElementSet,
      const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  if (ElementSet.Equals(BRIEF_MAGIC))
    {
      DFD Dfd;
      INT entry = (Db->DfdtGetTotalEntries() > 1) ? 2 : 1;
      Db->DfdtGetEntry (entry, &Dfd);
      STRING ESet;
      Dfd.GetFieldName (&ESet);
      DOCTYPE::Present(ResultRecord, ESet, RecordSyntax, StringBuffer);
      StringBuffer->Pack ();
    }
  else
    {
      DOCTYPE::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
    }
}

SGMLTAG::~SGMLTAG() {
}

/*
What:	Given a buffer of sgml-tagged data:
		returns a list of char* to all characters immediately following
			each '<' character in the buffer.
		replaces all '>' with '\0' character.
		
Pre:	b = character buffer with valid sgml marked-up text
	len = length of b
	tags = Empty char**

Post:	On success, return value is filled with char pointers to first 
		character of every sgml tag (first character after the '<').  
		Each string is NULL-terminated (where '>' used to reside).
		The tags array is terminated by a NULL char*.
	On failure, returns NULL.
*/
#define OK 0			// two state conditions for parser
#define NEED_END 1
char **SGMLTAG::parse_tags(char *b, off_t len)
{
	char **t;		// array of pointers to first char of tags
	size_t tc=0;		// tag count
	int State = OK;		// initialize the state

	// You should allocate character pointers (to tags) as you need them.  
	// Start with TAG_GROW_SIZE of them.
	t = (PCHR *)tagsBuffer.Want (256, sizeof(PCHR));
	if(!t) {
		logf (LOG_ERRNO, "SGMLTAG::parse_tags(): couldn't allocate tag");
		return t;
	}

	// Step through every character in the buffer looking for '<' and '>'
	for(off_t i=0;i < len;i++) {
		switch(b[i]) {
			case '>':
				if(State != NEED_END)
					break;
				b[i] = '\0';
				if ((t = (PCHR *)tagsBuffer.Expand(++tc, sizeof(PCHR))) == NULL) {
					logf (LOG_ERRNO, "SGMLTAG::parse_tags(): couldn't allocate expanded tag");
					return t;
				}
				State = OK;
				break;
			case '<':
				{ char ch;
				  while ( (ch = b[i+1]) != '>' && (ch == ' ' || ch == '\r' || ch == '\n') && i < len) i++;
				  if (i==len || ch == '?' || ch == '!' || ch == '>')
				      break;
				}
				State = NEED_END;
				t[tc] = &b[i+1];
				break;
			default:
				break;
		}
	}
	t[tc] = (PCHR)NULL;
	return t;
}

/*
What:	Searches through string list t looking for "/" followed by tag, e.g. 
	if tag = "TITLE", looks for "/TITLE".

Pre:	t is list of string pointers each NULL-terminated.  The list itself
	should be terminated with a NULL character pointer.
	
Post:	Returns a pointer to found string or NULL.
*/
const char *find_end_tag(char **t, char *tag)
{
	if(*t == NULL || *t[0] == '/')
		return "";

	char *tt;
	size_t i;
	for(i=0, tt = *t; tt ; tt = t[++i]) {
		if(tt[0] == '/') {
			if(!(StrCaseCmp(&tt[1], tag)))
				return tt;
		}
	}
	return NULL;
}
