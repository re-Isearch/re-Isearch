#pragma ident  "@(#)referbib.cxx	1.12 04/20/01 14:23:55 BSN"
/************************************************************************
************************************************************************/

/*-@@@
File:		referbib.cxx
Version:	$Revision: 1.1 $
Description:	Class REFERBIB - Refer bibliographic records
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@-*/
/*
// Revision History
// ===============
// $Log: referbib.cxx,v $
// Revision 1.1  2007/05/15 15:47:29  edz
// Initial revision
//
// Revision 1.7  1996/04/19  20:43:01  edz
// Modified fieldnames and extended to better support HCI
// bibliography records.
//
// Revision 1.6  1995/12/13  09:47:12  edz
// Minor bugs in parser fixed.
//
// Revision 1.5  1995/12/06  10:32:01  edz
// Minor mods to match DOCTYPE arch.
//
// Revision 1.4  1995/11/29  09:36:24  edz
// Sync
//
// Revision 1.3  1995/11/25  22:43:14  edz
// Minor mods to HtmlRecordSyntax and HTML header.
//
// Revision 1.2  1995/11/19  02:18:02  edz
// Support HTML Record Syntax Presentation
//
*/

//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <errno.h>
//#include <ctype.h>
#include "referbib.hxx"
#include "common.hxx"
#include "doc_conf.hxx"


#define WANT_MISC 1 /* 1=> Store "unclassified" in "misc", 0=> ignore these */
/* Refer databases can contain some lookbib(1) pre-processor records */
#define IGNORE_LOOKBIB_RECORDS 0 /* 1==> Don't bother with fields, 0=> Collect fields */


/*-----------------------------------------------------------------*/

REFERBIB::REFERBIB (PIDBOBJ DbParent, const STRING& Name) :
	DOCTYPE (DbParent, Name)
{
}

const char *REFERBIB::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("REFERBIB");
  if (Doctype != ThisDoctype && List->IsEmpty())
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);
  DOCTYPE::Description(List);
  return "Refer bibliographic record format (used by several systems).\n\
The default field parser recognizes the extended names used by many systems\n\
including EndNote, Papyrus and the HCI extensions.";
}

void REFERBIB::SourceMIMEContent(PSTRING StringPtr) const
{
// MIME/HTTP Content type for Refer documents
  *StringPtr = "Application/X-Refer"; 
}

void REFERBIB::AddFieldDefs ()
{
  DOCTYPE::AddFieldDefs ();
}

void REFERBIB::ParseRecords (const RECORD& FileRecord)
{
  // Break up the document into Medline records
  off_t Start = FileRecord.GetRecordStart ();
  off_t End = FileRecord.GetRecordEnd();
  off_t Position = 0;
  off_t SavePosition = 0;
  off_t RecordEnd = End;

  const STRING Fn (FileRecord.GetFullFileName ());
  PFILE Fp = ffopen (Fn, "rb");
  if (!Fp)
    {
      message_log(LOG_ERRNO, "Could not access \"%s\"", Fn.c_str());
      return;			// File not accessed
    }
  RECORD Record (FileRecord);	// Easy way
  // Move to start if defined
  if (Start > 0)
    fseek (Fp, Start, SEEK_SET);

  enum { HUNTING, CONTINUING, LOOKING } State = LOOKING;
  int Ch;
  while ((Ch = getc (Fp)) != EOF)
    {
      Position++;
      if (End > 0 && Position > End) break; // End of Subrecord
      if (Ch == '\n' && State == HUNTING)
	State = LOOKING;
      else if (Ch == '\n' && State == LOOKING)
	{
	  // Have a record boundary ( a blank line )
	  State = CONTINUING;
	  SavePosition = Position;
	  Record.SetRecordStart (Start);
	  RecordEnd = (SavePosition == 0) ? 0 : SavePosition - 1;

	  if (RecordEnd > Start)
	    {
	      Record.SetRecordEnd (RecordEnd);
	      Db->DocTypeAddRecord (Record);
	      Start = SavePosition;
	    }
	}
      else if (Ch == '%' && State == CONTINUING)
	State = HUNTING;	// Now in the body
      // @@@ BUGFIX edz: Wed Dec 13 00:31:50 MET 1995:
      // Accept ' ' or '\t' in record boundary
      else if (Ch != ' ' && Ch != '\t' && State == LOOKING)
	State = HUNTING;
    }				// while

  ffclose (Fp);

  Record.SetRecordStart (Start);
  RecordEnd = (SavePosition == 0) ? 0 : Position - 1;

  // @@@ BUGFIX edz: Wed Dec 13 10:18:20 MET 1995
  // Add record if no boundary (single record bib)
  if (RecordEnd > Start || (RecordEnd == 0 && End == 0))
    {
      Record.SetRecordEnd (RecordEnd);
      Db->DocTypeAddRecord (Record);
    }
}


