/************************************************************************
************************************************************************/
#pragma ident  "@(#)mismedia.cxx  1.22 02/24/01 17:45:24 BSN"

/*-@@@
File:		mismedia.cxx
Version:	$Revision: 1.1 $
Description:	Class MISMEDIA - MISMEDIA documents
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@-*/

//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <errno.h>
//#include <ctype.h>
#include "mismedia.hxx"
#include "doc_conf.hxx"
//#include "common.hxx"

static struct Standort {
  int code;
  const char *name;
} Standorts[] = {
 // Sort in numerical order..
 {0, "LMZ"},
 {1, "Ahrweiler"},
 {2, "Altenkirchen"},
 {3, "Birkenfeld"},
 {4, "Cochem-Zell"},
 {5, "Bad Ems"},
 {6, "Koblenz"},
 {7, "Bad Kreuznach"},
 {8, "Mayen"},
 {9, "Montabaur"},
 {10, "Neuwied"},
 {11, "Simmern"},
 {12, "Bitburg-Pruem"},
 {13, "Daun"},
 {14, "Saarburg"},
 {15, "Trier"},
 {16, "Bernkastel-Wittlich"},
 {17, "Alzey-Worms"},
 {18, "Mainz-Bingen"},
 {19, "Germersheim"},
 {20, "Kaiserslautern"},
 {21, "Kirchheimbolanden"},
 {22, "Kusel"},
 {23, "Landau"},
 {24, "Ludwigshafen"},
 {25, "Mainz"},
 {26, "Bad Duerkheim"},
 {27, "Pirmasens"},
 {28, "Speyer"},
 {29, "Worms"},
 {30, "Zweibruecken"},
 {31, "Neustadt"},
 {41, "LFD-Mainz"},
 {42, "LFD-Koblenz"},
 {43, "LFD-Neustadt"},
 {44, "LFD-Saarbruecken"}
};

static int orts_compare(const void *node1, const void *node2)
{
  return ((const struct Standort *)node1)->code - ((const struct Standort *)node2)->code;
}

STRING& MISMEDIA::Standort(const char *Code, PSTRING Value) const
{
  if (Code == NULL || *Code == '\0')
    return *Value = "???";
  STRING Default = "unknown";
  const int codeNum = atoi(Code);
  struct Standort *node_ptr, node;
  node.code = codeNum;

  if ((node_ptr = (struct Standort *)
	bsearch(&node, Standorts, sizeof(Standorts)/sizeof(struct Standort),
	sizeof(struct Standort), orts_compare)) != NULL)
    Default = node_ptr->name;
  if (tagRegistry)
    tagRegistry->ProfileGetString("Standorts", Code, Default, Value);
  else
    *Value = Default;
  return *Value;
}

static const char MIME_TYPE[] = "Application/X-MISmedia";

void MISMEDIA::DefineField(const STRING& Name, const STRING& Default,
	PSTRING Value)
{
  if (Value->GetLength() == 0)
    {
      if (tagRegistry)
	tagRegistry->ProfileGetString("General", Name, Default, Value);
      else
	*Value = Default;
    }
}

MISMEDIA::MISMEDIA (PIDBOBJ DbParent, const STRING& Name) :
	COLONGRP (DbParent, Name)
{
  DefineField ("DateField",   "Update",     &DateField);
  DefineField ("CallNumber",  "Signatur",   &KeyField);
  DefineField ("Title",       "Titel",      &Title);
  DefineField ("DateOfIssue", "Ersch_jahr", &DateOfIssue);
  DefineField ("Keywords",    "Schlagwort", &Keywords);
  DefineField ("Standorts",   "",           &StandortsURL);

  STRLIST Scratch;
  STRING S;

  if (tagRegistry)
    {
      tagRegistry->ProfileGetString ("General", "EmbedFields",
	Keywords, &Scratch);
    }
  else
    {
      Scratch.AddEntry(Keywords);
    }
  for (const STRLIST *p = Scratch.Next(); p != &Scratch; p=p->Next())
    {
      UnifiedName(p->Value(), &S);
      if (S.GetLength())
	EmbedFields.AddEntry(S);
    }
}


const char *MISMEDIA::Description(PSTRLIST List) const
{
  List->AddEntry ("MISMEDIA");
  COLONGRP::Description(List);
  return "MIS (Rheinland-Pfalz) colondoc mediagraphic records.";
}


void MISMEDIA::SourceMIMEContent(const RESULT& ResultRecord,
	PSTRING StringPtr) const
{
  DOCTYPE::SourceMIMEContent(ResultRecord, StringPtr);
}

void MISMEDIA::SourceMIMEContent(PSTRING sPtr) const
{
  *sPtr = MIME_TYPE;
}

static const char *URLencode(const STRING& Str, PSTRING Code)
{
  GDT_BOOLEAN quot = GDT_FALSE;
  static const char HEX[] = "0123456789ABCDEF";
  STRING q;
  for(const UCHR *tp = (const UCHR *)Str; *tp; tp++)
      {
	if (*tp == '\\')
	  {
	    UCHR ch = 0;
	    switch (*(tp+1)) {
	      case 'b': ch = '\b'; break;
	      case 'n': ch = '\n'; break;
	      case 'r': ch = '\r'; break;
	      case 'v': ch = '\v'; break;
	      case 't': ch = '\t'; break;
	      default:
		q.Cat(*tp++); // Output backslash
		ch = *tp;
	    }
	    if (isalnum(ch))
	      {
	        q.Cat(ch);
	      }
	    else
	      {
		q.Cat('%');
		q.Cat(HEX[(ch & '\377') >> 4]);
		q.Cat(HEX[(ch & '\377') % 16]);
	      }
	  }
        else if (*tp == '"')
	  {
            quot = !quot;
	    q.Cat("%22");
	  }
        else if (*tp == '/' && !quot)
	  {
            q.Cat("%252F");
	  }
        else if (isspace(*tp))
	  {
	    q.Cat(",");
	  }
        else if (isalnum(*tp) || *tp == '.' ||
		*tp == '!' || *tp == '?' || *tp == '@' ||
                *tp == '*' || *tp == '~' || *tp == '\'')
	  {
            q.Cat(*tp);
	  }
        else {
          // Encode as HEX
          q.Cat('%');
          q.Cat(HEX[(*tp & '\377') >> 4]);
          q.Cat(HEX[(*tp & '\377') % 16]);
        }
     }
  return (const char *)(*Code = q);
}

