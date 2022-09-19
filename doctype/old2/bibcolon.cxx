/************************************************************************
************************************************************************/

/*-@@@
File:		bibcolon.cxx
Version:	1.1
Description:	Extended COLONDOC doctype for Bibliographies
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@-*/

//#include <iostream.h>
//#include <fstream.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "bibcolon.hxx"
#include "idb.hxx"

#ifndef NO_MMAP
# include <sys/stat.h>
# include <sys/types.h>
# include <fcntl.h>
# include "mmap.hxx"
#endif


BIBCOLON::BIBCOLON (PIDBOBJ DbParent, const STRING& Name):
	COLONDOC (DbParent, Name)
{
}

const char *BIBCOLON::Description(PSTRLIST List) const
{
  List->AddEntry ("BIBCOLON");
  COLONDOC::Description(List);
  return "COLONDOC for bibliographies\n\
Special fields are:\n\
  Template: Class   // First field (mandatory)\n\
  Handle: UniqueID  // 2nd field (mandatory)\n\
  Name:             // Name associated with record\n\
  Organization:     // Associated organization for name above\n\
  Email:            // Internet email for name above\n";

}



void BIBCOLON::SourceMIMEContent(PSTRING StringBufferPtr) const
{
  *StringBufferPtr = "Application/X-BIBCOLON";
}

void BIBCOLON::BeforeIndexing()
{
  COLONDOC::BeforeIndexing();
}


void BIBCOLON::AfterIndexing()
{
  tmpBuffer.Free();
  tmpBuffer2.Free();
  COLONDOC::AfterIndexing();
}

void BIBCOLON::ParseRecords (const RECORD& FileRecord)
{
  // Break up the document into records
  GPTYPE Start = FileRecord.GetRecordStart();
  GPTYPE End = FileRecord.GetRecordEnd();
  GPTYPE Position = 0;
  GPTYPE SavePosition = 0;
  const STRING fn = FileRecord.GetFullFileName ();;

#ifndef NO_MMAP
  int fd = open(fn, O_RDONLY);
  if (fd == -1)
    {
      logf (LOG_ERRNO, "Could not access %s", fn.c_str());
      return;                   // File not accessed
    }
#else
  PFILE fp = Db->ffopen (fn, "rb");
  if (!fp)
    {
      logf (LOG_ERRNO, "%s::ParseRecords: Could not access '%s'", Doctype.c_str(), fn.c_str());
      return;			// File not accessed
    }
#endif

  RECORD Record (FileRecord); // Easy way

#ifndef NO_MMAP
  struct stat s;
  if (fstat(fd, &s) == -1 || (s.st_mode & S_IFMT) != S_IFREG)
    {
      close (fd);
      return;
    }
  if (End == 0)
    End = s.st_size;

  if (End - Start <= 0)
    {
      logf (LOG_WARN, "zero-length record - '" + fn);
      close (fd);
      return;
    }

  size_t MemSize = End - Start + 1;
  MMAP MemoryMap(fd, Start, End, MapSequential);
  close (fd);

  if (MemoryMap.Ok())
    {
      logf (LOG_WARN, "Could not map '%s' (%ld-%ld)",
	(const char *)fn, (long)Start, (long)End);
      return;
    }
  PCHR RecBuffer = (PCHR)MemoryMap.Ptr();
  GPTYPE ActualLength = MemSize;
#else
  if (End == 0)
    End = GetFileSize (fp);
 
  if (End - Start <= 0)
    {
      logf (LOG_WARN, "zero-length record - '" + fn);
      Db->ffclose (fp);
      return;
    }
  // Move to start if defined
  if (Start > 0)
    if (fseek (fp, Start, 0) == -1)
      {
	logf (LOG_ERRNO, "%s::ParseRecords(): Seek failed - %s", Doctype.c_str(), fn.c_str());
	Db->ffclose (fp);
	return;
      }
  GPTYPE RecLength = RecEnd - RecStart + 1 ;
  PCHR RecBuffer = new CHR [RecLength + 2];
  GPTYPE ActualLength = (GPTYPE) fread (RecBuffer, 1, RecLength, fp);
  if (ActualLength == 0)
    {
      logf (LOG_ERRNO, "%s::ParseRecords(): Failed to fread %s", Doctype.c_str(), fn.c_str());
      delete[]RecBuffer;
      Db->ffclose (fp);
      return;
    }
  Db->ffclose (fp);
  if (ActualLength != RecLength)
    {
      logf (LOG_ERRNO, "%s::ParseRecords(): Failed to fread %d bytes, got %d",
	Doctype.c_str(), RecLength, ActualLength);
      delete[]RecBuffer;
      return;
    }
  RecBuffer[ActualLength] = '\0';	// NULL-terminate the buffer for strfns
#endif

  // Template: ..... then Template: ...
  for (size_t i=0; i<=ActualLength; i++)
    {
    }
  
#ifdef NO_MMAP
  delete[]RecBuffer;
#endif
}



STRING& BIBCOLON::DescriptiveName (const STRING& FieldName, PSTRING Value) const
{
  if (FieldName ^= "Value-only")
    *Value = "";
  else
    COLONDOC::DescriptiveName (FieldName, Value);
  return *Value;
}


