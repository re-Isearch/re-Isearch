#pragma ident  "@(#)medline.cxx	1.30 05/08/01 21:49:13 BSN"
/************************************************************************
************************************************************************/


/*-@@@
File:		medline.cxx
Version:	$Revision: 1.1 $
Description:	Class MEDLINE - Medline Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

// TODO: Clean-up Record parser and fix to leave off junk between records

//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <errno.h>
//#include <ctype.h>
//#include <sys/stat.h>
#include "medline.hxx"
#include "doc_conf.hxx"
#include "common.hxx"

#ifndef NO_MMAP
#include "mmap.hxx"
#endif

const char *MEDLINE::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("MEDLINE");
  if (Doctype != ThisDoctype && List->IsEmpty())
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);
  DOCTYPE::Description(List);
  return "Medline Document Type";
}

/* A few user-customizable parser features... */
#ifndef USE_UNIFIED_NAMES
# define USE_UNIFIED_NAMES 1 
#endif

MEDLINE::MEDLINE (PIDBOBJ DbParent, const STRING& Doc): DOCTYPE (DbParent, Doc)
{
 if (DateField.IsEmpty())
    DateField = "DP";
}

void MEDLINE::SourceMIMEContent(PSTRING StringPtr) const
{
  // MIME/HTTP Content type for Medline Sources 
  *StringPtr = "Application/X-Medline"; 
}

void MEDLINE::AddFieldDefs ()
{
  DOCTYPE::AddFieldDefs ();
}


void MEDLINE::BeforeIndexing()
{

}


void MEDLINE::AfterIndexing()
{
  recBuffer.Free();
  tmpBuffer.Free();
  tagsBuffer.Free();
  subtagsBuffer.Free();
  DOCTYPE::AfterIndexing();
}


void MEDLINE::ParseRecords (const RECORD& FileRecord)
{
  // Break up the document into Medline records
  off_t Start = FileRecord.GetRecordStart();
  off_t End = FileRecord.GetRecordEnd();
  off_t Position = 0;
  off_t SavePosition = 0;
  off_t RecordEnd;

  const STRING Fn = FileRecord.GetFullFileName ();
#ifdef NO_MMAP
  PFILE Fp = Db->ffopen (Fn, "rb");
  if (!Fp)
    {
      logf (LOG_ERRNO, "Could not access %s",
	(const char *)Fn );
      return;			// File not accessed

    }
#endif

  RECORD Record (FileRecord); // Easy way

  // Search for Medline Seperator
  enum { LOOK, HUNT, FOUND, START } State = HUNT;
  int Ch;
  UCHR buf[7];
  size_t pos = 0;

#ifndef NO_MMAP
  MMAP mapping (Fn, Start, End, MapSequential);
  if (!mapping.Ok())
    {
       logf(LOG_FATAL|LOG_ERRNO, "Couldn't map '%s' into memory",
		(const char *)Fn);
       return;
    }
  const UCHR *Buffer  = (const UCHR *)mapping.Ptr();
  const size_t MemSize = mapping.Size();
#else

  if (End > 0 && End - Start <= 0)
    {
      logf (LOG_WARN, "zero-length record - '%s' [%ld-%ld]",
	(const char *)Fn, (long)Start, (long)End);
      Db->ffclose (Fp);
      return;
    }

  // Move to start if defined
  if (Start > 0)  fseek(Fp, Start, SEEK_SET);
#endif

#ifndef NO_MMAP
  for (size_t i=0; i<= MemSize && (Ch = Buffer[i]) != 0; i++)
#else
  while ( (Ch = getc (Fp)) != EOF)
#endif
    {
      Position++;
      if (End > 0 && Position > End) break; // End of Subrecord
      if (Ch == '\n')
	{
	  // New Line
	  if (State == LOOK)
	    {
	      if (pos < 5)
		State = FOUND;
	      else if (!isspace (buf[0]) && !isalpha (buf[0]))
		State = FOUND;	// not space and not letter
	    }
	  else
	    State = LOOK;
	  if (State == FOUND)
	    SavePosition = Position - pos - 1;
	  else
	    SavePosition = Position;
	  pos = 0;
	}
      else
	{
	  // A token
	  if (pos < 6)
	    {
	      buf[pos] = (UCHR)Ch;
	      // Do we have the start of a record with a new field?
	      if (State == FOUND
		  && pos > 4 
		  && (buf[4] == '-' || buf[4] == ' ' || buf[3] == ' ')
//		  && (buf[4] == '-' || buf[4] == ' ')

		  && isalpha (buf[0])
		  && isalnum (buf[1])
		  && (buf[2] == ' ' || isalnum (buf[2]))
		  && (buf[3] == ' ' || isalnum (buf[3]))
		  && (buf[2] != ' ' || buf[3] == ' ' ) )
		{
		  State = START;
		}
	    }
	  pos++;		// Line Position;

	}

      if (State == START)
	{
	  State = HUNT;
	  Record.SetRecordStart (Start);
	  RecordEnd = (SavePosition == 0) ? 0 : SavePosition - 1;

	  if (RecordEnd > Start)
	    {
	      Record.SetRecordEnd (RecordEnd);
	      Db->DocTypeAddRecord(Record);
	      Start = SavePosition;
	    }
	}
    }				// while
#ifdef NO_MMAP
  Db->ffclose (Fp);
#endif

  Record.SetRecordStart (Start);
  RecordEnd = (SavePosition == 0) ? 0 : Position - 1;

  if (RecordEnd > Start)
    {
      Record.SetRecordEnd (RecordEnd);
      Db->DocTypeAddRecord(Record);
    }
}

