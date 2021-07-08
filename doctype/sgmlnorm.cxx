#pragma ident  "@(#)sgmlnorm.cxx	1.67 04/20/01 14:23:57 BSN"
/* ########################################################################

               Tag and Entity Normalized SGML Document Handler (DOCTYPE)

   File: sgmlnorm.cxx
   Description: Class SGMLNORM - Normalized SGML Document Type 
   Last maintained by: Edward C. Zimmermann

   ########################################################################

   Note: None

   ########################################################################


   ######################################################################## */
/*
// $Log: sgmlnorm.cxx,v $
// Revision 1.1  2007/05/15 15:47:29  edz
// Initial revision
//
// Revision 1.8  1996/04/28  15:00:18  edz
// Bugfix: Did not correctly handle nested declarations.. Added check to
// make sure that we are not in a declaration before we mark the start of
// a declaration--- see (3) in code
//
// Revision 1.7  1996/04/28  11:28:07  edz
// Support for <tag/contents of tag/ style markup
// Ignore -- in declarations since some text seems to leave the end-of-comment
// out in declarations.
//
// Revision 1.6  1996/02/09  12:28:20  edz
// Added some parser features.
//
*/

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "common.hxx"
#include "doctype.hxx"
#include "sgmlnorm.hxx"
#include "doc_conf.hxx"
#include "soundex.hxx"

#if BSN_EXTENSIONS
# if BSN_EXTENSIONS < 3
#  define ATTRIB_SEP '@'
# endif
#endif

#ifndef RSS_HACK
# define RSS_HACK 0
#endif

#if RSS_HACK
static const char rss_magic[]   = "rss version";
static const char rss_doctype[] = "RSS2";
#endif

SGMLNORM::SGMLNORM (PIDBOBJ DbParent, const STRING& Name) :
	DOCTYPE (DbParent, Name)
{
  // Read doctype options
  StoreComplexAttributes = Getoption("Complex", "True").GetBool();
  IgnoreTagWords         = Getoption("IgnoreTagWords", "True").GetBool();

  logf (LOG_DEBUG, "SGMLNORM class inited");

}

const char *SGMLNORM::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("SGMLNORM");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  DOCTYPE::Description(List);
  return "Tag and Entity normalized SGML documents\n\
  Index time Options:\n\
    Complex arg    // Takes 0 (ignore), 1 (accept) [default is 1]\n\
    IgnoreTagWords arg   // as above, store tag names or ignore them?\n\
    These can also be specified in the doctype.ini [General] section as\n\
    Complex=0 or 1.\n\
    IgnoreTagWords=0 or 1\n\
  Search time Options:\n\
    In the section [<DTD Name>] (example [-//Free Text Project//DTD Play//EN])\n\
    Headline=<Headline format>.\n\
    In the section [Catalog] the DTDs can be maped as\n\
    DTD=useDTD";
}


void SGMLNORM::SourceMIMEContent(PSTRING StringPtr) const
{ 
  // MIME/HTTP Content type for SGML documents
  StringPtr->AssignCopy(9, "text/sgml");  
}

void SGMLNORM::SourceMIMEContent(const RESULT& Record, PSTRING StringPtr) const
{
  // <!DOCTYPE XXX ...
  STRING dtd;
  Present(Record, "!DOCTYPE", NulString, &dtd);
  if (dtd.GetLength())
    *StringPtr = "application/sgml";
  else
    SourceMIMEContent(StringPtr);
}

void SGMLNORM::ParseRecords (const RECORD& FileRecord)
{
#if 0
  // Break up the document into XML-Like records
  GPTYPE Start = FileRecord.GetRecordStart();
  GPTYPE End = FileRecord.GetRecordEnd();
  GPTYPE Position = 0;
  GPTYPE SavePosition = 0;
  GPTYPE RecordEnd;

  const STRING Fn = FileRecord.GetFullFileName ();

  RECORD Record (FileRecord); // Easy way
  size_t pos = 0;

  MMAP mapping (Fn, Start, End, MapSequential);
  if (!mapping.Ok())
    {
       logf(LOG_FATAL|LOG_ERRNO, "Couldn't map '%s'", Fn.c_str());
       return;
    }
  const UCHR *Buffer  = (const UCHR *)mapping.Ptr();
  const size_t MemSize = mapping.Size();
#else
  DOCTYPE::ParseRecords (FileRecord);
#endif
}

void SGMLNORM::BeforeIndexing()
{
  DOCTYPE::BeforeIndexing();
}

void SGMLNORM::AfterIndexing()
{
  tmpBuffer.Free("SGMLNORM", "tmpBuffer");
  tmpBuffer2.Free("SGMLNORM", "tmpBuffer2");
  tagsBuffer.Free("SGMLNORM", "tagsBuffer");
  DOCTYPE::AfterIndexing();
}


void SGMLNORM::AddFieldDefs()
{
  DOCTYPE::AddFieldDefs();
}

INT SGMLNORM::ReadFile(FILE *Fp, STRING *StringPtr, off_t Offset, size_t Length) const
{
  int count = DOCTYPE::ReadFile(Fp, StringPtr, Offset, Length);
  if (count)
    Entities.normalize((char *)(StringPtr->c_str()), count); // WARNING: WARNING: WARNING (Hack!)
  return count;
}

INT SGMLNORM::ReadFile(FILE *Fp, CHR *Buffer, off_t Offset, size_t Length) const
{
  int count = DOCTYPE::ReadFile(Fp, Buffer, Offset, Length);
  if (count)
    Entities.normalize(Buffer, count);
  return count;
}

INT SGMLNORM::GetRecordData(FILE *Fp, STRING *StringPtr, off_t Offset, size_t Length) const
{
  int count = DOCTYPE::ReadFile(Fp, StringPtr, Offset, Length);
  if (count)
    Entities.normalize2((char *)(StringPtr->c_str()), count); // WARNING: WARNING: WARNING (Hack!)
  return count;
}

INT SGMLNORM::GetRecordData(FILE *Fp, CHR *Buffer, off_t Offset, size_t Length) const
{
  int count = DOCTYPE::ReadFile(Fp, Buffer, Offset, Length);
  if (count)
    Entities.normalize2(Buffer, count);
  return count;
}


// Used by GetIndirect Buffer.. Need to read more!!
INT SGMLNORM::GetTerm(const STRING& Filename, CHR *Buffer, off_t Offset, size_t Length)
{
  int segment = 10;
  if (Offset > segment)
    Offset -= segment;
  else
    segment = 0;
  size_t        want = Length*5 + 12 + segment; // Should be enough
  char         *tmp = (char *)tmpBuffer2.Want(want+1);

//cerr << "SGMLNORM::GetTerm called offset=" << Offset << "   len=" << Length << endl;

  if (tmp == NULL)
    {
      logf (LOG_PANIC|LOG_ERRNO, "%s GetTerm buffer exhausted, wanted %d bytes", Doctype.c_str(), want);
      Buffer[0] = '\0';
      return 0;
    }
  size_t        count = 0;
  FILE         *fp = ffopen(Filename, "rb");
  if (fp)
    {
      count = DOCTYPE::ReadFile(fp, tmp, Offset, want);
      ffclose(fp);
    }
  if (count)
    {
      if (segment)
	{
	  if (tmp[segment] == ';')
	    {
	      while (--segment > 0)
		{
		  if (tmp[segment] == '&')
		    {
		      if (segment) segment--;
		      break;
		    }
		}
	    }
	  count -= segment;
	  tmp += segment;
	}
      Entities.normalize2(tmp, count);

      size_t i = 0;
      size_t j = 0;
      for (;j < count && tmp[j] == ' '; j++)
	/* loop */;
      while (i < Length && j < count)
	{
	  if ((Buffer[i++] = tmp[j]) == ' ')
	    while (tmp[++j] == ' ');
	  else
	    j++;
	}
      Buffer[count = i] = '\0';
    }
  else
    Buffer[0] = '\0';

  if (count > Length)
    logf (LOG_PANIC, "%s::GetTerm: read too much", Doctype.c_str());

//cerr << "Got " << Buffer << endl;
//cerr << "Length = " << Length << endl;
//cerr << "count = " << count << endl;

  return count;
}

