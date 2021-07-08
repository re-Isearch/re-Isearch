#pragma ident  "@(#)colondoc.cxx	1.34 04/20/01 14:23:50 BSN"
/************************************************************************
************************************************************************/

/*-@@@
File:		colondoc.cxx
Version:	$Revision: 1.2 $
Description:	Class METADOC - IAFA and other Colon Document Type
Author:		Edward C. Zimmermann, edz@bsn.com
@@@-*/

//#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <alloca.h>
#include "colondoc.hxx"
#include "common.hxx"
#include "doc_conf.hxx"

static const STRING TableKeyword ("Table");
static const STRING ListKeyword  ("List");

static const char AUTODETECT_FIELDTYPES[] = "AutodetectFieldtypes";

METADOC::METADOC (PIDBOBJ DbParent, const STRING& Name):
	DOCTYPE (DbParent, Name)
{
  STRING S = DOCTYPE::Getoption("AllowWhiteBeforeField");
  if (!S.IsEmpty())
    allowWhiteBeforeField = S.GetBool();
  else
    allowWhiteBeforeField = GDT_TRUE;
  SetPresentType(ListKeyword);

  autoFieldTypes  =  Getoption(AUTODETECT_FIELDTYPES, "Y").GetBool();
  recordsAdded = 0;

  IgnoreTagWords = Getoption("IgnoreTagWords", "True").GetBool();
//  SetSepChar(':');
}

FIELDTYPE METADOC::GuessFieldType(const STRING& FieldName, const STRING& Contents)
{
  const size_t fLen = FieldName.GetLength();

  if (Db && Contents.GetLength() && fLen > 0)
    {
      FIELDTYPE ft ( CheckFieldType(FieldName) );

      // We already have a type?
      if (ft.Defined())
	return ft;

      // We don't want to bother with fields longer than 1024 bytes!
      if (autoFieldTypes && fLen < 1024)
	{
	  if (Contents.IsGeoBoundedBox())      ft = FIELDTYPE::box;
	  else if (Contents.IsNumberRange())   ft = FIELDTYPE::numericalrange;
	  else if (Contents.IsNumber())
	    {
	      // CCYYMMDD
	      if (Contents.GetLong() > 19000101)
		{
		  // Number can also be date?
		  SRCH_DATE date (Contents);
		  if (date.Ok() && date.IsDayDate())
		    ft = FIELDTYPE::date;
		  else
		    ft = FIELDTYPE::numerical;
		}
	      else
		ft = FIELDTYPE::numerical;
            }
	  else if (Contents.IsDateRange())     ft = FIELDTYPE::daterange;
	  else if (Contents.IsDate())          ft = FIELDTYPE::date;
	  else if (Contents.IsCurrency())      ft = FIELDTYPE::currency;
	  else if (Contents.IsDotNumber())     ft = FIELDTYPE::dotnumber;

	  else ft = FIELDTYPE::text;
	  logf (LOG_INFO, "%s: Field '%s' autotyped as '%s'", Doctype.c_str(), FieldName.c_str(), ft.c_str());
	}
      else ft = FIELDTYPE::text;
      Db->AddFieldType(FieldName, ft);
      return ft;
    }
  return FIELDTYPE::text;
}


void METADOC::SetPresentType(const STRING& Default)
{
  STRING S = DOCTYPE::Getoption("Style", Default);
  if (S ^= ListKeyword)
    UseTable = GDT_FALSE;
  else if (S ^= TableKeyword)
    UseTable = GDT_TRUE;
  // else default style
  else
    UseTable = GDT_FALSE;
}


void METADOC::SetSepChar(const STRING& Default)
{
  STRING S = DOCTYPE::Getoption("SepChar", Default);
  if (S.GetLength() == 1)
    {
      sepChar = S[0U];
    }
  else if (S.IsNumber())
    {
      sepChar = (CHR)S.GetInt();
    }
  else if (S[0U] == 'x')
    {
      sepChar = strtoul(S.c_str()+1, NULL, 16);
    }
  else if (S[0U] == '\\')
    {
      if (S[1U] == '\\')
	sepChar = '\\';
      else
	sepChar = strtoul(S.c_str()+1, NULL, 8);
    }
  else if (S ^= "<space>")
    {
      sepChar = ' ';
    }
  else if (S ^= "<equals>")
    {
      sepChar = '=';
    }
  else if (S ^= "<tab>")
    {
      sepChar = '\t';
    }
  else
    sepChar = '=';
  logf (LOG_DEBUG, "%s: Seperator Character:='%c' (%d)", Doctype.c_str(), sepChar, sepChar);
}


