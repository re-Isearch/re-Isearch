#pragma ident  "@(#)iafadoc.cxx	1.8 05/09/01 13:48:04 BSN"
/************************************************************************
************************************************************************/

/*-@@@
File:		iafadoc.cxx
Version:	$Revision: 1.1 $
Description:	Class IAFADOC - IAFA documents
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@-*/

//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <errno.h>
//#include <ctype.h>
#include "iafadoc.hxx"
#include "doc_conf.hxx"


IAFADOC::IAFADOC (PIDBOBJ DbParent, const STRING& Name) :
	COLONDOC (DbParent, Name)
{
}

const char *IAFADOC::Description(PSTRLIST List) const
{
  List->AddEntry ("IAFADOC");
  COLONDOC::Description(List);
  return "IAFA Documents (Colon defined)";
}


void IAFADOC::SourceMIMEContent(const RESULT& ResultRecord,
	PSTRING StringPtr) const
{
  SourceMIMEContent(StringPtr);
}

void IAFADOC::SourceMIMEContent(PSTRING StringBufferPtr) const
{
  *StringBufferPtr = "Application/X-IAFA";
}

#if 0
void IAFADOC::ParseFields (PRECORD NewRecord)
{
  STRING fn;
  NewRecord->GetFullFileName (&fn);
  PFILE fp = fopen (fn, "rb");
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
  PCHR RecBuffer = new CHR[RecLength + 1];
  GPTYPE ActualLength = fread (RecBuffer, 1, RecLength, fp);
  fclose (fp);
  RecBuffer[ActualLength] = '\0';

  PCHR *tags = parse_tags (RecBuffer, ActualLength);
  if (tags == NULL || tags[0] == NULL)
    {
      STRING doctype;
      NewRecord->GetDocumentType (&doctype);
      if (tags)
	{
	  message_log (LOG_WARN, "No `" + doctype + "' fields/tags in " + fn);
	}
      else
	{
	  message_log (LOG_ERROR, "Unable to parse `" + doctype + "' record in " + fn);
	}
      delete[]RecBuffer;
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
  GDT_BOOLEAN sawHandle = GDT_FALSE; // Do we have a handle?
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

      PCHR unified_name = UnifiedName (*tags_ptr);
      // Ignore "unclassified" fields
      if (unified_name == NULL)
	continue;		// ignore these

      FieldName = unified_name;
      FieldName.ToUpper(); // Upper-Case

      dfd.SetFieldType( Db->GetFieldType(FieldName) ); // Get the type added 30 Sep 2003
      dfd.SetFieldName (FieldName);
      // Keep track of the list of template-types for later storage.
      if (FieldName == "TEMPLATE-TYPE")
	{
	  CHR *pstr = (CHR *)alloca(val_len + 2);
	  memcpy (pstr, RecBuffer + val_start, val_len + 1);
	  pstr[val_len + 1] = '\0';
	  ((IDB *) Db)->AddTemplate (pstr);
	}
      // Depending on the type have different handles
      // Handle                  Unique identifier for this
      // Organization-Handle     Handle of organization.
      // Admin-Handle            Administrator

      else if (FieldName.Search("HANDLE") &&
	(!sawHandle || (FieldName == "HANDLE")) )
	{
	  CHR *pstr = (CHR *)alloca(val_len + 2);
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
      Db->DfdtAddEntry (dfd);
      fc.SetFieldStart (val_start);
      fc.SetFieldEnd (val_start + val_len);
      df.SetFct (fc);
      df.SetFieldName (FieldName);
      pdft->AddEntry (df);
    }
  NewRecord->SetDft (*pdft);
  delete pdft;
  delete[]RecBuffer;
}
#endif


/* Based upon the IAFA guides to FTP publication from the IAFA-WG
  IAFA DOC I:	Draft 92.10.14
  IAFA DOC II:	Draft 92.10.19
*/
void IAFADOC::
Present (const RESULT& ResultRecord, const STRING& ElementSet, const STRING& RecordSyntax,
	PSTRING StringBuffer) const
{
  if (ElementSet == SOURCE_MAGIC)
    {
      DOCTYPE::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
    }
  else if (ElementSet == BRIEF_MAGIC)
    {

      *StringBuffer = "";

      // Build Headline (a journey to find one...)
      const struct {
	const CHR *tag;
	const CHR *caption;
      } TagList[] = {
	// First look for Title
	{"Title",            NULL},
	{"Package-Name",     "Package: "},
	{"Service-Name",     "Service: "},
	{"Preferred-Name",   "Site-Info: "},
	{"Mailinglist-Name", "Mailing-list: "},
	{"Newsgroup-Name",   "Newsgroup: "}
	/* NO NULL Please! */
      };

      // Brief Headline is some tag (a journey to find one...)
      STRING name;
      for (size_t i=0; i < sizeof(TagList)/sizeof(TagList[0]); i++)
	{
	  DOCTYPE::Present (ResultRecord, TagList[i].tag, RecordSyntax, &name);
	  if (name.GetLength())
	    {
	      if (TagList[i].caption != NULL)
	        StringBuffer->Cat(TagList[i].caption);
	      break; // Have something
	    }
	}

      if (name.GetLength ())
	{
	  STRING Author;
	  DOCTYPE::Present (ResultRecord, "Author", RecordSyntax, &Author);
	  if (Author.GetLength() == 0)
	    {
	      DOCTYPE::Present (ResultRecord, "Authors", RecordSyntax, &Author);
	    }
	  Author.Pack ();
	  StringBuffer->Cat (name);
	  if (Author.GetLength ())
	    {
	      StringBuffer->Cat (" (");
	      StringBuffer->Cat (Author);
	      StringBuffer->Cat (") ");
	    }
	}
      else
	{
	  // No Name so use bits of the Description...
	  DOCTYPE::Present (ResultRecord, "Description", RecordSyntax, &name);
	  if (name.GetLength())
	    {
	      STRINGINDEX x = name.Search('\n');
	      if (x) name.EraseAfter(x-1);
	      name.Pack();
#define CUT_OFF 67
	      if (name.GetLength() >= CUT_OFF)
		{
		  name.EraseAfter(CUT_OFF-3); 
		  name.Cat ("...");
		}
	      else if (x)
		name.Cat ("...");
	      StringBuffer->Cat("Description: ");
	      StringBuffer->Cat(name);
	    }
	  else
	    {
	      // Ugh... just present anything...
	      DOCTYPE::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
	    }
	}
      // Do we have a template (Added Feb'97)
      DOCTYPE::Present (ResultRecord, "Template-Type", RecordSyntax, &name);
      if (name.GetLength())
	{
	  StringBuffer->Cat(" [");
	  StringBuffer->Cat(name);
	  StringBuffer->Cat("]");
	}
    }
  else if ((RecordSyntax == HtmlRecordSyntax) &&
	((ElementSet ^= "MAINTAINED-BY") || (ElementSet ^= "AUTHOR")))
    {
      StringBuffer->Clear();

      // Kluge alert!
      STRING Value;
      DOCTYPE::Present (ResultRecord, ElementSet, &Value);
      HtmlCat(Value, StringBuffer);
      StringBuffer->Replace("&lt; ", "&lt;");
      StringBuffer->Replace(" &gt;", "&gt;");
    }
  else
    COLONDOC::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}

IAFADOC::~IAFADOC ()
{
}