GPTYPE SGMLNORM::ParseWords(UCHR* DataBuffer, GPTYPE DataLength,
  GPTYPE DataOffset, GPTYPE* GpBuffer, GPTYPE GpLength)
{
  if (IgnoreTagWords) {
    const UCHR zapChar = ' ';
    int        zap = 0;
    int        quote = 0;
    UCHR       Ch;

//cerr << "XXXXXX Last Character = " << DataBuffer[DataLength] << endl;
//cerr << "       last to read   = " << DataBuffer[DataLength-1] << endl;
 
    for (register GPTYPE Position =0; Position < DataLength; Position++)
      {
        // Zap all tags but comments
        if (DataBuffer[Position] == '<' && (DataLength-Position) > 3 &&
          DataBuffer[Position+1] != '!' && DataBuffer[Position+2] != '-')
          zap=1;
        // Zap untill end of tag or '='
        if (zap || quote)
          {
            if ((Ch = DataBuffer[Position]) == '>') {
              zap = 0;
              quote = 0;
            } else if (quote && ((Ch == quote) || (quote == '=' && isspace(Ch)))) {
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
                quote = Ch;
              }
          }
        if (zap) DataBuffer[Position] = zapChar; // Zap these...
      }
    if (zap || quote) logf(LOG_WARN, "%s Record ended in a tag?", Doctype.c_str());
  }

  // Convert "&amp;xxx &lt;yyyy" to
  //         "&xxxx    <yyyy   "
  Entities.normalize((char *)DataBuffer, DataLength);

  // Now parse the words
  return DOCTYPE::ParseWords(DataBuffer, DataLength, DataOffset, GpBuffer, GpLength);
}


STRING SGMLNORM::UnifiedName (const STRING& Tag, PSTRING Value) const
{
  return DOCTYPE::UnifiedName(Tag, Value);
}

// To ignore a field return NULL
STRING SGMLNORM::UnifiedName (const STRING& Dtd, const STRING& Tag, PSTRING Value) const
{
  if (tagRegistry && Dtd.GetLength())
    {
      /* Look up config file for DTD */;
      tagRegistry->ProfileGetString(Dtd, Tag, "???", Value);
      if (!Value->Equals("???"))
	return *Value;
    }
  // Now Map this field to ...
  return UnifiedName(Tag, Value);
}

void SGMLNORM::ExtractDTD(const STRING& Decl, PSTRING Dtd) const
{
  *Dtd = Decl;
  // <!DOCTYPE greeting SYSTEM "hello.dtd">
  STRINGINDEX n;
  if ((n = Dtd->Search(" SYSTEM ")) > 2 
	|| (n = Dtd->Search(" PUBLIC ")) > 2)
    {
      // <!DOCTYPE play PUBLIC "-//Free Text Project//DTD Play//EN">
      STRINGINDEX m = Dtd->GetLength();
      //doctype = DTD.SubString(1, n - 1);
      if (Dtd->GetChr(m) == '>') m--;
      if (Dtd->GetChr(m) == '"') m--;
      else if (Dtd->GetChr(m) == '\'') m--;
      if (Dtd->GetChr(n+8) == '"') n++;
      else if (Dtd->GetChr(n+8) == '\'') n++;
      Dtd->EraseAfter(m); // remove >
      Dtd->EraseBefore(n+8);
      if (tagRegistry)
	{
	  STRING S = *Dtd;
	  tagRegistry->ProfileGetString("Catalog", S, S, Dtd);
	}
      logf (LOG_DEBUG, "DTD defined as %s", Dtd->c_str());
    }
}