void BIBCOLON::ParseFields (RECORD *NewRecord)
{
  STRING fn;
  NewRecord->GetFullFileName (&fn);
  PFILE fp = ffopen (fn, "rb");
  if (!fp)
    {
      NewRecord->SetBadRecord();
      return;			// ERROR
    }

  GPTYPE RecStart = NewRecord->GetRecordStart ();
  GPTYPE RecEnd = NewRecord->GetRecordEnd ();
  if (RecEnd == 0)
    {
      fseek (fp, 0, 2);
      RecStart = 0;
      RecEnd = ftell (fp) - 1;
    }
  fseek (fp, RecStart, 0);
  GPTYPE RecLength = RecEnd - RecStart + 1;
  PCHR RecBuffer = (PCHR) tmpBuffer.Want (RecLength + 1);
  GPTYPE ActualLength = fread (RecBuffer, 1, RecLength, fp);
  ffclose (fp);
  RecBuffer[ActualLength] = '\0';

  PCHR *tags = parse_tags (RecBuffer, ActualLength);
  if (tags == NULL || tags[0] == NULL)
    {
      if (tags)
	logf (LOG_WARN, "No `%s' fields/tags in %s", Doctype.c_str(), fn.c_str());
      else
	logf (LOG_ERROR, "Unable to parse `%s' record in %s", Doctype.c_str(), fn.c_str());
      NewRecord->SetBadRecord();
      return;
    }

  FC fc;
  DF df;
  PDFT pdft;
  pdft = new DFT ();
  DFD dfd;
  STRING FieldName;
  // Walk though tags
  INT cnt = 0;
  for (PCHR * tags_ptr = tags; *tags_ptr; tags_ptr++)
    {
      cnt++;
      PCHR p = tags_ptr[1];	// end of field

      if (p == NULL)		// If no end of field
	p = &RecBuffer[RecLength];	// use end of buffer

      // eg "Author:"
      size_t off = strlen (*tags_ptr) + 1;
      INT val_start = (*tags_ptr + off) - RecBuffer;
      // Skip while space after the ':'
      while (isspace (RecBuffer[val_start]))
	val_start++, off++;
      // Also leave off the \n
      INT val_len = (p - *tags_ptr) - off - 1;
      // Strip potential trailing while space
      while (val_len > 0 && isspace (RecBuffer[val_len + val_start]))
	val_len--;
      if (val_len <= 0)
	continue;		// Don't bother with empty fields

      if (UnifiedName (*tags_ptr, &FieldName).GetLength() == 0)
	continue;		// ignore these

      dfd.SetFieldType( Db->GetFieldType(FieldName) ); // Get the type added 30 Sep 2003
      dfd.SetFieldName (FieldName);
// Addition (TG) - Keeps track of the list of template-types for later storage.
      if (cnt < 3)
	{
	  CHR *pstr = (CHR *)tmpBuffer2.Want(val_len + 2);
	  memcpy (pstr, RecBuffer + val_start, val_len + 1);
	  pstr[val_len + 1] = '\0';
	  if (cnt == 1)
	    {
	      if (FieldName != "Template")
		{
		  logf (LOG_ERROR, "Record in \"%s\" does not begin with a Template type!", fn.c_str());
		  return;
		}
	      else
		((IDB *) Db)->AddTemplate (pstr);
	    }
	  if (cnt == 2)
	    {
	      if (FieldName != "Handle")
		{
		  logf (LOG_ERROR, "Record in \"" + fn + "\" does not have a Handle as its 2nd field");
		  return;
		}
	      if (Db->KeyLookup (pstr))
		logf (LOG_ERROR, "Record in \"" + fn + "\" uses a non-unique Handle '%s'", pstr);
	      else
		NewRecord->SetKey (pstr);
	    }
	}
// End Addition
      Db->DfdtAddEntry (dfd);
      fc.SetFieldStart (val_start);
      fc.SetFieldEnd (val_start + val_len);
      df.SetFct (fc);
      df.SetFieldName (FieldName);
      pdft->AddEntry (df);
// Addition (TG) - All non field-name information stored in field "Value-only"
      FieldName = "Value-only";
      dfd.SetFieldName (FieldName);
      Db->DfdtAddEntry (dfd);
      df.SetFieldName (FieldName);
      pdft->AddEntry (df);
    }
// End addition 
  NewRecord->SetDft (*pdft);
  delete pdft;
}


/*
   Template: USER
   Handle: RECORD1
   Name: Frank Smith

   Headline: USER: RECORD1
 */
void BIBCOLON::
Present (const RESULT& ResultRecord,
	 const STRING& ElementSet, const STRING& RecordSyntax,
	 PSTRING StringBuffer) const
{
  *StringBuffer = "";
  if (ElementSet == BRIEF_MAGIC)
    {
      STRING Headline;
      STRING Template, Handle, Misc;
      COLONDOC::Present (ResultRecord, "Template", SutrsRecordSyntax, &Template);
      COLONDOC::Present (ResultRecord, "Handle", SutrsRecordSyntax, &Handle);
      COLONDOC::Present (ResultRecord, "Name", SutrsRecordSyntax, &Misc);
      if (Misc.GetLength () == 0)
	{
	  COLONDOC::Present (ResultRecord, "Organisation", SutrsRecordSyntax, &Misc);
	  if (Misc.GetLength () == 0)
	    {
	      COLONDOC::Present (ResultRecord, "Organization", SutrsRecordSyntax, &Misc);
	      if (Misc.GetLength () == 0)
		{
		  COLONDOC::Present (ResultRecord, "Email", SutrsRecordSyntax, &Misc);
		}
	    }
	}
      Headline << Template << ": " << Handle;
      if (Misc.GetLength ())
	{
	  Headline << " (" << Misc << ")";
	}
      if (RecordSyntax == HtmlRecordSyntax)
	HtmlCat (Headline, StringBuffer, GDT_FALSE);
      else
	StringBuffer->Cat (Headline);
      return;
    }
  COLONDOC::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}


BIBCOLON::~BIBCOLON ()
{
}
