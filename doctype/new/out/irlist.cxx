#pragma ident  "@(#)irlist.cxx	1.8 02/24/01 17:45:01 BSN"
/************************************************************************
************************************************************************/
/*-@@@
File:		irlist.cxx
Version:	$Revision: 1.1 $
Description:	Class IRLIST - IRList Mail Digest Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <errno.h>
//#include <ctype.h>
#include "irlist.hxx"

IRLIST::IRLIST (PIDBOBJ DbParent, const STRING& Name) :
	MAILFOLDER (DbParent, Name)
{
}

const char *IRLIST::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("IRLIST");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  MAILFOLDER::Description(List);
  return "IRList Mail digests";
}

const char IRLIST_Magic[] = "\n*********";

const CHR  *IRLIST::Seperator() const
{
  return IRLIST_Magic;
}


void IRLIST::SourceMIMEContent(PSTRING StringPtr) const
{
  // MIME/HTTP Content type for records
  *StringPtr = "Application/X-IRList";
}

void IRLIST::ParseRecords (const RECORD& FileRecord)
{
  // Break up the document into Mail message records
  GPTYPE Start = FileRecord.GetRecordStart ();
  GPTYPE End = FileRecord.GetRecordEnd();
  unsigned  count = 0;

  const STRING Fn = FileRecord.GetFullFileName ();
  PFILE Fp = ffopen (Fn, "rb");
  if (!Fp)
    {
      logf (LOG_ERRNO, "Couldn't access '%s'", Fn.c_str());
      return;			// File not accessed
    }
  RECORD Record (FileRecord); // Easy way

  if (-1 == fseek (Fp, Start, SEEK_SET))
   {
     ffclose(Fp);
     logf (LOG_ERRNO, "%s: Bad record boundary", Doctype.c_str()); 
     return; // Bad start
   }
  GPTYPE SavePosition = Start;
  GPTYPE Position = Start;
  GPTYPE RecordEnd;

  char buf[BUFSIZ];
  const char *magic = Seperator();
  if (*magic == '\n')
    magic++;
  const size_t magic_len = strlen(magic);

  // Read lines from file and search for record seperation
  while (fgets(buf, sizeof(buf)/sizeof(char)-1, Fp) != NULL)
    {
      // Search for "magic" line type or mail "from "
      size_t line_len = strlen(buf);
      if ((line_len > magic_len && strncmp(buf, magic, magic_len) == 0)
	|| IsMailFromLine(buf) )
	{
	  if (buf[0] == magic[0]) Position += line_len;
	  SavePosition = Position;
	  Record.SetRecordStart (Start);
	  RecordEnd = SavePosition - 1;

	  if (RecordEnd > Start)
	    {
	      Record.SetRecordEnd (RecordEnd);
	      Db->DocTypeAddRecord(Record);
	      Start = SavePosition;
	      count++;
	    }
	  if (buf[0] != magic[0]) Position += line_len;
	}
      else
       Position += line_len;
      if (End > 0 && Position > End)
	break; // Done
    }

  ffclose (Fp);

  Record.SetRecordStart (Start);
  RecordEnd = Position - 1;

  if (RecordEnd > Start)
    {
      Record.SetRecordEnd (RecordEnd);
      Db->DocTypeAddRecord(Record);
    }
}


IRLIST::~IRLIST ()
{
}