void SGMLNORM::ParseFields (PRECORD NewRecord)
{
  if (NewRecord == NULL) return; // Error

  // Open the file
  const STRING fn = NewRecord->GetFullFileName ();
  FILE  *fp = ffopen(fn, "rb");

  if (fp == NULL)
    {
      logf (LOG_ERRNO, "Couldn't access '%s'", fn.c_str());
      NewRecord->SetBadRecord();
      return;			// ERROR
    }
  SRCH_DATE Datum;

  off_t RecStart = NewRecord->GetRecordStart ();
  off_t RecEnd = NewRecord->GetRecordEnd ();
  if (RecEnd == 0)
    RecEnd = GetFileSize(fp);

  if (RecEnd <= RecStart)
    {
      logf (LOG_WARN, "zero-length %s record - '%s' [%u - %u]",
	Doctype.c_str(), fn.c_str(),
	(unsigned)RecStart, (unsigned)RecEnd );
      logf (LOG_NOTICE, "%s skipping record", Doctype.c_str());
      ffclose(fp);
      NewRecord->SetBadRecord();
      return; // ERR
    }

  if (RecStart && -1 == fseek (fp, RecStart, SEEK_SET))
    {
      logf (LOG_ERRNO, "%s::ParseRecords(): Seek to %ld failed - '%s'",
	Doctype.c_str(), (long)RecStart, fn.c_str());
      ffclose(fp);
      NewRecord->SetBadRecord();
      return; // ERR
    }
  // Read the whole record in a buffer
  off_t RecLength = RecEnd - RecStart + 1;
  PCHR RecBuffer = (PCHR)tmpBuffer.Want(RecLength + 1);
  off_t ActualLength = fread (RecBuffer, sizeof(CHR), RecLength, fp);
  RecBuffer[ActualLength] = '\0';	// ASCIIZ

  ffclose (fp);

  PCHR *tags = parse_tags (RecBuffer, ActualLength);
  if (tags == NULL)
    {
      logf (LOG_WARN,
#ifdef _WIN32
	"Unable to parse `%s' tags in file %s[%I64d-%I64d]. No tags?" 
#else
	"Unable to parse `%s' tags in file %s[%lld-%lld]. No tags?"
#endif
	,Doctype.c_str(), fn.c_str(), (long long)RecStart, (long long)RecEnd );
      NewRecord->SetBadRecord();
      return;
    }

  STRING FieldName, Key;
  DF df;
  DFD dfd;

  PDFT pdft = new DFT ();
  DTD.Empty(); // Unknown DTD
  int tagCount = 0;
  for (PCHR * tags_ptr = tags; *tags_ptr; tags_ptr++)
    {
// cerr << "TAG = " << *tags_ptr << endl;
      if ((*tags_ptr)[0] == '/')
	continue; // End tag
      if ((*tags_ptr)[0] == '?')
	{
//cerr << "DEBUG: ignore <?.." << endl;
	  continue; // Ignore <?...?> for now
	}

      const size_t tag_len = strlen (*tags_ptr);
      // XML empty element?
      if (isalnum((*tags_ptr)[0]) && (*tags_ptr)[tag_len - 1] == '/')
	{
	  if (strchr (*tags_ptr, '='))
	    {
	      store_attributes (pdft, RecBuffer, *tags_ptr, GDT_FALSE, &Key, &Datum);
	    }
	  continue; // No content
	}

      if ((*tags_ptr)[0] == '!') {
	PCHR p = *tags_ptr;
	size_t val_start =  (*tags_ptr - 1) - RecBuffer;
	size_t val_end = val_start + tag_len + 1;

	// Now cut at the !DOCTYPE, !ENTITY,...
	while (*p && !isspace(*p)) p++;
	*p = '\0';
	FieldName = *tags_ptr;
	if (FieldName == "!DOCTYPE")
	  {
	    ExtractDTD(p+1, &DTD);
	  }
	dfd.SetFieldName (FieldName);
	Db->DfdtAddEntry (dfd);
	df.SetFct ( FC(val_start, val_end) );
	df.SetFieldName (FieldName);
	pdft->AddEntry (df);
	continue;
      }
      if (++tagCount == 1)
	{
#if RSS_HACK
	  if (strncmp(*tags_ptr, rss_magic, sizeof(rss_magic)-1) == 0)
	    {
	      logf (LOG_WARN, "%s: Identified %s instead as RSS!", Doctype.c_str(), fn.c_str() );
	      NewRecord->SetDocumentType(rss_doctype);
	      Db->ParseFields (NewRecord);
	      return;
	    }
#endif
	}

      const PCHR p = find_end_tag ((const char *const *)tags_ptr, (const char *)*tags_ptr);
      GDT_BOOLEAN have_attribute_val = (NULL != strchr (*tags_ptr, '='));

      if (p != NULL)
	{
#if 1
	  if (have_attribute_val)
	    {
	      STRING MetaField(".");
	      dfd.SetFieldName (MetaField);
	      Db->DfdtAddEntry (dfd);
	      df.SetFct ( FC(*tags_ptr - RecBuffer,  p - RecBuffer + strlen(p)) );
	      df.SetFieldName (MetaField);
	      pdft->AddEntry (df);
	    }
#endif

	  // We have a tag pair
	  size_t val_start = (*tags_ptr + tag_len + 1) - RecBuffer;
	  int val_len = (p - *tags_ptr) - tag_len - 2;

	  // Shorthand?
	  if (p[1] == '\0' && RecBuffer[val_start + val_len] != '<')
	    {
	      val_len++; // Yes
	    }
	  // Skip leading white space
	  // BUGFIX: edz Sun Jan 10 16:47:57 MET 1999 
	  while (isspace (RecBuffer[val_start]) && RecBuffer[val_start] != '\240' /* &nbsp */)
	    val_start++, val_len--;
	  // Leave off trailing white space
	  while (val_len > 0 && isspace (RecBuffer[val_start + val_len - 1]))
	    val_len--;

	  // Don't bother storing empty fields
	  if (val_len > 0)
	    {
	      // Cut the complex values from field name
	      CHR orig_char = 0;
	      char* tcp;
	      for (tcp = *tags_ptr; *tcp; tcp++)
		if (isspace (*tcp))
		  {
		    orig_char = *tcp;
		    *tcp = '\0';
		    break;
		  }

#if ACCEPT_SGML_EMPTY_TAGS
	      if (*tags_ptr[0] == '\0')
		{
		  // Tag name is the name of the last element
		  if (FieldName.IsEmpty())
		    {
		      // Give some information
		      logf (LOG_INFO, "%s Warning: \"%s\"(%u): Bad use of empty tag feature, skipping field.",
			Doctype.c_str(), fn.c_str(), (unsigned)(*tags_ptr - RecBuffer) );
		      continue;
		    }
		}
	      else
		{
#endif
		  if (UnifiedName(DTD, *tags_ptr, &FieldName).IsEmpty())
		    continue; // ignore these
//		  FieldName.ToUpper(); // Store SGML tags uppercase (not needed yet)
#if ACCEPT_SGML_EMPTY_TAGS 
		}
#endif
	      if (orig_char)
		*tcp = orig_char;

	      dfd.SetFieldName (FieldName);
	      Db->DfdtAddEntry (dfd);
	      df.SetFct ( FC(val_start, val_start + val_len - 1) );
	      df.SetFieldName (FieldName);
	      pdft->AddEntry (df);
//	      logf (LOG_DEBUG, "%s processing %s", Doctype.c_str(), FieldName.c_str());
	      // Our index-time Headline field
	      if (FieldName ^= Headline)
		{
		  // Clone into field infomation
		  FieldName = "Headline";
		  dfd.SetFieldName (FieldName);
		  Db->DfdtAddEntry (dfd);
		  df.SetFieldName (FieldName);
		  pdft->AddEntry (df);
		}
	      if (IsSpecialField(FieldName))
	        {
		  char *entry_id = (char *)tmpBuffer2.Want(val_len + 1);
		  strncpy (entry_id, &RecBuffer[val_start], val_len);
		  entry_id[val_len] = '\0';
		  HandleSpecialFields(NewRecord, FieldName, entry_id);
		  if (FieldName ^= KeyField)
		    Key = NewRecord->GetKey(); // fetch the key
		}
	    }
	}

      if (have_attribute_val)
	{
	  store_attributes (pdft, RecBuffer, *tags_ptr, GDT_FALSE, &Key, &Datum);
	}
      else if (p == NULL)
	{
	  // Give some information
	  // Is this already happened?
	  if (EmptyTagList.Search( tags_ptr[0] ) == 0)
	    {
	      // No? Have we already seen a well-founded
	      // use of this tag?
	      if (Db->GetMainDfdt()->GetFileNumber(tags_ptr[0]) > 0)
		{
		  // Looks like a markup error!
		  logf (LOG_WARN, "%s: \"%s\"(%u): \
Missing end tag for <%s>, SHORTREF? skipping field.",
			Doctype.c_str(), fn.c_str(),
			(unsigned)(*tags_ptr - RecBuffer), tags_ptr[0] );
		}
	      else
		{
		  // Assume that its an EMPTY element.
		  logf (LOG_INFO, "%s: \"%s\"(%u): \
No end tag for <%s> found, EMPTY or SHORTREF? skipping field.",
			Doctype.c_str(), fn.c_str(),
			(unsigned)(*tags_ptr - RecBuffer), tags_ptr[0] );
		  EmptyTagList.AddEntry( tags_ptr[0] );
		}
	    }
	}
    }

  NewRecord->SetDft (*pdft);
  if (Datum.Ok()) NewRecord->SetDate( Datum );
  
  if (Key.IsEmpty())
    {
      if (ActualLength > 128)
	{
	  Key = OneWayHash(RecBuffer, ActualLength);
	  logf (LOG_DEBUG, "OneWay Hash of Record content = %s", Key.c_str());
	  RESULT Result;
	  if (Db->KeyLookup (Key, &Result))
	    {
	      // Check if its the same...
	      if (fn == Result.GetFullFileName() && RecStart == Result.GetRecordStart()) 
		{
		   // Already in database
		   logf (LOG_WARN, "'%s'[%ld-%ld] already in index.", fn.c_str(),
			RecStart, RecEnd);
		   NewRecord->SetBadRecord(GDT_TRUE);
		}
	    }
	  else
	    NewRecord->SetKey (Key);
	}
    }
  else
    {
      if (Db->KeyLookup (Key))
	{
	  logf (LOG_WARN, "Record in \"%s\" used a non-unique %s '%s'",
		fn.c_str(), KeyField.c_str(), Key.c_str());
	  Db->MdtSetUniqueKey(NewRecord, Key);
	}
      else
	NewRecord->SetKey (Key);
    }
  // Clean up;
  delete pdft;
}

void SGMLNORM::XmlMetaPresent(const RESULT &ResultRecord, const STRING &RecordSyntax,
   STRING *StringBuffer) const
{
  DocPresent (ResultRecord, SOURCE_MAGIC, RecordSyntax, StringBuffer);
}

void SGMLNORM::DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const
{
  if (ElementSet.Equals(SOURCE_MAGIC) || ElementSet.Equals(FULLTEXT_MAGIC))
    {
      Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
    }
  else
    {
      DOCTYPE::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
    }
}