INT MEDLINE::UnifiedNames (const TagTable_t *Table, const size_t Elements,
	const STRING& Tag, PSTRLIST Value) const
{
  INT n;

  // Go via the registry..
  if ((n = DOCTYPE::UnifiedNames (Tag, Value, GDT_FALSE)) != 0)
    {
      return n;
    }

  for (size_t i=0; i < Elements; i++)
    {
      if ((n = Tag.Compare(Table[i].key)) == 0)
	{
	  Value->Split(",", (PCHR)Table[i].name); // Return "our" unified name
	  return Value->GetTotalEntries();
	}
    }
  // Not in list
#if WANT_MISC
  Value->AddEntry("Other_fields");
  return 1;
#else
  return 0;
#endif
}

INT MEDLINE::UnifiedNames (const STRING& Tag, PSTRLIST Value) const
{
  static const TagTable_t Table[] = {
  /* SORTED LIST */
  /* NOTE: No white space and only Alphanumeric and - and . are allowed. */
  {"AA",  "Author-Address"},
  {"AB",  "Abstract"},
  {"AD",  "author-address"},
  {"AN",  "Accession-Number"},
  {"AR",  "Access-Restrictions"},
  {"AT",  "Article-Title"},
  {"AU"   "Author"},
  {"CA",  "Corporate-Author"},
  {"CD",  "Coden"},
  {"CI",  "Column-inches"},
  {"CL",  "Column-number"},
  {"CP",  "Captions"},
  {"DP",  "Date-of-publication"},
  {"DT",  "Document-Type"},
  {"DY",  "Day-of-the-week"},
  {"ED",  "Edition"},
  {"FR",  "Frequency"},
  {"GC",  "Geographic-Code"},
  {"GD",  "Government-Document-Num"},
  {"GI",  "GPO-Item-Num"},
  {"IB",  "ISBN"},
  {"IL",  "Illustrations"},
  {"IP",  "Issue-part"},
  {"IS",  "Issue"},
  {"JA",  "Jo.Author"},
  {"JO",  "Journal"},
  {"JT",  "Item-Title"},
  {"LA",  "Language"},
  {"LC",  "LCCN"},
  {"LO",  "Location"},
  {"LT",  "Linked-PE"},
  {"MH",  "Keywords"},
  {"MK",  "Musical-Key"},
  {"MN",  "Music-Publisher-Num"},
  {"MO",  "Month"},
  {"MT",  "Title-Main"},
  {"NR",  "issue-num"},
  {"NT",  "Notes"},
  {"OE",  "Other-Entries"},
  {"OR",  "Organization"},
  {"OT",  "Other-Titles"},
  {"OW",  "Ownership"},
  {"PA",  "Personal-author"},
  {"PG",  "Pagination"},
  {"PH",  "History"},
  {"PL",  "Place"},
  {"PN",  "Producer-Num"},
  {"PU",  "Publisher"},
  {"RE",  "Reprint"},
  {"RF",  "Reference"},
  {"RN",  "Report-Numb"},
  {"RT",  "Related-Titles"},
  {"SB",  "Subfile"},
  {"SD",  "Std-Ind-code"},
  {"SE",  "Series"},
  {"SL",  "Scale"},
  {"SO",  "Source"},
  {"SR",  "Series-added"},
  {"SS",  "ISSN"},
  {"ST",  "Stock-Num"},
  {"SU",  "Subject"},
  {"TA",  "Title-Abbrev"},
  {"TD",  "Technical-Details"},
  {"TH",  "Thematic"},
  {"TI",  "Title"},
  {"UI",  "Journal-code"},
  {"UT",  "Uniform-Title"},
  {"UR",  "URL"},
  {"VN",  "Vendor-Num"},
  {"VO",  "Volume"},
  {"VO",  "issue-vol"},
  {"YE",  "Year"},
  {"XX",  "Message"},
  {"ZZ",  "End-of-Record"}
 };

  return UnifiedNames (Table, sizeof(Table)/sizeof(TagTable_t), Tag, Value);
}

