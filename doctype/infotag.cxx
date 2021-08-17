#pragma ident  "@(#)infotag.cxx	1.4 05/08/98 13:36:18 BSN"
/************************************************************************
************************************************************************/

/*-@@@
File:		infotag.cxx
Version:	1.03
Description:	Class INFOTAG - SGML-like Text
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
////#include <sys/time.h>
//#include <errno.h>
#include "common.hxx"
#include "infotag.hxx"

// Local prototypes
static char *find_end_tag(char **t, char *tag);
static char **parse_tags(char *b, int len);

INFOTAG::INFOTAG(PIDBOBJ DbParent, const STRING& Name) :
	COLONDOC(DbParent, Name)
{
}

const char *INFOTAG::Description(PSTRLIST List) const
{
  List->AddEntry ("INFOTAG");
  DOCTYPE::Description(List);
  return "Simple XML-like tagged text";
}



void INFOTAG::SourceMIMEContent(PSTRING StringPtr) const
{
  *StringPtr =  "text/xml";
}

/*
- Open the file.
- Read two copies of the file into memory (implementation feature;-)
- Build an index into the SGML-like tag pairs.  An SGML-like tag pair is one
	that begins and ends with precisely the same text, excluding the
	closing tag's slash.  For example:

		<title> </title>	- xml-like
		<dog> </dog>		- xml-like
		<!-- test>		- NOT sgml-like
		<a href=> </a>		- NOT sgml-like
- For each valid tag pair, hence field, add the field to the Isearch record
	structure.
- Cleanup
*/
void INFOTAG::ParseFields(PRECORD NewRecord) {
	PFILE 	fp;
	STRING 	fn;
	GPTYPE 	RecStart, 
		RecEnd, 
		RecLength, 
		ActualLength;
	PCHR 	RecBuffer, 
		OrigRecBuffer;
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
			 message_log (LOG_ERRNO, "INFOTAG::ParseRecords(): Seek failed - '%s'",
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
			message_log (LOG_WARN, Message);
			message_log (LOG_NOTICE, "INFOTAG skipping record");
			return;
		}
		//RecEnd -= 1;
	}

	// Make two copies of the record in memory
	if(fseek(fp, RecStart, 0) == -1) {
		message_log (LOG_ERRNO, "INFOTAG::ParseRecords(): Seek failed - '%s'",
			(const char *)fn);
		Db->ffclose(fp);
		return;	
	}
	RecLength = RecEnd - RecStart;
	
	RecBuffer = new CHR[RecLength + 1];
	if(!RecBuffer) {
		Db->ffclose(fp);
		STRING Message;
		Message << "INFOTAG::ParseRecords(): Failed to allocate " <<
			(INT)(RecLength + 1) << " bytes - " << fn;
		message_log (LOG_ERRNO, Message);
		return;
	}
	OrigRecBuffer = new CHR[RecLength + 1];
	if(!OrigRecBuffer) {
		delete [] RecBuffer;
		Db->ffclose(fp);
		STRING Message; 
		Message << "INFOTAG::ParseRecords(): Failed to allocate " <<
			(INT)(RecLength + 1) << " bytes - " << fn;
                message_log (LOG_ERRNO, Message); 
		return;
	}

	ActualLength = (GPTYPE)fread(RecBuffer, 1, RecLength, fp);
	if(ActualLength == 0) {
		message_log (LOG_ERRNO, "INFOTAG::ParseRecords(): Failed to fread");
		delete [] RecBuffer;
		delete [] OrigRecBuffer;
		Db->ffclose(fp);
		return;
	}
	Db->ffclose(fp);
	if(ActualLength != RecLength) {
		message_log (LOG_ERROR, "INFOTAG::ParseRecords(): fread less bytes");
		delete [] RecBuffer;
		delete [] OrigRecBuffer;
		return;
	}
	memcpy(OrigRecBuffer, RecBuffer, RecLength);
	OrigRecBuffer[RecLength] = '\0';

	// Parse the record and add fields to record structure
	STRING FieldName;
	FC fc;
	PFCT pfct;
	DF df;
	PDFT pdft;
	PCHR *tags;
	PCHR *tags_ptr;
	PCHR p;
	INT val_start;
	INT val_len;
	DFD dfd;

	pdft = new DFT();
	if(!pdft) {
		message_log (LOG_ERRNO, "INFOTAG::ParseRecords(): Failed to allocate DFT");
		delete [] RecBuffer;
		delete [] OrigRecBuffer;
		return;
	}
	tags = parse_tags(RecBuffer, RecLength);
	if(tags == NULL) {
		message_log (LOG_ERROR, "Unable to parse XML file %s", (const char *)fn);
		delete pdft;
		delete [] RecBuffer;
		delete [] OrigRecBuffer;
		return;
	}
	tags_ptr = tags;	
	while(*tags_ptr) {
		p = find_end_tag(tags_ptr, *tags_ptr);
		if(p) {
			// We have a tag pair
			val_start = (*tags_ptr + strlen(*tags_ptr) + 1) - 
				RecBuffer;
			val_len = (p - *tags_ptr) - strlen(*tags_ptr) - 2;
			FieldName = *tags_ptr;
			dfd.SetFieldName(FieldName);
			Db->DfdtAddEntry(dfd);
			fc.SetFieldStart(val_start);
			fc.SetFieldEnd(val_start + val_len -1);
			pfct = new FCT();
			pfct->AddEntry(fc);
			df.SetFct(*pfct);
			df.SetFieldName(FieldName);
			pdft->AddEntry(df);
			delete pfct;
		}
		tags_ptr++;
	}

	NewRecord->SetDft(*pdft);
	delete pdft;
	delete [] RecBuffer;
	delete [] OrigRecBuffer;
	delete tags;
}

