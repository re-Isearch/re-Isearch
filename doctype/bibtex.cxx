/*-

File:        bibtex.cxx
Version:     1
Description: class BIBTEX - index documents by paragraphs 
Author:      Erik Scott, Scott Technologies, Inc.
Modifications: Edward C. Zimmermann, edz@nonmonotonic.net
*/
// TODO: Use mmap/madvise to speed up and reduce resources!

#include <ctype.h>
#include <string.h>
#include "bibtex.hxx"
#include "doc_conf.hxx"
#include "common.hxx"

#define O_BRACE '{'
#define C_BRACE '}'


BIBTEX:: BIBTEX (PIDBOBJ DbParent, const STRING& Name):
	DOCTYPE (DbParent, Name)
{
}

const char *BIBTEX::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("BIBTEX");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  DOCTYPE::Description(List);
  return "BibTeX bibliographic format files.";
}

void BIBTEX::SourceMIMEContent(PSTRING StringPtr) const
{ 
  // MIME/HTTP Content type for BibTeX records
  *StringPtr = "Application/X-Bibtex";  
} 

void BIBTEX::BeforeIndexing()
{
  DOCTYPE::BeforeIndexing();
}


void BIBTEX::AfterIndexing()
{
  recBuffer.Free();
  tmpBuffer.Free();
  DOCTYPE::AfterIndexing();
}


