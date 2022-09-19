#pragma ident  "@(#)resourcedoc.cxx   1.14 05/08/01 21:49:04 BSN"
/************************************************************************
************************************************************************/

/*-@@@
File:		resourcedoc.cxx
Version:	$Revision: 1.1 $
Description:	Class RESOURCEDOC - Synthetic resource (IAFA) documents
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@-*/

//#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "resourcedoc.hxx"
#include "doc_conf.hxx"

RESOURCEDOC::RESOURCEDOC (PIDBOBJ DbParent, const STRING& Name) :
	ROADSDOC (DbParent, Name)
{
}

const char *RESOURCEDOC::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("RESOURCE");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  ROADSDOC::Description(List);
  return "Synthetic resource (IAFA) documents";
}


void RESOURCEDOC::SourceMIMEContent(const RESULT& ResultRecord,
	PSTRING StringPtr) const
{
  DOCTYPE::Present(ResultRecord, "Format-v1", StringPtr);
  // Do we have a format? Is it a MIME type??
  if (StringPtr->GetLength() == 0 || StringPtr->Search('/') == 0)
    SourceMIMEContent(StringPtr); // No then default..
}

void RESOURCEDOC::SourceMIMEContent(PSTRING StringPtr) const
{
  *StringPtr = "Application/Octet-Stream"; 
}

STRING& RESOURCEDOC::DescriptiveName(const STRING& FieldName, PSTRING Value) const
{
  if (FieldName ^= "Local-Path")
    Value->AssignCopy(3, "URL");
  else
    DOCTYPE::DescriptiveName(FieldName, Value);
  return *Value;
}

void RESOURCEDOC::DocPresent (const RESULT& ResultRecord,
       const STRING& ElementSet, const STRING& RecordSyntax,
       PSTRING StringBuffer) const
{
  if ((ElementSet == FULLTEXT_MAGIC) || (ElementSet == SOURCE_MAGIC))
    Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
  else
   COLONDOC::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}

/* Based upon the ROADS IAFA templates */
void RESOURCEDOC::Present (const RESULT& ResultRecord,
	const STRING& ElementSet,
	const STRING& RecordSyntax,
	PSTRING StringBuffer) const
{
  StringBuffer->Clear();
  // FULLTEXT is binary source
  if ((ElementSet == SOURCE_MAGIC) && (RecordSyntax == HtmlRecordSyntax))
    {
      STRING Path;
      // Read file contents..
      DOCTYPE::Present(ResultRecord, "Local-Path", &Path);
      STRINGINDEX pos = Path.Search('\n');
      if (pos) Path.EraseAfter(pos-1);
      StringBuffer->ReadFile (Path);
      if (RecordSyntax == HtmlRecordSyntax)
	{
	  STRING Mime;
	  // Get Content-type
	  SourceMIMEContent(ResultRecord, &Mime);
	  Path.form("Content-Type: %s\n\n", (const char *)Mime);
	  StringBuffer->Insert(1, Path);
	}
    }
  else if ((ElementSet == SOURCE_MAGIC) || (ElementSet == FULLTEXT_MAGIC))
    {
      COLONDOC::DocPresent(ResultRecord, ElementSet, RecordSyntax,
	StringBuffer);
    }
  else if (ElementSet == BRIEF_MAGIC)
    {
      STRING S;
      // Brief Headline is some tag (a journey to find one...)
      DOCTYPE::Present (ResultRecord, "Title", StringBuffer);
      if (StringBuffer->GetLength() == 0)
	{
	  // No Title so use bits of the Description...
	  DOCTYPE::Present (ResultRecord, "Description", &S);
	  if (S.GetLength())
	    {
	      STRINGINDEX x = S.Search('\n');
	      if (x) S.EraseAfter(x-1);
	      S.Pack();
#define CUT_OFF 67
	      if (S.GetLength() >= CUT_OFF)
		{
		  S.EraseAfter(CUT_OFF-3); 
		  S.Cat ("...");
		}
	      else if (x)
		S.Cat ("...");
	      if (RecordSyntax == HtmlRecordSyntax)
		HtmlCat(S, StringBuffer, GDT_FALSE);
	      else
		StringBuffer->Cat(S);
	    }
	  else
	    {
	      // Ugh... just present anything...
	      ROADSDOC::Present (ResultRecord, ElementSet, RecordSyntax,
		StringBuffer);
	      return;
	    }
	}
      DOCTYPE::Present (ResultRecord, "Size-v1", &S);
      if (S.GetLength())
	*StringBuffer << " [" << S << "]";
    }
  else if (ElementSet ^= "Local-Path")
    {
      // Don't Publish Local Paths
#ifdef CGI_FETCH
      if (RecordSyntax == HtmlRecordSyntax)
	{
	  STRING Value, MIME;

	  Db->GetFieldData (ResultRecord, ElementSet, &Value);
	  *StringBuffer << "<A TARGET=\"" <<  ResultRecord.GetKey () << "\" HREF=\"";
	  if (Value.Search("://") < 3)
	    {
	      STRING furl;
	      Db->URLfrag(ResultRecord, &furl);
	      Value = CGI_FETCH;
	      Value.Cat (furl);
	      Value.Cat ("+");
	      Value.Cat (SOURCE_MAGIC);
	    }
	  *StringBuffer << Value << "\">" << Doctype << " Resource</A>";
	  // Is it an image?
	  SourceMIMEContent(ResultRecord, &MIME);
	  if (MIME.Search("image"))
	    {
	      *StringBuffer << "<BR><IMG SRC=\"" << Value << "\">";
	    }
	}
#endif
    }
  else
    {
      DOCTYPE::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer); 
    }
}

RESOURCEDOC::~RESOURCEDOC ()
{
}