const char *METADOC::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("METADOC");
  if (Doctype != ThisDoctype && List->IsEmpty())
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);
  DOCTYPE::Description(List);
  return "Text style meta-document format files.\n\
Additional Indexing options (defined in .ini):\n\
  [General]\n\
  Style=List|Table // To use Lists or Tables for HTML presentations.\n\
  AllowWhiteBeforeField=True|False // Allow leading white space.\n\
  AutodetectFieldtypes=True|False // Guess fieldtypes\n\
  IgnoreTagWords=True|False        // Store tag names as searchable words?\n\
  SepChar=Character\n\
specified as Character or <space> (for ' '), <tab> (for '\t'), <equals>\n\
(for '='), \\nnn (octal number for character) or xNN (hexidecimal). If not\n\
specified the default is '='.";
}


void METADOC::SourceMIMEContent(const RESULT& Result, STRING *StringPtr) const
{
  DOCTYPE::SourceMIMEContent(Result, StringPtr);
}

void METADOC::SourceMIMEContent(STRING *StringBufferPtr) const
{ 
  if (StringBufferPtr)
    {
      if (sepChar == ':')
	StringBufferPtr->AssignCopy(22, "Application/X-COLONDOC");
      else if (sepChar == ' ')
	StringBufferPtr->AssignCopy(21, "Application/X-dialogB");
      else
	{
	  StringBufferPtr->AssignCopy(21, "Application/X-METADOC");
	  if (sepChar == '=')
	    StringBufferPtr->Cat("-EQUALS");
	  else if (sepChar == '\t')
	    StringBufferPtr->Cat("-TAB");
	}
    }
}


void METADOC::LoadFieldTable()
{
  DOCTYPE::LoadFieldTable();

}

void METADOC::AddFieldDefs ()
{
  DOCTYPE::AddFieldDefs ();
}

void METADOC::ParseRecords (const RECORD& FileRecord)
{
  // For records use the default parsing mechanism
  DOCTYPE::ParseRecords (FileRecord);
}


GPTYPE METADOC::ParseWords(UCHR* DataBuffer, GPTYPE DataLength,
        GPTYPE DataOffset, GPTYPE* GpBuffer, GPTYPE GpLength)
{
  // sepChar should NOT be a TermChar since its bad practice.. But we can live with it.
  if (IgnoreTagWords || IsTermChar( sepChar ))
    {
       // We need to zap the chars so they don't end-up in words
       UCHR    zap   = ' ';
       size_t  start = 1;
       // Need to zap the bits
       for (size_t i=0; i < DataLength; i++)
	{
	  if (start && (DataBuffer[i] == sepChar))
	    {
	      if (IgnoreTagWords)
		{
		  if (i == 0 || !isspace(DataBuffer[i-1]))
		    memset(DataBuffer+start-1, zap, i-start+2);
		}
	      else
		DataBuffer[i] = zap;
	      start = 0;
	    }
	  else if (DataBuffer[i] == '\r' || DataBuffer[i] == '\n' ||
		DataBuffer[i] == '\f' || DataBuffer[i] == '\v')
	    {
	      start = i + 1; // edz: 2010 April. Was =i, lost last character
	    }
	}
    }
  return DOCTYPE::ParseWords(DataBuffer, DataLength, DataOffset, GpBuffer, GpLength);
}


void METADOC::SetPresentStyle(GDT_BOOLEAN useTable)
{
  UseTable = useTable;
}

SRCH_DATE  METADOC::ParseDate(const STRING& Buffer, const STRING&) const
{
  return DOCTYPE::ParseDate(Buffer);
}

NUMERICOBJ METADOC::ParseComputed(const STRING& FieldName, const STRING& Buffer) const
{
  return DOCTYPE::ParseComputed(FieldName, Buffer);
}

FIELDTYPE METADOC::CheckFieldType(const STRING& FieldName)
{
   if (Db) return Db->GetFieldType(FieldName);
   return FIELDTYPE::any;
}


