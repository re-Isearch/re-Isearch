/************************************************************************
************************************************************************/

/*-@@@
File:		digesttoc.cxx
Version:	$Revision: 1.1 $
Description:	Class DIGESTTOC - Internet Mail Digest Table of Contents
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@-*/

//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <errno.h>
//#include <ctype.h>
#include "doc_conf.hxx"
#include "digesttoc.hxx"
#include "common.hxx"

DIGESTTOC::DIGESTTOC (PIDBOBJ DbParent, const STRING& Name):
	MAILDIGEST (DbParent, Name)
{
}

const char *DIGESTTOC::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("DIGESTTOC");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  MAILDIGEST::Description(List);
  return "Internet Mail Digest (RFC1173, Listserver, Lyris and Mailman) Table of Contents";
}


void DIGESTTOC::SourceMIMEContent(PSTRING StringPtr) const
{
  // MIME/HTTP Content type for Mail folder records
  *StringPtr = "Message/rfc822";
}

const CHR *DIGESTTOC::Seperator() const
{
  return "\n----------------------------------------------------------------------";
}

void DIGESTTOC::Present(const RESULT& ResultRecord, const STRING& ElementSet,
  const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  *StringBuffer = "";
  if ((ElementSet == BRIEF_MAGIC) && (RecordSyntax == HtmlRecordSyntax))
    {
      // Headline is only the subject line
      STRING Headline;
      MAILDIGEST::Present(ResultRecord, "SUBJECT", SutrsRecordSyntax, &Headline);
      HtmlCat(Headline, StringBuffer, GDT_FALSE);
    }
  else
    {
      MAILDIGEST::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
    }
}

void DIGESTTOC::DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
      const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  if ((RecordSyntax == HtmlRecordSyntax) && (ElementSet == FULLTEXT_MAGIC))
    {
      // Some kludgy code to find where in the TOC section of the page
      // the contents are listed.
      // Assumes a line such as
      // Blah Blah:
      //      Topic one
      // The Topics according to the RFC are generally indented or centered.
      //
      // Instead of the contents list we insert our own contents with hyperlinks
      // to the maildigest article.

      STRING tmpBuf;
      MAILDIGEST::DocPresent(ResultRecord, ElementSet, RecordSyntax, &tmpBuf);
      const size_t siz = tmpBuf.GetLength();
      CHR ch;
      enum {Init, Scan, Look, Have, Had} TOC = Init;
      for (size_t i = 1; i <= siz; i++)
	{
	  ch = tmpBuf.GetChr(i);
	  if (ch == '\n' && TOC != Init && TOC != Had)
	    {
	      if (tmpBuf.GetChr(i+1) == ' ')
		{
		  if (TOC == Look)
		    {
		      TOC = Have;
		      StringBuffer->Cat ("</PRE>");
		      goto contents;
		    }
		}
	      else
		{
		  if (TOC == Scan) TOC = Look;
		  if (TOC == Have)
		    {
		      TOC = Had;
		    }
		}
	    }
	  else if (TOC < Have &&
		i > 4 &&
		ch == '>' &&
		// Note: MAILFOLDER write <PRE> and </PRE>
		tmpBuf.GetChr(i-1) == 'E' &&
		tmpBuf.GetChr(i-2) == 'R' &&
		tmpBuf.GetChr(i-3) == 'P')
	    {
	      if (TOC == Init) TOC = Scan;
	      else if (TOC < Have && tmpBuf.GetChr(i-4) == '/')
		{
		  StringBuffer->Cat (">");
		  TOC = Had;
contents:
		  GDT_BOOLEAN saw_content = GDT_FALSE;
		  RESULT tmpResult;
		  STRING Key, xKey, tmp;
		  ResultRecord.GetKey (&Key);

		  STRING furl;
		  Db->URLfrag(&furl);
		  for (int j=1;;j++)
		    {
		      xKey.form("%s:%d", (const char *)Key, j);
		      if (Db->KeyLookup (xKey))
			{
			  tmpResult.SetKey (xKey);
			  Present(tmpResult, "SUBJECT", RecordSyntax, &tmp);
#define CGI_FETCH "ifetch"
			  if (tmp != "")
			    {
			      if (saw_content == GDT_FALSE)
				{
				  if (TOC == Had)
				    StringBuffer->Cat ("<H2>Contents:</H2>\n");
				  StringBuffer->Cat ("<!-- Synthetic TOC Links --><OL>\n");
				  saw_content = GDT_TRUE;
				}
			      *StringBuffer << "<LI>" << 
				"<A HREF=\"" << CGI_FETCH << furl << xKey <<
				"\"><CITE>" << tmp << "</CITE></A>";

			      Present(tmpResult, "AUTHOR", RecordSyntax, &tmp);
			      if (tmp != "")
				*StringBuffer << " (" << tmp << ")";
			      *StringBuffer << "\n";
			    }
			}
		      else
			break; // Done
		    }
		  if (saw_content)
		    {
		      StringBuffer->Cat ("</OL>\n");
		    }
		  if (TOC == Have)
		    StringBuffer->Cat ("<PRE>");
		  continue;
		}
	    }

	  if (TOC != Have) StringBuffer->Cat (ch);
	}
    }
  else
    { 
      MAILDIGEST::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
    }
}


DIGESTTOC::~DIGESTTOC ()
{
}
