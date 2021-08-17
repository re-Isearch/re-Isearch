#pragma ident  "@(#)iknowdoc.cxx	1.13 05/08/01 21:49:06 BSN"
/************************************************************************
************************************************************************/

/*-@@@
File:		iknowdoc.cxx
Version:	1.1
Description:	Extended COLONDOC doctype, allowing value-only searches
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Modification:	Tim Gemma, stone@cnidr.org, edz@nonmonotonic.net
@@@-*/

//#include <iostream.h>
//#include <fstream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <errno.h>
//#include <ctype.h>
#include "iknowdoc.hxx"
#include "idb.hxx"

IKNOWDOC::IKNOWDOC (PIDBOBJ DbParent, const STRING& Name) :
	COLONDOC (DbParent, Name)
{
}


const char *IKNOWDOC::Description(PSTRLIST List) const
{
  List->AddEntry ("IKNOWDOC");
  COLONDOC::Description(List);
  return "Template extended COLONDOC format for whois++ reference implementations.";
}


void IKNOWDOC::SourceMIMEContent(PSTRING StringBufferPtr) const
{
  *StringBufferPtr = "Application/X-IKNOW";
}


void IKNOWDOC::ParseRecords (const RECORD& FileRecord)
{
  DOCTYPE::ParseRecords (FileRecord);
}

STRING& IKNOWDOC::DescriptiveName (const STRING& FieldName,
	PSTRING Value) const
{
  if (FieldName ^= "Value-only")
    *Value = "";
  else
    DOCTYPE::DescriptiveName (FieldName, Value);
  return *Value;
}


void IKNOWDOC::ParseFields (PRECORD NewRecord)
{
  const STRING fn ( NewRecord->GetFullFileName () );
  PFILE fp = ffopen (fn, "rb");
  if (!fp)
    {
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
  GPTYPE RecLength = RecEnd - RecStart;
  PCHR RecBuffer = (PCHR)tmpBuffer.Want (RecLength + 1);
  GPTYPE ActualLength = fread (RecBuffer, 1, RecLength, fp);
  ffclose (fp);
  RecBuffer[ActualLength] = '\0';

  PCHR *tags = parse_tags (RecBuffer, ActualLength);
  if (tags == NULL || tags[0] == NULL)
    {
      if (tags)
	{
	  message_log (LOG_WARN, "No `" + Doctype + "' fields/tags in " + fn);
	}
      else
	{
	  message_log (LOG_ERROR, "Unable to parse `" + Doctype + "' record in " + fn);
	}
      return;
    }

  FC fc;
  DF df;
  PDFT pdft;
  pdft = new DFT ();
  DFD dfd;
  STRING FieldName;
  // Walk though tags
  size_t cnt = 0;
  GDT_BOOLEAN sawHandle = GDT_FALSE;

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

      if (NULL == (const char *)UnifiedName (*tags_ptr, &FieldName))
	continue; // Don't bother
      // Keep track of the list of template-types for later storage.
      if (cnt < 3)
	{
	  if (cnt == 1)
	    {
	      // Template (IKNOW) or Template-Type (IAFA/ROADS)
	      if ((FieldName != "Template") && (FieldName != "Template-Type"))
		{
		  message_log (LOG_ERROR, "\
%s Record in \"%s\" (%ld-%ld) does not begin with a Template type!",
			(const char *)Doctype, (const char *)fn, RecStart, RecEnd);
		  return;
		}
	      else
		{
		  CHR *pstr = (CHR *)tagBuffer.Want(val_len + 2);
		  memcpy (pstr, RecBuffer + val_start, val_len + 1);
		  pstr[val_len + 1] = '\0';
		  ((IDB *) Db)->AddTemplate (pstr);
		}
	    }
	  else if (cnt == 2 && (Doctype == "IKNOWDOC") && (FieldName != "Handle"))
	    {
	      message_log (LOG_ERROR, "\
Record in \"%s\" does not have 'Handle' as its 2nd field", (const char *)fn);
	      return;
	    }
	}
      if (!sawHandle && (FieldName ^= "HANDLE"))
	{
	  CHR *pstr = (CHR *)tagBuffer.Want(val_len + 2);
	  memcpy (pstr, RecBuffer + val_start, val_len + 1);
	  pstr[val_len + 1] = '\0';
	  if (Db->KeyLookup (pstr))
	    {
	      message_log (LOG_ERROR, "Record in \"%s\" uses a non-unique %s '%s'",
		(const char *)fn, (const char *)FieldName, pstr);
	    }
	  else
	    {
	      NewRecord->SetKey (pstr);
	      sawHandle = GDT_TRUE; // We have one
	    }
	}

      // If fieldname defined then store..
      if (FieldName.GetLength())
	{
	  dfd.SetFieldName (FieldName);
	  Db->DfdtAddEntry (dfd);
	  fc.SetFieldStart (val_start);
	  fc.SetFieldEnd (val_start + val_len);
	  df.SetFct (fc);
	  df.SetFieldName (FieldName);
	  pdft->AddEntry (df);
 	}
      // All non field-name information stored in field "Value-only"
      FieldName = "Value-only";
      dfd.SetFieldName (FieldName);
      Db->DfdtAddEntry (dfd);
      fc.SetFieldStart (val_start);
      fc.SetFieldEnd (val_start + val_len);
      df.SetFct (fc);
      df.SetFieldName (FieldName);
      pdft->AddEntry (df);
    }

  if (!sawHandle)
    message_log(LOG_INFO, "\
Record in \"%s\" did not specify a handle.", (const char *)fn);

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
void IKNOWDOC::
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


IKNOWDOC::~IKNOWDOC ()
{
}
