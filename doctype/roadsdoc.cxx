#pragma ident  "@(#)roadsdoc.cxx	1.6 05/08/01 21:49:05 BSN"
/************************************************************************
************************************************************************/

/*-@@@
File:		roadsdoc.cxx
Version:	$Revision: 1.1 $
Description:	Class ROADSDOC - IAFA documents
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@-*/

//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <errno.h>
//#include <ctype.h>
#include "roadsdoc.hxx"
#include "doc_conf.hxx"


ROADSDOC::ROADSDOC (PIDBOBJ DbParent, const STRING& Name) :
	IKNOWDOC (DbParent, Name)
{
}

void ROADSDOC::SourceMIMEContent(const RESULT& ResultRecord,
	PSTRING StringPtr) const
{
  SourceMIMEContent(StringPtr);
}

const char *ROADSDOC::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("ROADS++");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  DOCTYPE::Description(List);
  return "ROADS++ extended IAFA documents.";
}



void ROADSDOC::SourceMIMEContent(PSTRING StringBufferPtr) const
{
  *StringBufferPtr = "Application/X-IAFA";
}

/* Based upon the ROADS IAFA templates */
void ROADSDOC::
Present (const RESULT& ResultRecord, const STRING& ElementSet, const STRING& RecordSyntax,
	PSTRING StringBuffer) const
{
  *StringBuffer = "";
  if (ElementSet == SOURCE_MAGIC)
    {
      DOCTYPE::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
    }
  else if (ElementSet == BRIEF_MAGIC)
    {
      // Brief Headline is some tag (a journey to find one...)
      STRING templat;
      DOCTYPE::Present(ResultRecord, "TEMPLATE-TYPE", &templat);
      STRING Author;
      STRING S;
      if (templat ^= "EVENT")
	{
	  STRING End;
	  DOCTYPE::Present (ResultRecord, "Start-Date", &Author);
	  DOCTYPE::Present (ResultRecord, "End-Date", &End);
	  if (End.GetLength())
	    {
	      Author.Cat("-");
	      Author.Cat(End);
	    }
	}
      else if (templat ^= "USER")
	{
	  DOCTYPE::Present (ResultRecord, "Department", &Author);
	}
      else if (templat ^= "ORGANIZATION")
	{
	  DOCTYPE::Present (ResultRecord, "Type", &Author);
	}
      else if (templat ^= "SERVICE")
	{
	  DOCTYPE::Present (ResultRecord, "Category", &Author);
	}
      else
	{
	  int i = 1;
	  do {
	    STRING field;
	    field.form("Author-Name-v%d", i);
	    DOCTYPE::Present (ResultRecord, field, &S);
	    if (S.GetLength())
	      {
		if (i++ > 1) Author.Cat(", ");
		Author.Cat(S);
	      }
	    else
	      i = 0;
	  } while (i);
	}

      if ((templat ^= "USER") || (templat ^= "ORGANIZATION"))
	{
	  DOCTYPE::Present (ResultRecord, "Name", StringBuffer);
	}
      else
	{
	  DOCTYPE::Present (ResultRecord, "Title", StringBuffer);
	}
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
	      DOCTYPE::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
	    }
	}

      // Do we have an author?
      if (Author.GetLength())
	{
	  Author.Pack ();
	  StringBuffer->Cat (" (");
	  StringBuffer->Cat (Author);
	  StringBuffer->Cat (") ");
	}
      // Do we have a template?
      if (templat.GetLength())
	{
	  StringBuffer->Cat(" [");
	  StringBuffer->Cat(templat);
	  StringBuffer->Cat("]");
	}
    }
#ifdef CGI_FETCH
  else if ((RecordSyntax == HtmlRecordSyntax) &&
	ElementSet.SearchAny("-HANDLE-V"))
    {
      STRING Value;
      DOCTYPE::Present (ResultRecord, ElementSet, &Value);
      if (Value.GetLength() && Db->KeyLookup (Value))
	{
	  STRING title;

	  STRING ES = ElementSet;
	  ES.ToUpper();
	  ES.Replace("-HANDLE-", "-NAME-");
	  DOCTYPE::Present (ResultRecord, ES, &title);

	  STRING furl;
	  Db->URLfrag(&furl);
	  *StringBuffer << "<A HREF=\"" << CGI_FETCH << furl << Value;
	  if (title.GetLength())
	    {
	      StringBuffer->Cat ("\" TITLE=\"");
	      HtmlCat(title, StringBuffer, GDT_FALSE);
	    }
	  StringBuffer->Cat ("\">");
	  HtmlCat(Value, StringBuffer, GDT_FALSE);
	  StringBuffer->Cat("</A>");
	}
    }
#endif
  else if ((RecordSyntax == HtmlRecordSyntax) &&
	((ElementSet ^= "MAINTAINED-BY") || (ElementSet ^= "AUTHOR")))
    {
      // Kluge alert!
      STRING Value;
      DOCTYPE::Present (ResultRecord, ElementSet, &Value);
      HtmlCat(Value, StringBuffer);
      StringBuffer->Replace("&lt; ", "&lt;");
      StringBuffer->Replace(" &gt;", "&gt;");
    }
  else
    IKNOWDOC::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}

ROADSDOC::~ROADSDOC ()
{
}
