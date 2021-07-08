//#include <ctype.h>
#include "common.hxx"
#include "htmlhead.hxx"
#include "doc_conf.hxx"

/*-@@@
File:           htmlhead.cxx
Version:        2.0
Description:    Class HTMLHEAD - HTML documents, <HEAD> only
Author:         Edward C. Zimmermann <edz@nonmonotonic.net>
@@@*/

#pragma ident  "@(#)htmlhead.cxx  1.20 02/24/01 17:45:06 BSN"

HTMLHEAD::HTMLHEAD (PIDBOBJ DbParent, const STRING& Name) : HTMLMETA (DbParent, Name)
{
}

HTMLHEAD::~HTMLHEAD()
{
}

const char *HTMLHEAD::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("HTMLHEAD");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  HTMLMETA::Description(List);
  return "HTML Doctype for use with spiders/crawlers.\n\
Processes META, TITLE and LINK elements in <HEAD>...</HEAD>\n\
or <METAHEAD>...</METAHEAD>";
}

void HTMLHEAD::SourceMIMEContent(STRING *StringPtr) const
{
  // MIME/HTTP Content type for HTML Documents
  StringPtr->AssignCopy(9, "text/html");
}

void HTMLHEAD::SourceMIMEContent(const RESULT& Record, PSTRING StringPtr) const
{
  static const STRING Encoding ("$Content-Encoding");
  DOCTYPE::Present (Record, Encoding, StringPtr);
  if (StringPtr->IsEmpty())
    SourceMIMEContent(StringPtr);
}

void HTMLHEAD::DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
        const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  if (ElementSet == METADATA_MAGIC)
    Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
  else if (ElementSet == FULLTEXT_MAGIC)
    DOCTYPE::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
  else
    COLONDOC::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}

void HTMLHEAD::Present (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  if (ElementSet == BRIEF_MAGIC)
    {
      static const STRING Title ("title");
      DOCTYPE::Present (ResultRecord, Title, StringBuffer);
      if (StringBuffer->IsEmpty())
	URL(ResultRecord, StringBuffer, GDT_FALSE);
      else
	StringBuffer->Pack();
    }
  else if (ElementSet == METADATA_MAGIC)
    {
      HTMLMETA::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
    }
  else
    COLONDOC::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}


HTMLZERO::HTMLZERO (PIDBOBJ DbParent, const STRING& Name) : HTMLHEAD (DbParent, Name)
{
}

const char *HTMLZERO::Description(PSTRLIST List) const
{
  List->AddEntry (Doctype);
  HTMLHEAD::Description(List);
  return "HTML Zero: Native HTML indexing that does not treat 'most' tagnames as words.\n\
In contrast to rendered HTML it still supports fielded search all the metadata and\n\
allows one to use HTML comments to supplement documents with searchable but not\n\
'visible' information.";
}

GPTYPE HTMLZERO::ParseWords(UCHR* DataBuffer, GPTYPE DataLength,
        GPTYPE DataOffset, GPTYPE* GpBuffer, GPTYPE GpLength)
{
  const UCHR zapChar = ' ';
  int        zap = 0;
  int        quote = 0;
  UCHR       Ch;
  GPTYPE     qPosition=0;

  for (register GPTYPE Position =0; Position < DataLength; Position++)
    {
      // Zap all tags but comments
      if (DataBuffer[Position] == '<' && (DataLength-Position) > 3 &&
	DataBuffer[Position+1] != '!' && DataBuffer[Position+2] != '-')
	zap=1;
      // Zap untill end of tag or '='
      if (zap || quote)
	{
#if 1
	  // Special case.. Zap everything in a script
	  if (quote == 0 && strncasecmp((const char *)&DataBuffer[Position], "<script", 7) == 0) {
	   GPTYPE newPos = Position + 6;
	   while (++newPos < (DataLength-10)) {
	      if (strncasecmp((const char *)&DataBuffer[newPos], "</script", 8) == 0) {
		memset((void *)&DataBuffer[Position], zapChar, newPos - Position+1);
		Position = newPos;
		continue;
	      }
	   }
	  }
#endif
#if 1
	  // Zap http-equiv=".." and name="..." stuff
	  if (quote == 0 && (
		strncasecmp((const char *)&DataBuffer[Position], "http-equiv", 10) == 0 ||
		strncasecmp((const char *)&DataBuffer[Position], "name", 4) == 0 ) ) { 
	     GPTYPE newPos = Position+4;
	     while ((Ch = DataBuffer[newPos]) != '>' && Ch != '=' && newPos<DataLength)
	       newPos++; 
	     if (Ch == '=') {
		// Can now start to zap
		if ((Ch = DataBuffer[++newPos]) == '"' || Ch == '\'') {
		  quote = Ch;
		  while ((Ch = DataBuffer[++newPos]) != quote && newPos <DataLength)
		    /* loop */;
		  if (Ch == quote) {
		    // Now Zap!	
		    memset((void *)&DataBuffer[Position], zapChar, newPos - Position+1);
		    Position = newPos;
		    quote = 0;
		    continue; 
		  }
		}
	     } 
	  }
#endif
	  if ((Ch = DataBuffer[Position]) == '>') {
	    zap = 0;
	    quote = 0;
	  } else if (quote && ((Ch == quote) || (quote == '=' && isspace(Ch)))) {
	    // End of quotation
#if 0
	    if ((Position - qPosition) > 2 && (Position - qPosition) < 20) {
	      char tmp[20];
	      while (isspace(DataBuffer[qPosition])) qPosition++;
	      strncpy (tmp, (char *) &DataBuffer[qPosition], Position-qPosition) ;
	      tmp[Position-qPosition]='\0';
	      if (strncasecmp(tmp, "Java", 4) == 0 ||
		  strncasecmp(tmp, "content", 7) == 0 ||
		  strncasecmp(tmp, "html", 4) == 0 ||
		  strncasecmp(tmp, "http", 4) == 0)
		memset((char *) &DataBuffer[qPosition], zapChar, Position-qPosition) ;
//cerr << "Contents = \"" << tmp << "\"" << endl;
	    }
#endif
	    zap = 1;
	    quote = 0;
	  } else if (quote == 0 && (Ch == '"' || Ch == '\'' || Ch == '='))
	    {
	      DataBuffer[Position] = zapChar;
	      zap = 0;
	      if (Ch == '=') {
		if ((Ch = DataBuffer[++Position]) != '"' && Ch != '\'')
		  Ch = '=';
		else
		  DataBuffer[Position] = zapChar;
	      }
	      qPosition = Position;
	      quote = Ch;
	    }
	}
      if (zap) DataBuffer[Position] = zapChar; // Zap these...
    }
  if (zap || quote) logf(LOG_WARN, "%s Record ended in a tag?", Doctype.c_str());
  
  return HTMLHEAD::ParseWords(DataBuffer, DataLength, DataOffset, GpBuffer, GpLength);
}