void METADOC::ParseFields (RECORD *NewRecord)
{
  const STRING fn ( NewRecord->GetFullFileName () );

  PFILE fp = ffopen(fn, "rb");
  if (fp == NULL)
    {
      logf (LOG_ERRNO, "Couldn't access '%s'", (const char *)fn);
      NewRecord->SetBadRecord();
      return;
    }
  off_t RecStart = NewRecord->GetRecordStart ();
  off_t RecEnd = NewRecord->GetRecordEnd ();

  if (RecEnd == 0)
    RecEnd = GetFileSize(fp); 
  if (RecEnd <= RecStart)
    {
      ffclose(fp);
      NewRecord->SetBadRecord();
      return;		// ERROR
    }
  if (-1 == fseek (fp, RecStart, SEEK_SET))
    logf(LOG_ERRNO, "Seek error on \"%s\"", fn.c_str());
  off_t RecLength = RecEnd - RecStart + 1;
  PCHR RecBuffer = (PCHR)tmpBuffer.Want (RecLength + 1, sizeof(CHR));
  off_t ActualLength = fread (RecBuffer, 1, RecLength, fp);
  ffclose (fp);
  RecBuffer[ActualLength] = '\0';

  GDT_BOOLEAN autodetect_Sepchar = GDT_FALSE;

  if (sepChar == '\0')
    {
      autodetect_Sepchar = GDT_TRUE;
      // Try to guess
      int saw_alpha = 0;
      for (size_t i = 0; i <= (size_t)ActualLength && sepChar == '\0'; i++)
	{
	  if (isalnum(RecBuffer[i])) 
	    saw_alpha = 1;
	  else if (!saw_alpha && ispunct(RecBuffer[i]))
	    {
	      // Skip to end of line
	      while ( i <= (size_t)ActualLength && RecBuffer[i] != '\n')
		i++;
	    }
	  else if (RecBuffer[i] == '\n')
	    saw_alpha = 0;
	  else if (saw_alpha && strchr("=:\t", RecBuffer[i]))
	    sepChar = RecBuffer[i];
	}
      if (sepChar)
	logf (LOG_DEBUG, "Detected SepChar as '%c'(0x%x)", sepChar, sepChar);
    }
  if (sepChar == '\0')
    {
      logf (LOG_ERROR, "Usage Error: No Seperator Character specified. Call \
%s::SetSepChar(Char)", Doctype.c_str());
      NewRecord->SetBadRecord();
      return;
    }

{ const char msg[] = "SepChar '%c'(0x%x) is %s. This is not good practice!";
  if (IsTermChar( sepChar ))
    logf (LOG_WARN, msg, sepChar, sepChar, "a term character");
  else if (IsDotInWord( sepChar ) ||  IsAfterDotChar(sepChar) )
    logf (LOG_WARN, msg, sepChar, sepChar, "possibly part of terms");
}

  PCHR *tags = parse_tags (RecBuffer, ActualLength);

  if ( autodetect_Sepchar) sepChar = '\0'; // We detect on a per-file basis!

  if (tags == NULL || tags[0] == NULL)
    {
      if (tags)
	logf (LOG_WARN,
#ifdef _WIN32
	  "No `%s' fields/tags in %s[%I64d,%I64d]", Doctype.c_str(), fn.c_str(), (__int64)RecStart, (__int64)RecEnd);
#else
	  "No `%s' fields/tags in %s[%lld,%lld]" , Doctype.c_str(), fn.c_str(), (long long)RecStart, (long long)RecEnd);
#endif
      else
	logf (LOG_ERROR,
#ifdef _WIN32
	  "Unable to parse `%s' record in %s[%I64d,%I64d]", Doctype.c_str(), fn.c_str(), (__int64)RecStart, (__int64)RecEnd);
#else
	  "Unable to parse `%s' record in %s[%lld,%lld]", Doctype.c_str(), fn.c_str(), (long long)RecStart, (long long)RecEnd);
#endif
      NewRecord->SetBadRecord();
      return;
    }
  if ((tags[0] - RecBuffer) > 127 || strlen(tags[0]) > 512)
    {
      logf (LOG_WARN,
#ifdef _WIN32
	"%s[%I64d,%I64d] does not seem to be in '%s' format (%lu).",
	fn.c_str(), (__int64)RecStart, (__int64)RecEnd, Doctype.c_str(), (unsigned long)(tags[0]-RecBuffer));

#else
	"%s[%lld,%lld] does not seem to be in '%s' format (%lu).",
	fn.c_str(), (long long)RecStart, (long long)RecEnd, Doctype.c_str(), (unsigned long)(tags[0]-RecBuffer));
