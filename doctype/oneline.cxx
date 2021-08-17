#pragma ident  "@(#)oneline.cxx	1.4 02/24/01 17:45:33 BSN"
/*

File:        oneline.cxx
Version:     1
Description: class ONELINE - index documents one line long (like phonebooks)
Author:      Erik Scott, Scott Technologies, Inc.
*/

#include <ctype.h>
#include "oneline.hxx"

ONELINE::ONELINE(PIDBOBJ DbParent, const STRING& Name) :
	DOCTYPE(DbParent, Name)
{
}

const char *ONELINE::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("ONELINE");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  DOCTYPE::Description(List);
  return "Each line is a record (unfielded)";
}


void ONELINE::ParseRecords(const RECORD& FileRecord)
{
  STRING Fn (FileRecord.GetFullFileName () );
  PFILE Fp = DOCTYPE::ffopen (Fn, "rb");
  if (!Fp)
    {
      message_log (LOG_ERRNO, "Could not access '%s'", Fn.c_str());
      return;			// File not accessed
    }

  RECORD Record (FileRecord);

  off_t GlobalStart = FileRecord.GetRecordStart();
  off_t GlobalEnd   = FileRecord.GetRecordEnd();

  if (GlobalStart != 0) 
    if ((fseek(Fp, GlobalStart, SEEK_SET)) == -1)
      {
	message_log (LOG_ERRNO, "Can't see to %ld on '%s'. Skipping", GlobalStart, Fn.c_str());
	fclose(Fp);
	return;
      }
  off_t Start    = GlobalStart; 
  off_t Position = Start;
  int   ci = 0;

  while (ci != EOF && (Position <= GlobalEnd || GlobalEnd == 0))
    {
      for (; (ci != '\n') && (ci != EOF); ci=fgetc(Fp), Position++)
	/* loop */;
      if (Position > Start + 2 )
	{
	  Record.SetRecordStart(Start);
	  Record.SetRecordEnd(  Position - ( 1 + (ci == EOF)) );
	  Db->DocTypeAddRecord(Record);
	  Start = Position;
	}
      if (ci=='\n') ci=0;// save an EOF, but hide a newline so it will loop again
    }
  DOCTYPE::ffclose(Fp);
}

ONELINE::~ONELINE() {
}