STRING& MEDLINE::DescriptiveName(const STRING& Language, const STRING& FieldName, PSTRING Value) const
{
  return DOCTYPE::DescriptiveName(Language, FieldName, Value);
}

STRING& MEDLINE::DescriptiveName (const TagTable_t *Table, const size_t Elements,
	const STRING& FieldName, PSTRING Value) const
{
  STRING Tmp;
  *Value = FieldName;
  for (size_t i=0; i < Elements; i++)
    {
      if (Value->CaseEquals (UnifiedName((PCHR)Table[i].key, &Tmp)) ||
	Value->CaseEquals(Table[i].key))
	{
	  return *Value = Table[i].name;
	}
    }
  // Lowercase
  Value->ToLower();
  // Make first Character UpperCase
  CHR first_char[2] = {toupper ( Value->GetChr(1) ), 0};
  Value->EraseBefore(2);
  Value->Insert(1, first_char);
  // Replace '-' or '_' to ' '
  Value->Replace("-", " ");
  Value->Replace("_", " ");
  Value->Replace(".", " ");
  return *Value;
}


STRING& MEDLINE::DescriptiveName(const STRING& FieldName, PSTRING Value) const
{
  static const TagTable_t Table[] = {
  /* SORTED LIST */
  {"AA",  "Author Address"},
  {"AB",  "Abstract"},
  {"AD",  "Author Address"},
  {"AN",  "Accession Number"},
  {"AR",  "Access Restrictions"},
  {"AT",  "Article Title"},
  {"AU"   "Author"},
  {"CA",  "Corporate Author"},
  {"CD",  "Coden"},
  {"CI",  "Column inches"},
  {"CL",  "Column number"},
  {"CP",  "Captions"},
  {"DP",  "Date of publication"},
  {"DT",  "Document Type"},
  {"DY",  "Day of the week"},
  {"ED",  "Edition"},
  {"FR",  "Frequency"},
  {"GC",  "Geographic Code/Class"},
  {"GD",  "Government Document No."},
  {"GI",  "GPO Item No."},
  {"IB",  "ISBN"},
  {"IL",  "Illustrations"},
  {"IP",  "Issue part/section"},
  {"IS",  "Issue"},
  {"JA",  "Jo. Author/Main work"},
  {"JO",  "Journal"},
  {"JT",  "Journal/Host item Title"},
  {"LA",  "Language"},
  {"LC",  "LCCN"},
  {"LO",  "Location (call no.)"},
  {"LT",  "Linked PE title"},
  {"MH",  "Keywords"},
  {"MK",  "Musical Key"},
  {"MN",  "Music Publisher No."},
  {"MO",  "Month"},
  {"MT",  "Main Title"},
  {"NR",  "Number of issue"},
  {"NT",  "Notes"},
  {"OE",  "Other Entries"},
  {"OR",  "Organization"},
  {"OT",  "Other Titles"},
  {"OW",  "Ownership"},
  {"PA",  "Personal author"},
  {"PG",  "Pagination/Description"},
  {"PH",  "Publishing History"},
  {"PL",  "Place of Publication"},
  {"PN",  "Producer Number"},
  {"PU",  "Publisher"},
  {"RE",  "Reprint"},
  {"RF",  "Reference"},
  {"RN",  "Report Number"},
  {"RT",  "Related Titles"},
  {"SB",  "Subfile/subset"},
  {"SD",  "Standard Industrial code"},
  {"SE",  "Series"},
  {"SL",  "Scale (of map)"},
  {"SO",  "Source"},
  {"SR",  "Series added entries"},
  {"SS",  "ISSN"},
  {"ST",  "Stock Number"},
  {"SU",  "Subject(s)"},
  {"TA",  "Title Abbreviation"},
  {"TD",  "Technical Details"},
  {"TH",  "Thematic"},
  {"TI",  "Title"},
  {"UI",  "Journal Code"},
  {"UT",  "Uniform Title"},
  {"VN",  "Vendor Number"},
  {"VO",  "Volume"},
  {"VO",  "Volume of issue"},
  {"YE",  "Year"},
  {"XX",  "System message"},
  {"ZZ",  "End of record"},
};
 return DescriptiveName(Table, sizeof(Table)/sizeof(TagTable_t), FieldName, Value);
}