#endif
      NewRecord->SetBadRecord();
      return;
    }

  FC fc;
  DF df;
  PDFT pdft;
  pdft = new DFT ();
  DFD dfd;
  STRING FieldName, SectionName;
  // Walk though tags
  for (PCHR * tags_ptr = tags; *tags_ptr; tags_ptr++)
    {
      // Handle Sections
      if ((*tags_ptr)[0] == '[')
	{
	  SectionName = STRING(*tags_ptr + 1).Strip(STRING::both).Pack();
	  if (SectionName.GetLength())
	    SectionName.Cat(":");
	  continue; // Handle Next
	}

      // The rest (normal)
      PCHR p = tags_ptr[1]; // end of field
      if (p == NULL) // If no end of field
	p = &RecBuffer[RecLength]; // use end of buffer
      // eg "Author:"
      size_t off = strlen (*tags_ptr) + 1;
      INT val_start = (*tags_ptr + off) - RecBuffer;
      // Skip while space after the ':'
      while (isspace (RecBuffer[val_start]))
	val_start++, off++;
      // Also leave off the \n
      INT val_len = (p - *tags_ptr) - off - 1;

      // Strip potential trailing while space
      while (val_len >= 0 && isspace (RecBuffer[val_len + val_start])) 
	val_len--;
      if (val_len < 0) continue; // Don't bother with empty fields

      (FieldName = SectionName).Cat(*tags_ptr);

      if (UnifiedName(FieldName, &FieldName).GetLength() == 0)
	continue; // ignore these

      if (IsSpecialField(FieldName))
	{
#ifdef SVR4
          char *entry_id = (char *)alloca(sizeof(char)*(val_len +1));
#else
	  char entry_id[ val_len + 1];
#endif
	  strncpy (entry_id, &RecBuffer[val_start], val_len);
	  entry_id[val_len] = '\0';
	  HandleSpecialFields(NewRecord, FieldName, entry_id);
	}

      FIELDTYPE ft ( CheckFieldType(FieldName) );
      if (recordsAdded < 100 && !ft.Defined())
	{
	  char *entry_id = (char *)alloca( val_len + 1);
	  strncpy (entry_id, &RecBuffer[val_start], val_len);
	  entry_id[val_len] = '\0'; 
	  ft = GuessFieldType(FieldName, entry_id);
	}
      dfd.SetFieldType( ft ); // Set the type

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


void METADOC::BeforeIndexing()
{
  DOCTYPE::BeforeIndexing();
}


void METADOC::AfterIndexing()
{
  tmpBuffer.Free();
  tagBuffer.Free();
  DOCTYPE::AfterIndexing();
}


STRING& METADOC::DescriptiveName(const STRING& FieldName, STRING *Value) const
{
  *Value = FieldName;
  if (Value->GetLength() > 2)
    {
      // Lowercase
      Value->ToLower();
      // Make first Character UpperCase
      Value->SetChr(1, toupper(Value->GetChr(1)));
      // Replace '-' or '_' to ' '
      Value->Replace("-", " ");
      Value->Replace("_", " ");
    }
  return *Value;
}


void METADOC::Present (const RESULT& ResultRecord,
	const STRING& ElementSet, const STRING& RecordSyntax,
	STRING *StringBuffer) const
{
  DOCTYPE::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}

void METADOC::
DocPresent (const RESULT& ResultRecord,
	    const STRING& ElementSet, const STRING& RecordSyntax,
	    STRING *StringBuffer) const
{
//cerr << "XXXXX METADOC::DocPresent()" << endl;

  if (ElementSet.Equals (SOURCE_MAGIC))
    {
      // Call "virtual" presentation
      Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
      return;
    }

  STRING Value;
  GDT_BOOLEAN UseHtml = (RecordSyntax == HtmlRecordSyntax);
  if (UseHtml)
    HtmlHead (ResultRecord, ElementSet, StringBuffer);

  if (ElementSet.Equals (FULLTEXT_MAGIC))
    {
//cerr << "XXXXX FULLTEXT....." << endl;
     
      STRING Language ( ResultRecord.GetLanguageCode() );
      // METADOC class documents are 1-deep so just collect the fields
      DFDT Dfdt;
      STRING Key ( ResultRecord.GetKey() );

      if (GetRecordDfdt (Key, &Dfdt) == GDT_FALSE)
	logf (LOG_ERROR, "Could not find record with key \"%s\"", Key.c_str());

      STRING FieldName;
      DFD  Dfd;
      // Walth-though the DFD
      GDT_BOOLEAN First_value = GDT_TRUE;
      const size_t Total = Dfdt.GetTotalEntries();

//cerr << "XXXXX Total = " << Total << endl;

      for (INT i = 1; i <= Total; i++)
	{
	  Dfdt.GetEntry (i, &Dfd);
	  Dfd.GetFieldName (&FieldName);

	  if (FieldName == FULLTEXT_MAGIC)
	    {
	      logf (LOG_ERROR, "Reserved fieldname '%s' used as data field. Skipping",
		FieldName.c_str());
	      continue;
	    }

	  // Get Value of the field, use parent
//cerr << "XXXXX Present field " << FieldName << endl;
	  Present (ResultRecord, FieldName, RecordSyntax, &Value);	// @@@

	  if (!Value.IsEmpty())
	    {
	      if (First_value && UseHtml)
		{
		  StringBuffer->Cat (UseTable ?  "<TABLE BORDER=\"0\">\n" : "<DL COMPACT=\"COMPACT\">\n");
		  First_value = GDT_FALSE;
		}

	      STRING LongName;
	      DOCTYPE::DescriptiveName(Language, FieldName, &LongName);
	      if (LongName.GetLength())
		{
		  if (UseHtml)
		    {
		      StringBuffer->Cat (UseTable ?
			  "<TR><TH ALIGN=\"Left\" VALIGN=\"Top\">" :
			  "<DT>");
		      *StringBuffer << "<!-- " << FieldName << " -->";
		      HtmlCat (LongName, StringBuffer);
		    }
		  else
		    {
		      StringBuffer->Cat (LongName);
		    }
	          *StringBuffer << ": ";
	          if (UseHtml)
		    {
		      if (UseTable)
			StringBuffer->Cat ("</TH><TD VALIGN=\"Top\">");
		      else
			StringBuffer->Cat ("<DD>");
		    }
		  StringBuffer->Cat (Value);
		  if (UseHtml && UseTable)
		    StringBuffer->Cat ("</TD></TR>");
		  StringBuffer->Cat ("\n");
		}
	    }
	}			/* for */
//cerr << "XXXXX Strings built" << endl;
      if (First_value == GDT_TRUE)
	{
	  StringBuffer->Clear();
//cerr << "XXXXX First Value is true" << endl;
	  DOCTYPE::DocPresent (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
	  return;
	}
      else if (UseHtml)
	StringBuffer->Cat (UseTable ? "</TABLE>" : "</DL>");
//cerr << "XXXXX DONE" << endl;
    }
  else
    {
      if (UseHtml)
	{
	  StringBuffer->Cat ("<DL><DT>");
	  HtmlCat (ElementSet, StringBuffer);
	}
      else
	StringBuffer->Cat (ElementSet);
      StringBuffer->Cat (": ");
      if (UseHtml)
	StringBuffer->Cat ("<DD>");
      Present (ResultRecord, ElementSet, RecordSyntax, &Value);
      StringBuffer->Cat (Value);
      StringBuffer->Cat ("\n");
      if (UseHtml)
	StringBuffer->Cat ("</DL>");
    }

  if (UseHtml)
    {
      HtmlTail (ResultRecord, ElementSet, StringBuffer); // Tail bits
    }
}




METADOC::~METADOC ()
{
}

/*-
   What:        Given a buffer of ColonTag (eg. IAFA) data:
   returns a list of char* to all characters pointing to the TAG

   Colon Records:
TAG1: ...
.....
TAG2: ...
TAG3: ...
...
....

1) Fields are continued when the line has no tag
2) Field names may NOT contain white space
3) The space BEFORE field names MAY contain white space
4) Between the field name and the ':' NO white space is
   allowed.

-*/
PCHR * METADOC::parse_tags (PCHR b, off_t len)
{
  PCHR *t;			// array of pointers to first char of tags
  size_t tc = 0;		// tag count
#define TAG_GROW_SIZE 32
  size_t max_num_tags = TAG_GROW_SIZE;	// max num tags for which space is allocated
  enum { HUNTING, SECTION, STARTED, CONTINUING } State = HUNTING;

  /* You should allocate these as you need them, but for now... */
  max_num_tags = TAG_GROW_SIZE;
  t = (PCHR *)tagBuffer.Want (max_num_tags, sizeof(PCHR));
  for (off_t i = 0; i < len; i++)
    {
      if (b[i] == '\r' || b[i] == '\v' || b[i] == '\f')
	{
	  if (State == SECTION) State = HUNTING; // No new line allowed in [ xxx ]
 	  continue; // Skip over
	}
      if (State == HUNTING && b[i] == '[')
	{
	  t[tc] = &b[i];
	  State = SECTION;
	}
      else if (State == HUNTING && !isspace(b[i]))
	{
	  t[tc] = &b[i];
	  State = STARTED;
	}
      else if ( ((State == STARTED) && (b[i] == sepChar)) ||
	((State == SECTION) && b[i] == ']') )
	{
	  b[i] = '\0';
	  // Expand memory if needed
	  t = (PCHR *)tagBuffer.Expand(++tc, sizeof(PCHR));
	  State = CONTINUING;
	}
      else if ((State == STARTED) && (b[i] == ' ' || b[i] == '\t'))
	{
	  State = CONTINUING;
	}

      // Bugfix Steve Ciesluk <cies@gsosun1.gso.uri.edu>
      // was State == CONTINUING 
      else if ((State != HUNTING) && (b[i] == '\n'))
	{
	  State = HUNTING;
	}
/* Define !allowWhiteBeforeField to NOT allow white space
   before field names */
      else if (!allowWhiteBeforeField && State == HUNTING)
	State = CONTINUING;
    }
  t[tc] = (PCHR) NULL;
  return t;
cerr << "returning " << endl;
}

///////////////// Code to handle the Heirarchical Structure

struct DTAB {
    DFD Dfd;
    FC Fc;
};

static int DfdtCompare (const void *p1, const void *p2)
{
  int dif = ((struct DTAB *) p1)->Fc.GetFieldStart() -
	((struct DTAB *) p2)->Fc.GetFieldStart();
  if (dif == 0)
    dif = ((struct DTAB *) p2)->Fc.GetFieldEnd() -
	((struct DTAB *) p1)->Fc.GetFieldEnd();
  return dif;
}


GDT_BOOLEAN METADOC::GetRecordDfdt (const STRING& Key, PDFDT DfdtBuffer) const
{
  PMDT MainMdt = Db->GetMainMdt ();
  PDFDT MainDfdt = Db->GetMainDfdt ();

  DfdtBuffer->Clear();

  MDTREC Mdtrec;
  if (!MainMdt->GetMdtRecord (Key, &Mdtrec))
    return GDT_FALSE;

  const GPTYPE MdtS = Mdtrec.GetGlobalFileStart () + Mdtrec.GetLocalRecordStart ();
  const GPTYPE MdtE = Mdtrec.GetGlobalFileStart () + Mdtrec.GetLocalRecordEnd ();

  FC MdtFc (MdtS, MdtE);

  DFD dfd;
  STRING FieldName, Fn;

  const size_t TotalEntries = MainDfdt->GetTotalEntries ();

  size_t count = 0;
  struct DTAB *Table = new struct DTAB[TotalEntries];

  for (size_t x = 1; x <= TotalEntries; x++)
    {
      MainDfdt->GetEntry (x, &dfd);
      dfd.GetFieldName (&FieldName);

      if (FieldName ^= "VALUE-ONLY")
	continue;

      Db->DfdtGetFileName (FieldName, &Fn);
      PFILE Fp = Fn.Fopen ("rb");
      if (Fp)
	{
	  // Get Total coordinates in file
	  const size_t Total = GetFileSize (Fp) / sizeof (FC);	// used fseek(Fp)

	  if (Total)
	    {
	      size_t Low = 0;
	      size_t High = Total - 1;
	      size_t X = (High / 2);
	      INT OldX;
	      FC Fc;	// Field Coordinates

	      do
		{
		  OldX = X;
		  if (-1 == fseek (Fp, X * sizeof (FC), SEEK_SET) ||
			Fc.Read (Fp) == GDT_FALSE)
		    {
		      // Read Error
		      if (++X >= Total) X = Total - 1;
		      continue;
		    }
		  if (MdtFc.Contains(Fc) == 0)
		    {
		      Table[count].Dfd = dfd;
		      Table[count++].Fc = Fc;
		      break;	// Done this loop
		    }
		  if (MdtE < Fc.GetFieldEnd()) 
		    High = X;
		  else
		    Low = X + 1;
		  // Check range
		  if ((X = (INT)(Low + High) / 2) >= Total)
		    X = Total - 1;
		}
	      while (X != OldX);
	    }
	  fclose (Fp);
	}
    }				// for()


  // qsort Table and put into DfdtBuffer so
  // that it represents the "order" in the record
  QSORT (Table, count, sizeof (struct DTAB), DfdtCompare);
  // Now add things NOT inside groups
  GPTYPE lastEnd = 0;
  DfdtBuffer->Resize(count+1);

//cerr << "Got " << count << endl;


  // Highest nodes in tree
  for (int i=0; i < count; i++)
    {
/*
STRING XX;
Table[i].Dfd.GetFieldName (&XX);
cerr << "FieldName=" << XX << endl;
cerr << "Table[i].Fc.GetFieldStart()=" << Table[i].Fc.GetFieldStart() << endl;
cerr << "Table[i].Fc.GetFieldEnd()=" << Table[i].Fc.GetFieldEnd() << endl;
cerr << "lastEnd=" << lastEnd << endl;
*/
      if ((lastEnd <= Table[i].Fc.GetFieldEnd()) &&
		(Table[i].Fc.GetFieldStart() >= lastEnd)) 
	{
	  DfdtBuffer->FastAddEntry (Table[i].Dfd);
	  lastEnd = Table[i].Fc.GetFieldEnd(); 
	}
    }
  delete[] Table;

  return GDT_TRUE;
}


COLONDOC::COLONDOC(PIDBOBJ DbParent, const STRING& Name) : METADOC (DbParent, Name)
{
  SetSepChar(':');
}

const char *COLONDOC::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("COLONDOC");
  if (Doctype != ThisDoctype && List->IsEmpty())
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);
  METADOC::Description(List);
  return "Colon document format files. Like \"METADOC\" but the default sep is ':'.";
}