void SGMLNORM::Present (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const
{
//cerr << "PRESENT " << ElementSet << endl;
  StringBuffer->Clear();
  if (ElementSet.Equals(BRIEF_MAGIC))
    {
//cerr << "XXXXXXX Present " << ElementSet << " for " << Doctype << endl; 
      // Have we specified a headline format?
      if (tagRegistry)
	{
	  STRING dtd, DTD, HeadlineFormat;
	  Present(ResultRecord, "!DOCTYPE", NulString, &dtd);
	  ExtractDTD(dtd, &DTD);
	  if (DTD.IsEmpty())
	    DTD = "*";
	  tagRegistry->ProfileGetString("HEADLINE", DTD, NulString, &HeadlineFormat);
	  if (HeadlineFormat.GetLength() &&
		DOCTYPE::Headline(HeadlineFormat, ResultRecord, RecordSyntax, StringBuffer))
	    return; // Done
	}
      // Try to see if we have a headline or title tag
      DFDT Dfdt;
      STRING Key;
      ResultRecord.GetKey(&Key);
      Db->GetRecordDfdt (Key, &Dfdt);
      const size_t Total = Dfdt.GetTotalEntries();

      DFD Dfd;
      STRING FieldName, HeadlineElement;
      // Search for title
      enum {Look, Start, Guess, Found} State = Look;

      logf (LOG_DEBUG, "Looking for Headline");
      for (size_t i = 1; State != Found && i <= Total; i++) {
	Dfdt.GetEntry (i, &Dfd);
	Dfd.GetFieldName (&FieldName);
	if ((FieldName ^= "TITLE") || (FieldName ^= "HEADLINE"))
	  {
	    HeadlineElement = FieldName;
	    logf (LOG_DEBUG, "Found %s", FieldName.c_str());
	    State = Found; // Got it 
	  }
	else if (State != Guess && FieldName.GetChr(1) != '!')
	  {
	    if (State == Start && FieldName.Search(ATTRIB_SEP) == 0)
	      {
		HeadlineElement = FieldName; // Second-level tag
		logf (LOG_DEBUG, "Using %s (Second Level)", FieldName.c_str());
		State = Guess; // If nothing else use this
	      }
	    else
	      {
		State = Start; 
	      }
	  }
      }		// for()
      if (State == Guess)
	State = Found; // Will use the guess
      if (State == Found)
	{
	  // Can have multiple headline fields? Yuck!
	  // -- so go directly to the source
	  STRLIST Data;
	  logf (LOG_DEBUG, "GetFieldData for %s", HeadlineElement.c_str());
	  Db->GetFieldData (ResultRecord, HeadlineElement, &Data);
	  if (!Data.IsEmpty())
	    {
	      STRING Entry;
	      size_t Count =  Data.GetTotalEntries();
	      logf (LOG_DEBUG, "Got %d elements", Count);
	      // Go hunting for content
	      for (size_t i=1; i <= Count; i++)
		{
		  Data.GetEntry(i, &Entry);
		  Entry.XMLCommentStrip(); // Zap <!-- --> stuff
		  Entry.Pack ();
		  if (!Entry.IsEmpty())
		    break;
		}
	      if (RecordSyntax == HtmlRecordSyntax)
		HtmlCat(ResultRecord, Entry, StringBuffer);
	      else
		StringBuffer->Cat(Entry);
	    }
	  else
	    logf (LOG_DEBUG, "Got nothing");
	}
      else
	{
	  // Ask your parents for advice!
	  logf (LOG_DEBUG, "Passing to DOCTYPE::Present for %s", ElementSet.c_str());
	  DOCTYPE::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
	}
    }
  else if ((ElementSet == SOURCE_MAGIC) || (RecordSyntax == SgmlRecordSyntax))
    {
      STRING Value;
      DOCTYPE::Present (ResultRecord, ElementSet, &Value); 
      // Preface with HTTP content
      if (RecordSyntax == HtmlRecordSyntax)
	{
	  STRING mime;
	  SourceMIMEContent(ResultRecord, &mime);
          *StringBuffer << "Content-type: " << mime << "\n\n";
	}
      *StringBuffer << Value;
    }
  else if (ElementSet.Equals (FULLTEXT_MAGIC))
    {
      if ((RecordSyntax == HtmlRecordSyntax) ||
	  (RecordSyntax == SutrsRecordSyntax) ||
	  (RecordSyntax == UsmarcRecordSyntax) )
	{
	  STRING Fn, Path;
	  ResultRecord.GetPath(&Path);
	  ResultRecord.GetFileName(&Fn);
	  STRINGINDEX pos = Fn.SearchReverse('.');
	  if (pos) Fn.EraseAfter(pos - 1);
	  STRING Fullpath, mime; 
	  const char * const *exts = NULL;
	  if (RecordSyntax == HtmlRecordSyntax)
	    {
	      static const char *_html[] =
		{".htm", ".html", ".HTM", NULL};
	      exts = _html;
	      mime = "text/html";
	    }
	  else if (RecordSyntax == UsmarcRecordSyntax)
	    {
	      static const char *_usmarc[] =
		{".usmarc", ".marc", ".mrc", ".MRC", NULL};
	      exts = _usmarc;
	      mime = "application/usmarc";
	    }
	  else // Default
	    {
	      // SutrsRecordSyntax
	      static const char *_sutrs[] =
		{".sut", ".sutrs", ".txt", ".ascii", ".SUT", ".TXT", NULL};
	      exts = _sutrs;
	      mime = "text/plain";
	    }
	  // Now look for the preformated document
	  for (int i=0; exts[i]; i++)
	    {
	      Fullpath = Path + Fn + exts[i];
	      if (FileExists (Fullpath) )
		break;
	    }
	  if (FileExists (Fullpath))
	    {
	      STRING scratch;
	      scratch.ReadFile(Fullpath);
	      *StringBuffer << "Content-type: " << mime << "\n\n";
	      StringBuffer->Cat (scratch);
	    }
	  else
	    {
	      // Could spawn a process to convert to desired record
	      // syntax and store the object for the next try...
	      Present (ResultRecord, SOURCE_MAGIC, RecordSyntax, StringBuffer);
	    }
	} else
	  Present (ResultRecord, SOURCE_MAGIC, RecordSyntax, StringBuffer);
    }
  else
    {
      DOCTYPE::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer); 
      if (StringBuffer) StringBuffer->XMLCommentStrip();
    }
}

SGMLNORM::~SGMLNORM ()
{
#if 0
  // Debug stuff to see that the buffers were OK
  tmpBuffer.Free(Doctype.c_str(), "tmpBuffer");
  tmpBuffer2.Free(Doctype.c_str(), "tmpBuffer2");
  tagsBuffer.Free(Doctype.c_str(), "tagsBuffer");
#endif

}

/*----------------------------------------------------------------------------------*/
/*------------- Support Functions used by both SGMLTAG and HTML classes ------------*/
/*----------------------------------------------------------------------------------*/
#if STORE_SCHEMAS
// HTML META Support Function
//
// Cougar DTD defines:
//
// <!ENTITY % i18n
// "lang        NAME       #IMPLIED  -- RFC 1766 language value --
//  dir         (ltr|rtl)  #IMPLIED  -- default directionality --"
//  >
//
// <!ATTLIST META
//   %i18n;                      -- lang, dir for use with content string --
//   http-equiv NAME   #IMPLIED  -- HTTP response header name  --
//   name       NAME   #IMPLIED  -- metainformation name --
//   content    CDATA  #REQUIRED -- associated information --
//   scheme     CDATA  #IMPLIED  -- select form of content --
//   >
//
// But current DTD requires it be placed in CONTENT..
//
// We don't yet map LANG=XXX to (Lang=XXX) 
//
// Only the keywords Type, Schema and Schema are supported.
// (Type=XXX) --> .XXX
// (Schema=XXX) --> (XXX)
// Multiple Type and Schema are handled.
// (Type=XXX)(Type=YYY) --> .XXX.YYY
// (Schema=XXX)(Schema=YYY) --> (XXX.YYY)
// (Type=XXX)(Schema=YYY) --> .XXX(YYY)
// The schema is tacked on last so: (Type=X)(Schema=Y) == (Schema=Y)(Type=X)
//
// returns TRUE if had a Type or Schema
//
static GDT_BOOLEAN grabSchema(PSTRING SchemeBuffer, PSTRING TypBuffer,
	PCHR val)
{
  GDT_BOOLEAN result = GDT_FALSE;
  // TODO: Make this more modular for other keywords
  PCHR tcp = val;
  if (*tcp == '\'' || *tcp == '"') tcp++;
  // Skip leading white space
  while (isspace(*tcp)) tcp++;
  while (*tcp == '(')
    {
      char tmp[32]; // We limit this (arbitrary) to 32 characters..
      // Skip white space..
      do {
	tcp++;
      } while (isspace(*tcp));
      // Look for keyword...
      INT haveS = 2; // strlen("Schema") - strlen("Type")
      // Recognize "Schema" and "Type"
      if (StrNCaseCmp(tcp, "Schema", 6) == 0 ||
	  StrNCaseCmp(tcp, "Scheme", 6) == 0 ||
	  StrNCaseCmp(tcp, "Unit", 4)   == 0 ||
	  (haveS=StrNCaseCmp(tcp, "Type", 4)) == 0)
	{
	  // (Schema=RFC822) or (Type=Person)
	  tcp += (4 + haveS);
	  while (isspace(*tcp)) tcp++;
	  if (*tcp=='=')
	    {
	      size_t i = 0;
	      tcp++;
/* Tag names can be Letter | Digit | '.' | '-' | '_' | ':' | */

#define isTAGC(_c) (isalnum(_c) || (_c) == '.' || (_c) == '-' || (_c) == '_' || (_c) == ':')
	      while (isTAGC(*tcp) && i<sizeof(tmp)/sizeof(char))
		{
		  tmp[i++] = *tcp++;
		}
#undef isTAGC
	      if (i)
		{
		  // Have something...
		  result = GDT_TRUE;
		  tmp[i] = '\0';
		  if (haveS)
		    {
		      // Multiple Schema?
		      if (SchemeBuffer->GetLength())
			SchemeBuffer->Cat (".");
		      SchemeBuffer->Cat (tmp);
		    }
		  else
		    {
		      // Type means .TYPE
		      TypBuffer->Cat (".");
		      TypBuffer->Cat (tmp);
		    }
		}
	    }		/* *tcp == '=' */
	}	/* have a "registered" keyword */
      else
	{
	  if (StrNCaseCmp(tcp, "PICS", 4))
	    {
	      char *tp = strrchr(tcp, '=');
	      if (tp)
		{
		  *tp = '\0';
		  logf(LOG_INFO, "Unknown keyword '%s' in attribute: '(%s'", tcp, tp+1);
		  *tp = '='; // Fix back
		}
	      else
		logf(LOG_INFO, "Non-supported attribute scheme: '(%s'", tcp);
	    }
	  break;
	}
      // Scan to end..
      while (*tcp && *tcp != ')') tcp++;
      // Skip ')'
      if (*tcp) tcp++;
      // Scan past white space..
      while (isspace(*tcp)) tcp++;
      // Looking at next '('?..
    }		/* while() */
  return result;
}
// Get val from " val  "
static void sVal(PSTRING StringBuffer, PCHR val)
{
  PCHR tcp = val, tp;
  if (*tcp == '\'' || *tcp == '"') tcp++;
  // Skip leading white space
  while (isspace(*tcp)) tcp++;
  tp = tcp + strlen(tcp) - 1;
  if (*tp == '\'' || *tp == '"') *tp-- = '\0';
  while (isspace(*tp) && tp > tcp) *tp-- = '\0';
  if (*tcp)
    *StringBuffer =  tcp;
}
#endif          /* STORE_SCHEMAS */