/*
Refer Fields/Tags:

%A := author             Author's name
%B := book               Book containing article referenced
%C := place              City (place of publication)
%D := date               Date of publication
%E := editor             Editor of book containing article referenced
%F  (ignore)             Footnote number or label (supplied by refer)
%G := DocID              Government order number, ISBN, ISSN, URLs etc
%H := comment            Header commentary, printed before reference
%I := publisher          Issuer (publisher)
%J := journal            Journal containing article
%K := keywords           Keywords to use in locating reference
%L  (ignore)             Label field used by -k option of refer
%M := cdat               modification info was Bell Labs Memorandum (undefined)
%N := number             Number within volume
%O := comment            Other commentary, printed at end of reference
%P := pages              Page number(s)
%Q := corp_author        Corporate or Foreign Author (unreversed)
%R := report             Report, paper, or thesis (unpublished)
%S := series             Series title
%T := title              Title of article or book
%U := annote             User annotation
%V := volume             Volume number
%X := abstract           Abstract - used by roffbib, not by refer
%Y := toc                Table of Contents-- ignored by refer
%Z := references         Contains URLs-- ignored by refer
%$ := price              Purchase Price (extension)
%* := copyright          Copyright notice (extension)
%^ := container          Contained parts (extension)


Tags maked (ignore) and tags not in list should probably be ignored by Isearch
or thown into a "misc" field

NOTE:
  The above tag name list should be "unified" with other bibliographic
formats that might be added to Isearch, eg. BibTeX
*/

STRING REFERBIB::UnifiedName (const STRING& Tag, PSTRING Value) const
{
  const char *Table[] = {
    /* A */ "author",
    /* B */ "book",
    /* C */ "place",
    /* D */ "date",
    /* E */ "editor",
    /* F */ NULL,
    /* G */ "DocID",
    /* H */ "comment",
    /* I */ "publisher",
    /* J */ "journal",
    /* K */ "keywords",
    /* L */ NULL,
    /* M */ "cdat",
    /* N */ "number",
    /* O */ "comment",
    /* P */ "pages",
    /* Q */ "corp_author",
    /* R */ "report",
    /* S */ "series",
    /* T */ "title",
    /* U */ "Anote",
    /* V */ "volume",
    /* W */ NULL,
    /* X */ "abstract",
    /* Y */ "toc",
    /* Z */ "references"
  };

  const char *tag = Tag.c_str();

  // Make sure its a "legal" tag;
  if (tag[0] != '%' || tag[2] != '\0')
    {
      Value->Clear(); // Not a REFER field
      return *Value;
    }
  // HCI project extensions
  if (tag[1] == '$') return DOCTYPE::UnifiedName("price", Value);
  if (tag[1] == '*') return DOCTYPE::UnifiedName("copyright", Value);
  if (tag[1] == '^') return DOCTYPE::UnifiedName("container", Value);

  // 0-9
  if (tag[1] >= '0' && tag[1] <= '9')
    {
    }
  // Ignore lower case tags
  else if (tag[1] < 'A' || tag[1] > 'Z')
#if WANT_MISC
    return DOCTYPE::UnifiedName("Other_fields", Value);
#else
    return *Value = "";
#endif
  // Return unified field name
  return DOCTYPE::UnifiedName(Table[(unsigned)tag[1] - (unsigned)'A'], Value);
}