void MEDLINE::ParseFields (PRECORD NewRecord)
{
  const STRING fn = NewRecord->GetFullFileName ();
  PFILE fp = Db->ffopen(fn, "rb");
  if (fp == NULL) return;	// ERROR

  off_t RecStart = NewRecord->GetRecordStart ();
  off_t RecEnd = NewRecord->GetRecordEnd ();
  if (RecEnd == 0)
    RecEnd = GetFileSize(fp); // End of File
  if (RecEnd <= RecStart)
    {
      Db->ffclose(fp);
      return;
    }
  if (-1 == fseek (fp, RecStart, SEEK_SET))
    logf(LOG_ERRNO, "Couldn't seek on \"%s\"", (const char *)fn);
  off_t RecLength = RecEnd - RecStart + 1;
  PCHR RecBuffer = (PCHR)recBuffer.Want (RecLength + 1);
  off_t ActualLength = fread (RecBuffer, 1, RecLength, fp);
  Db->ffclose (fp);
  RecBuffer[ActualLength] = '\0';

  PCHR *tags = parse_tags (RecBuffer, ActualLength);
  if (tags == NULL || tags[0] == NULL)
    {
      if (tags)
	{
	  logf (LOG_WARN, "Warning: No `%s' fields/tags in \"%s\" record.",
		(const char *)Doctype, (const char *)fn);
	}
       else
	{
	  logf (LOG_ERROR, "Unable to parse `%s' record in \"%s\".",
		(const char *)Doctype, (const char *)fn);
	}

      NewRecord->SetBadRecord();
      return;
    }

  FC fc;
  DF df;
  PDFT pdft;
  pdft = new DFT ();
  DFD dfd;
  STRLIST FieldNames;
  STRING FieldName;

  // Walk though tags
  for (PCHR * tags_ptr = tags; *tags_ptr; tags_ptr++)
    {
      PCHR p = tags_ptr[1];
      if (p == NULL)
	p = &RecBuffer[RecLength];
      // eg "AB  -" XXXXX
      int off = 4; // was 5 Medline format constant
      INT val_start = (*tags_ptr + off) - RecBuffer;
      // Support Medline
      if (RecBuffer[val_start] == '-')
	val_start++, off++;
      // Skip ' 's 
      while (isspace((UCHR)RecBuffer[val_start]))
	val_start++, off++;
      // Also leave off the \n
      INT val_len = (p - *tags_ptr) - off - 1;
      // Strip potential trailing while space
      while (val_len > 0 && isspace ((UCHR)RecBuffer[val_len + val_start]))
	val_len--;

      // Do we have some content?
      if (val_len <= 0) continue; // Skip blank fields

#if BSN_EXTENSIONS
      if (DateField == *tags_ptr)
	{
	  NewRecord->SetDate( &RecBuffer[val_start] );
	} else
#endif
      if (KeyField == *tags_ptr)
	{
	  int k;
	  STRING basekey;
	  if (1 == sscanf(&RecBuffer[val_start], "%d", &k))
	    {
	      basekey = k; // Numeric key
	    }
	  else // Alphanumeric
	    {
	      PCHR entry_id = (PCHR)tmpBuffer.Want(val_len + 2);
	      strncpy (entry_id, &RecBuffer[val_start], val_len + 1);
	      entry_id[val_len + 1] = '\0';
	      basekey = entry_id;
	    }
	  STRING Key;
	  for(int i=0; i < 655; i++)
	    {
	      Key = basekey;
	      if (i) Key << "." << (INT)i;
	      if (! Db-> MdtLookupKey ( Key ))
		{
		  NewRecord->SetKey (Key);
		  break;
		}
	    }
	}
/*
Need to handle Sources:
SO  - Adv Exp Med Biol 1997;412:349-55
SO  - Acta Neuropathol 1992 AUG;84(3):225-233
SO  - J Immunol 1997 Sep 15;159(6):2563-6
   JO --> J Immunol
   YE --> 1997
   MO --> Sep
   DA(?) --> 15
   VO --> 159
   IS --> 6
   PG --> 2563-6
*/
#if 1
// Parse Source Field
if (strcmp(*tags_ptr, "SO" ) == 0) {
  FC pg, is, vo, dp, jo;

  // point at tail
  char *ptr = &RecBuffer[val_start + val_len];
  if (isdigit(*ptr))
    {
      int error = 0;
      char *old_ptr = ptr;
      while (ptr > &RecBuffer[val_start] &&
		(isdigit(*ptr) || *ptr == '-' || *ptr == ','))
	ptr--;
      if (*ptr == ':')
	{
	  pg.SetFieldStart( ptr - RecBuffer + 1 );
	  pg.SetFieldEnd ( old_ptr - RecBuffer );
	  old_ptr = --ptr;
	  if (*ptr == ')')
	    {
	      while (ptr > &RecBuffer[val_start] && *ptr != '(')
		ptr--;
	      if (*ptr != '(' || (old_ptr - ptr) == 1)
		error++;
	      else
		{
		  is.SetFieldStart ( ptr - RecBuffer + 1 );
		  is.SetFieldEnd ( old_ptr - RecBuffer - 1 );
		  old_ptr = --ptr;
		}
	    }
	  if (*ptr != ';')
	    {
	      while (ptr > &RecBuffer[val_start] && *ptr != ';')
		ptr--;
	      if (*ptr != ';')
		error++;
	      vo.SetFieldStart ( ptr - RecBuffer + 1 );
	      vo.SetFieldEnd ( old_ptr - RecBuffer );
	      old_ptr = --ptr;
	    }
	  if (isdigit(*ptr))
	    {
	      // Looking at optional day of month
	      // Or year (then no 3 letter month)
	      while (ptr > &RecBuffer[val_start] && !isspace(*ptr))
		ptr--;
	      if (!isspace(*ptr))
		error++;
	      else
		{
		  if (old_ptr - ptr > 3)
		    {
		      // Was year not day
		      ptr++; // point to digit
		    }
		  // Skip white space
		  while (ptr > &RecBuffer[val_start] && isspace(*ptr))
		    ptr--;
		}
	    }
	  if (isalpha(*ptr))
	    {
	      // Looking at 3 letter month
	      if ((ptr - &RecBuffer[val_start]) < 5 || !isspace(ptr[-3]))
		error++;
	      else
		ptr -= 4;
	    }
	  if (isdigit(*ptr))
	    {
	      // Looking at year
	      while (ptr > RecBuffer && isdigit(*ptr))
		ptr--;
	      if (!isspace(*ptr))
		error++;
	    }
	  if (isspace(*ptr))
	    {
	      // Looking at date, eg. 1999 MAY 15 
	      dp.SetFieldStart ( ptr - RecBuffer + 1 );
	      dp.SetFieldEnd ( old_ptr - RecBuffer );
	      while (ptr > RecBuffer && isspace(*ptr))
		ptr--;
	    }
	  if (ptr > &RecBuffer[val_start])
	    {
	      jo.SetFieldStart ( val_start );
	      jo.SetFieldEnd ( ptr - RecBuffer);
	    }
	  else
	    error++; // No journal listed
	}
      else
	error++;
      if (error)
	logf(LOG_WARN, "SO in '%s' not a Medline Source field",
		(const char *)fn);
      else
	{
	  STRING S;
	  for (int q=0;;q++)
	    {
	      if      (q == 0) fc = jo, S = "JO";
	      else if (q == 1) fc = dp, S = "DP";
	      else if (q == 2) fc = vo, S = "VO";
	      else if (q == 3) fc = is, S = "IS";
	      else if (q == 4) fc = pg, S = "PG";
	      else break; // no more
	      if (0 < fc)
		{
#if BSN_EXTENSIONS
		  if (DateField == S)
		    {
		      char *tp = &RecBuffer[fc.GetFieldStart()];
		      tp[fc.GetFieldEnd() - fc.GetFieldStart() + 1] = '\0';
		      NewRecord->SetDate( tp );
		    }
#endif
		  if (UnifiedName (S, &FieldName).GetLength())
		    {
		      dfd.SetFieldName (FieldName);
		      Db->DfdtAddEntry (dfd);
		      df.SetFct (fc);
		      df.SetFieldName (FieldName);
		      pdft->AddEntry (df);
		    }
		}
	    } // For kludge
	}
    }
}
#endif

      const INT Total = UnifiedNames(*tags_ptr, &FieldNames);
      if (Total)
	{
	  fc.SetFieldStart (val_start);
	  fc.SetFieldEnd (val_start + val_len);
	  // Now Walk through list (backwards)..
	  df.SetFct(fc);
	  for (INT i=1; i<=Total; i++)
	    {
	      FieldNames.GetEntry(i, &FieldName);
              dfd.SetFieldType( Db->GetFieldType(FieldName) ); // Get the type 30 Sept 2003
	      dfd.SetFieldName (FieldName);
	      Db->DfdtAddEntry (dfd);
	      df.SetFieldName (FieldName);
	      pdft->AddEntry (df);
	   }
	}
      else continue; // Skip this field
#if 1
      // This is where one would hook the subtag parser in..
    PCHR *subtags = parse_subtags(&RecBuffer[val_start], val_len);
    if (subtags)
      {
	for (PCHR *tagsptr = subtags; *tagsptr; tagsptr++)
	  {
	    PCHR p = tagsptr[1];	// end of field
	    if (p == NULL)		// If no end of field
	      p = &RecBuffer[val_start + val_len + 1 ];	// use end of buffer

	    size_t off = strlen (*tagsptr) + 1;
	    INT valstart = (*tagsptr + off) - RecBuffer;
	    while (isspace (RecBuffer[valstart]))
	      valstart++, off++;
	    // Also leave off the \n
	    INT vallen = (p - *tagsptr) - off - 1;
	    // Strip potential trailing while space
	    while (vallen >= 0 && isspace (RecBuffer[vallen + valstart]))
	      vallen--;
	    if (vallen < 0)
	      continue;		// Don't bother with empty fields
	    if (strcasecmp(*tagsptr, *tags_ptr) == 0)
	      continue;		// Don't do embeded..
	    if (UnifiedName (*tagsptr, &FieldName).GetLength() == 0)
	      continue;		// ignore these
	    dfd.SetFieldName (FieldName);
	    Db->DfdtAddEntry (dfd);
	    fc.SetFieldStart (valstart);
	    fc.SetFieldEnd (valstart + vallen);
	    df.SetFct (fc);
	    df.SetFieldName (FieldName);
	    pdft->AddEntry (df);
	}
    }
#endif
    }

  NewRecord->SetDft (*pdft);

  // Clean up
  delete pdft;
}