/* Collect all the <tag foo=bar ..> stuff as tag@foo ...  */

void SGMLNORM::store_attributes (PDFT pdft, PCHR base_ptr, PCHR tag_ptr,
  GDT_BOOLEAN UseHTML, STRING* Key, SRCH_DATE *Datum) const
{
  const GDT_BOOLEAN storeIt = StoreComplexAttributes ? StoreComplexAttributes :
		StoreTagComplexAttributes(tag_ptr);

  //
  // hook here to allow for people, resp. doctypes, to define
  // on a tag-by-tag basis what might be allowed to be stored as complex..
  // example: in RSS one might want <A HREF=..> and <IMG SRC=".."> stuff to
  // be stored (even if it does not really belong in the standard).

  if (!storeIt && (Key == NULL || KeyField.IsEmpty()) && (Datum == NULL || DateField.IsEmpty()))
    return; // Don't store for me..


  const CHR Sep = ATTRIB_SEP;	// Magic sep. tag:attribute

  PCHR name, val, attribute;
  enum STATES
    {
      INIT, OK, COLLECT, QUOTE, SQUOTE
    }
  State = INIT, OldState = INIT;
  size_t val_start, val_end;

  if (tag_ptr == NULL)
    return;

  // Skip Leading Space
  while (isspace (*tag_ptr))
    tag_ptr++;
  name = tag_ptr;
  if (!isalpha (*name))
    return;			// Not NAMEFIRST character

  val = attribute = NULL;

  DFD dfd;
  DF df;
  STRING FieldName;

#if 1
  STRING MetaName;
  STRING Schema, Typ;
  FC ContentFC;
  GDT_BOOLEAN HaveContent = GDT_FALSE;
  GDT_BOOLEAN sawSchema = GDT_FALSE;
#endif

  while (*tag_ptr)
    {
      if (*tag_ptr == '"')
	{
	  if (State == QUOTE)
	    {
	      State = OldState;
	    }
	  else
	    {
	      OldState = State;
	      State = QUOTE;
	    }
	}
      else if (*tag_ptr == '\'')
	{
	  if (State == SQUOTE)
	    {
	      State = OldState;
	    }
	  else
	    {
	      OldState = State;
	      State = SQUOTE;
	    }
	}
      else if (*tag_ptr == '=')
	{
	  if (State == OK)
	    {
	      *tag_ptr++ = '\0';
	      State = COLLECT;
	      /* Skip White space */
	      while (isspace (*tag_ptr))
		tag_ptr++;
	      val = tag_ptr--;	// Backup

	    }
	}
      else if (isspace (*tag_ptr))
	{
	  if (State == INIT)
	    {
	      // Store whole bit
	      *tag_ptr++ = '\0';
	      State = OK;
	      val_start = name - base_ptr - 1;
              val_end = val_start + tag_ptr - name + strlen (tag_ptr) + 1;
	      const FC fc(val_start, val_end);
	      FieldName = name;
	      FieldName.Cat (Sep);
//	      FieldName.ToUpper(); // Store SGML tags uppercase (not needed yet)
	      STRING Tmp;

	      if (storeIt && UnifiedName(DTD, FieldName, &Tmp).GetLength())
		{
		  FieldName = Tmp;
		  dfd.SetFieldName (FieldName);
		  Db->DfdtAddEntry (dfd);
		  df.SetFct (fc);
		  df.SetFieldName (FieldName);
		  pdft->AddEntry (df);
		}
	    }
	  else if (State == COLLECT)
	    {
	      const int off = (*val == '"' || *val == '\'') ? 1 : 0;
	      *tag_ptr++ = '\0';
	      State = OK;
	      val_start = val - base_ptr + off;
	      val_end = val - base_ptr + strlen (val) - 1 - off;
	      if (off)
		base_ptr[val_end + 1] = '\0';
	      const FC fc(val_start, val_end);
	      FieldName = name;
	      FieldName.Cat (Sep);
	      FieldName.Cat (attribute);
//	      FieldName.ToUpper(); // Store SGML tags uppercase (not needed yet)
	      STRING Tmp;
	      if (storeIt && UnifiedName(DTD, FieldName, &Tmp).GetLength())
		{
		  FieldName = Tmp;
		  dfd.SetFieldName (FieldName);
		  Db->DfdtAddEntry (dfd);
		  df.SetFct (fc);
		  df.SetFieldName (FieldName);
		  pdft->AddEntry (df);
		}
#if 1 /* Example: Reuters's large collection */
	   if (StrCaseCmp(attribute, "value") == 0)
	      {
		// <DC ELEMENT="..." VALUE="...">
		ContentFC = fc;
		HaveContent = GDT_TRUE;
	      }
	   else if (StrCaseCmp(attribute, "element") == 0)
	     {
		PCHR tcp = val;
		CHR quot = 0;
		while (!isalnum(*tcp)) {
		  if (quot == 0 && (*tcp == '"' || *tcp == '\''))
		    quot = *tcp;
		  tcp++;
		}
		MetaName.Clear();
		// Need to handle non-quoted
		while (*tcp && (*tcp != quot || (quot == 0 && !isspace(*tcp))))
		  MetaName.Cat ( *tcp++ );
	     }
	  else
#endif
#if 1    
	   if (UseHTML) {
	      if (StrCaseCmp(attribute, "content") == 0)
		{
		  // <META CONTENT="..." NAME="...">
		  ContentFC = fc;
		  HaveContent = GDT_TRUE;
#if STORE_SCHEMAS
		  if (!sawSchema)
		    {
		      grabSchema(&Schema, &Typ, val);
		    }
#endif
		}
	      else if (StrCaseCmp(attribute, "dir") == 0)
		{
		  //  dir (ltr|rtl)  #IMPLIED  -- default directionality --
		  PCHR tcp = val;
		  while (isspace(*tcp)) tcp++;
		  if (*tcp == '\'' || *tcp == '"')
		    tcp++;
		  while (isspace(*tcp)) tcp++;
		  if (*tcp != 'r' && *tcp != 'R' && *tcp!='\'' &&
			*tcp != '"')
		    {
		      logf(LOG_WARN, "\
Sorry can only handle %s='%s' and not %s, ignoring",
			attribute, "RTL", val);
		    }
		}
	      else if (StrCaseCmp(attribute, "scheme") == 0)
		{
		  sawSchema = GDT_TRUE;
		  // Experimental Cougar DTD from W3O
		  if (Schema.GetLength())
		    logf(LOG_WARN, "W30 Cougar/4.x DTD detected, \
Scheme overridding (Scheme=..) content");
		  sVal(&Schema, val);
		}
	      else if (StrCaseCmp(attribute, "name") == 0 ||
			StrCaseCmp(attribute, "http-equiv") == 0)
		{
		  PCHR tcp = val;
		  CHR quot = 0;
		  while (!isalnum(*tcp))
		    {
		      if (quot == 0 && (*tcp == '"' || *tcp == '\''))
			quot = *tcp;
		      tcp++;
		    }
		  // Need to handle non-quoted
		  while (*tcp && (*tcp != quot || (quot == 0 && !isspace(*tcp))))
		    MetaName.Cat ( *tcp++ );
		}
	      }
#endif
	    }
	  /* Skip Repeated White space */
	  while (isspace (*tag_ptr))
	    tag_ptr++;
	  if (State != QUOTE && State != SQUOTE)
	    val = attribute = tag_ptr--;
	}
      tag_ptr++;
    }				/* while (*tag_ptr) */
  /* Rest attribute/value pair */
  if (attribute && attribute != val)
    {
      const int off = (*val == '"' || *val == '\'') ? 1 : 0;

      val_start = val - base_ptr + off;
      val_end =  val - base_ptr + strlen (val) - 1 - off;
      const FC fc(val_start, val_end);
      if (off)
	base_ptr[val_end+1] = '\0';

      FieldName = name;
      FieldName.Cat (Sep);
      FieldName.Cat (attribute);
//    FieldName.ToUpper(); // Store SGML tags uppercase (not needed yet)

      STRING Tmp;
      if (storeIt && UnifiedName(DTD, FieldName, &Tmp).GetLength() != 0)
	{
	  FieldName = Tmp;
	  dfd.SetFieldName (FieldName);
	  Db->DfdtAddEntry (dfd);
	  df.SetFct (fc);
	  df.SetFieldName (FieldName);
	  pdft->AddEntry (df);
	}


#if 1 /* Example: Reuters's large collection */
	   ContentFC = fc;
           if (StrCaseCmp(attribute, "value") == 0)
              {
                // <DC ELEMENT="..." VALUE="...">
                ContentFC = fc;
                HaveContent = GDT_TRUE;
              }
           else if (StrCaseCmp(attribute, "element") == 0)
             {
                PCHR tcp = val;
                CHR quot = 0;
                while (!isalnum(*tcp)) {
                  if (quot == 0 && (*tcp == '"' || *tcp == '\''))
                    quot = *tcp;
                  tcp++;
                }
                MetaName.Clear();
                // Need to handle non-quoted
                while (*tcp && (*tcp != quot || (quot == 0 && !isspace(*tcp))))
                  MetaName.Cat ( *tcp++ );
             }
          else
#endif   


#if 1
   if (UseHTML) {
      if (StrCaseCmp(attribute, "content") == 0) 
	{
	  // <META NAME="..." CONTENT="...">
	  // <DC   ELEMENT="..." VALUE="...">

	  // Store coordinates
	  ContentFC = fc;
	  HaveContent = GDT_TRUE;
#if STORE_SCHEMAS
	  if (!sawSchema)
	    {
	      grabSchema(&Schema, &Typ, val);
	    }
#endif		/* STORE_SCHEMAS */
	}
      else if (StrCaseCmp(attribute, "scheme") == 0)
	{
	  sawSchema = GDT_TRUE;
	  // Experimental Cougar DTD from W3O
	  if (Schema.GetLength())
	    logf(LOG_WARN, "W30 Cougar DTD/4.x detected, Scheme overridding (Scheme=..) content");
	  sVal(&Schema, val);
	}
      else if (StrCaseCmp(attribute, "name") == 0 ||
	       StrCaseCmp(attribute, "http-equiv") == 0)
	{
	  PCHR tcp = val;
	  CHR quot = 0;
	  while (!isalnum(*tcp))
	    {
	      if (quot == 0 && (*tcp == '"' || *tcp == '\''))
		quot = *tcp;
	      tcp++;
	    }
	  // Need to handle non-quoted
	  while (*tcp && (*tcp != quot || (quot == 0 && !isspace(*tcp))))
	    MetaName.Cat ( *tcp++ ); 
	} 
      }
#endif
    }

#if 1
  if (HaveContent && (!MetaName.IsEmpty()))
    {
      PCHR tcp;
      FieldName.Clear();
      // While in HTML both NAME= and HTTP-EQUIV are ways
      // to name a meta-object they are different!
      // We store <META XXX-equiv="Date" .. under DATE
      // but <META NAME="Date" goes under META.DATE
      // The Exception to the rule is Keywords since this
      // should always be NAME= but some use HTTP-EQUIV..
      if ((tcp = strchr(attribute, '-')) == NULL ||
		StrCaseCmp(tcp, "-equiv") != 0 ||
		(MetaName ^= "Keywords"))
	{
	  FieldName = name;
	  FieldName.Cat (".");
	}
      FieldName.Cat (MetaName);
//    FieldName.ToUpper(); // Store SGML tags uppercase (not needed yet)

      STRING fieldName;

      if (UnifiedName (DTD, FieldName, &fieldName).GetLength())
	{
	  if (Key && (fieldName ^= KeyField))
	    {
	      if (Key)
		*Key = base_ptr+ContentFC.GetFieldStart();
	    }
	  if (Datum && (fieldName ^= DateField))
	    {
	      const char *ptr    = base_ptr+ContentFC.GetFieldStart();
	      if (Datum->Ok())
		{
		  logf (LOG_DEBUG, "Date field (%s) override with: '%s'", fieldName.c_str(), ptr);
		}
	      *Datum = ptr;
	      if (!Datum->Ok())
		logf (LOG_WARN, "Unsupported/Unrecognized date format: '%s'", ptr);
	    }
	  if (storeIt)
	    {
	      dfd.SetFieldType(  Db->GetFieldType(fieldName) ); // Get/Set the type   
	      dfd.SetFieldName (fieldName);
	      Db->DfdtAddEntry (dfd);
	      df.SetFct (ContentFC);
	      df.SetFieldName (fieldName);
	      pdft->AddEntry (df);
	    }
	}
      if (Typ.GetLength() || Schema.GetLength())
	{
	  // Add Typ
	  if (Typ.GetLength())
	    {
	      FieldName.Cat (Typ);
	    }
	  // The (Schema=XXX) goes at the end...
	  if (Schema.GetLength())
	    {
	      FieldName.Cat ("(");
	      FieldName.Cat (Schema);
	      FieldName.Cat (")");
	    }
	  // Get mapping...
	  STRING Tmp;
	  if (storeIt && UnifiedName (DTD, FieldName, &Tmp).GetLength())
	    {
	      FieldName = Tmp;
	      // Clone into.. 
	      dfd.SetFieldName (FieldName);
	      Db->DfdtAddEntry (dfd);
	      df.SetFieldName (FieldName);
	      pdft->AddEntry (df);
	    }
	}
    }
   else if (Key && (FieldName ^= KeyField))
     {
	*Key = base_ptr + ContentFC.GetFieldStart();
     }
    else if (Datum && (FieldName ^= DateField))
     {
	const char *ptr =  base_ptr+ContentFC.GetFieldStart();
	SRCH_DATE  newDate ( ptr );
	if (newDate.Ok())
	  {
	    if (Datum->Ok())
	      logf (LOG_DEBUG, "Date field (%s) override with: '%s'", FieldName.c_str(), newDate.RFCdate().c_str());
	    *Datum = newDate;
	   }
	else if (*ptr)
	  logf (LOG_WARN, "Unsupported/Unrecognized  date format '%s' in field %s", ptr, FieldName.c_str());
     }
#endif
}