void BIBTEX::ParseRecords (const RECORD& FileRecord)
{
  STRING fn (FileRecord.GetFullFileName ());
  RECORD Record (FileRecord); // Easy way

#ifndef NO_MMAP
  MMAP mapping (fn, FileRecord.GetRecordStart(), FileRecord.GetRecordEnd(), MapSequential);
  if (!mapping.Ok())
    {
      message_log (LOG_ERRNO, "%s::ParseRecords: Could not map '%s' into memory", Doctype.c_str(), fn.c_str());
      return;
    }
  PCHR RecBuffer = (PCHR)mapping.Ptr();
  GPTYPE ActualLength = mapping.Size();

#else
  GPTYPE RecStart = FileRecord.GetRecordStart();
 GPTYPE RecEnd = FileRecord.GetRecordEnd();
  PFILE fp = Db->ffopen (fn, "rb");
  if (!fp)
    {
      message_log (LOG_ERRNO, "%s::ParseRecords: Could not access '%s'",
	Doctype.c_str(), fn.c_str());
      return;			// File not accessed
    }
  if (RecEnd == 0)
    RecEnd = GetFileSize (fp);

  if (RecEnd - RecStart <= 0)
    {
      message_log (LOG_WARN, "zero-length record '%s'[%ld-%ld] -- skipping",
	(const char *)fn, (long)RecStart, (long)RecEnd);
      ffclose (fp);
      return;
    }
  if (fseek (fp, RecStart, 0) == -1)
    {
      message_log (LOG_ERRNO, "%s::ParseRecords(): Seek '%s' to %ld failed",
	Doctype.c_str(), fn.c_str(), RecStart);
      ffclose (fp);
      return;
    }

  GPTYPE RecLength = RecEnd - RecStart + 1;

  PCHR RecBuffer = (PCHR)recBuffer.Want (RecLength + 2);

  GPTYPE ActualLength = (GPTYPE) fread (RecBuffer, 1, RecLength, fp);
  if (ActualLength == 0)
    {
      message_log (LOG_ERRNO, "%s::ParseRecords(): Failed to fread '%s'",
	Doctype.c_str(), fn.c_str());
      ffclose (fp);
      return;
    }
  ffclose (fp);
  if (ActualLength != RecLength)
    {
      message_log (LOG_ERRNO, "%s::ParseRecords(): Failed to fread %d bytes, got %d",
	Doctype.c_str(), RecLength, ActualLength);
      return;
    }
  RecBuffer[ActualLength] = '\0';	// NULL-terminate the buffer for strfns
#endif

#if 1 /* NEW CODE */
  int quote = 0;
  off_t Start = 0, End = 0;
  int    brace = 0; // need signed
  enum {ScanToStart, InPrologue, InRecord} State = ScanToStart; 
  for (off_t i = Start; i < ActualLength; i++)
    {
      if (RecBuffer[i] == '\\')
        i++;
      else if (RecBuffer[i] == '"')
        quote = !quote; // Toggle
      else if (quote)
	{
	  // In quote.. so ignore...
	}
      else if (RecBuffer[i] == '%')
	{
	  // Comment to end of line..
	  while (++i < ActualLength)
	    {
	      if (RecBuffer[i] == '\n' || RecBuffer[i] == '\r')
		break;
	    }
	}
      else if (State == ScanToStart)
	{
	  if (RecBuffer[i] == '@')
	    {
	      // Here is start
	      State = InPrologue;
	    }
	}
      else if (State == InPrologue)
	{
	  if (RecBuffer[i] == O_BRACE)
	    {
	      State = InRecord;
	      brace++;
	    }
	}
      else if (State == InRecord)
	{
	  if (RecBuffer[i] == O_BRACE)
	    brace++;
	  else if (RecBuffer[i] == C_BRACE)
	    {
	      if (--brace == 0)
		{
		  // Found end ...
		  // Goto end of white space
		  while (++i < ActualLength && isspace(RecBuffer[i]))
		    /* loop */;
		  State = ScanToStart;
		  Record.SetRecordStart (Start);
		  Record.SetRecordEnd (i-1);
		  Db->DocTypeAddRecord (Record);
		  Start = i;
		}
	    }
	}
    }
  int errors = 0;
  if (quote)
    {
      message_log(LOG_ERROR, "Runaway quote..");
      errors++;
    }
  if (State == InPrologue)
    {
      message_log(LOG_ERROR, "@ started but no object {} defined");
      errors++;
    }
  if (State == InRecord)
    {
      message_log(LOG_ERROR, "Record did not end..");
      errors++;
    }
  if (brace != 0)
    {
      message_log(LOG_ERROR, "Input was not wellformed! %d too many %c", 
	brace > 0 ? brace : -brace,
	brace > 0 ? O_BRACE : C_BRACE);
      errors++;
    }
  if (Start < ActualLength)
    {
      if (errors)
        {
	  message_log (LOG_WARN, "Marking %s %ld-%ld deleted",
		(const char *)fn, Start, ActualLength);
        }
      else if (Start != 0)
	{
	  message_log (LOG_INFO, "Ignoring %s trailing bytes %ld-%ld",
		(const char *)fn, Start, ActualLength);
	}
      else
	message_log (LOG_INFO, "Ignoring '%s', bogus %s record", (const char *)fn, Doctype.c_str());
      Record.SetBadRecord(); // Delete this record
      Record.SetDocumentType ( "<NIL>" ); // Delete this record
      Record.SetRecordStart (Start);
      Record.SetRecordEnd (ActualLength);
      Db->DocTypeAddRecord (Record);
    }
#else
  int lastBrace = 0;            // this is an int because I need signed.
  for (int w = ActualLength; w > 0; w--)
    if (RecBuffer[w] ==  C_BRACE)
      {
	lastBrace = w;
	w = -1;			// to break out of loop.
      }

  int quote = 0;
  GPTYPE Start = 0;
  for (GPTYPE i = Start; i <= ActualLength; i++)
    {
      if (RecBuffer[i] == '\\')
	i++;
      else if (RecBuffer[i] == '"')
	quote = !quote; // Toggle
      else if (RecBuffer[i] == '%' && !quote)
	{
	  while (++i < ActualLength)
	    {
	      if (RecBuffer[i] == '\n')
		break;
	    }
	}
      else if (!quote && RecBuffer[i] == C_BRACE)
	{			// did we find the end of a record?  Good.

	  if (i == (GPTYPE)lastBrace)
	    {			// we're on the very last one.
	      i = ActualLength - 1;	// so we mark it at the very end, after the whitespace.
	    }
	  Record.SetRecordStart (Start);
	  Record.SetRecordEnd (i);
	  Db->DocTypeAddRecord (Record);
	  Start = i + 1;
	}
    }
#endif

}

//
//
// The new goal:  scan the record looking for (title = ") and (") pairs
// and mark them as a field named "title".
//
//