/* WARNING: Code has hooks into virtual functions! */
void MEDLINE::
DocPresent (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const
{
  if (ElementSet.Equals(SOURCE_MAGIC))
   {
      // Call "virtual" presentation
      Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
      return;
   }

  STRING Value;
  GDT_BOOLEAN UseHtml =(RecordSyntax == HtmlRecordSyntax);
  if (UseHtml)
    {
      HtmlHead (ResultRecord, ElementSet, StringBuffer);
      *StringBuffer << "<DL COMPACT=\"COMPACT\">\n";
    }

  STRING Language ( ResultRecord.GetLanguageCode() );

  if (ElementSet.Equals (FULLTEXT_MAGIC))
    {
      DFDT Dfdt;
      STRING Key;
      ResultRecord.GetKey (&Key);
      Db->GetRecordDfdt (Key, &Dfdt);
      const size_t Total = Dfdt.GetTotalEntries();
      STRING FieldName, LongName;
      DFD Dfd;
      // Medline class documents are 1-deep so just collect the fields
      for (INT i = 1; i <= Total; i++)
	{
	  Dfdt.GetEntry (i, &Dfd);
	  Dfd.GetFieldName (&FieldName);

	  DescriptiveName(Language, FieldName, &LongName);
	  // Is it a published field?
	  if (LongName.GetChr(1) != ' ') {
	    // Get Value of the field, use parent
	    Present (ResultRecord, FieldName, RecordSyntax, &Value); // @@@
//	    Value.Pack ();
	    if (!Value.IsEmpty())
	      {
	        if (UseHtml) StringBuffer->Cat ("<DT>");
		if (UseHtml) HtmlCat(LongName, StringBuffer);
		else         StringBuffer->Cat(LongName);
	        *StringBuffer << ": ";
	        if (UseHtml) StringBuffer->Cat ("<DD>");
	        *StringBuffer << Value << "\n";
	      }
	  }
	} /* for */
    }
  else 
    {
      DescriptiveName(Language, ElementSet, &Value);
      if (UseHtml) StringBuffer->Cat("<DT>");
      if (UseHtml) HtmlCat(Value, StringBuffer);
      else         StringBuffer->Cat(Value);
      StringBuffer->Cat(": ");
      if (UseHtml) StringBuffer->Cat("<DD>");
      Present(ResultRecord, ElementSet, RecordSyntax, &Value);
      StringBuffer->Cat(Value);
      StringBuffer->Cat("\n");
    }

  if (UseHtml)
    {
      StringBuffer->Cat ("</DL>");
      HtmlTail (ResultRecord, ElementSet, StringBuffer); // Tail bits
    }
}

void MEDLINE::
Present (const RESULT& ResultRecord,
	 const STRING& ElementSet, const STRING& RecordSyntax,
	 PSTRING StringBuffer) const
{
  STRING Tmp;
  StringBuffer->Clear();
  if (ElementSet.Equals (SOURCE_MAGIC))
    {
      DOCTYPE::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
    }
  else if (ElementSet.Equals (BRIEF_MAGIC))
    {
      // Brief Headline is: "Title" (Source) Author
      STRING Title;
      DOCTYPE::Present (ResultRecord,
	UnifiedName ("TI", &Tmp), &Title);
      Title.Pack ();

      STRING Source;
      DOCTYPE::Present (ResultRecord,
	UnifiedName ("SO", &Tmp), &Source);
      Source.Pack ();

      STRING Author;
      DOCTYPE::Present (ResultRecord,
	UnifiedName ("AU", &Tmp), &Author);
      Author.Pack ();

      STRING Headline;

      if (Title.GetLength())
	{
	  Headline << "\"" << Title << "\" ";
	}
      if (Source.GetLength ())
	{
	  Headline << "(" << Source << ") ";
	}
      if (Author.GetLength())
	{
	  Headline << Author;
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
  else if (ElementSet.Equals (FULLTEXT_MAGIC))
    {
      DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
    }
  else
    {
      // Get the value of the field, use parent
      DOCTYPE::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
      if (StringBuffer->GetLength() == 0) 
	{
	  if (!(ElementSet.CaseEquals( UnifiedName (ElementSet, &Tmp) )))
	    {
	      DOCTYPE::Present(ResultRecord, Tmp, RecordSyntax, StringBuffer);
	    }
	}
    }
  if (StringBuffer->Len() && !(ElementSet.Equals (FULLTEXT_MAGIC) || ElementSet.Equals (SOURCE_MAGIC)))
    StringBuffer->Pack ();
}


MEDLINE::~MEDLINE ()
{
}


/*-
   What:        Given a buffer of MEDLINE data:
   returns a list of char* to all characters pointing to the TAG

   Medline Records:
A<tok><tok><tok><sep> ...
| |                 |_____________ Value of field
| |___ A - Z, 0 - 9 or space
|____ A - Z

Tags are left aligned and can be 2, 3 or 4 characters long!

Fields are continued when the line has no tag. A single space
is sufficient in this implementation to guarantee a continuation.

The <sep> (' ' or '-') character is MANDATORY!
-*/
PCHR * MEDLINE::parse_tags (PCHR b, off_t len)
{
  off_t i;
  PCHR *t;			// array of pointers to first char of tags

  if (b == NULL || *b == '\0') return NULL;

  size_t tc = 0;		// tag count
#define TAG_GROW_SIZE 64
  size_t max_num_tags = TAG_GROW_SIZE;	// max num tags for which space is allocated

  enum { HUNTING, STARTED, CONTINUING } State = HUNTING;

  /* You should allocate these as you need them, but for now... */
  max_num_tags = TAG_GROW_SIZE;
  t = (PCHR *)tagsBuffer.Want (max_num_tags, sizeof(PCHR));

  // Skip leading White space and sep number
  for (i = 0; i < len - 4; i++)
    if (!isspace(b[i]) && !isdigit(b[i])) break;

  while (i < len - 4)
    {
      // Do we have a field?
      if ((State == HUNTING)
	  && (b[i + 4] == '-' || b[i + 4] == ' ' || b[i+3] == ' ')
//	  && (b[i + 4] == '-' || b[i + 4] == ' ')
	  && isalpha (b[i])
	  && isalnum (b[i + 1])
	  && (isalnum (b[i + 2]) || b[i + 2] == ' ')
	  && (isalnum (b[i + 3]) || b[i + 3] == ' ')
	  && (b[i + 2] != ' ' || b[i + 3] == ' ') )
	{

	  t[tc] = &b[i];
	  State = STARTED;
	  // Expand if needed
	   t = (PCHR *)tagsBuffer.Expand(++tc, sizeof(PCHR));
	}
      else if ((State == STARTED) && (b[i] == ' ' || b[i] == '-'))
	{
	  b[i] = '\0';
	  State = CONTINUING;
	}
      else if ((State == CONTINUING) && (b[i] == '\n'))
	{
	  State = HUNTING;
	}
      else if (State == HUNTING)
	State = CONTINUING;
      i++; // increment
    }
  t[tc] = (PCHR) NULL;
  return t;
}

PCHR *MEDLINE::parse_subtags (PCHR, off_t)
{
  return NULL; // No subtags
}

/*
// ISI - Common Export Format 
void ISI_CIW::SourceMIMEContent(PSTRING StringBuffer) const
{
  *StringBuffer = "Application/x-Inst-for-Scientific-InfoFile";
}

const char *ISI_CIW::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("CURRENT-CITES");
  if (Doctype != ThisDoctype && List->IsEmpty())
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);
  MEDLINE::Description(List);
  return "ISI Common Export Format used by Current Citations and other applications";
}

*/

void MEDLINE_RIS::SourceMIMEContent(PSTRING StringBuffer) const
{
  *StringBuffer = "Application/X-Research-Info-Systems";
}


