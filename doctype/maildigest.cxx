#pragma ident  "@(#)maildigest.cxx	1.12 02/24/01 17:44:59 BSN"
/************************************************************************
************************************************************************/

/*-@@@
File:		maildigest.cxx
Version:	$Revision: 1.1 $
Description:	Class MAILDIGEST - Internet Mail Digest Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@-*/

//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <errno.h>
//#include <ctype.h>
#include "maildigest.hxx"
#include "common.hxx"

#include <sys/stat.h>

MAILDIGEST::MAILDIGEST (PIDBOBJ DbParent, const STRING& Name) :
	MAILFOLDER (DbParent, Name)
{
//OldKey->Clear();
  Count = 0;
  OldInode = -1;
  OldDevice = -1;
}

const char *MAILDIGEST::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("MAILDIGEST");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);
  MAILFOLDER::Description(List);
  return "Internet Mail Digests";
}

void MAILDIGEST::SourceMIMEContent(PSTRING StringPtr) const
{
  // MIME/HTTP Content type for records
  *StringPtr = "Application/X-Maildigest";
}

const CHR *MAILDIGEST::Seperator() const
{
  // 30 hyphens
  return "\n------------------------------";
}

// TODO: Check to make sure it's a maildigest..
// else if it looks like a listdigest pass it.. or
// change the Doctype to Mailfolder for a mail message..
void MAILDIGEST::ParseRecords (const RECORD& FileRecord)
{
  // Break up the document into Mail message records
  const GPTYPE Start = FileRecord.GetRecordStart ();
  const GPTYPE End = FileRecord.GetRecordEnd();

  GPTYPE Position = 0;
  GPTYPE SavePosition = 0;
  GPTYPE RecordStart = Start;
  GPTYPE RecordEnd = End;

  const STRING Fn ( FileRecord.GetFullFileName () );
  PFILE Fp = Db->ffopen (Fn, "rb");
  if (!Fp)
    {
      message_log (LOG_ERRNO, "%s: Couldn't access '%s'", Doctype.c_str(), Fn.c_str());
      return;			// File not accessed
    }

  if (RecordStart)
    {
      if (-1 == fseek (Fp, RecordStart, SEEK_SET))
	{
	  Db->ffclose(Fp);
	  message_log (LOG_ERRNO, "%s: Bad record boundary", Doctype.c_str());
	  return; // Bad start
	}
      SavePosition = Position = RecordStart;
    }

  RECORD Record (FileRecord); // Easy way

  char buf[512];
  const char *magic = Seperator();
  if (*magic == '\n') magic++;
  const size_t magic_len = strlen(magic);

  size_t Elements = 0; // Number of messages in digest

  // Read lines from file and search for record seperation
  enum {SCAN, LOOK, HAVE, SCANEND, LOOKEND, HAVEEND} State = SCAN;
  while (fgets(buf, sizeof(buf)/sizeof(char)-1, Fp) != NULL)
    {
      // Search for "magic" line type
      size_t line_len = strlen(buf);
      Position += line_len;

      if (End > 0 && Position > End)
	break; // Done

      if (buf[0] == '\n' || buf[0] == '\r')
	{
	  if (State == HAVE || State == HAVEEND)
	    {
	      SavePosition = Position;
	      Record.SetRecordStart (RecordStart);
	      RecordEnd = SavePosition - 1;
	      if (State == HAVEEND)
		{
		  RecordEnd -= 19; // "END oF DIGEST" + "---" ....
		}
	      if (RecordEnd > RecordStart)
		{
		  Record.SetRecordEnd (RecordEnd /* - line_len */);
		  Db->DocTypeAddRecord(Record);
		  RecordStart = SavePosition;
		  Elements++;
		}
	      State = SCAN;
	    }
	  else if (State == SCANEND)
	    State = LOOKEND;
	  else
	    State = LOOK;
	}
      else if (State == LOOKEND && strncmp(buf, "END OF DIGEST", 13) == 0 &&
	(buf[13] == '\n' || buf[13] == '\r'))
	{
	  State = HAVEEND;
	}
      else if (State == LOOK && line_len >= magic_len && !strncmp(buf, magic, magic_len))
	{
	  State = HAVE;
	}
      else if (State == LOOK && (line_len == 4 || line_len == 5) &&
	(buf[3] == '\n' || buf[3] == '\r') && strncmp(buf, "---", 3) == 0)
	{
	  State = SCANEND;
	}
      else
	{
	  State = SCAN;
	}
    }

  // Do we need the last bits?
  fseek(Fp, RecordStart, SEEK_SET);
  bool SkipEnd = true; // Usually NOT!

  GPTYPE Off = Position; 
  // NOTE: In new engine we must only look to tail of record (END)
  while (SkipEnd && fgets(buf, sizeof(buf)/sizeof(char)-1, Fp) != NULL)
    {
      if (strncmp("Date: ", buf, 6) == 0 ||
	  strncmp("From: ", buf, 6) == 0 ||
	  strncmp("Subject: ", buf, 9) == 0)
	{
	  SkipEnd = false; // Oops, looks like real stuff
	}
	Off += strlen(buf);
    }

  if (SkipEnd == false)
    {
//cerr << "Position = " << Position << endl;
//cerr << "\tRecord End = " << RecordEnd << endl;
//cerr << "\tRecord Start = " << RecordStart << endl;

      RecordStart = RecordEnd + 1; // Added this line 7 Jan 2000 
      Record.SetRecordStart (RecordStart);

      if ((RecordEnd = Position - 1) < End)
	RecordEnd = End;

      if (RecordEnd > RecordStart)
	{
#if 1
	  if (Off >= RecordEnd)
	    {
	      struct stat sb;
	      if (_IB_fstat (fileno(Fp), &sb) >= 0)
		{
		  STRING key;

		  key.form("#%X%x%x", sb.st_ino, RecordStart, RecordEnd);
		  for (unsigned int i = 0;; i++)
		    {
		      if (!Db->KeyLookup (key, NULL))
			break;
		      key.form("#%X%x%X%x%X", sb.st_dev, sb.st_ino, RecordStart, RecordEnd, i);
		    }
		  Record.SetKey(key);
		}
	       Record.SetDocumentType ("<NIL>");
	    }
#endif

	  Record.SetRecordEnd (RecordEnd);
	  Db->DocTypeAddRecord(Record);
	  Elements++;
	}
    }

  Db->ffclose (Fp);

  if (Elements == 0)
    {
      // Not a digest so..
      static const DOCTYPE_ID newDoctype ("MAILFOLDER");
      Record.SetRecordStart (Start);
      Record.SetRecordEnd (End);

      if (End > 0)
	message_log (LOG_INFO, "Setting \"%s\" record %ld-%ld to %s.", Fn.c_str(), Start, End, newDoctype.c_str());
      else
	message_log (LOG_INFO, "Setting \"%s\" to %s", Fn.c_str(), newDoctype.c_str());
      Record.SetDocumentType (newDoctype);
      Db->DocTypeAddRecord(Record);
    }
}