void MISMEDIA::Present (const RESULT& ResultRecord,
	const STRING& ElementSet, const STRING& RecordSyntax,
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
      STRING Call, Titel, Year;
      DOCTYPE::Present (ResultRecord, UnifiedName(KeyField, &Tmp), &Call);
      DOCTYPE::Present (ResultRecord, UnifiedName(Title, &Tmp), &Titel);
      DOCTYPE::Present (ResultRecord, UnifiedName(DateOfIssue,&Tmp), &Year);
      GDT_BOOLEAN got = GDT_FALSE;

      StringBuffer->Clear();
      if (Call.GetLength())
	{
	  Headline.Cat (Call);
	  got = GDT_TRUE;
	}
      if (Titel.GetLength() == 0)
	DOCTYPE::Present (ResultRecord, "Title", &Titel);
      if (Titel.GetLength())
	{
	  if (got)
	    Headline.Cat (": ");
	  Headline.Cat (Titel);
	  got = GDT_TRUE;
	}
      if (Year.GetLength())
	{
	  if (got)
	    Headline.Cat ("; ");
	  Headline.Cat (Year);
	  got = GDT_TRUE;
	}
      if (! got)
	{
	  COLONGRP::Present (ResultRecord,ElementSet,RecordSyntax,StringBuffer);
	  return;
	}
      if (UseHtml)
	HtmlCat(Headline, StringBuffer, GDT_FALSE);
      else
	StringBuffer->Cat (Headline);
    }
  else if (ElementSet ^= "Standort")
    {
      *StringBuffer = "";
      DOCTYPE::Present (ResultRecord, ElementSet, "", &Tmp);
      if (Tmp.GetLength())
        {
	  GDT_BOOLEAN showS = (UseHtml && StandortsURL.GetLength());
          // These are seperated by ';'
          UCHR *buf = Tmp.NewUCString();
          UCHR *tp2, *tp = buf;
          while ((tp2 = (UCHR *)strchr((char *)tp, ';')) != NULL)
            {
              *tp2++='\0';
              while (isspace(*tp)) tp++;
	      if (showS)
		*StringBuffer << "<A HREF=\"" << StandortsURL << tp << ".html\">";
	      *StringBuffer << Standort((const char *)tp, &Tmp);
	      if (showS)
		*StringBuffer << "</A>";
	      *StringBuffer << "; ";
              tp = tp2;
            }
          if (*tp)
            {
              while (isspace(*tp)) tp++;
	      if (showS)
		*StringBuffer << "<A HREF=\"" << StandortsURL << tp << ".html\">";
	      *StringBuffer << Standort((const char *)tp, &Tmp);
	      if (showS)
		*StringBuffer << "</A>";
            }
          delete[]buf;
        }
    }
#if 0
  else if (UseHtml && (ElementSet ^= "Annotation"))
    {
      // Special Case
      DOCTYPE::Present (ResultRecord, ElementSet, "", &Tmp);
      if (Tmp.GetLength())
	{
	  UCHR *buf = Tmp.NewUCString();
	  UCHR *tp2, *tp = buf;
	  INT   count, old_count = 0;
	  *StringBuffer = "<OL>";
	  while ((tp2 = (UCHR *)strchr((char *)tp, ';')) != NULL)
	    {
	      *tp2++='\0';
	      while (isspace(*tp)) tp++;
	      if (isdigit(*tp) &&
		(count = atoi((const char *)tp)) == (old_count+1))
		{
		  while (isdigit(*tp))
		    tp++;
		  if (*tp == '.') tp++;
		  old_count = count;
		}
	      StringBuffer->Cat ("<LI>");
	      HtmlCat(tp, StringBuffer);
              tp = tp2;
            }
	  if (*tp)
	    {
	      while (isspace(*tp)) tp++;
	      if (isdigit(*tp) &&
		(count = atoi((const char *)tp)) == (old_count+1))
		{
		  while (isdigit(*tp)) tp++;
		  if (*tp == '.') tp++;
		}
	      StringBuffer->Cat ("<LI>");
	      HtmlCat(tp, StringBuffer);
	    }
	  StringBuffer->Cat ("</OL>");
	  delete[] buf;
	}
    }
#endif
  else if (UseHtml && (
	ElementSet.SearchAny (UnifiedName("_MOTBIS", &Tmp)) ||
	ElementSet.SearchAny (UnifiedName("_TEE", &Tmp)) ||
	EmbedFields.SearchCase(ElementSet) ))
    {
      *StringBuffer = "";
      // Subject_MOTBIS: iducation civique; France; 
      DOCTYPE::Present (ResultRecord, ElementSet, "", &Tmp);
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
  else
    COLONGRP::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}

MISMEDIA::~MISMEDIA ()
{
}