void BIBTEX::ParseFields (RECORD *NewRecord)
{
  STRING fn;
  GPTYPE RecStart, RecEnd;

  // Open the file
  NewRecord->GetFullFileName (&fn);
  PFILE fp = ffopen (fn, "rb");
  if (!fp)
    {
      message_log (LOG_ERRNO, "%s::ParseFields(): Failed to open '%s'", Doctype.c_str(), fn.c_str());
      return;
    }

  // Determine the start and size of the record
  RecStart = NewRecord->GetRecordStart ();
  RecEnd = NewRecord->GetRecordEnd ();

  if (RecEnd == 0)
    RecEnd = GetFileSize (fp);
  if (RecEnd - RecStart <= 0)
    {
      ffclose (fp);
      message_log (LOG_WARN, "zero-length record - '%s' [%ld-%ld] - skipping", fn.c_str(), RecStart, RecEnd);
      return;
    }

  // Make two copies of the record in memory
  if (fseek (fp, RecStart, 0) == -1)
    {
      message_log (LOG_ERRNO, "%s::ParseFields(): Seek failed - %s", Doctype.c_str(), fn.c_str());
      ffclose (fp);
      return;
    }
  size_t RecLength = RecEnd - RecStart + 1;

  PCHR RecBuffer = (PCHR)recBuffer.Want(RecLength + 1, sizeof(CHR));

  size_t ActualLength = (GPTYPE) fread (RecBuffer, 1, RecLength, fp);
  if (ActualLength == 0)
    {
      message_log (LOG_ERRNO, "%s::ParseFields(): Failed to fread %s",
	Doctype.c_str(), fn.c_str());
      ffclose (fp);
      return;
    }
  ffclose (fp);
  if (ActualLength != RecLength)
    {
      message_log (LOG_ERRNO, "%s::ParseFields(): Failed to fread %d bytes, got %d",
        Doctype.c_str(), RecLength, ActualLength);
      return;
    }
  RecBuffer[RecLength] = '\0';

  // Parse the record and add fields to record structure
  STRING FieldName;
  DF df;
  PDFT pdft;
  INT val_start = 0;
  INT val_end = 0;
  DFD dfd;

  pdft = new DFT ();

  // OK - we need to scan RecBuffer and find the "title" element, then fast-
  // forward to the next '"' character.  That will be the title field val_start.
  // Then we go forward again until we see another '"' character, and that will
  // be the field end.
  // If someone (a) knows how BibTeX represents a literal quotation mark and
  // (b) wants to hack this to do the right thing, be my guest.  I'm a long-
  // time [nt]roff user, myself. :-)

  struct
    {
      const char *keyword;
      UCHR len;
    }
  keywords[] =
  {
    // Reverse Alphabetical order!
    { "year",            4 },
    { "volume",          6 },
    { "uniform",         7 },
    { "type",            4 },
    { "title",           5 },
    { "thesaurus",       9 },
    { "subject",         7 },
    { "series",          6 },
    { "school",          6 },
    { "review",          6 },
    { "publisher",       9 },
    { "price",           5 },
    { "pageswhole",     10 },
    { "pages",           5 },
    { "organization",   12 },
    { "number",          6 },
    { "note",            4 },
    { "month",           5 },
    { "language",        8 },
    { "keywords",        8 },
    { "key",             3 }, // Special
    { "journal",         7 },
    { "institution",    11 },
    { "howpublished",   12 },
    { "editor",          6 },
    { "edition",         7 },
    { "crossref",        8 },
    { "copyright",       9 },
    { "confsponsor",    11 },
    { "conflocation",   12 },
    { "confdate",        8 },
    { "coden",           5 },
    { "chapter",         7 },
    { "classification", 14 },
    { "booktitle",       9 },
    { "bibdate",         7 }, 
    { "author",          6 },
    { "availability",   12 },
    { "annote",          6 },
    { "affiliation",    11 },
    { "address",         7 },
    { "acknowledgement",15 },
    { "abstract",        8 },
    { "URL",             3 },
    { "LCCN",            4 },
    { "ISSN",            4 },
    { "ISBN",            4 }
  };

  size_t i;
  for (i = 0; i < ActualLength; i++)
    {
      if (RecBuffer[i] == '@')
	val_start = i+1;
      else if (RecBuffer[i] == O_BRACE)
	{
	  val_end = i-1;
	  if (val_start)
	  break;
	}

    }
  if (val_end > val_start)
    {
      // First Lets store the object type
      FieldName = "Object";
      dfd.SetFieldName (FieldName);
      Db->DfdtAddEntry (dfd);
      df.SetFct ( FC(val_start, val_end) );
      df.SetFieldName (FieldName);
      pdft->AddEntry (df);
      if ((val_end - val_start) == 5 && StrNCaseCmp(&RecBuffer[val_start], "String", 6) == 0)
	{
	  val_start = val_end + 2;
	  while (val_start < ActualLength && isspace(RecBuffer[val_start])) val_start++;
	  val_end = val_start;	
	  while (++val_end < ActualLength)
	    {
	      if (RecBuffer[val_end] == '=')
		{
		  int meta_start = val_end;
		  while (--val_end > val_start && isspace(RecBuffer[val_end]))
		    /* loop */;
		  char *abbrev = (char *)tmpBuffer.Want(val_end - val_start + 1);
//		  char abbrev[val_end - val_start + 1];
		  strncpy(abbrev, &RecBuffer[val_start], val_end-val_start+1);
		  abbrev[val_end-val_start + 1] = '\0';
		  NewRecord->SetKey(abbrev);

		  // Now Set the value for the "abbrev" meta object
		  while (++meta_start < ActualLength && isspace(RecBuffer[meta_start]))
		    /* loop */;
		  // Now looking at the start 
		  if (meta_start >= ActualLength || RecBuffer[meta_start] != '"')
		    {
		      message_log (LOG_NOTICE|LOG_INFO,
			"Couldn't find \" after %s =, skipping string", abbrev);
		      break;
		    }
		  // Now Search for end...
		  int meta_end = ++meta_start;
		  for (;meta_end < ActualLength;meta_end++)
		    {
		      if (RecBuffer[meta_end] == '\\')
			meta_end++;
		      else if (RecBuffer[meta_end] == '"')
			break;
		    }
		  if (meta_end == ActualLength)
		    {
		      message_log (LOG_NOTICE, "Couldn't find end \" after %s, skipping string", abbrev);
		      break;
		    }
		  // Have a start/end pair
		  if (meta_end - meta_start > 2)
		    {
		      // Its something to store
		      FieldName = "@String"; // Meta value
		      dfd.SetFieldName (FieldName);
		      Db->DfdtAddEntry (dfd);
		      df.SetFct ( FC(meta_start, meta_end - 1) );
		      df.SetFieldName (FieldName);
		      pdft->AddEntry (df);
		    }

		  break;
		}
	    }
	}
      else
	{
#if 1
	  // Add Object template
	  if (val_end - val_start < 127)
	    {
	      char templ[129];
	      strncpy(templ, &RecBuffer[val_start], val_end - val_start + 1);
	      templ[val_end - val_start + 1] = '\0';
	      Db->AddTemplate (templ);
	    }
#endif
	  // Set key
	  val_start = val_end + 2;
	  while (isspace(RecBuffer[val_start]) && val_start < ActualLength)
	    val_start++;
	  val_end = val_start;
	  while (++val_end < ActualLength && RecBuffer[val_end] != ',' &&
		RecBuffer[val_end] != '"' && RecBuffer[val_end] != '=' )
	    /* loop */;
	  if (RecBuffer[val_end] == ',' && val_end < ActualLength)
	    {
	      while (--val_end > val_start && isspace(RecBuffer[val_end]))
		/* loop */;
	      if (val_end > val_start)
		{
		  // Its something to store
		  FieldName = "RefId";
		  dfd.SetFieldName (FieldName);
		  Db->DfdtAddEntry (dfd);
		  df.SetFct ( FC(val_start, val_end) );
		  df.SetFieldName (FieldName);
		  pdft->AddEntry (df);
		  if (val_end - val_start < DocumentKeySize)
		    {
		      // Also Have a bib key
		      char *newkey = (char *)tmpBuffer.Want(val_end - val_start + 1);
//		      char newkey[val_end - val_start + 1];
		      strncpy(newkey, &RecBuffer[val_start], val_end-val_start+1);
		      newkey[val_end-val_start + 1] = '\0';
		      NewRecord->SetKey(newkey);
		    }
		}
	    }
	}
    }

  val_start = 0;

  // Basically, the following God-awful excuse for a state machine will
  // look for title not occuring inside quotation marks.
  enum STATE {LOOKING, INQUOTES, INCOMMENT, FOUND} state = LOOKING;
  for (i = 0; i < ActualLength; i++)
    {
      if (state == LOOKING)
	{
	  if (RecBuffer[i] == '%')
	    {
	      state = INCOMMENT;
	      continue;
	    }
	  size_t j;
	  for (j = 0; j < sizeof (keywords) / sizeof (keywords[0]); j++)
	    {
	      if (((i + keywords[j].len) < ActualLength) &&
		  memcmp (&RecBuffer[i], keywords[j].keyword, keywords[j].len) == 0)
		{
		  i += keywords[j].len;
		  while (i < ActualLength && isspace(RecBuffer[i])) i++;
		  if (RecBuffer[i] == '=')
		    {
		      state = FOUND;
		      break;
		    }
		}
	    }
	  if (state == FOUND)
	    {
	      CHR EndMark = '"';
	      state = LOOKING;
	      // look for the quotation mark
	      while (++i < ActualLength && isspace(RecBuffer[i]))
		/* loop */;
	      if (i >= ActualLength || RecBuffer[i] != '"')
		EndMark = ','; // Indirection ?
	      val_start = i;
	      // Look for end
	      while (++i < ActualLength)
		{
		  if (RecBuffer[i] == '\\')
		    i++;
		  else if (RecBuffer[i] == EndMark
			/* added 2004 -> */ || (EndMark == ',' && RecBuffer[i] == '}'))
		    break;
		}
	      if (i == ActualLength)
		{
		  message_log (LOG_NOTICE, "Couldn't find end %c after %s, skipping field",
			EndMark, keywords[j].keyword);
		  break;
		}
	      else
		{
		  val_end = i++;
		  if (RecBuffer[val_end] == ',')
		    val_end--;
		  if (RecBuffer[val_end] == '"')
		    val_end--;
		  // We have a tag pair
		  FieldName = keywords[j].keyword;
		  dfd.SetFieldType( Db->GetFieldType(FieldName) ); // Get the type added 30 Sep 2003
		  dfd.SetFieldName (FieldName);
		  Db->DfdtAddEntry (dfd);
		  df.SetFct ( FC(val_start, val_end) );
		  df.SetFieldName (FieldName);
		  pdft->AddEntry (df);
		  if (FieldName ^= "Key")
		    {
		      // Set Record Key
		      char *newkey = (char *)tmpBuffer.Want(val_end - val_start);
//		      char newkey[val_end - val_start];
		      strncpy(newkey, &RecBuffer[val_start+1], val_end-val_start);
		      newkey[val_end-val_start] = '\0';
		      if (! Db-> MdtLookupKey ( newkey ))
			{
			  NewRecord->SetKey(newkey);
			}
		      else
			{
			  message_log (LOG_NOTICE, "Duplicate BibTeX key \"%s\"", newkey);
			}
		    }
		  // Record Date?
		  else if (FieldName ^= "bibdate") 
		    {
		      NewRecord->SetDate( &RecBuffer[val_start + 1] );
		    }
		  val_start = 0;
		}
	    }			// end of if we found title
	  else if (RecBuffer[i] == '"')
	    state = INQUOTES;
	}			// end of looking
      else if (state == INQUOTES)
	{
	  if (RecBuffer[i] == '"')
	    state = LOOKING;
	}
      else if (state == INCOMMENT)
	{
	  if (RecBuffer[i] == '\n')
	    state = LOOKING;
	}
    }				// end of for loop

  NewRecord->SetDft (*pdft);
  delete pdft;
}