void MAILDIGEST::ParseFields(PRECORD NewRecord)
{
  MAILFOLDER::ParseFields(NewRecord);

  STRING Key ( NewRecord->GetKey() );
  const STRING Fn ( NewRecord->GetFullFileName() );

  long inode = 0;
  long device = 0;
  struct stat sb;
  if ((_IB_stat (Fn, &sb) >= 0) && ((sb.st_mode & S_IFMT) == S_IFREG))
    {
      inode = sb.st_ino;
      device = sb.st_dev;
    }

  if (Key.IsEmpty())
    {
      if (OldInode == inode && !OldKey.IsEmpty())
	{
	  Key.form("%s:%u", OldKey.c_str(), ++Count);
	}
       else
	{
	  for (unsigned int i = 0;; i++)
	    {
	      Key.form (i ? "DI%lX%lX.%u" : "DI%lX%lX", inode, device, i);
	      if (!Db->KeyLookup (Key, NULL))
		break;
	    }
	  OldKey = Key;
	  Count = 0;
	}
    }
  else
    {
      OldKey = Key;
      Count = 0;
    }
  if (Count == 0)
    {
      static const DOCTYPE_ID Digesttoc ("DIGESTTOC");
      NewRecord->SetDocumentType (Digesttoc);
    }
  OldInode = inode;
  NewRecord->SetKey (Key);
}


MAILDIGEST::~MAILDIGEST ()
{
}
