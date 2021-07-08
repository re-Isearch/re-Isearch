/************************************************************************
************************************************************************/
#pragma ident  "@(#)euromedia.cxx  1.25 12/12/98 16:18:58 BSN"

/*-@@@
File:		euromedia.cxx
Version:	$Revision: 1.1 $
Description:	Class EUROMEDIA - EUROMEDIA documents
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@-*/

//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <errno.h>
//#include <ctype.h>
#include "euromedia.hxx"
#include "doc_conf.hxx"
//#include "common.hxx"
//#include "lang-codes.hxx"


static const char MIME_TYPE[] = "Application/X-Euromedia";

// Idea:
// For each language, in the presentation, MAP the field
// names to the names in the language of the record.
// This way an Italian record is completely in Italian!
//

EUROMEDIA::EUROMEDIA (PIDBOBJ DbParent, const STRING& Name) :
	COLONGRP (DbParent, Name)
{
  if (LanguageField.IsEmpty())
    LanguageField = "Code_language";
  if (DateField.IsEmpty())
    DateField = "Date_last_modified"; 
}

const char *EUROMEDIA::Description(PSTRLIST List) const
{
  List->AddEntry ("EUROMEDIA");
  COLONGRP::Description(List);
  return "\
EUROMEDIA multinational mediagraphic format files.\n\
  Default Language Field is 'Code_language'\n\
  Default Date Field is 'Date_last_modified'";
}


void EUROMEDIA::SourceMIMEContent(const RESULT& ResultRecord,
	PSTRING StringPtr) const
{
  DOCTYPE::SourceMIMEContent(ResultRecord, StringPtr);
}

void EUROMEDIA::SourceMIMEContent(PSTRING sPtr) const
{
  *sPtr = MIME_TYPE;
}

STRING& EUROMEDIA::DescriptiveName(const STRING& Language,
	const STRING& FieldName, PSTRING Value) const
{
  if (Language.GetLength() && tagRegistry)
    {
      STRING lang;
      tagRegistry->ProfileGetString("Code_language", Language, NulString, &lang);
      if (lang.GetLength())
	{
	  tagRegistry->ProfileGetString(lang, FieldName, NulString, Value);
	  if (Value->GetLength())
	    return (*Value); // Done
	}
    }
  DOCTYPE::DescriptiveName(Language, FieldName, Value);
  return (*Value);
}


void EUROMEDIA::
DocPresent (const RESULT& ResultRecord,
	    const STRING& ElementSet, const STRING& RecordSyntax,
	    PSTRING StringBuffer) const
{
  // FullText HTML or SUTRS (Plain Text)
  if (ElementSet.Equals (FULLTEXT_MAGIC) &&
	((RecordSyntax == HtmlRecordSyntax) || 
	 (RecordSyntax == SutrsRecordSyntax)) )
    {
      GDT_BOOLEAN useHTML = (RecordSyntax == HtmlRecordSyntax);
      if (useHTML)
	HtmlHead (ResultRecord, ElementSet, StringBuffer);
      // Documents are 1-deep so just collect the fields
      DFDT Dfdt;
      STRING Key;
      ResultRecord.GetKey (&Key);
      GetRecordDfdt (Key, &Dfdt);
      const size_t Total = Dfdt.GetTotalEntries();
      DFD Dfd;
      STRING Value, FieldName, LongName, Language;
      // Get the language for the descriptive names
      Present (ResultRecord, UnifiedName("Code_language", &Value),
	SutrsRecordSyntax, &Language);
      if (useHTML && Language.GetLength())
	*StringBuffer << "<!-- Use '" << Language << "' names -->\n";
      // Walk-though the DFD
      if (useHTML) StringBuffer->Cat ("<TABLE BORDER=\"0\">\n");
      for (INT i = 1; i <= Total; i++)
	{
	  Dfdt.GetEntry (i, &Dfd);
	  Dfd.GetFieldName (&FieldName);

	  // Get Value of the field, use parent
	  Present (ResultRecord, FieldName, RecordSyntax, &Value);
	  if (Value.GetLength())
	    {
	      DescriptiveName(Language, FieldName, &LongName);
	      if (LongName.GetLength())
		{
		  if (useHTML)
		    {
		      StringBuffer->Cat ("<TR><TH ALIGN=\"Left\" VALIGN=\"Top\">");
		      *StringBuffer << "<!-- " << FieldName << " -->";
		      HtmlCat (LongName, StringBuffer);
		      StringBuffer->Cat (":</TH><TD VALIGN=\"Top\">");
		      StringBuffer->Cat (Value);
                      StringBuffer->Cat ("</TD></TR>\n");
		    }
		  else *StringBuffer << LongName << ":\t" << Value << "\n";
		}
	    }
	}			/* for */
      if (useHTML)
	{
	  StringBuffer->Cat ("</TABLE>");
	  HtmlTail (ResultRecord, ElementSet, StringBuffer); // Tail bits
	}
    }
  else
    {
      COLONGRP::DocPresent (ResultRecord, ElementSet, RecordSyntax,
	StringBuffer);
    }
}

