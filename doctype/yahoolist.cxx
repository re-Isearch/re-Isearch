/************************************************************************
************************************************************************/

/*-@@@
File:		yahoolist.cxx
Version:	$Revision: 1.1 $
Description:	Class YAHOOLIST - Yahoo's Mail Digest Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@-*/

//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <errno.h>
//#include <ctype.h>
#include "yahoolist.hxx"

YAHOOLIST::YAHOOLIST (PIDBOBJ DbParent, const STRING& Name) :
	LISTDIGEST (DbParent, Name)
{
}

const char *YAHOOLIST::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("YAHOOLIST");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  LISTDIGEST::Description(List);
  return "Yahoo's RFC-noncompliant Listserver Mail Digests";
}


const CHR *YAHOOLIST::Seperator() const
{
  return "\n\
________________________________________________________________________";
}


void YAHOOLIST::ParseRecords (const RECORD& FileRecord)
{
  // Break up the document into Mail message records
  off_t Start = FileRecord.GetRecordStart ();
  off_t End = FileRecord.GetRecordEnd();

  off_t Position = 0;
  off_t SavePosition = 0;
  off_t RecordEnd;

  const STRING Fn = FileRecord.GetFullFileName ();
  PFILE Fp = ffopen (Fn, "rb");
  if (!Fp)
    {
      logf (LOG_ERRNO, "Couldn't access '%s'", Fn.c_str());
      return;			// File not accessed
    }

  if (Start)
    {
      if (-1 == fseek (Fp, Start, SEEK_SET))
	{
	  ffclose(Fp);
	  logf (LOG_ERRNO, "%s: Bad record boundary", Doctype.c_str()); 
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

  // Read lines from file and search for record seperation
  int seen = 0; // Sep is twice so we count..
  int processed = 0; 
  int sub_messages = 0;
  while (fgets(buf, sizeof(buf)/sizeof(char)-1, Fp) != NULL)
    {
      // Search for "magic" line type
      size_t line_len = strlen(buf);
      Position += line_len;
      if (End > 0 && Position > End)
	break; // Done
      if (line_len > magic_len && strncmp(buf, magic, magic_len) == 0 )
	{
	  if (++seen > 1)
	    {
	      SavePosition = Position;
	      Record.SetRecordStart (Start);
	      RecordEnd = SavePosition - 1;
	      if (RecordEnd > Start)
		{
		  Record.SetRecordEnd (RecordEnd);
		  Db->DocTypeAddRecord(Record);
		  Start = SavePosition;
		  sub_messages++;
		}
	      processed++;
	    }
	   else
	    continue; // Keep looking
	}
      seen = 0;
    }

  ffclose (Fp);

  if (processed == 0)
    {
      logf (LOG_INFO, "%s fallthrough, passing to MAILDIGEST::", Doctype.c_str());
      MAILDIGEST::ParseRecords (FileRecord);
      return;
    }

/**** Last bit is junk at YAHOO */
  Record.SetRecordStart (Start);
  RecordEnd = Position - 1;

  if (RecordEnd > Start)
    {
      Record.SetRecordEnd (RecordEnd);
      Record.SetDocumentType("PLAINTEXT"); // No fields
      Record.SetBadRecord(); // Ignore this stuff!
      Db->DocTypeAddRecord(Record);
    }
  logf (LOG_DEBUG, "%s: %d sub-messages", Doctype.c_str(), sub_messages);
}



PCHR *YAHOOLIST::parse_tags (PCHR b, off_t len)
{
  size_t tc = 0;		// tag count
#define TAG_GROW_SIZE 32
  size_t max_num_tags = TAG_GROW_SIZE;	// max num tags for which space is allocated
  enum { HUNTING, STARTED, CONTINUING } State = HUNTING;

  // Skip leading bogus white space
  while (isspace(*b)) b++;
  // Is it a mail or news folder?
  if (IsMailFromLine(b) || IsNewsLine(b)) { 
    // Now skip the From/Article line
    while (*b != '\n') b++; // looking at end of line
    while (*b == '\n') b++; // Looking at first character
    // Should now be looking at fist tag line
  }

  /* You should allocate these as you need them, but for now... */
  max_num_tags = TAG_GROW_SIZE;
  PCHR *t = (PCHR *)TagBuffer.Want(max_num_tags, sizeof(PCHR));

  for (off_t i = 0; i < len; i++)
    {
      if (State == HUNTING)
	{
	  // Skip leading spaces
	  while (i < len && (b[i] == ' ' || b[i] == '\t')) i++;

	  t[tc] = &b[i];
	  State = STARTED;
	  if (b[i] == '\n' || b[i] == '\r')
	   {
	      b[i] = '\0';
	      tc++;
	      break; // looking at body, done
	   }

	}
      // Handle continuation
      else if ((State == STARTED) && (b[i] == ' ' || b[i] == '\t'))
	{
	  State = CONTINUING;
	}
      else if ((State == STARTED) && (b[i] == ':'))
	{
	  b[i] = '\0';
	  // Expand memory if needed
	  t = (PCHR *)TagBuffer.Expand(++tc,  sizeof(PCHR));
	  State = CONTINUING;
	}
      else if ((State == CONTINUING) && (b[i] == '\n' || b[i] == '\r'))
	{
	  State = HUNTING;
	}
    }
  t[tc] = (PCHR) NULL;
  return t;
}


YAHOOLIST::~YAHOOLIST ()
{
}