// edz@nonmonotonic.net: Added (Use second-level tag)
void INFOTAG::
Present (const RESULT& ResultRecord, const STRING& ElementSet,
      const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  if (ElementSet.Equals(BRIEF_MAGIC))
    COLONDOC::Present(ResultRecord, "TITLE", RecordSyntax, StringBuffer);
  else
    DOCTYPE::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}

INFOTAG::~INFOTAG() {
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
#define TAG_GROW_SIZE 128	// allocate memory in chunks
#define OK 0			// two state conditions for parser
#define NEED_END 1
static char **parse_tags(char *b, int len)
{
	char **t, **u;		// array of pointers to first char of tags
	int tc=0;		// tag count
	int i,j;			// iterator
	int max_num_tags;	// max num tags for which space is allocated
	int State = OK;		// initialize the state

	// You should allocate character pointers (to tags) as you need them.  
	// Start with TAG_GROW_SIZE of them.
	max_num_tags = TAG_GROW_SIZE;
	t = new PCHR[max_num_tags];
	if(!t) {
		message_log (LOG_ERRNO, "INFOTAG::parse_tags(): couldn't allocate tag");
		return NULL;
	}

	// Step through every character in the buffer looking for '<' and '>'
	for(i=0;i < len;i++) {
		switch(b[i]) {
			case '>':
				if(State != NEED_END)
					break;
				b[i] = '\0';
				tc += 1;
				State = OK;
				break;
			case '<':
				State = NEED_END;
				t[tc] = &b[i+1];
				break;
			default:
				break;
		}
		if(tc == max_num_tags - 1) {
			// allocate more space
			max_num_tags += TAG_GROW_SIZE;
			u = new PCHR[max_num_tags];
			if(u == NULL) {
				delete [] t;
				return NULL;
			}
			for (j=0; j<=tc; j++)
				u[j] = t[j];
			delete [] t;
			t = u;
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
char *find_end_tag(char **t, char *tag)
{
	char *tt;
	int i;

	if(*t == NULL)
		return NULL;

	if(*t[0] == '/')
		return NULL;

	for(i=0, tt = *t; tt ; tt = t[++i]) {
		if(tt[0] == '/') {
			if(!(strcmp(&tt[1], tag)))
				return tt;
		}
	}
	return NULL;
}
