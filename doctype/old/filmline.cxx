#pragma ident  "@(#)filmline.cxx	1.9 03/28/98 19:32:08 BSN"
/************************************************************************
************************************************************************/

/*-@@@
File:		filmline.cxx
Version:	2.00
Description:	Class FILMLINE - Filmline 1.x Document Type
		(Only Presentation and Semantics vary from Medline)
Author:		Edward C. Zimmermann, edz@bsn.com
@@@-*/

#include <ctype.h>
#include "common.hxx"
#include "filmline.hxx"
#include "doc_conf.hxx"

/* A few user-customizable parser features... */
#ifndef USE_UNIFIED_NAMES
# define USE_UNIFIED_NAMES 1 
#endif


FILMLINE::FILMLINE (PIDBOBJ DbParent, const STRING& Name):
	MEDLINE (DbParent, Name)
{
  if (DateField.IsEmpty())
    DateField = "DA";
}

const char *FILMLINE::Description(PSTRLIST List) const
{
  List->AddEntry ("FILMLINE");
  MEDLINE::Description(List);
  return "JFF Common Mediagraphic Record Format";
}


void FILMLINE::SourceMIMEContent(PSTRING StringPtr) const
{
// MIME/HTTP Content type for Filmline Sources
  *StringPtr = "Application/X-Filmline";
}

// Hooks into the Field parser from Medline
INT FILMLINE::UnifiedNames (const STRING& Tag, PSTRLIST Value) const
{
 static const TagTable_t Table[] = {
  /* Sorted List! */
    {"AB", "abstract"},
    {"AD", "address"},
    {"AU", "author"},
    {"AW", "awards"},
    {"CG", "camera"},
    {"CL", "clue"},
    {"CO", "country"},
    {"CR", "critics"},
    {"CS", "charset"},
    {"DA", "date-collected"},
    {"DE", "description"},
    {"DI", "director"},
    {"DK", "documentation"},
    {"DT", "distribution"},
    {"ED", "editor"},
    {"FD", "distribution"},
    {"FP", "fee-purchase"},
    {"FR", "fee-rent"},
    {"GE", "genre"},
    {"GM", "GEMA"},
    {"LA", "language"},
    {"KW", "keywords"},
    {"LN", "length"},
    {"LT", "lit-author"},
    {"MD", "media-type"},
    {"MM", "Content-type"},
    {"MU", "music"},
    {"PO", "parent"},
    {"PR", "producer"},
    {"PS", "ps"},
    {"PU", "publisher"},
    {"RA", "ratings"},
    {"SA", "collections"},
    {"SE", "series"},
    {"SH", "sort-title,title"},
    {"SS", "series-title,title"},
    {"SI", "signature"},
    {"SL", "alt-lang"},
    {"SM", "children"},
    {"SO", "other-source"},
    {"ST", "other-credits"},
    {"SU", "subtitle,title"},
    {"SY", "systematic"},
    {"TD", "technical-data"},
    {"TE", "title-uniform,title"},
    {"TI", "title-cover,title"},
    {"TU", "title-caption,title"},
    {"VL", "volume"},
    {"VT", "sync"},
    {"YR", "date"},
    {"ZU", "URL"}
    /* NO NULL please! */
  };
  return MEDLINE::UnifiedNames (Table, sizeof(Table)/sizeof(TagTable_t), Tag, Value);
}

STRING& FILMLINE::DescriptiveName(const STRING& FieldName, PSTRING Value) const
{
  return DOCTYPE::DescriptiveName(FieldName, Value);
}

// FILMLINE Handler
void FILMLINE::
Present (const RESULT& ResultRecord, const STRING& ElementSet,
      const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  STRING Tmp;
  *StringBuffer = "";
  if (ElementSet.Equals(BRIEF_MAGIC))
    {
      // Brief Headline is "Title (Director, Country/year)"
      STRING Title;
      DOCTYPE::Present (ResultRecord,
	UnifiedName("TI", &Tmp), &Title);
      if (Title.GetLength() == 0)
	{
	  // Should not really happen, heck use Original title
	  DOCTYPE::Present (ResultRecord,
		UnifiedName("TO", &Tmp), &Title);
	}

      STRING Director;
      DOCTYPE::Present (ResultRecord,
	UnifiedName("DI", &Tmp), &Director);
      if (Director.GetLength() == 0)
	{
	  // No director, then use author, scriptwriter
	  DOCTYPE::Present (ResultRecord,
		UnifiedName("AU", &Tmp), &Director);
	  if (Director.GetLength() == 0)
	    {
	      // No author?? Use editor if we have one
	      DOCTYPE::Present (ResultRecord,
		UnifiedName("ED", &Tmp), &Director);
	    }
	}

      STRING Country;
      DOCTYPE::Present (ResultRecord,
	UnifiedName("CO", &Tmp), &Country);

      STRING Year;
      DOCTYPE::Present (ResultRecord,
	UnifiedName("YR", &Tmp), &Year);

      STRING Headline;
      Headline = Title;

      STRINGINDEX HaveDirector = Director.GetLength();
      STRINGINDEX HaveYear = Year.GetLength();
      if (HaveDirector || HaveYear)
	{
	  Headline.Cat(" (");
	  if (HaveDirector) Headline.Cat(Director);
	  if (HaveDirector && HaveYear) Headline.Cat(", ");
	  if (Country.GetLength()) {
	    if (HaveDirector && !HaveYear) Headline.Cat(" ");
	    Headline.Cat(Country);
	    if (HaveYear) Headline.Cat("/");
	  }
	  if (HaveYear) Headline.Cat(Year);
	  Headline.Cat(")");
	}
      else
	{
	  // Do we have any credits?
	  DOCTYPE::Present (ResultRecord,
		UnifiedName("ST", &Tmp), &Director);
	  if (Director.GetLength())
	    {
	      Headline.Cat (" (");
	      Headline.Cat (Director);
	      Headline.Cat (")");
	    }
	}
      if (RecordSyntax == HtmlRecordSyntax)
	{
	  HtmlCat(Headline, StringBuffer);
	}
      else
	{
	  *StringBuffer = Headline;
	}
    }
  else
    MEDLINE::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}

FILMLINE::~FILMLINE()
{
}