void BIBTEX::
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
      STRING Object;
      Present (ResultRecord, "Object", RecordSyntax, &Object);
      if (Object ^= "STRING")
	{
	  STRING Key;
	  ResultRecord.GetKey(&Key);
	  if (UseHtml)
	    StringBuffer->Cat ("<DT>");
	  StringBuffer->Cat (Key);
	  if (UseHtml)
	    StringBuffer->Cat ("<DD>");
	  else
	    StringBuffer->Cat (" := ");
	  Present (ResultRecord, "@STRING", RecordSyntax, &Object);
	  StringBuffer->Cat (Object);
	  if (!UseHtml)
	    StringBuffer->Cat ("\n");
	}
      else
	{
	  DFDT Dfdt;
	  STRING Key;
	  ResultRecord.GetKey(&Key);
	  Db->GetRecordDfdt (Key, &Dfdt);
	  size_t Total = Dfdt.GetTotalEntries();
	  DFD Dfd;
	  STRING FieldName;
	  // BibTeX class documents are 1-deep so just collect the fields
	  for (size_t i = 1; i <= Total; i++)
	    {
	      Dfdt.GetEntry (i, &Dfd);
	      Dfd.GetFieldName (&FieldName);
	      // Get Value of the field, use parent
	      Present (ResultRecord, FieldName, RecordSyntax, &Value);
	      if (!Value.IsEmpty())
		{
		  if (UseHtml)
		    *StringBuffer << "<DT>";
		  if ((FieldName != "ISBN") && (FieldName != "ISSN") &&
			(FieldName != "LCCN") && FieldName != "URL")
		    {
		      FieldName.ToLower();
		      *StringBuffer << (UCHR)toupper(FieldName.GetChr(1))
			<< FieldName.SubString(2, 0) << ": ";
		    }
	          else
		    {
		      *StringBuffer << FieldName << ": ";
		    }
		  if (UseHtml)
		    *StringBuffer << "<DD>";
		  if ((FieldName ^= "OBJECT") && (Value ^= "STRING"))
		    {
		      ResultRecord.GetRecordData (&Value);
		    }
	          *StringBuffer << Value << "\n";
		}
	    }		/* for */
	}
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