void COLONDOC::SourceMIMEContent(STRING *StringBufferPtr) const
{
  METADOC::SourceMIMEContent(StringBufferPtr);
}

COLONDOC::~COLONDOC()
{

}



#if 0
//// Electronic Resource Citations

// Who, What, When, Where
//
// Who := Author
// What := Abstract, brief description
// When := Date Format
// Where := URL
//

ERCDOC::ERCDOC(PIDBOBJ DbParent, const STRING& Name) : METADOC (DbParent, Name)
{
  SetSepChar(':');
}

//
// ERC must be exactly in the order
// Who, What, When, Where
//
//
static const STRING Who    ("Who");
static const STRING What   ("What");
static const STRING When   ("When");
static const STRING Where  ("Where");

STRING ERCDOC::UnifiedName (const STRING& tag) const
{
  if (tag == "erc" || tag.CaseCompare("erc-", 4) == 0)
    return NulString; // Ignore fields named erc and erc- 
  // We will accept the reserved names in any order
  if (!((tag ^= Who) || (tag ^= What) || (tag ^= When) || (tag ^= Where)))
    {
      switch (FieldCount)
	{
	  case 0: return UnifiedName(Who);
	  case 1: return UnifiedName(What);
	  case 2: return UnifiedName(When);
	  case 3: return UnifiedName(Where);
	  default: break;
	}
    }

  // We don't want / in field names..
  if (tag.Search('/')
    {
      // Rewrite X/Y as X\Y 
      STRING newName (tag);
      newName.Replace("/", "\\", GDT_TRUE);
      return newName;
    }
  return DOCTYPE::UnifiedName(tag);
}


void ERCDOC::ParseFields (RECORD *NewRecord)
{
  FieldCount = 0;
  METADOC::ParseFields(NewRecord);
}


FIELDTYPE ERCDOC::CheckFieldType(const STRING& Fieldname)
{
  FIELDTYPE fType = Db ? Db->GetFieldType(Fieldname) : FIELDTYPE::any;

  if (fType == FIELDTYPE::any)
    {
      if (++FieldCount == 3)
	fType = FIELDTYPE::date;
      else
        fType = ((0 == CaseCompare("when", 4)) ? FIELDTYPE::date : FIELDTYPE::text);
      if (Db) Db->AddFieldTtype(Fieldname, fType);
    }
  return fType;
}


void ERCDOC::LoadFieldTable()
{
  if (Db)
    {
      Db->AddFieldTtype("When", FIELDTYPE::date);
    }
  DOCTYPE::LoadFieldTable();
}

         
const char *ERCDOC::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("ERCDOC");
  if (Doctype != ThisDoctype && List->IsEmpty())
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);
  METADOC::Description(List);
  return "Electronic Resource Citation (ERC) document format files.\n\
A Colondoc type record with the tags Who, What, When and Where\n\
Who:  Author of the work\n\
What: Abtract or brief description\n\
When: Date\n\
Where: URI\n";
}    


