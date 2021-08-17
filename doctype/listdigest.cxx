#pragma ident  "@(#)listdigest.cxx	1.7 02/24/01 17:45:00 BSN"
/************************************************************************
************************************************************************/

/*-@@@
File:		listdigest.cxx
Version:	$Revision: 1.1 $
Description:	Class LISTDIGEST - Listserver Mail Digest Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <errno.h>
//#include <ctype.h>
#include "listdigest.hxx"


LISTDIGEST::LISTDIGEST (PIDBOBJ DbParent, const STRING& Name) :
	MAILDIGEST (DbParent, Name)
{
}

const char *LISTDIGEST::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("LISTDIGEST");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  MAILDIGEST::Description(List);
  return "Listserver Mail digest mails.";
}



void LISTDIGEST::SourceMIMEContent(PSTRING StringPtr) const
{
  // MIME/HTTP Content type for records
  *StringPtr = "Application/X-ListDigest";
}

const CHR *LISTDIGEST::Seperator() const
{
  return "\n========================================";
}

void LISTDIGEST::ParseRecords (const RECORD& FileRecord)
{
  // Break up the document into Mail message records
  GPTYPE Start = FileRecord.GetRecordStart ();
  GPTYPE End = FileRecord.GetRecordEnd();

  GPTYPE Position = 0;
  GPTYPE SavePosition = 0;
  GPTYPE RecordEnd;

  const STRING Fn (FileRecord.GetFullFileName ());
  PFILE Fp = ffopen (Fn, "rb");
  if (!Fp)
    {
      message_log (LOG_ERRNO, "Couldn't access '%s'", Fn.c_str());
      return;			// File not accessed
    }

  if (Start)
    {
      if (-1 == fseek (Fp, Start, SEEK_SET))
	{
	  ffclose(Fp);
	  message_log (LOG_ERRNO, "%s: Bad record boundary", Doctype.c_str()); 
	  return; // Bad start
	}
      SavePosition = Position = Start;
    }

  RECORD Record (FileRecord); // Easy way

  char buf[BUFSIZ];
  const char *magic = Seperator();
  if (*magic == '\n')
    magic++;
  const size_t magic_len = strlen(magic);
  int          saw_magic = 0;

  // Read lines from file and search for record seperation
  while (fgets(buf, sizeof(buf)/sizeof(char)-1, Fp) != NULL)
    {
      // Search for "magic" line type
      size_t line_len = strlen(buf);
      Position += line_len;
/*
      if (line_len >= (magic_len*2 - 2))
	{
	}
*/
      if (End > 0 && Position > End)
	break; // Done
      if (line_len > magic_len && strncmp(buf, magic, magic_len) == 0 )
	{
	  saw_magic++;
	  SavePosition = Position;
	  Record.SetRecordStart (Start);
	  RecordEnd = SavePosition - 1;

	  if (RecordEnd > Start)
	    {
	      Record.SetRecordEnd (RecordEnd);
	      Db->DocTypeAddRecord(Record);
	      Start = SavePosition;
	    }
	}
    }

  ffclose (Fp);

  Record.SetRecordStart (Start);
  RecordEnd = Position - 1;

  if (RecordEnd > Start)
    {
      if (saw_magic == 0)
	{
	  Record.SetDocumentType ("MAILFOLDER");
	}
      Record.SetRecordEnd (RecordEnd);
      Db->DocTypeAddRecord(Record);
    }
}

LISTDIGEST::~LISTDIGEST ()
{
}