void EUROMEDIA::
Present (const RESULT& ResultRecord, const STRING& ElementSet, const STRING& RecordSyntax,
	PSTRING StringBuffer) const
{
  const GDT_BOOLEAN UseHtml = (RecordSyntax == HtmlRecordSyntax);
  STRING Tmp;
  if (ElementSet == SOURCE_MAGIC)
    {
      COLONGRP::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
    }
  else if (ElementSet == BRIEF_MAGIC)
    {
      STRING Headline;
      STRING Call, Title, Language;

      DOCTYPE::Present (ResultRecord, UnifiedName("Local_number", &Tmp), &Call);
      DOCTYPE::Present (ResultRecord, UnifiedName("Title", &Tmp), &Title);
      DOCTYPE::Present (ResultRecord, UnifiedName("Code_language", &Tmp), &Language);
      GDT_BOOLEAN got = GDT_FALSE;

      StringBuffer->Clear();
      if (Title.GetLength() == 0)
	{
	  DOCTYPE::Present (ResultRecord, UnifiedName("Title_series", &Tmp), &Title);
	  if (Title.GetLength() == 0)
	    DOCTYPE::Present (ResultRecord,
		UnifiedName("Title_other_variant", &Tmp), &Title);
	}
      if (Call.GetLength())
	{
	  Headline.Cat (Call);
	  got = GDT_TRUE;
	}
      if (Title.GetLength())
	{
	  if (got)
	    Headline.Cat (": ");
	  Headline.Cat (Title);
	  got = GDT_TRUE;
	}
      if (! got)
	{
	  COLONGRP::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
	  return;
	}
      if (Language.GetLength() == 0)
	{
	  Language = ResultRecord.GetLanguageCode();
	}
      if (Language.GetLength())
	{
	  STRING Value;
	  if (tagRegistry)
	    {
	      tagRegistry->ProfileGetString("Code_language", Language, NulString, &Value);
	    }
	  if (Value.GetLength())
	    Language = Value;
 	  else
	    Language = Lang2Language ((const char *)Language); 
	  Headline.Cat (" (");
	  Headline.Cat (Language);
	  Headline.Cat (")");
	}
      if (UseHtml)
	HtmlCat(Headline, StringBuffer, GDT_FALSE);
      else
	StringBuffer->Cat (Headline);
    }
#if 1		/* EXPERIMENTAL */
  else if (UseHtml && (ElementSet ^= UnifiedName("Dewey_classification", &Tmp)))
    {
      StringBuffer->Clear();
      // Dewey_classification: 111.11 (number)
      // Subject_MOTBIS: iducation civique; France;
      DOCTYPE::Present (ResultRecord, ElementSet, NulString, &Tmp);
      if (Tmp.GetLength())
	{
	  STRING DBname, Anchor;
	  Db->DbName(&DBname);
	  RemovePath(&DBname);
	  Anchor << "<A TARGET=\"" << ElementSet << "\" \
onMouseOver=\"self.status='Find " << ElementSet << "'; return true\" \
HREF=\"i.search?DATABASE%3D" << DBname << "/TERM%3D%22" << ElementSet << "/";
	  UCHR *buf = Tmp.NewUCString();
	  UCHR *tp = buf, *tcp = buf;
	  UCHR ch;

	  for (;;)
	    {
	      while (*tcp && !isdigit(*tcp)) tcp++;
	      ch = *tcp;
	      *tcp = '\0';
	      if (*tp) StringBuffer->Cat (tp);
	      *tcp = ch;
	      if (!*tcp) break; // Done
	      tp = tcp; // pointing at start of digits
	      while (isdigit(*tcp) || *tcp == '.') tcp++;
	      ch = *tcp;
	      *tcp = '\0';
	      *StringBuffer << Anchor << URLencode(tp, &Tmp) << "%22\">";
	      HtmlCat(tp, StringBuffer, GDT_FALSE);
	      *StringBuffer << "</A>; ";
	      *tcp = ch;
	      tp = tcp; // pointing at after digits
	    }
	  delete[] buf;
	}
    }
  else if (UseHtml &&
	   ((ElementSet ^= UnifiedName("Subject_MOTBIS", &Tmp)) ||
	   (ElementSet ^= UnifiedName("Subject_TEE", &Tmp)) ||
	   (ElementSet ^= UnifiedName("Local_subject_index", &Tmp)) ))
    {
      StringBuffer->Clear();
      // Subject_MOTBIS: iducation civique; France; 
      DOCTYPE::Present (ResultRecord, ElementSet, NulString, &Tmp);
      if (Tmp.GetLength())
	{
	  STRING DBname, Anchor;
	  Db->DbName(&DBname);
	  RemovePath(&DBname);
	  Anchor << "<A TARGET=\"" << ElementSet << "\" \
onMouseOver=\"self.status='Find " << ElementSet << "'; return true\" \
HREF=\"i.search?DATABASE%3D" << DBname
	<< "/TERM%3D%22" << ElementSet << "/";
	  // These are seperated by ';'
	  UCHR *buf = Tmp.NewUCString();
	  UCHR *tp2, *tp = buf;
	  while ((tp2 = (UCHR *)strchr((char *)tp, ';')) != NULL)
	    {
	      *tp2++='\0';
	      while (isspace(*tp)) tp++;
	      *StringBuffer << Anchor << URLencode(tp, &Tmp) << "%22\">";
	      HtmlCat(tp, StringBuffer, GDT_FALSE);
	      *StringBuffer << "</A>; ";
	      tp = tp2;
	    }
	  if (*tp)
	    {
	      while (isspace(*tp)) tp++;
	      *StringBuffer << Anchor
		<< URLencode(tp, &Tmp)
		<< "%22\">";
	      HtmlCat(tp, StringBuffer, GDT_FALSE);
	      *StringBuffer << "</A>";
	    }
	  delete[]buf;
	}
    }
#endif
  else
    COLONGRP::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}

EUROMEDIA::~EUROMEDIA ()
{
}