// Convert TeX Entities
static void Decode (PSTRING Input)
{
  STRINGINDEX Len = Input->GetLength ();
  const CHR *Buffer = (const CHR *) (*Input);
  enum
    {
      PLAIN, GRAVE, ACUTE, CIRC, UMLAUT, TILDE, DOT, CEDIL
    }
  State = PLAIN;
  int brace = 0;
  STRING Output;
  CHR Ch;
  for (register size_t i = 0; i < Len; i++)
    {
      Ch = Buffer[i];
      while (Ch == O_BRACE && (brace || Buffer[i + 1] == '\\'))
	{
	  brace++;
	  Ch = Buffer[++i];
	}
      while (Ch == C_BRACE && brace)
	{
	  brace--;
	  Ch = Buffer[++i];
	}
      if (Ch == '\\' && i < Len)
	{
	  Ch = Buffer[++i];
	  switch (Ch)
	    {
	    case '`':  State = GRAVE;  break;
	    case '\'': State = ACUTE;  break;
	    case '^':  State = CIRC;   break;
	    case '"':  State = UMLAUT; break;
	    case '~':  State = TILDE;  break;
	    case '.':  State = DOT;    break;
	    case ',':  State = CEDIL;  break;
	    case O_BRACE:  brace++;    break;

	    default:
	      // Special Cases
	      if (Ch == '$' || Ch == '\\')
		{
		  Output += Ch;
		}
	      else if (strncmp(&Buffer[i], "path", 4) == 0)
		{
		  // Special case
		  i += 4;
		  const char *tcp = strchr(&Buffer[i+1], Buffer[i]);
		  if (tcp != NULL)
		    {
		       const size_t len = tcp - &Buffer[++i];
		       for (size_t k=0; k< len; k++)
			{
			  Output += Buffer[i++];
			}
		       Output += (CHR)' ';
		    }
		}
	      else
		{
		  static const struct {
		    const char *entity;
		    char len;
		    const char *val;
		  } Special[] = {
		    {"/",	  1,    " "  },
		    {"AA",        2, "\305"  }, 
		    {"AE",        2, "\306"  }, 
		    {"BibTeX",    6, "BibTeX"},
		    {"LaTeX",     5, "LaTeX" },
		    {"TeX",       3, "TeX"   },
		    {"O",         1, "\330"  },
		    {"aa",        2, "\345"  },
		    {"ae",        2, "\346"  },
		    {"cdots",     5, "..."   },
		    {"cents",     5, "\242"  },
		    {"copyright", 9, "\251"  },
		    {"deg",       3, "\260"  },
		    {"div",       3, "\367"  },
		    {"ldots",     5, "..."   },
		    {"o",         1, "\370"  },
		    {"pounds",    6, "\243"  },
		    {"slash",     5,    "/"  },
		    {"ss",        2, "\337"  },
		    {"times",     5, "\327"  },
		    {"yen",       3, "\245"  }
		  };

		  // Look through table
		  int notfound = 1;
		  for (size_t j=0; notfound && j<sizeof(Special)/sizeof(Special[0]); j++)
		    {
		      if ( (Special[j].len + i) < Len &&
			memcmp(&Buffer[i], Special[j].entity, Special[j].len) == 0 &&
			!isalnum(Buffer[i + Special[j].len]) )
			{
			  Output += Special[j].val;
			  i += Special[j].len - 1;
			  if (isspace(Buffer[i])) i++;
			  notfound = 0;
			}
		    }
		  if (notfound)
		    {
		      // Who know's
		      State = PLAIN;
		      Output += (CHR) '\\';
		      Output += Ch;
		    }

		}
	    }
	}
      else
	{
	  CHR rest = 0;
	  switch (State)
	    {
	    case PLAIN:
	      if (Ch) Output += Ch;
	      break;
	    case UMLAUT:
	      {
		static const CHR src[] = "AEIOUaeiouy";
		const CHR code[] = {'\304','\313','\317','\326','\334',
			'\344','\353','\357','\366','\374','\377' }; 
		// Output += "&"; Output += Ch; Output += "uml;";
		const char *tcp = strchr(src, Ch);
		if (tcp)
		  Output += code[tcp-src];
		else
		  rest = '"';
	      }
	      break;
	    case CIRC:
	      {
		static const CHR src[] = "AEIOUaeiou";
		const CHR code[] = {'\302','\312','\316','\324','\333',
			'\342','\352','\356','\364','\373' };
		// Output += "&"; Output += Ch; Output += "circ;";
		const char *tcp = strchr(src, Ch);
		if (tcp)
		  Output += code[tcp-src];
		else
		  rest = '^';
	      }
	      break;
	    case DOT:
	      {
		// Output += "&"; Output += Ch; Output += "dot;";
		rest = '.';
	      }
	      break;
	    case CEDIL:
	      {
		// Output += "&"; Output += Ch; Output += "cedil;";
		if (Ch == 'C')
		  Output += (UCHR)'\307'; // 199
		else if (Ch == 'c')
		  Output += (UCHR)'\347'; // 231
		else
		  rest = ',';
	      }
	      break;
	    case TILDE:
	      {
		static const CHR src[] = "ANOano"; 
		const CHR code[] = {'\303','\321','\325','\343','\361','\365' };
		// Output += "&"; Output += Ch; Output += "tilde;";
		const char *tcp = strchr(src, Ch); 
		if (tcp) 
		  Output += code[tcp-src]; 
		else 
		  rest = '~';
	      }
	      break;
	    case ACUTE:
	      {
		static const CHR src[] = "AEIOUYaeiouy";
		const CHR code[] = {'\301','\311','\315','\323','\332',
			'\335','\341','\351','\355','\363','\372','\375' };
	        // Output += "&"; Output += Ch; Output += "acute;";
		const char *tcp = strchr(src, Ch);
		if (tcp)  
		  Output += code[tcp-src];
		else 
		  rest = '\'';
	      }
	      break;
	    case GRAVE:
	      {
		static const CHR src[] = "AEIOUaeiou"; 
		const CHR code[] = {'\300','\310','\314','\322','\331',
			'\340','\350','\354','\362','\371' };
	        // Output += "&"; Output += Ch; Output += "grave;";
		const char *tcp = strchr(src, Ch); 
		if (tcp)  
		  Output += code[tcp-src]; 
		else 
		  rest = '`';
	      }
	      break;
	    }
	  if (rest)
	    {
	      // Code back, since we don't know what to do  
	      Output += "{\\\"";
	      Output += rest;
	      Output += "{";  
	      Output += Ch;      
	      Output += "}}"; 
	    }
	  State = PLAIN;
	}
    }				// for

  *Input = Output;
}