void ERCDOC::SourceMIMEContent(STRING *StringBufferPtr) const
{
  if (StringBufferPtr) StringBufferPtr->AssignCopy(17, "Application/X-ERC");
}
    
ERCDOC::~ERCDOC()
{
 
}

#endif

///// DIALOG-B

DIALOGB::DIALOGB(PIDBOBJ DbParent, const STRING& Name) : METADOC (DbParent, Name)
{
  allowWhiteBeforeField = GDT_FALSE;
  SetSepChar(' ');
  SetPresentType(TableKeyword);
}

const char *DIALOGB::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("DIALOG-B");
  if (Doctype != ThisDoctype && List->IsEmpty())
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);
  METADOC::Description(List);
  return "Dialog-B format files.";
}


void DIALOGB::SourceMIMEContent(STRING *StringBufferPtr) const
{
  if (StringBufferPtr)
    StringBufferPtr->AssignCopy(21, "Application/X-dialogB");
}

void DIALOGB::ParseRecords (const RECORD& FileRecord)
{
  // Break up the document into records
  off_t Start = FileRecord.GetRecordStart();
  off_t End = FileRecord.GetRecordEnd();
  off_t Position = 0;
  off_t SavePosition = 0;
  off_t RecordEnd;

  const STRING Fn = FileRecord.GetFullFileName ();

  RECORD Record (FileRecord); // Easy way

  MMAP mapping (Fn, Start, End, MapSequential);
  if (!mapping.Ok())
    {
       logf(LOG_FATAL|LOG_ERRNO, "Couldn't map '%s' into memory", Fn.c_str());
       return;
    }
  const UCHR *Buffer  = (const UCHR *)mapping.Ptr();
  const size_t MemSize = mapping.Size();

  // Search for Dialog-B Seperator
  enum { HUNT, LOOK } State = HUNT;
  int Ch;
  UCHR buf[8];
  size_t pos = 0;
  for (size_t i=0; i<= MemSize; i++)
    {
      Ch = Buffer[i];
      Position++;
      if (End > 0 && Position > End) break; // End of Subrecord
      if (Ch == sepChar || (pos == 0 && isspace(Ch)))
	{
	  State = HUNT;
	}
      else if (Ch == '\n')
	{
	  // New Line
	  if (State == LOOK)
	    {
	      if (pos < 3 || (!isspace (buf[0]) && !isalnum (buf[0])) || atoi((const char *)buf)!=0)
		{
		  // New Record...
		  buf[pos] = '\0';
		  State = HUNT;
		  SavePosition = Position - pos - 1;
		  Record.SetRecordStart (Start);
		  RecordEnd = (SavePosition == 0) ? 0 : SavePosition - 1;
		  if (RecordEnd > Start)
		    {
		      Record.SetRecordEnd (RecordEnd);
		      Db->DocTypeAddRecord(Record);
		      Start = SavePosition;
		    }
		}
	    }
	  else
	    State = LOOK;
	  SavePosition = Position;
	  pos = 0;
	}
      else if (pos < sizeof(buf)/sizeof(UCHR))
	{
	  // A token
	  buf[pos++] = (UCHR)Ch;
	}
    }				// while
  Record.SetRecordStart (Start);
  RecordEnd = (SavePosition == 0) ? 0 : Position - 1;

  if (RecordEnd > Start)
    {
      Record.SetRecordEnd (RecordEnd);
      Db->DocTypeAddRecord(Record);
    }
}


DIALOGB::~DIALOGB()
{
}