/*
   What:        Given a buffer of sgml-tagged data:
   It searches for "tags" and
   (1) returns a list of char* to all characters immediately
   following each '<' character in the buffer.

   Pre: b = character buffer with valid sgml marked-up text
   len = length of b
   tags = Empty char**

   Post:        tags is filled with char pointers to first character of every sgml 
   tag (first character after the '<').  The tags array is 
   terminated by a NULL.
   Returns the total number of tags found or -1 if out of memory
 */
PCHR *SGMLNORM::parse_tags (register PCHR b, const off_t len)
{
#ifndef TAG_GROW_SIZE
#define TAG_GROW_SIZE 1022 
#endif
  const size_t growth = TAG_GROW_SIZE;
#undef TAG_GROW_SIZE
  size_t tc = 0;		// tag count
  size_t maxtags = growth;	// max num tags for which space is allocated
  signed short bracket = 0;	// Declaration bracket depth
  GDT_BOOLEAN InComment = GDT_FALSE; // In a comment?
  enum { OK, NEED_END, IN_DECL, QUOTE, SHORTHAND } State = OK; // State Info
  CHR qChar = 0; // Quote Character

  // Array of pointers to first char of tags 
  // You should allocate character pointers (to tags) as you need them.
  PCHR *t = (PCHR *)tagsBuffer.Want (maxtags+1, sizeof(PCHR *));
  if (t == NULL) return NULL; // Alloc failure

  // Step though every character in the buffer looking for '<' and '>'

//cerr << "Length = " << len << endl;
  for (register off_t i = 0; i < len; i++)
    {
      if (InComment && b[i] != '-')
	continue;
#if 0
 cerr << "b[" << i << "] = " << b[i] << " ";
 if (State == OK) cerr << "OK";
 if (State == NEED_END) cerr << "NEED_END";
 if (State == IN_DECL) cerr << "IN_DECL";
 if (State == QUOTE) cerr << "QUOTE";
 if (State == SHORTHAND) cerr << "SHORTHAND";
 cerr << endl;
#endif
      switch (b[i])
	{
	case 1: case 2: case 3: case 4: case 5: case 6:
	case 7: case 8: case 12: case 14: case 15: case 16:
	case 17: case 18: case 19: case 20: case 21: case 22:
	case 23: case 24: case 25: case 26: case 27: case 28:
	case 29: case 30: case 31: case 127:
	  logf (LOG_DEBUG, "%s Shunchar Control 0x%x encountered (pos=%ld)!",
	  Doctype.c_str(), b[i], i);
	  b[i] = ' ';
	  break;

	case '-':
	  if (State == NEED_END || State == IN_DECL)
	    {
	      if (b[i-1] == '-')
		{
		  if (State == IN_DECL && bracket > 1)
		    logf (LOG_INFO, "Ignoring comment in declaration");
		  else
		    InComment = !InComment;
		}
	    }
	  break;

	case '[':
	  if (State == IN_DECL)
	    bracket++;
	  break;
	case ']':
	  if (State == IN_DECL)
	    if (--bracket <= 0)
	      bracket = 0;
	  break;

#if 1 /* Kludge for some BAD HTML */
	case '=':
	  if (State == NEED_END && (i+2 < len))
	    {
	      // Kluge to handle some BAD html
	      // Can be ' or "
	      if (b[i+1] != '"' && b[i+1] != '\'' && !isspace(b[i+1]))
		{
		  // Should have been a " but ...
		  do {
		    i++;
		  } while (i < len && b[i+1] != '>' && !isspace(b[i+1]));
		}
	    }
	  break;
#endif

	case '"':
	case '\'':
	  if (State == NEED_END)
	    {
	      State = QUOTE;
	      qChar = b[i];
	    }
	  else if (State == QUOTE && b[i] == qChar)
	    {
//cerr << "Quote Ended " << qChar << endl;
	      State = NEED_END;
	      qChar = 0;
	    }
	  break;

	case '/':
	  if (State == SHORTHAND)
	    {
//cerr << "SHORTHAND '/' seen at tc = " << tc << endl;
	      State = NEED_END;
	      t[tc] = &b[i++];
	    }
	  else if (&b[i] - t[tc] < 2)
	    {
	      break;
	    }
	  if (State == IN_DECL) break;
	  // fall into..
	case '>':
	  if (State == NEED_END || (State == IN_DECL && bracket == 0))
	    {
// cerr << "t[" << tc << "] = \"" << t[tc] << "\" ";
	      if (b[i] == '/' && b[i-1] == '\0')
		State = SHORTHAND;
	      else
		State = OK;
// cerr << "b[i] = " << b[i];
	      if (b[i] == '/') i++;
	      b[i] = '\0';
// cerr << ", TAG = " << t[tc] << endl;
	      // Expand memory if needed
	      t = (PCHR *)tagsBuffer.Expand(++tc, sizeof(PCHR));
	      t[tc] = NULL;
#if RSS_HACK
	      if (tc < 3 && strncmp(t[tc-1], rss_magic, sizeof(rss_magic)-1) == 0)
		{
// cerr << "Rss  tc = " << tc << endl;
		  return t; // Don't need to continue;
		}
#endif
	    }
	  break;

	case '<':
// cerr << "XXX Looking at " << b[i] << b[i+1] << b[i+2] << b[i+3] << b[i+4] << b[i+5] << b[i+6] << b[i+7] << b[i+8] << b[i+9] << b[i+10] << b[i+11] << "..." <<endl;
	  // Is the tag a comment or control?
	  if (b[i + 1] == '!')
	    {
	      /* The SGML was not parsed! */
	      i++;
	      if (b[i + 1] == '>')
		{
		  // Sun Oct 12 21:48:37 MET DST 1997
		  // Handle an empty comment: <!>
		  i++;
		}
	      else if (b[i + 1] == '-' && b[i + 2] == '-')
		{
		  // SGML comment <!-- blah blah ... -->
		  while (i < len)
		    {
		      if (b[++i] != '-') continue;
		      if (b[++i] != '-') continue;
		      if (b[++i] != '>') continue;
		      break;	// End of comment found
		    }		// while
		}
	      else if (b[i+1] == '-' && isspace(b[i+2]) )
		{
		  logf (LOG_INFO,
			"SGML/XML ERROR: Bogus control. Comment <!- error at %lu?",
			(long)i);
		  while (++i < len && b[i] != '>')
		    /* loop */;
		}
	      else if (State != IN_DECL)	// (3) Declaration <!XXX [ <...> <...> ]>
		{
		  State = IN_DECL;
		  t[tc] = &b[i];
		}
	    }			// if <!
	  else if (b[i + 1] == '?' && State != IN_DECL)
	    {
//cerr << "In XML declaration" << endl;
	      // Sun Oct 12 21:48:37 MET DST 1997
	      // <? .... ?> XML stuff
	      t[tc] = &b[++i];
	      State = IN_DECL;
	    }
	  else if (State == OK)
	    {
	      // Skip over leading white space
	      do
		{
		  i++;
		}
	      while (isspace (b[i]));
	      t[tc] = &b[i];	// Save tag
#if ACCEPT_SGML_EMPTY_TAGS 
	      if (b[i] == '>')
		i--;		// empty tag so back up.. 
#endif
	      //cerr << "Tag t[" << tc << "]  needs end" << endl;
	      State = NEED_END;
	    }
	  break;

	default:
	  break;
	}			// switch

    }				// for

  // Any Errors underway?
  if (State != OK || InComment)
    {
      if (InComment)
	{
	  if (t[tc])
	    logf (LOG_INFO, "SGML/XML ERROR: Dangling comment (started = %u)", (unsigned)(t[tc] - b));
	}
      else
	{
	  switch (State)
	    {
	      case NEED_END:
		logf(LOG_INFO, "SGML/XML ERROR: Need > (%s)", t[tc]);
		break;
	      case QUOTE:
		logf(LOG_INFO, "SGML/XML ERROR: Runaway Quotation in complex attribute");
		break;
	      case IN_DECL:
		logf(LOG_INFO, "SGML/XML ERROR: Still in Declaration");
		break;
	      default:
		logf(LOG_INFO, "SGML/XML ERROR: Bad State");
		break;
	    }
	}
      return NULL;		// Parse ERROR
    }

  t[tc] = (PCHR) NULL; // Mark end of list

#define DEBUG 0
#if DEBUG
  char **stack = new char *[tc];
  cout << "Tags Tree:" << endl;
  int incr = 0;
  for (i=0; i < tc; i++)
    {
      if (t[i][0] == '/')
	{
	  if (--incr < 0) incr = 0;
	  if (t[i][1] != 0)
	    {
	      cout << " </" ;
	      for (int j = 0; j < incr; j++)
		{
		  cout << ' ' << stack[j] << ' ';
		}
	      cout << stack[incr] << ">" << endl;
	    }
	  else if (t > 0)
	    {
	      cout << "</" << t[i-1] << ">" << endl;
	    }
	  continue;
	}
      else
	{
	  stack[incr++] = t[i][0] ? t[i] : stack[incr-1];
	}
      for (int k = 0; k < incr; k++)
	cout << ' ';

      cout << " <";
      for (int j=0; j < incr; j++)
	cout << ' ' << stack[j];
      cout << ">" << endl;
    }
  delete stack;
  cout << "--- END ----" << endl;

#endif
  return t;
}