void BIBTEX::Present (const RESULT& ResultRecord,
	 const STRING& ElementSet, const STRING& RecordSyntax,
	 PSTRING StringBuffer) const
{
  if (ElementSet.Equals(BRIEF_MAGIC))
    {
      // BibTex Headline is Object: Author, Title

      STRING Value, Author;
      // Get a Title
      Present (ResultRecord, "Title", RecordSyntax, &Value);
      if (Value.GetLength() == 0)
	{
	  Present (ResultRecord, "BookTitle", RecordSyntax, &Value);
	  if (Value.GetLength() == 0)
	    DOCTYPE::Present (ResultRecord, BRIEF_MAGIC, RecordSyntax, &Value);
	}
      if (Value ^= "Preamble")
	{
	  StringBuffer->Clear();
	}
      else if (Value ^= "String")
	{
	  *StringBuffer = Value;
	  ResultRecord.GetKey(&Value);
	}
      else
	{
	  Present (ResultRecord, "Object", RecordSyntax, StringBuffer);
	  Present (ResultRecord, "Author", SutrsRecordSyntax, &Author);
	  if (Author.GetLength() == 0)
	    {
	      Present (ResultRecord, "Editor", SutrsRecordSyntax, &Author);
	    }
	}
      if (StringBuffer->GetLength())
	StringBuffer->Cat(": ");
      if (Author.GetLength())
	{
	  if (RecordSyntax == HtmlRecordSyntax)
	    HtmlCat(Author, StringBuffer);
	  else
	    StringBuffer->Cat(Author);
	  StringBuffer->Cat(", ");
	}
      StringBuffer->Cat(Value);
      StringBuffer->Pack();
    }
#ifdef CGI_FETCH
  else if ((RecordSyntax == HtmlRecordSyntax) && (ElementSet ^= "crossref"))
    {
      STRING Value;
      Db->GetFieldData (ResultRecord, ElementSet, &Value);
      if (Value.GetChr(1) == '"')
	Value.EraseBefore(2);

      STRING furl;
      Db->URLfrag(&furl);
      StringBuffer->Clear();
      *StringBuffer << "<A HREF=\"" <<
	CGI_FETCH << furl << Value << "\">" <<
	Value << "</A>"; 
    }
#endif
  else if (ElementSet ^= "RefId")
    {
      Db->GetFieldData (ResultRecord, ElementSet, StringBuffer);
    }
  else
    {
      STRING Value;
      Db->GetFieldData (ResultRecord, ElementSet, &Value);
      Decode (&Value);
      if (Value.GetChr(1) == '"')
	{
	  Value.EraseBefore(2);
	}
      else
	{
	  // Indirection
	  RESULT RsRecord;
	  if (Db->KeyLookup (Value, &RsRecord))
	    {
	      Db->GetFieldData (RsRecord, "@STRING", &Value);
	      Decode (&Value);
	    }
	}
      if (RecordSyntax == HtmlRecordSyntax)
	{
	  StringBuffer->Clear();
	  HtmlCat (Value, StringBuffer);
	  StringBuffer->Replace("\n\n" , "\n<P>\n");
	  StringBuffer->Replace("---", "&mdash;");
	  StringBuffer->Replace("--", "&ndash;");
	  StringBuffer->Replace("\\ ", "&nbsp;");
	  if ((ElementSet != "Title") && (ElementSet != "BookTitle"))
	    {
	      StringBuffer->Replace("{", "<code>");
	      StringBuffer->Replace("}", "</code>");
	    }
	}
      else
	{
	  *StringBuffer = Value;
	  StringBuffer->Replace("\\ ", " ");
	}

      static const char textrule[] = "\n\
_________________________________________________________________\n";

      const char *rule = (RecordSyntax == HtmlRecordSyntax) ? "<HR>\n" : textrule;
      StringBuffer->Replace("\\rule", rule);
      StringBuffer->Replace("\\hrule", rule);

      // Generic things to replace
      StringBuffer->Replace("{", " ");
      StringBuffer->Replace("}", " ");
      StringBuffer->Replace("\\tt ", "");
      StringBuffer->Replace("\\it ", "");
      StringBuffer->Replace("\\em ", "");
      StringBuffer->Replace("\\b ", "");
    }
}


BIBTEX::~BIBTEX ()
{
}
