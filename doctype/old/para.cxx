#pragma ident  "@(#)para.cxx	1.7 05/08/01 21:49:11 BSN"
/*-
File:        para.cxx
Version:     1
Description: class PARA - index documents by paragraphs
Author:      Erik Scott, Scott Technologies, Inc.
Modifications: Edward C. Zimmermann, edz@bsn.com
*/

#include <ctype.h>
#include "common.hxx"
#include "para.hxx"
#include "doc_conf.hxx"

PARA::PARA(PIDBOBJ DbParent, const STRING& Name) :
	DOCTYPE(DbParent, Name)
{
}

const char *PARA::Description(PSTRLIST List) const
{
  List->AddEntry ("PARA");
  DOCTYPE::Description(List);
  return "Plaintext where each paragraph is a record";
}

void PARA::AfterIndexing()
{
  recBuffer.Free();
}


void PARA::ParseRecords(const RECORD& FileRecord)
{
   STRING          fn;
   FileRecord.GetFullFileName(&fn);
   PFILE           fp = fopen(fn, "rb");
   if (!fp)
   {
      logf(LOG_ERROR,  "Could not access '%s'", (const char *)fn);
      return;			// File not accessed
   }
   RECORD          Record (FileRecord);

   STRING doctype;
   FileRecord.GetDocumentType(&doctype);

   off_t RecStart = FileRecord.GetRecordStart();
   off_t RecEnd = FileRecord.GetRecordEnd();

   if (RecEnd == 0) RecEnd = GetFileSize (fp);

   if (RecEnd <= RecStart)
   {
      logf(LOG_ERROR, "%s::ParseRecords(): Skipping zero-length record - %s",
	(const char *)doctype, (const char *)fn);
      fclose(fp);
      return;
   }
   if (fseek(fp, RecStart, SEEK_SET) == -1)
   {
      logf(LOG_ERRNO, "%s::ParseRecords(): Seek failed - %s",
	(const char *)doctype, (const char *)fn);
      fclose(fp);
      return;
   }

   off_t RecLength = RecEnd - RecStart + 1 ;
   PCHR RecBuffer = (PCHR)recBuffer.Want (RecLength + 2);
   if (RecBuffer == NULL)
   {
      logf(LOG_ERROR, "%s::ParseRecords(): Failed to allocate %d bytes - %s",
	(const char *)doctype, RecLength + 1, (const char *)fn);
      fclose(fp);
      return;
   }

   off_t ActualLength = fread(RecBuffer, sizeof(char), RecLength, fp);
   if (ActualLength == 0)
   {
      logf(LOG_ERROR, "%s::ParseRecords(): Failed to fread", (const char *)doctype);
      fclose(fp);
      return;
   }
   fclose(fp);

   if (ActualLength != RecLength)
   {
      logf(LOG_ERROR, "%s::ParseRecords(): Failed to fread %d bytes. \
Actually read %d bytes - %s", (const char *)doctype,
	RecLength, ActualLength, (const char *)fn);
      return;
   }
   RecBuffer[ActualLength] = '\0';	// NULL-terminate the buffer for strfns
   // Now we can loop, scan for "\n\n", and use that to mark beginnings and
   // endings.

   off_t Start = 0;
   for (size_t i = 0; i < ActualLength - 1; i++)
   {
      if (
	  /* Vertical Break */
	  (RecBuffer[i] == '\v') ||
	  /* Unix */
	  (RecBuffer[i] == '\n' && RecBuffer[i + 1] == '\n') ||
	  /* MacOS */
	  (RecBuffer[i] == '\r' && RecBuffer[i + 1] == '\r') ||
	  /* MS-DOS */
	  ((RecBuffer[i] == '\r' || RecBuffer[i] == '\n') &&
	   (RecBuffer[i+1] == '\r' || RecBuffer[i+1] == '\n') &&
	   (RecBuffer[i+2] == '\r' || RecBuffer[i+2] == '\n')) )
      {
	 // We found a para marker, didn't we?
	 if ((i - 1) > Start)
	 {
	    // Now we need to burn "\n"s until we get to the start of the
	    // new para.
	    size_t          j = i;
	    while ((j < ActualLength) &&
		(RecBuffer[j]=='\n' || RecBuffer[j]=='\r' || RecBuffer[j]=='\v'))
	      j++; /* loop */
	    Record.SetRecordStart(RecStart + Start);
	    Record.SetRecordEnd(RecStart + j - 1);
	    Db->DocTypeAddRecord(Record);
	    Start = i = j;
	 }
      }
   }

   // Add the last record entry now
   if (Start != ActualLength)
   {
      Record.SetRecordStart(RecStart + Start);
      Record.SetRecordEnd(RecStart + ActualLength - 1);
      Db->DocTypeAddRecord(Record);
   }
}

//
//	Method name : ParseFields
//
//	Description : Parse to get the coordinates of the first sentence.
//	Input : Record Pointer
//	Output : None
//
//  A sentence is recognized as ending in ". ", "? ", "! ", ".\n" or "\n\n".
//
//
void PARA::ParseFields (PRECORD NewRecord)
 {
  STRING fn;
  NewRecord->GetFullFileName (&fn);
  PFILE fp = Db->ffopen (fn, "rb");
  if (!fp)
    {
      return;		// ERROR
    }

  off_t start = NewRecord->GetRecordStart();
  off_t End = NewRecord->GetRecordEnd();

  fseek(fp, start, SEEK_SET);

  int Ch;
  off_t end = 0;
  // Find start/end of first sentence
  // ". ", "? ", "! " or "\n\n" is end-of-sentence
  enum {Scan, Start, Newline, Punct} State = Scan;
  while ((Ch = getc(fp)) != EOF) {
    if (++end >= End && End != 0) break;
    if (State == Scan)
      {
	if (isalnum(Ch))
	  State = Start;
	start = end-1;
      }
    else if (Ch == '\r' || Ch == '\n')
      {
	if (State == Newline || State == Punct)
	  break; // Done
	else
	  State = Newline;
      }
    else if (State == Punct || State == Newline)
      {
	if (isspace(Ch))
	  break; // Done
      }
    else if (Ch == '.' || Ch == '!' || Ch == '?')
	State = Punct;
  }		// while
  if (State == Newline) end--; // Leave off NL
  if (isspace(Ch)) end--; // Leave off trailing white space
  Db->ffclose (fp);

  if (End && start >= End) return; // Nothing

  FC fc;
  DF df;
  PDFT pdft;
  pdft = new DFT ();
  DFD dfd;

  STRING FieldName;
  FieldName = "Headline";
  dfd.SetFieldName (FieldName);
  Db->DfdtAddEntry (dfd);
  fc.SetFieldStart (start);
  fc.SetFieldEnd (end-1);
  df.SetFct (fc);
  df.SetFieldName (FieldName);
  pdft->AddEntry (df);

  NewRecord->SetDft (*pdft);
  delete pdft;
}

void PARA::
Present (const RESULT& ResultRecord, const STRING& ElementSet,
         STRING *StringBufferPtr) const
{
  if (ElementSet == BRIEF_MAGIC)
    DOCTYPE::Present (ResultRecord, "Headline", StringBufferPtr);
  else
    DOCTYPE::Present (ResultRecord, ElementSet, StringBufferPtr);
}

PARA::~PARA()
{
}