/*
   Searches through string list t look for "/" followed by tag, e.g. if
   tag = "TITLE REL=XXX", looks for "/TITLE" or a empty end tag (</>).

   Pre: t is is list of string pointers each NULL-terminated.  The list
   should be terminated with a NULL character pointer.

   Post: Returns a pointer to found string or NULL.
 */
const PCHR SGMLNORM::find_end_tag (const char *const *t, const char *tag, size_t *offset) const
{
  if (tag == NULL || tag[0] == '/') return NULL; // End of an End???
  if (t == NULL || *t == NULL) return NULL; // Error
  if (*t[0] == '/') return NULL; // I'am confused!

  size_t len = 0;
  // Look for "real" tag name.. stop at white space
  while (tag[len] && !isspace(tag[len])) len++;
  // Have length

  // Now search for nearest matching close
  size_t i = 0;
  const char *tt = *t;
  const char firstCh = toupper(tag[0]);
  do
    {
      if (tt[0] == '/')
	{
#if ACCEPT_SGML_EMPTY_TAGS 
	  // for empty end tags: see SGML Handbook Annex C.1.1.1, p.68
	  if (tt[1] == '\0' || tag[0] == '\0')
	    break;
#endif
	  // SGML tags are mapped case INDEPENDENT (in our concrete syntax)
	  if ((firstCh == toupper(tt[1])) &&
		(0 == StrNCaseCmp (tt+1, tag, len)))
	    {
	      // Bugfix Feb '97: Moved check here
	      if (tt[1 + len] == '\0' || isspace (tt[1 + len]))
		break;	// Found it
	    }

	}
    }
  while ((tt = t[++i]) != NULL);

  if (offset) *offset = i;

  return (const PCHR) tt;
}