void REFERBIB::ParseFields (RECORD *NewRecord)
{
  const STRING fn ( NewRecord->GetFullFileName () );

  PFILE fp = ffopen (fn, "rb");
  if (!fp)
    {
      NewRecord->SetBadRecord();
      return;		// ERROR
    }

  off_t RecStart = NewRecord->GetRecordStart ();
  off_t RecEnd = NewRecord->GetRecordEnd ();
  if (RecEnd == 0)
    RecEnd = GetFileSize(fp);
  if (RecEnd <= RecStart)
    {
       ffclose(fp);
       NewRecord->SetBadRecord();
       return;
    }
  if (-1 == fseek (fp, RecStart, SEEK_SET))
    message_log(LOG_ERRNO, "Couldn't seek \"%s\"", fn.c_str());
  off_t RecLength = RecEnd - RecStart + 1;
  CHR *RecBuffer = (CHR *)Buffer.Want( RecLength + 1 );
  off_t ActualLength = fread (RecBuffer, 1, RecLength, fp);
  ffclose (fp);
  RecBuffer[ActualLength] = '\0';

  PCHR *tags = parse_tags (RecBuffer, ActualLength);
  if (tags == NULL || tags[0] == NULL)
    {
      if (tags)
	message_log(LOG_WARN, "No `" + Doctype + "' fields/tags in \"" + fn + "\" record.");
       else
	message_log(LOG_ERROR, "Unable to parse `" + Doctype + "' record in \"" + fn + "\".");
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
  for (PCHR * tags_ptr = tags; *tags_ptr; tags_ptr++)
    {
      PCHR p = tags_ptr[1];
      if (p == NULL)
	p = &RecBuffer[RecLength]; // End of buffer
      // eg "%A "
      int off = strlen (*tags_ptr) + 1;
      INT val_start = (*tags_ptr + off) - RecBuffer;
      // Skip while space after the ' '
      while (isspace (RecBuffer[val_start]))
	val_start++, off++;
      // Also leave off the \n
      INT val_len = (p - *tags_ptr) - off - 1;
      // Strip potential trailing while space
      while (val_len >= 0 && isspace (RecBuffer[val_len + val_start]))
	val_len--;
/*
      // Strip "ed values
      if (RecBuffer[val_start] == '"' && RecBuffer[val_len+val_start] == '"')
	{
	  val_start++, val_len -= 2;
	}
*/
      if (val_len < 0) continue; // forget empty fields

#if BSN_EXTENSIONS
      if (strncmp(*tags_ptr, "%D", 2) == 0)
	{
	  NewRecord->SetDate( *tags_ptr + 2 );
	}
#endif
      if (UnifiedName(*tags_ptr, &FieldName).GetLength() == 0)
	continue; // ignore these
      dfd.SetFieldName (FieldName);
      Db->DfdtAddEntry (dfd);
      fc.SetFieldStart (val_start);
      fc.SetFieldEnd (val_start + val_len);
      df.SetFct (fc);
      df.SetFieldName (FieldName);
      pdft->AddEntry (df);
    }

  NewRecord->SetDft (*pdft);
  delete pdft;
}


void REFERBIB::BeforeIndexing()
{
  DOCTYPE::BeforeIndexing();
}


void REFERBIB::AfterIndexing()
{
  Buffer.Free();
  tagBuffer.Free();
  DOCTYPE::AfterIndexing();
}

void REFERBIB::
DocPresent (const RESULT& ResultRecord,
	    const STRING& ElementSet, const STRING & RecordSyntax,
	    PSTRING StringBuffer) const
{
  if (ElementSet.Equals (SOURCE_MAGIC))
    {
      DOCTYPE::DocPresent (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
      return;
    }

  STRING Value;
  bool UseHtml = (RecordSyntax == HtmlRecordSyntax);
  if (UseHtml)
    {
      HtmlHead (ResultRecord, ElementSet, StringBuffer);
      *StringBuffer << "<DL COMPACT=\"COMPACT\">\n";
    }

  if (ElementSet.Equals (FULLTEXT_MAGIC))
    {
      DFDT Dfdt;
      STRING Key;
      ResultRecord.GetKey(&Key);
      Db->GetRecordDfdt (Key, &Dfdt);
      INT Total = Dfdt.GetTotalEntries();
      DFD Dfd;
      STRING FieldName;
      // Medline class documents are 1-deep so just collect the fields
      for (INT i = 1; i <= Total; i++)
	{
	  Dfdt.GetEntry(i, &Dfd);
	  Dfd.GetFieldName (&FieldName);
	  Present (ResultRecord, FieldName, RecordSyntax, &Value);
	  if (Value != "")
	    {
	      if (UseHtml)
		*StringBuffer << "<DT>";
#if 1
	      FieldName.ToLower();
	      *StringBuffer << (UCHR)toupper(FieldName.GetChr(1))
		<< FieldName.SubString(2, 0) << ": ";
#else
	      *StringBuffer << FieldName << ": ";
#endif
	      if (UseHtml)
		*StringBuffer << "<DD>";
	      *StringBuffer << Value << "\n";
	    }
	}			/* for */
    }
  else
    {
      if (UseHtml)
	*StringBuffer << "<DT>";
      *StringBuffer << ElementSet << ": ";
      if (UseHtml)
	*StringBuffer << "<DD>";
      Present (ResultRecord, ElementSet, RecordSyntax, &Value);
      *StringBuffer << Value << "\n";
    }

  if (UseHtml)
    {
      StringBuffer->Cat ("</DL>");
      HtmlTail (ResultRecord, ElementSet, StringBuffer); // Tail bits
    }
}

void REFERBIB::Present (const RESULT& ResultRecord,
	 const STRING& ElementSet, const STRING& RecordSyntax,
	 PSTRING StringBuffer) const
{
  STRING Tmp;
  if (ElementSet.Equals(BRIEF_MAGIC))
    {
      // Refer Headline is Title, Author[, Date]

      // Get a Title
      STRING Value;
      Present (ResultRecord,
	UnifiedName("%T", &Tmp), RecordSyntax, &Value);
      if (Value.GetLength() == 0)
	{
	  Present (ResultRecord,
	    UnifiedName("%B", &Tmp), RecordSyntax, &Value);
	  if (Value.GetLength() == 0)
	    {
	      // Journal Number
	      Present (ResultRecord,
		UnifiedName("%J", &Tmp), RecordSyntax, &Value);
	      if (Value.GetLength())
		{
		  STRING S;
		  Present (ResultRecord,
			UnifiedName("%V", &Tmp), RecordSyntax, &S);
		  if (S.GetLength())
		    {
		      Value << " " << S;
		      Present (ResultRecord,
			UnifiedName("%N", &Tmp), RecordSyntax, &S);
		      if (S.GetLength())
			Value << " " << S;
		    }
		}
	    }
	}
      *StringBuffer = Value;

      // Get an author
      Present (ResultRecord,
	UnifiedName("%A", &Tmp), SutrsRecordSyntax, &Value);
      if (Value.GetLength() == 0)
	{
	  Present (ResultRecord,
	    UnifiedName("%E", &Tmp), SutrsRecordSyntax, &Value);
	}

      if (Value.GetLength())
	{
	  if (StringBuffer->GetLength())
	    *StringBuffer << ", ";
	  HtmlCat(Value, StringBuffer);
	}
      // Get the date
      Present (ResultRecord,
	UnifiedName("%D", &Tmp), RecordSyntax, &Value);
      if (Value.GetLength())
	{
	  if (StringBuffer->GetLength())
	    *StringBuffer << ", ";
	  *StringBuffer << Value;
	}
      StringBuffer->Pack();
    }
  else
    {
      bool UseHtml = (RecordSyntax == HtmlRecordSyntax);
      // Get Value of the field, use parent
      if (UseHtml && (ElementSet ^= UnifiedName("%G", &Tmp)))
	DOCTYPE::Present (ResultRecord, ElementSet, SutrsRecordSyntax, StringBuffer);
      else
	DOCTYPE::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
      if (UseHtml)
	{
	  if (ElementSet ^= UnifiedName("%*", &Tmp))
	    {
	      // copyright
	      StringBuffer->Replace("(c)", "&copy;");
	    }
	  StringBuffer->Replace("\n%br", "<P>");
	  if (ElementSet ^= UnifiedName("%X", &Tmp))
	    {
	      StringBuffer->Replace("\n  ", "<P>");
	      StringBuffer->Replace("\n", "<BR>");
	    }
	}
      else
	{
	  StringBuffer->Replace("\n%br", "\n");
	}
    }
}

REFERBIB::~REFERBIB ()
{
}

/*-
   What:        Given a buffer of Refer data:
   returns a list of char* to all characters pointing to the TAG

   Refer Records:
%X ...
.....
%Y ...
%Z ...
...
....

A refer tag is % followed by a letter followed by a space.
Tags such as %br are NOT really tags but placeholders to
represent a blank line (break).

Fields are continued when the line has no tag

-*/
PCHR * REFERBIB::parse_tags (PCHR b, off_t len)
{
  PCHR *t;			// array of pointers to first char of tags
  size_t tc = 0;		// tag count
#define TAG_GROW_SIZE 32
  size_t max_num_tags = TAG_GROW_SIZE;	// max num tags for which space is allocated
  enum { HUNTING, STARTED, CONTINUING, LOOK_BIB} State = HUNTING;

  /* You should allocate these as you need them, but for now... */
  max_num_tags = TAG_GROW_SIZE;
  t = (PCHR *)tagBuffer.Want (max_num_tags, sizeof(PCHR));

  for (off_t i = 0; i < len - 1; i++)
    {
      if (b[i] == '\r' || b[i] == '\v')
 	continue; // Skip over
      else if (State == HUNTING
	&& b[i] == '%'
	&& (isalpha(b[i+1]) || b[i+1]=='*' || b[i+1]=='^' || b[i+1]=='$')
	&& isspace(b[i+2]) )
	{
	  t[tc] = &b[i]; 
	  State = STARTED;
	}
#if IGNORE_LOOKBIB_RECORDS
      else if (State = HUNTING && b[i] == '.' && b[i+1] == '[')
	{
	  // Lookbib stuff
	  State = LOOK_BIB; // Now in a lookbib(1) record
	}
      else if (State = LOOK_BIB && b[i] == '.' && b[i+1] == ']')
	{
	  // End Lookbib
	  State = HUNTING; // back to business
	} 
#endif
      else if (State == STARTED && isspace(b[i]))
	{
	  b[i] = '\0';
	  // Expand memory if needed
	  t = (PCHR *)tagBuffer.Expand(++tc, sizeof(PCHR));
	  State = CONTINUING;
	}
      else if ((State == CONTINUING) && (b[i] == '\n'))
	{
	  State = HUNTING;
	}
      else if (State == HUNTING)
	State = CONTINUING;
    }
  t[tc] = (PCHR) NULL;
  return t;
}


void REFERBIB_ENDNOTE::SourceMIMEContent(PSTRING StringBuffer) const
{
  *StringBuffer = "Application/X-EndNote-Refer";
}

void REFERBIB_PAPYRUS::SourceMIMEContent(PSTRING StringBuffer) const
{
  *StringBuffer = "Application/X-Papyrus-Refer";
}