SRCH_DATE  SGMLNORM::ParseDate(const STRING& Buffer, const STRING& FieldName) const
{
  return ParseDate(Buffer);

}

STRING SGMLNORM::_cleanBuffer(const STRING& Buffer) const
{
//cerr << "INPUT = \"" << Buffer << "\"" << endl;
  STRING Content (XMLCommentStrip(Buffer));
  size_t pos = Content.Search('<');
  if (pos) Content.EraseAfter(pos-1);
  Content.Trim(); // Remove trailing space
  Entities.normalize2((char *)(Content.c_str()), Content.length());
//cerr << "OUTPUT = \"" << Content << "\"" << endl << "-------" << endl;
  return Content;
}

const char *SGMLNORM::_cleanBuffer(char *DataBuffer, size_t DataLength) const
{
  const UCHR zapChar = ' ';
  int        zap = 0;
  int        quote = 0;
  UCHR       Ch;

  for (register size_t Position =0; Position < DataLength; Position++)
    {
      // Zap all tags but comments
      if (DataBuffer[Position] == '<' && (DataLength-Position) > 3 &&
	DataBuffer[Position+1] != '!' && DataBuffer[Position+2] != '-')
          zap=1;
      // Zap untill end of tag or '='
      if (zap || quote)
        {
          if ((Ch = DataBuffer[Position]) == '>') {
            zap = 0;
            quote = 0;
          } else if (quote && ((Ch == quote) || (quote == '=' && isspace(Ch)))) {
            zap = 1;
            quote = 0;
          } else if (quote == 0 && (Ch == '"' || Ch == '\'' || Ch == '=')) {
             DataBuffer[Position] = zapChar;
             zap = 0;
             if (Ch == '=') {
               if ((Ch = DataBuffer[++Position]) != '"' && Ch != '\'')
                 Ch = '=';
               else
                 DataBuffer[Position] = zapChar;
             }
             quote = Ch;
          }
        }
     if (zap) DataBuffer[Position] = zapChar; // Zap these...
    }
  if (zap || quote) logf(LOG_WARN, "%s field ended in a tag?", Doctype.c_str());
  // Convert "&amp;xxx &lt;yyyy" to
  //         "&xxxx    <yyyy   "
  Entities.normalize(DataBuffer, DataLength);
  return DataBuffer;
}

STRING SGMLNORM::ParseBuffer(const STRING& Buffer) const
{
  return DOCTYPE::ParseBuffer(STRING(_cleanBuffer(Buffer)).Pack());
}

size_t SGMLNORM::ParseWords2(const STRING& Buffer, WORDSLIST *ListPtr) const
{
  return DOCTYPE::ParseWords2(_cleanBuffer((char *)(Buffer.c_str()), Buffer.length()), ListPtr);
}

SRCH_DATE  SGMLNORM::ParseDate(const STRING& Buffer) const
{
  return DOCTYPE::ParseDate(_cleanBuffer(Buffer));
}

DATERANGE  SGMLNORM::ParseDateRange(const STRING& Buffer) const
{
  return DOCTYPE::ParseDateRange(_cleanBuffer(Buffer));
}

NUMERICOBJ SGMLNORM::ParsePhonhash(const STRING& Buffer) const
{
  return DOCTYPE::ParsePhonhash (_cleanBuffer(Buffer));
}

NUMERICOBJ SGMLNORM::ParseNumeric(const STRING& Buffer) const
{
  return DOCTYPE::ParseNumeric(_cleanBuffer(Buffer));
}

MONETARYOBJ SGMLNORM::ParseCurrency(const STRING& FieldName, const STRING& Buffer) const
{
  return DOCTYPE::ParseCurrency(FieldName, _cleanBuffer(Buffer));
}

GDT_BOOLEAN SGMLNORM::ParseRange(const STRING& Buffer, const STRING& FieldName,
        DOUBLE* fStart, DOUBLE* fEnd) const
{
  return DOCTYPE::ParseRange(_cleanBuffer(Buffer), FieldName, fStart, fEnd);
}

int        SGMLNORM::ParseGPoly(const STRING& Buffer, GPOLYFLD* gpoly) const
{
  return DOCTYPE::ParseGPoly(_cleanBuffer(Buffer), gpoly);
}

NUMERICOBJ SGMLNORM::ParseComputed(const STRING& FieldName, const STRING& Buffer) const
{
  return DOCTYPE::ParseComputed(FieldName, _cleanBuffer(Buffer));
}

long       SGMLNORM::ParseCategory(const STRING& Buffer) const
{
  return DOCTYPE::ParseCategory(_cleanBuffer(Buffer));
}

