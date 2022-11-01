/* ########################################################################

               Generic Document Handler (DOCTYPE)

   File: doctype.cxx
   Description: Class DOCTYPE - Document Type
   Last maintained by: Edward C. Zimmermann

   ######################################################################## */

#include <ctype.h>
#include "doctype.hxx"
#include "common.hxx"
#include "process.hxx"
#include "doc_conf.hxx"
#include "lang-codes.hxx"
#include "bboxfield.hxx"
#include "soundex.hxx"

static const STRING Ignore ("<Ignore>");
static const STRING MetaDataMaps("Metadata-Maps");


// The string below MAY NOT BE ALTERED WITHOUT PRIOR WRITTEN PERMISSION of
// NONMONOTONIC Networks or trustees of the re-Isearch project 
static const char Copyright[] = "\
###########################################################################\n\
  This Document has been automatically assembled and converted by\n\
  Smart Doctypes of the re-Isearch Project.\n\
\n\
  For project information visit: http://www.nonmonotonic.net/re-isearch/\n\
\n\
###########################################################################";

// This string below MUST be used to document any changes to this module.
static const char Modifications[] = "\
\n\
  Local Modifications\n\
  ====================\n\
  None\n\
\n\
###########################################################################";

// DTD Level
static const char DTD_URL[] = "http://www.w3.org/pub/WWW/MarkUp/Wilbur/HTML";
static const char DTD_VER[] = "3.2"; // "-//W3C//DTD HTML 3.2//EN" 
/* eg. http://www.w3.org/pub/WWW/MarkUp/Wilbur/HTML3.2 is where it is */

STRING DOCTYPE::Getoption(const STRING& Entry, const STRING& Default) const
{
  return Getoption(DoctypeOptions, Entry, Default);
}

static const STRING GeneralSection ("General");
static const STRING DbInfoSection  ("DbInfo"); // See idb.cxx
static const STRING FieldTypeSection ("FieldTypes");


STRING DOCTYPE::Getoption(const STRLIST *Strlist, const STRING& Entry, const STRING& Default) const
{
  STRING string;
  if (Strlist)
    Strlist->GetValue(Entry, &string);
  if (string.IsEmpty() && Db)
    Db->ProfileGetString(Doctype, Entry, NulString, &string);
  if (string.IsEmpty())
    {
      if (tagRegistry)
	tagRegistry->ProfileGetString(GeneralSection, Entry, Default, &string);
      else
	string = Default;
    }
  if (!string.IsEmpty())
    message_log (LOG_DEBUG, "%s set to %s", Entry.c_str(), string.c_str());
  return string;
}

extern STRING __ExpandPath(const STRING& name);

DOCTYPE::DOCTYPE()
{
  Db = NULL;
  Doctype = NulString;
  DoctypeOptions = NULL;
  tagRegistry = NULL;
}

DOCTYPE::DOCTYPE(IDBOBJ *DbParent)
{
  Db = DbParent;
  Doctype = NulString;
  DoctypeOptions = NULL;
  tagRegistry = NULL;
}

DOCTYPE::DOCTYPE (IDBOBJ * DbParent, const STRING& Name)
{
  const STRING Config ("Config");
  Db = DbParent;
  Doctype = Name;
  Doctype.ToUpper();

  // Read doctype options
  STRING iniName;
  // -o options over-ride .ini settings!
  if (Db)
    {
      if ((DoctypeOptions = Db->GetDocTypeOptionsPtr ()) != NULL)
	if (DoctypeOptions->GetValue (Config, &iniName) == 0 && !Doctype.IsEmpty())
	  Db->ProfileGetString(Doctype, Config, NulString, &iniName);
    }
  else
    DoctypeOptions = NULL;
  if (iniName.IsEmpty())
    {
      iniName = Doctype;
      iniName.ToLower();
      iniName.Cat(".ini");
      if (ResolveConfigPath(&iniName))
	{
	  if (Db) Db->ProfileWriteString(Doctype, Config, iniName);
	}
      else message_log (LOG_DEBUG, "%s: %s (optional) not found.", Doctype.c_str(), iniName.c_str());
    }
  else
    {
      iniName = __ExpandPath(iniName);
      message_log (LOG_DEBUG, "Profile specified [%s] .INI as %s", Doctype.c_str(), iniName.c_str());
    }
  // Process the doctype.ini (read into tagRegistry)
  if (!iniName.IsEmpty())
    {
      FILE  *Fp=fopen(iniName, "r");
      if (Fp)
	{
	  tagRegistry = new REGISTRY (Name);
	  tagRegistry->Read (Fp);
	  message_log (LOG_DEBUG, "%s configuration loaded from %s", Name.c_str(), iniName.c_str());
	  fclose (Fp);
	}
      else
	tagRegistry = NULL;
    }
  if (tagRegistry == NULL)
    message_log (LOG_DEBUG, "No tagRegistry available for doctype '%s'", Doctype.c_str());

  // Set the basic general options
  DateField         = Getoption(DoctypeOptions, "DateField");
  DateModifiedField = Getoption(DoctypeOptions, "DateModifiedField");
  DateCreatedField  = Getoption(DoctypeOptions, "DateCreatedField");
  DateExpiresField  = Getoption(DoctypeOptions, "DateExpiresField");
  TTLField          = Getoption(DoctypeOptions, "TTLField");

  LanguageField = Getoption(DoctypeOptions, "LanguageField");
  KeyField      = Getoption(DoctypeOptions, "KeyField");
  CategoryField = Getoption(DoctypeOptions, "CategoryField");
  PriorityField = Getoption(DoctypeOptions, "PriorityField");
  WordMaximum   = Getoption(DoctypeOptions, "MaxWordLength").GetInt();
  if (Db)
    {
      Charset = Db->GetLocale().Charset();
    }
  message_log (LOG_DEBUG, "Doctype base class Init");
}

void DOCTYPE::Help(STRING *StringPtr) const
{
  const int align = 3;
  STRLIST Strlist;
  const char *help = Description(&Strlist);
  if (tagRegistry)
    tagRegistry->ProfileGetString(GeneralSection, "Help", help, StringPtr);
  else
    *StringPtr = help;
  size_t tot = Strlist.GetTotalEntries();

  if (tot == 1)
    return; // At base....

  STRING s;

  int pad;
  char spaces[DocumentTypeSize+4];
  memset (spaces, ' ', DocumentTypeSize);
  StringPtr->Cat("\n\n");
  StringPtr->Cat (Name());
  StringPtr->Cat (" Class Tree:\n");
  while (tot > 0)
    {
      Strlist.GetEntry(tot, &s);
      pad =  (DocumentTypeSize - s.GetLength())/2 + align;
      if (pad > 0)
	{
	  spaces[pad] = '\0';
	  StringPtr->Cat (spaces);
	  spaces[pad] = ' ';
	}
      StringPtr->Cat(s);
      if (--tot >= 1)
	{
	  StringPtr->Cat('\n');
	  spaces[(DocumentTypeSize-1)/2 + align] = '\0';
	  StringPtr->Cat (spaces);
	  spaces[(DocumentTypeSize-1)/2 + align] = '\0';
	  StringPtr->Cat (tot == 1 ? 'v' : '|');
	  StringPtr->Cat ('\n');
	}
    }
}

const char *DOCTYPE::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("DOCTYPE");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  return "Generic Document parser base class\n\
DOCTYPE is the master base class that handles most of the options as well\n\
as basic conversion services\n\n\
Index options (defined in <doctype>.ini):\n\
  [General]\n\
  FieldType=<file to load for field definitions>  # default <doctype>.fields\n\
  DateField=<Field name to use for date of record>\n\
  DateModifiedField=<Date Modified field for record>\n\
  DateCreatedField=<Date of record creation field>\n\
  DateExpiresField=<Date of record expiration>\n\
  TTLField=<Time to live in minutes:  Now+TTL = DateExpires>\n\
  LanguageField=<Field name that contains the language code>\n\
  KeyField=<Field name that contains the record key>\n\
  CategoryField=Field name that contains the category (integer).\n\
  PriorityField=Field name that contains the priority (integer).\n\
  MaxWordLength=<Number, words longer than this won't get indexed>\n\
    The above options may also be specified as -o options as well as\n\
    in the \"Db.ini\" configuration file under the [<DOCTYPE>] section---\n\
    where <DOCTYPE> is the name of the records parser class.\n\
  Headline=<Headline format to over-ride default Brief record>\n\
    The format is %(FIELD1)...text...%(FIELD2) ... where the %(X)\n\
    declarations get replaced by the content of field (X). The\n\
    declaration %(X?:Y) may be used to use the content of field (Y)\n\
    should field (X) be empty or undefined for the record.\n\
  Headline/<RecordSyntax OID>=<Headline format for Record Syntax>\n\
    The Headline and Headline/<OID> may also be specified in the \"DB.ini\"\n\
    configuration file under the [DbInfo] section for all doctypes.\n\
  Summary=<Format to define a Summary> # See Headline\n\
  Summary/<RecordSyntax OID>=<Summary format for Record Syntax>\n\
    The Headline and Headline/<OID> may also be specified in the \"DB.ini\"\n\
    configuration file under the [DbInfo] section for all doctypes.\n\
  Content-Type=<MIME Content type for the doctype>\n\
  Help=<Help text>\n\
Alternative as the options, resp., DateField, LanguageField and KeyField\n\n\
Other groups are:\n\
  [Fields]\n\
  Field=Field1         # Map fields into others\n\
                       # Note: See also <db>.ini [BOUNDING-BOX]\n\n\
  [FieldTypes]\n\
  Field=<FieldType>    # define the type: text, num, computed, num-range, time,\n\
                       # date, date-range, gpoly\n\
                       # Note: Bounding box elements must be explicitly defined as numeric\n\n\
  [External/<RecordSyntax OID>]\n\
  Field=<program> # e.g. F=cat would pipe the record for full element presentation to cat\n\n\
  [Present <Language-Code>]\n\
  Field=<Display format name for Field when the record has language>\n\n\
  [Defaults] # <db>.ini as [Defaults Doctype]\n\
  Field=<Default Value> # Default value for Field if not in record.\n\
The .ini file is specified as <lowercase name>.ini or alternatively via\n\
   [<Uppercase Doctype Name>]\n\n\
   Config=<.ini path>\n\
in the database.ini or via the option CONFIG=<.ini path>\n\
See also database.ini documentation for database-specific over-rides and other options.";
}

// Should add since the MIME content might not be solely
// linked to the DOCTYPE
void DOCTYPE::SourceMIMEContent(const RESULT& Result, STRING *StringPtr) const
{
  StringPtr->Clear();
  if (tagRegistry)
    tagRegistry->ProfileGetString(GeneralSection, "Content-Type", NulString, StringPtr);
  if (StringPtr->GetLength() == 0)
    SourceMIMEContent(StringPtr);

  STRING charset =  Result.GetCharsetCode();
  if (charset.GetLength() && (charset != "us-ascii"))
    {
      *StringPtr << "; charset=" << charset;
    }    
}

static const char MIME_TYPE[] = "text/plain";

void DOCTYPE::SourceMIMEContent(STRING *stringPtr) const
{
  *stringPtr = MIME_TYPE;
}


STRING  DOCTYPE::Httpd_Content_type (const RESULT& ResultRecord, const STRING& MimeType) const
{
  STRING mime, result;
  SRCH_DATE    date;

  if (MimeType.IsEmpty())
    SourceMIMEContent(ResultRecord, &mime);
  else
    mime = MimeType;

  date = ResultRecord.GetDateExpires();
  if (date.Ok())
    endl (result << "Expires: " << date.RFCdate() );

  date = ResultRecord.GetDateCreated();
  if (date.Ok())
    endl (result << "Date-created: " << date.RFCdate()); 

  endl (result <<"Content-type: " <<  mime);
  date = ResultRecord.GetDateModified();
  if (date.Ok())
    endl (result << "Date-modified: " << date.RFCdate());

  endl (result);
  return result;
}

bool DOCTYPE::SetFieldType(const STRING& FieldName, const FIELDTYPE FieldType)
{
  if (Db && FieldName.GetLength())
    {
      if (!Db->GetFieldType(FieldName).Ok())
	{
	  Db->AddFieldType(FieldName, FieldType); // Was PriorityField,
	  return true;
	}
    }
  return false;
}


void DOCTYPE::LoadFieldTable()
{
  message_log (LOG_DEBUG, "DOCTYPE/%s::LoadFieldTable()", Doctype.c_str());
  if (Db)
    {
      SetFieldType(DateField,         FIELDTYPE::date);
      SetFieldType(DateCreatedField,  FIELDTYPE::date );
      SetFieldType(DateModifiedField, FIELDTYPE::date);
      SetFieldType(PriorityField,     FIELDTYPE::numerical);
      SetFieldType(CategoryField,     FIELDTYPE::numerical); // added 2021

    }
}


void DOCTYPE::AddFieldDefs ()
{
  STRING FieldTypeFilename = Getoption("FIELDTYPE", NulString);

  message_log (LOG_DEBUG, "DOCTYPE/%s::AddFieldDefs()", Doctype.c_str());

  if (Db == NULL) return;

  if (tagRegistry)
    {
      STRLIST   fieldslist;
      FIELDTYPE fieldType;
      STRING    s;

      tagRegistry->GetEntryList(FieldTypeSection, &fieldslist);
      for (register STRLIST *p = fieldslist.Next(); p!=&fieldslist ; p=p->Next())
	{
	  tagRegistry->ProfileGetString(FieldTypeSection, p->Value(), NulString, &s);
	  if (!(fieldType = s).IsText())
	    {
	      Db->AddFieldType(p->Value(), fieldType);
	    }
	}

    }

  if (FieldTypeFilename.IsEmpty())
    {
      FieldTypeFilename = Doctype;
//    size_t x = FieldTypeFilename(':'); if (x) FieldTypeFilename.EraseAfter(x-1);
      FieldTypeFilename.ToLower();
      FieldTypeFilename.Cat(".fields");
    }
  if (!ResolveConfigPath(&FieldTypeFilename))
    {
      message_log (LOG_DEBUG, "No field type definition file '%s' defined",
		FieldTypeFilename.c_str());
      return;
    }
  else if (!FileExists(FieldTypeFilename))
    {
      message_log (LOG_WARN, "%s: Field def file '%s' does not exist. Assuming all %s fields are text.",
	Doctype.c_str(),
	FieldTypeFilename.c_str(), Doctype.c_str());
      return;
    }
  FILE *fp = fopen(FieldTypeFilename, "r");
  if (fp == NULL)
    {
      message_log(LOG_ERRNO, "Could not open the field def file '%s'", FieldTypeFilename.c_str());
      return;
    }
  STRING sBuf;
  while (sBuf.FGet(fp))
    {
       STRINGINDEX w;
       if ((w = sBuf.Search("=")) > 0)
	{
	  const char *tcp = (const char *)sBuf + w;
	  if (strncasecmp(tcp, "text", 4) != 0 && strncasecmp(tcp, "any", 3) != 0)
	    {
	      Db->AddFieldType( sBuf.Strip(STRING::both) );
	      message_log (LOG_DEBUG, "Adding definition: \"%s\"", sBuf.c_str());
	    }
	}
    }
  fclose(fp);
}


void DOCTYPE::ParseRecords (const RECORD& FileRecord)
{
  if (Db) Db->DocTypeAddRecord (FileRecord);
}

// Regions are defined as:
// Fc.. where
//    Start = Offset from Start of Record to start indexing
//    End   = Offset from Start of Record to end indexing 
void DOCTYPE::SelectRegions(const RECORD& Record, FCT* FctPtr)
{
  // Select the entire document as one region.
  *FctPtr = FC(0, Record.GetLength());
}

bool DOCTYPE::IsStopWord(const UCHR* Word, STRINGINDEX MaxLen) const
{
  return Db ? Db->IsStopWord(Word, MaxLen, WordMaximum) : false;
}

INT DOCTYPE::ReadFile(const STRING& Filename, STRING *StringPtr, off_t Offset, size_t Length) const
{
  STRINGINDEX   result = 0;
  FILE         *fp = ffopen(Filename, "rb");
  if (fp)
    {
      result = ReadFile(fp, StringPtr, Offset, Length);
      ffclose(fp);
    }
  return (INT)result;
}


INT DOCTYPE::ReadFile(FILE *fp, STRING *StringPtr, off_t Offset, size_t Length) const
{
  return StringPtr->Fread(fp, Length, Offset);
}

INT DOCTYPE::ReadFile(const STRING& Filename, CHR *Buffer, off_t Offset, size_t Length) const
{
  STRINGINDEX   result = 0;
  FILE         *fp = ffopen(Filename, "rb");
  if (fp)
    {
      result = ReadFile(fp, Buffer, Offset, Length);
      if (fp == NULL)
	message_log (LOG_PANIC, "Clobered Stream pointer!"); 
      ffclose(fp);
    }
  return (INT)result;
}


INT DOCTYPE::ReadFile(FILE *fp, CHR *Buffer, off_t Offset, size_t Length) const
{
  size_t   result = 0;

  if (fp != NULL)
    result = pfread(fp, Buffer, Length, Offset);
  Buffer[result] = '\0'; // ASCIIZ
  return (INT)result;
}

INT DOCTYPE::GetRecordData(const STRING& Filename, STRING *StringPtr, off_t Offset, size_t Length) const
{
  STRINGINDEX   result = 0;
  FILE         *fp = ffopen(Filename, "rb");
  if (fp)
    {
      result = GetRecordData(fp, StringPtr, Offset, Length);
      if (fp == NULL)
        message_log (LOG_PANIC, "Clobered Stream pointer!");
      ffclose(fp);
    }
  return (INT)result;

}

INT DOCTYPE::GetRecordData(const STRING& Filename, CHR *Buffer, off_t Offset, size_t Length) const
{
  STRINGINDEX   result = 0;
  FILE         *fp = ffopen(Filename, "rb");
  if (fp)
    {
      result = GetRecordData(fp, Buffer, Offset, Length);
      if (fp == NULL)
        message_log (LOG_PANIC, "Clobered Stream pointer!");
      ffclose(fp);
    }
  return (INT)result;
}


INT DOCTYPE::GetTerm(const STRING& Filename, CHR *Buffer, off_t Offset, size_t Length)
{
  return ReadFile(Filename, Buffer, Offset, Length);
}

size_t DOCTYPE::ParseWords2(const STRING& Buffer, WORDSLIST *ListPtr) const
{
  if (Db)
    return Db->ParseWords2(Buffer, ListPtr);
  if (ListPtr) ListPtr->Clear();
  return 0;
}


// Break up SIS and lowercase terms..
// This is the core routine for building the GP lists..
// This gives us the GP (encoded address for a term) and word (SIS) maps.
//
GPTYPE DOCTYPE::ParseWords(UCHR* DataBuffer, GPTYPE DataLength,
	GPTYPE DataOffset, GPTYPE* GpBuffer, GPTYPE GpLength)
{
  GPTYPE GpListSize = 0;
  const  UCHR  zapChar = '\0'; // Was ' '

  if (DataBuffer == NULL)
    {
      if (DataLength)
	message_log (LOG_ERROR, "ParseWords: ***** NULL Data buffer ***** !");
      return 0;
    }
  if (GpBuffer == NULL || GpLength == 0)
    {
      message_log (LOG_ERROR, "**** Program ERROR: ParseWords Buffer%s? (Len=%d)", GpBuffer ? "" : " NULL", (int)GpLength);
      return 0; // ERROR
    }

#ifdef _WIN32
  message_log (LOG_DEBUG, "ParseWords: Len: Data=%lld Buff=%lld / off=%lld",
	(long long)DataLength, (long long)GpLength, (long long)DataOffset);
#endif

  for (register GPTYPE Position =0; Position < DataLength;)
    {
      // Skip non-word characters
      // TODO: For UTF8 need to rework this.
      //
      // One idea shall be to lower case and zap non term characters in a single
      // routine. See the utf8.cxx code
      //
      for (; (Position < DataLength) && (IsWordSep(DataBuffer[Position])); Position++)
	{
/*
	  // Added special case for \r (DOSismus)
	  if (DataBuffer[Position] == '\r') DataBuffer[Position++] = zapChar;
*/

	  if (!IsTermChr(DataBuffer+Position))
	    DataBuffer[Position] = zapChar; // Zap these...
	}

      // If the word is not in stoplist then add it to GpBuffer
      if (( (DataLength - Position) > 0) &&
	// BUGFIX: Was DataLength, now DataLength - Position
	DataBuffer[Position] &&
	(!(IsStopWord(&DataBuffer[Position], DataLength - Position))) )
	{
	  if (GpListSize >= GpLength)
	    {
	      // Should NOT HAPPEN!!
	      message_log (LOG_PANIC, "%s ParseWords: Memory overrun (%lu >= %lu)[Position=%lu of %lu][%d-bit]!",
			Doctype.c_str(), (long)GpListSize, (long)GpLength,
			(long)Position, (long)DataLength,
			8*sizeof(GPTYPE));
	      abort(); // Stop process!
	    }
	  else if (GpListSize > DataLength)
	    {
	      message_log (LOG_DEBUG, "INTERNAL ERROR STATE: Words Indexed=%lu / Position=%lu / DataLength=%lu",
		(long)GpListSize, (long)Position, (long)DataLength );
	      message_log (LOG_PANIC, "Detected an ABSURD _ib_IsTermChr() callback function!");
	      abort(); // Stop process
	    }
	  GpBuffer[GpListSize++] = DataOffset + Position;

	  if (GpListSize <= 0)
	    message_log (LOG_PANIC, "%s ParseWords: Address space overrun (>%lu)[%d-bit]!",
		Doctype.c_str(), (long)GpLength, 8*sizeof(GPTYPE));
	}

      // Skip over the word
      if (Position >= DataLength)
	break;
      int itr = IsTermChr(DataBuffer+Position);
      if (!itr)
	{
	  // Need to make sure that an illogical call-back was not defined
	  message_log (LOG_PANIC,  "Detected an ABSURD _ib_IsTermChr() callback function!");
	  if (GpListSize == 1) GpListSize = 0;
	  break;
	}
      while ( itr )
        {
          // Make lowercase
          // For UTF8 we'll need it reworked..
          DataBuffer[Position] = _ib_tolower( DataBuffer[Position] );
          if (++Position >= DataLength) break;
	  itr = IsTermChr(DataBuffer+Position);
        }
      // NOTE: If the first character was NOT a TermChr then we'll be at the same
      //       position. This only can happen when a callback makes no sense.
  
   }
   return GpListSize;
}  // Return # of GP's added to GpBuffer


NUMERICOBJ DOCTYPE::ParsePhonhash(const STRING& Buffer) const
{
  return SoundexEncodeName(Buffer);
}

NUMERICOBJ DOCTYPE::ParseNumeric(const STRING& Buffer) const
{
  return Buffer;
}

bool DOCTYPE::ParseRange(const STRING& Buffer, const STRING& FieldName,
        DOUBLE* fStart, DOUBLE* fEnd) const
{
  NUMERICALRANGE Range(Buffer);
  if (Range.Ok())
    {
      if (fStart) *fStart = Range.GetStart() ;
      if (fEnd)   *fEnd   = Range.GetEnd();
      return true;
    }
  return false;
}

STRING  DOCTYPE::ParseBuffer(const STRING& Buffer) const
{
  return Buffer.Strip(STRING::both);
}

SRCH_DATE  DOCTYPE::ParseDate(const STRING& Buffer) const
{
  if (Buffer.IsEmpty())
    return SRCH_DATE();
  SRCH_DATE date (Buffer); // Let SRCH_DATE do the magic
  if (!date.Ok())
    message_log (LOG_WARN, "%s::ParseDate: '%s' was not a parseable or well-defined date.",
		Doctype.c_str(), Buffer.c_str());
  return date;
}

DATERANGE  DOCTYPE::ParseDateRange(const STRING& Buffer) const
{
  DATERANGE range (Buffer);

  if (!range.Ok())
    {
      SRCH_DATE date(Buffer);
      if (date.Ok())
	{
	  message_log (LOG_DEBUG, "%s: Input '%s' not a daterange, assumming trivial range",
		Doctype.c_str(), Buffer.c_str());
	  range.SetStart(date);
	  range.SetEnd(date);
	}
      else
	message_log (LOG_WARN, "%s::ParseDateRange: Could not parse input '%s'", Doctype.c_str(),
		Buffer.c_str());
    }
  return range;
}


int  DOCTYPE::ParseGPoly(const STRING& Buffer, const STRING&, GPOLYFLD*fld) const
{
  return ParseGPoly(Buffer, fld);
}


int  DOCTYPE::ParseGPoly(const STRING&, GPOLYFLD*) const
{
  return 0;
}


int  DOCTYPE::ParseBBox(const STRING& Buffer, const STRING&, BBOXFLD*fld) const
{
  return ParseBBox(Buffer, fld);
}
  

int  DOCTYPE::ParseBBox(const STRING& Buffer, BBOXFLD* fld) const
{
  if (fld) return fld->Set(Buffer);
  return 0;
}

extern "C" {
extern long double (*_IB_parse_computed)(const char *doctype, const char *fieldname, const char *buffer, size_t len);
}

NUMERICOBJ DOCTYPE::ParseComputed(const STRING& FieldName, const STRING& Buffer) const
{
  if (_IB_parse_computed)
    return _IB_parse_computed(Doctype, FieldName, Buffer.c_str(), Buffer.GetLength()); 
  return NUMERICOBJ(Buffer);
}

MONETARYOBJ DOCTYPE::ParseCurrency(const STRING& FieldName, const STRING& Buffer) const
{
  if (_IB_parse_computed)
    return _IB_parse_computed(Doctype, FieldName, Buffer.c_str(), Buffer.GetLength());
  return MONETARYOBJ(Buffer);
}


NUMERICOBJ DOCTYPE::ParseTTL(const STRING& FieldName, const STRING& Buffer) const
{
  double    val = (int)Buffer;

  if (val != 0)
    {
      if (!Buffer.Search("min"))
	{
	  if (Buffer.SearchAny("sec"))
	    val /= 60.0;
	  else if (Buffer.SearchAny("hour"))
	    val *= 60;
	  else if (Buffer.SearchAny("day"))
	    val *= 60*24;
	  else if (Buffer.SearchAny("week"))
	    val *= 60*24*7;
	  else if (Buffer.SearchAny("month") || Buffer.SearchAny("year"))
	    {
	      SRCH_DATE now;
	      now.SetNow();

	      SRCH_DATE expiry (now);

	      if (Buffer.SearchAny("month"))
		expiry.PlusNmonths((int)val);
	      else
		expiry.PlusNyears((int)val);
	      val = expiry.MinutesDiff(now);
	    }
	}
    }
  else
    {
      // Try as date?
      SRCH_DATE date (Buffer);
      if (date.Ok())
	{
	  SRCH_DATE now;
	  now.SetNow();
	  val = date.MinutesDiff(now);
	}
    }
  return val;
}


long DOCTYPE::ParseCategory(const STRING& Buffer) const
{
  return Buffer.GetLong();
}


void DOCTYPE::ParseFields (RECORD *) { }


void DOCTYPE::BeforeIndexing ()
{
  message_log (LOG_DEBUG, "%s::BeforeIndexing called", Doctype.c_str());
}

void DOCTYPE::AfterIndexing ()
{
  message_log (LOG_DEBUG, "%s::AfterIndexing called", Doctype.c_str());
}

void DOCTYPE::BeforeSearching (QUERY *)
{
  message_log (LOG_DEBUG, "%s::BeforeSearching called", Doctype.c_str());
}

IRSET *DOCTYPE::AfterSearching(IRSET* ResultSetPtr)
{
  message_log (LOG_DEBUG, "%s::AfterSearching called", Doctype.c_str());
  return ResultSetPtr;
}



void DOCTYPE::BeforeRset (const STRING& RecordSyntax)
{
  message_log (LOG_DEBUG, "%s::BeforeRset(%s) called", Doctype.c_str(), RecordSyntax.c_str());

  static const char HeadlineTag[]  = "Headline";
  static const char SummaryTag[]   = "Summary";
  static const char checking_msg[] = "Checking \"%s\" [%s] for %s and %s";
  static const char db_ini[]       = "<db>.ini";
  static const char doctype_ini[]  = "<doctype>.ini";

  STRING Element1 ( HeadlineTag );
  STRING Element2 ( SummaryTag );

  // If Headline/xxxxx=
  if (Db)
    {
      message_log (LOG_DEBUG, checking_msg,
	db_ini, DbInfoSection.c_str(), Element1.c_str(), Element2.c_str());
      Db->ProfileGetString (DbInfoSection, Element1, NulString, &HeadlineFmt);
      Db->ProfileGetString (DbInfoSection, Element2, NulString, &SummaryFmt);
    }
  else
    {
      HeadlineFmt.Clear();
      SummaryFmt.Clear();
    }
  if (HeadlineFmt.IsEmpty())
    {
      message_log (LOG_DEBUG, "Checking option %s", Element1.c_str());
      HeadlineFmt = Getoption(Element1, NulString);
    }
  if (SummaryFmt.IsEmpty())
    {
      message_log (LOG_DEBUG, "Checking option %s", Element2.c_str());
      SummaryFmt = Getoption(Element2, NulString);
    }
  if (tagRegistry)
    {
      message_log (LOG_DEBUG, checking_msg, doctype_ini, GeneralSection.c_str(),
	Element1.c_str(), Element2.c_str());
      tagRegistry->ProfileGetString(GeneralSection, Element1, HeadlineFmt, &HeadlineFmt);
      tagRegistry->ProfileGetString(GeneralSection, Element2, SummaryFmt, &SummaryFmt);
    }
  if (Db)
    {
      message_log (LOG_DEBUG, checking_msg, db_ini, Doctype.c_str(),
	Element1.c_str(), Element2.c_str());
      Db->ProfileGetString (Doctype, Element1, HeadlineFmt, &HeadlineFmt);
      Db->ProfileGetString (Doctype, Element2, SummaryFmt, &SummaryFmt);
    }

  if (!RecordSyntax.IsEmpty())
    {
      Element1.Cat ("/");
      Element1.Cat (RecordSyntax);
      Element2.Cat ("/");
      Element2.Cat (RecordSyntax);

      if (Db)
	{
	  message_log (LOG_DEBUG, checking_msg, db_ini, DbInfoSection.c_str(),
	    Element1.c_str(), Element2.c_str());
	  Db->ProfileGetString (DbInfoSection, Element1, HeadlineFmt, &HeadlineFmt);
	  Db->ProfileGetString (DbInfoSection, Element2, SummaryFmt, &SummaryFmt);
	}
      if (tagRegistry)
	{
	  message_log (LOG_DEBUG, checking_msg, doctype_ini, GeneralSection.c_str(),
	    Element1.c_str(), Element2.c_str());
	  tagRegistry->ProfileGetString(GeneralSection, Element1, HeadlineFmt, &HeadlineFmt);
	  tagRegistry->ProfileGetString(GeneralSection, Element2, SummaryFmt, &SummaryFmt);
	}
      if (Db)
	{
	  message_log (LOG_DEBUG, checking_msg, db_ini, Doctype.c_str(),
	    Element1.c_str(), Element2.c_str());
	  Db->ProfileGetString (Doctype, Element1, HeadlineFmt, &HeadlineFmt);
	  Db->ProfileGetString (Doctype, Element2, SummaryFmt, &SummaryFmt);
	}
    }

  message_log (LOG_DEBUG, "%s Format = %s",
    HeadlineTag,
    HeadlineFmt.IsEmpty() ? "<none defined, using default>" : HeadlineFmt.c_str());
  message_log (LOG_DEBUG, "%s Format  = %s",
    SummaryTag,
    SummaryFmt.IsEmpty() ?  "<none defined, using default>" : SummaryFmt.c_str());
}

void DOCTYPE::AfterRset (const STRING&)
{
  message_log (LOG_DEBUG, "%s::AfterRset called", Doctype.c_str());
  // HeadlineFmt.Clear();
  // SummaryFmt.Clear();
}

// Hooks for the field parsers
INT DOCTYPE::UnifiedNames (const STRING& Tag, PSTRLIST Value,
	bool Use) const
{
  INT Total = 0;

  if (Value == NULL)
    {
      message_log (LOG_PANIC, "UnifiedNames got a NULL Ptr?");
      return 0;
    }

  Value->Clear();
  if (tagRegistry)
    {
      // We read the unified names from the [Fields] section
      //
      tagRegistry->ProfileGetString("Fields", Tag, Value);
      Total = Value->GetTotalEntries();
    }
  if (Total == 0 && Use)
    {
      Value->AddEntry( Tag );
      Total++;
    }
  return Total;
}

// Summary, e.g. Meta Description field
bool DOCTYPE::Summary(const RESULT&, const STRING&, STRING *StringBuffer) const
{
  StringBuffer->Clear();
  return false;
}

bool DOCTYPE::Summary(const RESULT& ResultRecord, STRING *StringBuffer) const
{
  return Summary(ResultRecord, NulString, StringBuffer);
}


REGISTRY *DOCTYPE::GetMetadata(const RECORD& record,
        const STRING& mdType, const REGISTRY* defaults)
{

  REGISTRY* meta = defaults->clone();
  RESULT result;
  result.SetKey( record.GetKey());
  result.SetPath( record.GetPath() );
  result.SetFileName( record.GetFileName() );
  result.SetDocumentType( record.GetDocumentType() );
  result.SetRecordStart(record.GetRecordStart());
  result.SetRecordEnd(record.GetRecordEnd());

  STRLIST position, value;

  // add the title
  position.AddEntry("locator");
  position.AddEntry("title");

  STRING s;
  if (Headline(result, &s) == false)
    s = "<Untitled>";
  value.AddEntry(s);
  meta->SetData(position, value);

  return meta;
}

bool DOCTYPE::Headline(const RESULT& ResultRecord, STRING *StringBuffer) const
{
  return Headline(ResultRecord, NulString, StringBuffer);
}

bool DOCTYPE::Headline(const RESULT& ResultRecord, const STRING& RecordSyntax,
        STRING *StringBuffer) const
{
  bool result = HeadlineFmt.IsEmpty() ? false :
	Headline(HeadlineFmt, ResultRecord, RecordSyntax, StringBuffer);

  if (result == false)
    {
      // Default for headline..
      if (RecordSyntax.GetLength())
	Present(ResultRecord,  BRIEF_MAGIC, RecordSyntax, StringBuffer);
      else
	Present (ResultRecord,  BRIEF_MAGIC, StringBuffer);

      // StringBuffer->Pack(); // 2022
      result = StringBuffer->GetLength() != 0;
    }
  return result;
}

bool DOCTYPE::Headline(const STRING& HeadlineFormat,
	const RESULT& ResultRecord, const STRING& RecordSyntax, STRING *StringBuffer) const
{
  const size_t hlen = HeadlineFormat.GetLength();
  StringBuffer->Clear(); // Zap old
  if (hlen)
    {
      CHR Ch, endCh;
      STRING S, ESet;
      STRLIST Elements;

      for (size_t i=1; i<= hlen; i++)
	{
	  if ((Ch = HeadlineFormat.GetChr(i)) == '%' && HeadlineFormat.GetChr(i+1) != '%')
	    {
	      switch((Ch = HeadlineFormat.GetChr(++i)))
		{
		  case '(': endCh = ')'; break;
		  case '{': endCh = '}'; break;
		  case '[': endCh = ']'; break;
		  case '<': endCh = '>'; break;
		  default:  endCh = '\0'; break;
		}
	      if (endCh )
		Ch = HeadlineFormat.GetChr(++i);
	      ESet.Empty();
	      do {
		ESet.Cat (Ch);
		Ch = HeadlineFormat.GetChr(++i);
	      } while (i <= hlen && Ch != endCh && !(endCh == 0 && !isspace(Ch)));
	      if (ESet.IsEmpty())
		{
		  message_log (LOG_ERROR, "Empty %%() declaration in: %s", S.c_str(), HeadlineFormat.c_str());
		}
	      else
		{
		  Elements.Split("?:", ESet);
		  for (const STRLIST *p = Elements.Next(); p != &Elements; p = p->Next())
		    {
		      if ((ESet == BRIEF_MAGIC) || (ESet == SOURCE_MAGIC) || (ESet == FULLTEXT_MAGIC))
			{
			  S.form("%%(%s)", ESet.c_str());
			  message_log (LOG_ERROR, "Declaration %%(%s) is not allowed in headline format", S.c_str());
			}
		      else if (RecordSyntax.GetLength())
			Present (ResultRecord, ESet, RecordSyntax, &S);
		      else
			Present (ResultRecord, ESet, &S);
		      if (!S.IsEmpty())
			{
			  StringBuffer->Cat (S);
			  break;
			}
		    }
		}
	    }
	  else
	    StringBuffer->Cat (Ch);
	} // for()
     if (StringBuffer->GetLength())
      return true;
   }
  return false;
}

bool DOCTYPE::GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const
{
  if (ResultRecord.GetRecordStart() == 0)
    {
      STRING Path = ResultRecord.GetFullFileName();
      if (Path.IsEmpty() || (GetFileSize(Path) - ResultRecord.GetLength() > 3))
	{
	  return false;
	}
      Db->_get_resource_path(&Path);
      if (StringBuffer) *StringBuffer = Path;
      return true;
    }
  return false;
}

static bool _valid_URL_method(const char *base, size_t len, bool *d)
{
  const struct {
    const char *method;
    size_t      len;
    bool dslash;
  } methods[] = {
    {"ftp",     3, true},
    {"urn",     3, true},
    {"file",    4, true},
    {"http",    4, true},
    {"news",    4, false},
    {"nntp",    4, true},
    {"ldap",    4, true},
    {"wais",    4, true},
    {"x500",    4, true},
    {"https",   5, true},
    {"shttp",   5, true},
    {"whois",   5, true},
    {"gopher",  6, true},
    {"z39_50s", 7, true},
    {"z39_50r", 7, true},
    {"prospero",8, true}
  };
  const size_t  nr_methods = sizeof(methods)/sizeof(methods[0]);

  for (size_t i=0; i < nr_methods; i++)
    {
      if (len < methods[i].len)
	break;
      if (len == methods[i].len && memcmp(methods[i].method, base, len) == 0)
	{
	  if (d) *d = methods[i].dslash;
	  return true;
	}
    }
  return false; // Nope
}

bool DOCTYPE::URL(const RESULT& ResultRecord, STRING *StringBuffer,
	bool OnlyRemote) const
{
  STRING DocumentRoot, Path;
  bool isMirror = false;

  StringBuffer->Clear(); // Clear the buffer

  if (GetResourcePath(ResultRecord, &Path) == false)
    {
      message_log (LOG_ERROR, "URL: No Resource Path defined");
      return false;
    }

  if (Db == NULL || ((Db->GetHTTP_root(&DocumentRoot, &isMirror) == false) && OnlyRemote))
    {
      //message_log (LOG_DEBUG, "URL: No Document Root (base) defined");
      return false;
    }
  const STRINGINDEX root_len = DocumentRoot.GetLength();

  if (root_len)
    {
      STRING HTTP_Server;
      if (isMirror || Db->GetHTTP_server(&HTTP_Server))
	{
	  // Is the document in the WWW tree?
	  message_log (LOG_DEBUG, "Looking for %s in %s", DocumentRoot.c_str(), Path.c_str());

	  if (Path.SubString(1, root_len) ==  DocumentRoot)
	    {
	      Path.EraseBefore(root_len + 1);
	      // MIRROR_LAYOUT
	      if (isMirror)
		{
		  // Special case is:
		  //  www. -> http
		  //  ftp. -> ftp
		  STRING s = Path.Left((size_t)4);
		  if (s == "www.") Path.Prepend("http/");
		  else if (s == "ftp.") Path.Prepend("ftp/");

		  //  http/furball.nonmonotonic.net_80/ --> http://furball.nonmonotonic.net:80/
		  //  news/rec.bicycles.tech/  --> news:rec.bicycles.tech
		  STRINGINDEX x = Path.Search("/");

		  // Special Case is where there is no method (then always http://
		  if (x > 4 && isdigit(Path.GetChr(x-1)) && isdigit(Path.GetChr(x-2)) &&
			(Path.GetChr(x-3) == '_' || isdigit(Path.GetChr(x-3))))
		    {
		      // Have at least nn/
		      Path.Prepend("http/");
		      x = 5;
		    }
 
		  bool dslash;
		  if (x > 3 && _valid_URL_method(Path, x-1, &dslash))
		    {
		      if (dslash)
			Path.Insert(x, ":/"); // turn / --> ://
		      else
			Path.SetChr(x, ':'); // turn / --> :
		      if ((x = Path.Search("_", x+1)) != 0 && isdigit (Path.GetChr(x+1)))
			{
			  Path.SetChr(x, ':'); 
#if 1 /* Get rid of the :80 stuff in http */
			  if (Path.GetChr(x+1) == '8' && Path.GetChr(x+2) == '0' && Path.GetChr(x+3) == '/' &&
				strncmp(Path, "htt", 3) == 0)
			    {
			      STRING rest = Path.c_str() + x + 2;
			      Path.EraseAfter(x-1);
			      Path.Cat (rest);
			    } 
#endif
			}
		      if (Path.Right ((size_t)9) == "/_._.html")
			Path.EraseAfter(Path.GetLength() - 9);
		      *StringBuffer = Path;
		      return true;
		    }
		  if (x > 1)
		    message_log (LOG_ERROR, "Unknown URL method in mirror tree: %s", Path.EraseAfter(x-1).c_str());
		  else
		    message_log (LOG_ERROR, "Bad Mirror layout: %s", Path.c_str());
		  return false; // Bad Mirror
		}
	      if (!isMirror)
		{
		  *StringBuffer << HTTP_Server << Path;
		  // message_log (LOG_DEBUG, "Returning URL: %s", StringBuffer->c_str());
		  return true;
		}
	    }
	}
      if (OnlyRemote)
	return false;
    }
  if (Path.GetLength())
    {
      *StringBuffer << "file://" <<  ExpandFileSpec(Path);
    }
  return true;
}

bool DOCTYPE::Full(const RESULT& ResultRecord, const STRING& RecordSyntax, STRING *StringBuffer) const
{
  DocPresent(ResultRecord, FULLTEXT_MAGIC, RecordSyntax, StringBuffer);
  return StringBuffer->GetLength() != 0;
}

INT DOCTYPE::UnifiedNames (const STRING& Tag, PSTRLIST Value) const
{
  return UnifiedNames (Tag, Value, true);
}

STRING DOCTYPE::UnifiedName (const STRING& tag) const
{
  return UnifiedName(tag, NULL);
}


// Return the first Entry (if multiple defined).
STRING DOCTYPE::UnifiedName (const STRING& Tag, STRING *Value) const
{
  STRING Result;

  if (Tag.IsEmpty())
    return Tag;

  if (Unified.GetValue(Tag, &Result) == 0)
    {
      STRLIST Names;

      if (UnifiedNames(Tag, &Names) == 0)
	{
	  Result.Clear();
	}
      else
	{
	  Names.GetEntry(1, &Result);
	}
    }
  if (Result == Ignore)
    Result.Clear();
  
  if (Value)
    *Value = Result;

  return Result;
}


STRING& DOCTYPE::DescriptiveName(const STRING& FieldName, STRING *Value) const
{
  *Value = FieldName;
  if (!Value->IsEmpty())
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

STRING& DOCTYPE::DescriptiveName(const STRING& Language,
	const STRING& FieldName, STRING *Value) const
{
  if (tagRegistry)
    {
      STRING Section ("Present");
      if (Language.GetLength())
	{
	  Section.Cat (" ");
	  Section.Cat (Language);
	}
      tagRegistry->ProfileGetString(Section, FieldName, "???", Value);
      // If set to anything then..
      if (!Value->Equals("???"))
	return (*Value); // Done
    }
  // Not set or no registry available
  return DescriptiveName(FieldName, Value);
}

void DOCTYPE::Present (const RESULT& ResultRecord, const STRING& ElementSet, STRING *StringBufferPtr) const
{

  StringBufferPtr->Clear();

  if (ElementSet == SOURCE_MAGIC)
    {
      ResultRecord.GetRecordData (StringBufferPtr);
    }
  else if (ElementSet == FULLTEXT_MAGIC)
    {
      if (Db)
	ResultRecord.GetRecordData (StringBufferPtr, Db->GetDocTypePtr ( ResultRecord.GetDocumentType()));
      else
	ResultRecord.GetRecordData (StringBufferPtr);
    }
  else if (Db && Db->DfdtGetTotalEntries ())
    {
      STRING FieldName;
      if (ElementSet == BRIEF_MAGIC)
	{
	  // Use 1st element
	  const STRING Key = ResultRecord.GetKey();
	  DFDT Dfdt;
	  if (!Db->GetRecordDfdt (Key, &Dfdt))
	    return;
	  size_t Total = Dfdt.GetTotalEntries();
	  if (Total)
	    {
	      STRING  field;
	      DFD Dfd;
	      for (size_t i=1; i<= Total; i++)
		{
		  Dfdt.GetEntry(i, &Dfd);
		  Dfd.GetFieldName (&field);
		  if (i == 1 || (field ^= "title") || (field ^= "headline"))
		    FieldName = "[1]" + field;
		}
	    }
	}
      else
	{
	  FieldName = ElementSet;
	}
      Db->GetFieldData (ResultRecord, FieldName, StringBufferPtr, this);
    }
  // Do we have something?
  if (StringBufferPtr->IsEmpty())
    {
      // No? Do we already know its default value?
      if (Defaults.GetValue(ElementSet, StringBufferPtr) == 0)
	{
	  // Try to see if we have a default...
	  static const STRING DefaultsEntry ("Defaults");
	  // Check if we have defaults...
	  if (tagRegistry)
	    tagRegistry->ProfileGetString(DefaultsEntry, ElementSet, NulString, StringBufferPtr);
	  if (Db)
	    Db->ProfileGetString(Doctype + " " + DefaultsEntry, ElementSet, *StringBufferPtr, StringBufferPtr);
	  // Defaults.AddEntry(ElementSet, *StringBufferPtr);
	}
    }

   // StringBufferPtr->Pack(); // 2022
}


void DOCTYPE::SetMetaIgnore(const STRING& Tag)
{
  if (Db)
    {
      STRING  Value, Section(Doctype + "/" + MetaDataMaps);
      Db->ProfileGetString(Section, Tag, NulString, &Value);
      if (Value.IsEmpty()) 
	Db->ProfileWriteString(Section, Tag, Ignore);
    }
}

bool DOCTYPE::IsMetaIgnoreField(const STRING& FieldName, STRING *Ptr) const
{
  STRING Value;
  if (Db)
    Db->ProfileGetString(Doctype+"/"+MetaDataMaps, FieldName, FieldName, &Value);
  else
    Value = FieldName;
  if (Value == FieldName && tagRegistry)
    tagRegistry->ProfileGetString(MetaDataMaps, FieldName, FieldName, &Value);

  if (Ptr)
    *Ptr = Value;

  if (Value.IsEmpty() || (Value ^= Ignore) || Value.GetChr(1) == '+')
     return true;               // Ignore this
  if (IsIgnoreMetaField(Value))
     return true;
  return false;
}



void DOCTYPE::XmlMetaPresent(const RESULT &ResultRecord, const STRING &RecordSyntax, STRING *StringBuffer) const
{
  STRING          Value;
  STRING          Key = ResultRecord.GetKey();
  DFDT            Dfdt;

  Db->GetRecordDfdt(Key, &Dfdt);
  const size_t    Total = Dfdt.GetTotalEntries();
  METADATA        Meta(Doctype);
  STRLIST         Position, Contents;

  Present(ResultRecord, BRIEF_MAGIC, NulString, &Value);
  if (!Value.IsEmpty()) {
    Position.AddEntry(Doctype);
    Position.AddEntry("LOCATOR");
    Position.AddEntry("BRIEF");
    Contents.AddEntry(Value);
    Meta.SetData(Position, Contents);
    Position.Clear();
    Contents.Clear();
  }
  // <LANGUAGE><CODE>...</CODE></LANGUAGE>
  Position.AddEntry(Doctype);
  Position.AddEntry("LOCATOR");
  Position.AddEntry("LANGUAGE");
  Position.AddEntry("CODE");
  Contents.AddEntry(ResultRecord.GetLanguageCode());
  Meta.SetData(Position, Contents);
  // <LANGUAGE><NAME>...</NAME></LANGUAGE>
  Position.Clear();
  Contents.Clear();
  Position.AddEntry(Doctype);
  Position.AddEntry("LOCATOR");
  Position.AddEntry("LANGUAGE");
  Position.AddEntry("NAME");
  Contents.AddEntry(ResultRecord.GetLanguageName());
  Meta.SetData(Position, Contents);
  SRCH_DATE TheDate = ResultRecord.GetDate ();
  if (TheDate.IsValidDate()) {
    Position.Clear();
    Contents.Clear();
    Position.AddEntry(Doctype);
    Position.AddEntry("LOCATOR");
    Position.AddEntry("DATE");
    Contents.AddEntry(TheDate.ISOdate());
    Meta.SetData(Position, Contents);
  }
  if (URL(ResultRecord, &Value, true)) {
    Position.Clear();
    Contents.Clear();
    Position.AddEntry(Doctype);
    Position.AddEntry("LOCATOR");
    Position.AddEntry("URL");
    Contents.AddEntry(Value);
    Meta.SetData(Position, Contents);
  }

  DFD             Dfd;
  STRING          FieldName;
  for (size_t i = 1; i <= Total; i++) {

    Dfdt.GetEntry(i, &Dfd);
    Dfd.GetFieldName(&FieldName);

    if (IsMetaIgnoreField(FieldName, &Value))
	continue;		// Ignore this

    if (FieldName != Value)
      FieldName = Value;

    if (FieldName.GetChr(1) == '+')
      continue;

    // Get Value of the field
    Db->GetFieldData(ResultRecord, FieldName, &Value, this);
    if (!Value.IsEmpty()) {
      Position.Clear();
      Contents.Clear();
      Position.AddEntry(Doctype);
      size_t          pos;
      STRING          S;
      while ((pos = FieldName.Search('.')) != 0) {
	S = FieldName;
	S.EraseAfter(pos - 1);
	FieldName.EraseBefore(pos + 1);
	Position.AddEntry(S);
      }
      while (Value.GetChr(1) == '(') {
	if ((pos = Value.Search( /* ( */ ')')) != 0) {
	  S = 1 + (const char *) Value;
	  Value.EraseBefore(pos + 1);
	  S.EraseAfter(pos - 2);
	  FieldName.Cat(' ');
	  if ((pos = S.Search('=')) != 0) {
	    if (S.GetChr(pos + 1) != '"' && S.GetChr(pos + 1) != '\'') {
	      char            qchar = S.Search('"') == 0 ? '"' : '\'';
	      S.Insert(pos + 1, qchar);
	      S.Cat(qchar);
	    }
	  }
	  FieldName.Cat(S);
	} else
	  break;
      }
      Position.AddEntry(FieldName);
      Contents.AddEntry(Value);
      Meta.SetData(Position, Contents);
    }
  }				/* for */
  if (RecordSyntax == HtmlRecordSyntax) {
    STRING          value = ResultRecord.GetCharsetCode();
    // HTTP Header (for transport)
    *StringBuffer = "Content-type: text/xml";
    if (!value.IsEmpty() && (value != "us-ascii"))
      *StringBuffer << "; charset=" << value;
    SRCH_DATE       date(ResultRecord.GetDate());
    if (date.Ok())
      *StringBuffer << "\nLast-Modified: " << date.RFCdate();
    *StringBuffer << "\nLanguage: " << ResultRecord.GetLanguageCode();
    Present(ResultRecord, "$ETAG", &Value);
    if (!Value.IsEmpty())
      *StringBuffer << "\nETag: " << Value;
    StringBuffer->Cat("\n\n");
    StringBuffer->Cat((STRING) Meta);
  } else
    *StringBuffer = (STRING) Meta;
}

// Build XML Head element
void DOCTYPE::XmlHead (const RESULT& ResultRecord, const STRING& ElementSet, STRING *StringBuffer) const
{
  // Get Document type
  STRING charset  = ResultRecord.GetCharsetCode();

  // HTTP Header (for transport)
  *StringBuffer = "Content-type: text/xml";
  if (!charset.IsEmpty() && (charset != "us-ascii"))
    *StringBuffer << "; charset=" << charset;
  SRCH_DATE date ( ResultRecord.GetDate());
  if (date.Ok())
    *StringBuffer << "\nLast-Modified: " <<  date.RFCdate();
  *StringBuffer << "\n\n<?XML VERSION=\"1.0\" ENCODING=\"" << charset << "\" ?>\n<!-- \n"
    << Copyright << "\n-->\n";
  if (!Doctype.IsEmpty())
    *StringBuffer << "<?xml-stylesheet href=\"/xml-css/" << Doctype << ".css\" type=\"text/css\"?>\n";
}

void DOCTYPE::XmlTail (const RESULT& ResultRecord, const STRING& ElementSet, STRING *StringBufferPtr) const
{
  StringBufferPtr->Cat ("<!-- End of ");
  StringBufferPtr->Cat (ElementSet);
  StringBufferPtr->Cat (" Document -->\n");
}


// Build SGML Head element
void DOCTYPE::SgmlHead (const RESULT& ResultRecord, const STRING& ElementSet, STRING *StringBuffer) const
{
  // Get MIME type
  STRING MIME;

  SourceMIMEContent(ResultRecord, &MIME);
  if (MIME.Search("html"))
    {
      // HTML is already SGML!
      HtmlHead (ResultRecord, ElementSet, StringBuffer);
      return;
    }
  // HTTP Header (for transport)
  *StringBuffer = "Content-type: text/sgml";
  SRCH_DATE date ( ResultRecord.GetDate());
  if (date.Ok())
    *StringBuffer << "\nLast-Modified: " <<  date.RFCdate();
  StringBuffer->Cat("\n\n");

  if ((MIME.Search("sgml") == 0 && MIME.Search("xml") == 0) ||
	(ElementSet != FULLTEXT_MAGIC))
    {
      const STRING level1 = (ElementSet != FULLTEXT_MAGIC) ? ElementSet : Doctype;
      *StringBuffer
	// You may NOT alter the line below without prior writen permission
	// from Basis Systeme netzwerk, Munich.
	<< "<!--\n" << Copyright << "\n"
	<< Modifications << "\n-->\n"
	<< "<!DOCTYPE " << Doctype << " PUBLIC \"-//BSN//DTD " <<
		level1 << "//EN\">\n<" << Doctype << ">\n";
    }
  else
    *StringBuffer << "<!-- Delivered by re-Isearch -->\n";
}

void DOCTYPE::SgmlTail (const RESULT& ResultRecord, const STRING& ElementSet , STRING *StringBuffer) const
{
  // Get MIME type
  STRING MIME;

  SourceMIMEContent(ResultRecord, &MIME);
  if (MIME.Search("html"))
    {
      // HTML is already SGML!
      HtmlTail (ResultRecord, ElementSet, StringBuffer);
      return;
    }
  if ((MIME.Search("sgml") == 0 && MIME.Search("xml") == 0) ||
        (ElementSet != FULLTEXT_MAGIC))
    {
      StringBuffer->Cat ("</");
      StringBuffer->Cat ((ElementSet != FULLTEXT_MAGIC) ? ElementSet :  Doctype );
      StringBuffer->Cat (">");
    }
  StringBuffer->Cat ("\n<!-- End of Document -->\n");
}

// Build HTML Head element
void DOCTYPE::HtmlHead (const RESULT& ResultRecord, const STRING& ElementSet,
	STRING *StringBuffer) const
{
  static const STRING Section ("FETCH");
  STRING BACKGROUND, BGCOLOR, TEXTCOLOR, LINKCOLOR, ALINKCOLOR, VLINKCOLOR, ONLOAD;

  Db->ProfileGetString (Section, "BACKGROUND", NulString, &BACKGROUND);
  Db->ProfileGetString (Section, "BGCOLOR", "#FAF0E6", &BGCOLOR);
  Db->ProfileGetString (Section, "TEXT",    "#000000", &TEXTCOLOR);
  Db->ProfileGetString (Section, "LINK",    "#2F50F0", &LINKCOLOR);
  Db->ProfileGetString (Section, "ALINK",   "#7A67EE", &ALINKCOLOR);
  Db->ProfileGetString (Section, "VLINK",   "#4169E1", &VLINKCOLOR);
  Db->ProfileGetString (Section, "ONLOAD",  NulString, &ONLOAD);

  // Get Document type
//  STRING Doctype = ResultRecord.GetDocumentType ().DocumentType();
//  Doctype.ToUpper(); // Want uppercase name

  if (ONLOAD.IsEmpty())
    {
      ONLOAD = "self.defaultStatus='converted";
      if (Doctype.GetLength())
	ONLOAD << " from " << Doctype;
      ONLOAD << " to HTML by re-Isearch';return true";
    }

  // Get Record Key
  STRING Key;
  ResultRecord.GetKey(&Key);

  // Get Database name
  STRING DBname;
#ifdef WWW_SUPPORT
  Db->DbName(&DBname);
  RemovePath(&DBname); // Strip path

  // Get URL
  STRING URL;
  Db->URLfrag(ResultRecord, &URL);
#endif

  *StringBuffer = "Content-type: text/html";

  STRING charset  = ResultRecord.GetCharsetCode();
  if (!charset.IsEmpty() && (charset != "us-ascii"))
    *StringBuffer << "; charset=" << charset;

#ifdef SHOW_DATE
  SRCH_DATE date ( ResultRecord.GetDate());

  if (date.Ok())
    {
      *StringBuffer << "\nLast-Modified: " << date.RFCdate();
    }
#endif
  STRING ETag;
#ifdef WWW_SUPPORT
  ETag.form("%lx-%lx-%lx", Key.Hash(), ElementSet.Hash(), DBname.Hash());
#else
  ETag.form("%lx-%lx-%lx", Key.Hash(), ElementSet.Hash(), Doctype.CaseHash());
#endif
  *StringBuffer << "\nETag: " << ETag;

  // Get Language
  STRING lang =  ResultRecord.GetLanguageCode();
  if (!lang.IsEmpty())
    *StringBuffer << "\nLanguage: " << lang;

  // You may NOT alter the line below without prior writen permission
  // from NONMONOTONIC Networks http://www.nonmonotonic.net
  *StringBuffer
	<< "\n\n<!--\n" << Copyright << "\n"
	<< Modifications << "\n-->\n"
	<< "\
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML " << DTD_VER << "//EN\">\n\
<HTML><HEAD>\n\
<LINK REV=DTD HREF=\"" << DTD_URL << DTD_VER << ".dtd\"\
 TITLE=\"DTD this document\" />\
<LINK REV=\"made\" HREF=\"" << _IB_HTDOCS_HOME;
  if (Doctype.GetLength())
    *StringBuffer << Doctype << ".html";
  *StringBuffer << "\" TITLE=\"NONMONOTONIC Smart " << Doctype << " Doctype\" />\n";

#ifdef WWW_SUPPORT
  *StringBuffer << "\
<!-- Forward Links for toolbars/menus -->\n\
<LINK REL=Index HREF=\"" << CGI_IINFO << URL << "\" TITLE=\"Record Info\" />\n\
<LINK REL=Contents HREF=\"" << CGI_IINFO << "?" << DBname << "\" TITLE=\"Database Info\" />\n\
<LINK REL=Home HREF=\"" << CGI_FORM << "?" << DBname << "\" TITLE=\"Search (" << DBname << ")\" />\n\
<LINK REL=Bookmark HREF=\"" << CGI_FETCH << URL << "+" << SOURCE_MAGIC << "\" TITLE=\"Raw";
  if (Doctype.GetLength())
    {
      *StringBuffer << " (" << Doctype << ")";
    }   
  *StringBuffer << " Record\" />\n";

  if (ElementSet != FULLTEXT_MAGIC)
    {
      *StringBuffer << "<LINK REL=Up HREF=\"" << CGI_FETCH << 
	URL << "+" << FULLTEXT_MAGIC << "\" TITLE=\"Full Record\" />\n";
    }

  Db->URLfrag(&URL);
  STRING S;
  if (!(S=Db->FirstKey()).IsEmpty() && (S != Key))
   {
     *StringBuffer << "<LINK REL=First HREF=\"" << CGI_FETCH << URL <<
	S << "\" TITLE=\"First Record\" />\n";
   }

  if (!(S=Db->PrevKey(Key)).IsEmpty() && (S != Key))
   {
     *StringBuffer << "<LINK REL=Previous HREF=\"" << CGI_FETCH << URL <<
	S << "\" TITLE=\"Previous Record\" />\n";
   }

  if (!(S=Db->NextKey(Key)).IsEmpty() && (S != Key))
    {
      *StringBuffer << "<LINK REL=Next HREF=\"" << CGI_FETCH << URL <<
	S << "\" TITLE=\"Next Record\" />\n";
    }
  if (!(S=Db->LastKey()).IsEmpty() && (S != Key))
    {
      *StringBuffer << "<LINK REL=Last HREF=\"" << CGI_FETCH << URL <<
	S << "\" TITLE=\"Last Record\" />\n";
    }
#endif
  *StringBuffer << "<LINK REL=Help HREF=\"" << _IB_HTDOCS_HOME << " \
TITLE=\"re-Isearch Online Documentation\" />\n";


  // Now Meta(s)
  *StringBuffer << "<META NAME=\"Generator\" CONTENT=\"re-Isearch/1.152 (";
  if (Doctype.GetLength())
    *StringBuffer << "on-the-fly " << Doctype << " to HTML conversion/";
  *StringBuffer << __HostPlatform << ")[NONMONOTONIC]\" />\n\
<META NAME=\"Format\" CONTENT='(Schema=imt)text/html; charset=" << charset << "' />";

  STRING Rights;
  Db->GetCopyright(&Rights);
  if (Rights.GetLength())
    {
      STRING quot = "\"";
      if (Rights.Search('"'))
	{
	  if (Rights.Search("'"))
	    Rights.Replace("\"", "&quot;");
	  else
	    quot = "'";
	}
      *StringBuffer << "\n<META NAME=\"Rights\" CONTENT=" <<
	quot << Rights << quot << " />";
    }

  // Print the owner of the resource...
  STRING Path = ResultRecord.GetFullFileName();

  if (StringBuffer)
    {
      STRING Owner (  ResourceOwner(Path) );
      if (Owner.GetLength())
	*StringBuffer << "\n<META NAME=\"Record-Owner\" CONTENT=\"" << Owner << "\" />";
    }

  // Publisher
  Path = ( DBname = Db->GetDbFileStem());
  Path.Cat (".ini");
  if (StringBuffer)
    {
      STRING Publisher ( ResourcePublisher(Path) );
      if (Publisher.GetLength())
	*StringBuffer << "\n<META NAME=\"Record-Publisher\" CONTENT=\"" << Publisher <<  "\" />";
    }

  {
    STRING Name, Address;
    Db->GetMaintainer(&Name, &Address);
    if (Name.GetLength())
      {
	StringBuffer->Cat ("\n<META NAME=\"Maintainer.Name\" CONTENT=\"");
	Charset.HtmlCat(Name, StringBuffer, false);
	StringBuffer->Cat ("\" />");
      }
    if (Address.GetLength())
      {
	StringBuffer->Cat ("\n<META NAME=\"Maintainer.Email\" CONTENT=\"");
	Charset.HtmlCat(Address, StringBuffer, false);
	StringBuffer->Cat ("\" />");
      }
  }

  RemovePath(&DBname); // Strip path

  STRING Value;
  // Show Language
  if (!lang.IsEmpty())
    {
// <META HTTP-EQUIV="Content-Language" CONTENT="en-GB">
// Uses the language-dialect convention
      *StringBuffer << "\n<META HTTP-EQUIV=\"Content-Language\" CONTENT=\"" << lang << "\" />\n\
<META NAME=\"Language-of-Record\" CONTENT=\"" << ResultRecord.GetLanguageName() << "\" />\n";
    }
  *StringBuffer << "<META HTTP-EQUIV=\"ETag\" CONTENT=\"" << ETag << "\" />\n";
#ifdef SHOW_DATE
  *StringBuffer << "<META HTTP-EQUIV=\"Last-Modified\" CONTENT=\"" << date.RFCdate() << "\" />\n";
  // the date of the conversion
  *StringBuffer << "<META NAME=\"Date\" CONTENT=\"(Schema=ISO8601)" << ISOdate(0) << "\" />\n";
  if (date.Ok())
    {
      // The date of the record..
      *StringBuffer << "<META NAME=\"Date-of-Record-Publication\" CONTENT=\"(Schema=ISO8601)"
	<< date.ISOdate() << "\" />\n";
    }
#endif

#ifdef WWW_SUPPORT
  STRING HTTP_Server;
  if (Db->GetHTTP_server(&HTTP_Server))
    {
      STRING base;

      const char *host = getenv ("SERVER_NAME");
      const char *port = getenv ("SERVER_PORT");

      if (host && *host)
	{
	  int portNum = port ? atoi(port) : 80;
	  if (portNum && portNum != 80)
	    base.form("http://%s:%d/", host, portNum);
	  else
	    base.form("http://%s/", host);
	}
      else
	base = HTTP_Server;

      STRING url("?");
      const char *p = getenv("SCRIPT_NAME");
      if (p && *p)
	base << (*p == '/' ? p+ 1 : p);
      else
	base << CGI_DIR << "/" << CGI_FETCH;
      if ((p = getenv("QUERY_STRING")) != NULL && *p)
	{
	  url << p;
	}
      else
	{
	  url << DBname << "+" << Key;
	  if (!ElementSet.IsEmpty())
	    url << "+" << ElementSet;
	}

      *StringBuffer << "<META NAME=\"identifier\" CONTENT=\"(Schema=URL)" << base << url << "\" />\n";
      *StringBuffer << "<META NAME=\"server\" CONTENT=\"(Schema=URL)" << HTTP_Server << "\" />\n";
      *StringBuffer << "<BASE HREF=\"" << base << url << "\" />\n";

      *StringBuffer << "<script language=\"JavaScript\">\
<!--\nif (top.length==0)\nwindow.location=\"i.fetch" << 
	url << "\";\n// -->\n</script>\n";
    }
  else
#endif
    {
      *StringBuffer  << "<META NAME=\"Identifier.DB\" CONTENT=\"" << DBname
	<< "\" />\n<META NAME=\"Identifier.Key\" CONTENT=\"" << Key
	<< "\" />\n<META NAME=\"Identifier.ES\" CONTENT=\"" << ElementSet << "\" />\n";
    }

  // Get Headline (Use Virtual code!)
  Present (ResultRecord, BRIEF_MAGIC, NulString, &Value); // BUGFIX: Use NON-HTML
  if (!Value.IsEmpty())
    {
      *StringBuffer << "<TITLE>";
      HtmlCat(ResultRecord, Value, StringBuffer, false); // BUGFIX: Convert to HTML
      if (ElementSet != FULLTEXT_MAGIC)
	*StringBuffer << " (" << ElementSet << ")";
      *StringBuffer << "</TITLE>";
    }

  *StringBuffer << "</HEAD>\n<BODY onLoad=\"" << ONLOAD;
  if (BGCOLOR.GetLength())
    *StringBuffer << "\" BGCOLOR=\""    << BGCOLOR;
  if (TEXTCOLOR.GetLength())
    *StringBuffer << "\" TEXT=\""       << TEXTCOLOR;
  if (LINKCOLOR.GetLength())
    *StringBuffer << "\" LINK=\""       << LINKCOLOR;
  if (ALINKCOLOR.GetLength())
    *StringBuffer << "\" ALINK=\""      << ALINKCOLOR;
  if (VLINKCOLOR.GetLength())
    *StringBuffer << "\" VLINK=\""      << VLINKCOLOR;
  if (BACKGROUND.GetLength())
    *StringBuffer << "\" BACKGROUND=\"" << BACKGROUND;
  *StringBuffer << "\"><A HREF=\"" <<  _IB_HTDOCS_HOME << " \
onMouseOver=\"self.status='re-Isearch Documentation'; return true\">\
<IMG BORDER=0 ALIGN=Right SRC=\"/IB/Asset/logo.gif\" \
ALT=\"re-Isearch " << __IB_Version << "\" WIDTH=\"128\" \
HEIGHT=\"28\" /></A>\n";
}


void DOCTYPE::HtmlTail (const RESULT&, const STRING&, STRING *StringBuffer) const
{
  STRING copyright;
  Db->GetCopyright(&copyright);
  StringBuffer->Cat ("\n<!-- Tail -->");
  if (copyright.GetLength())
    {
      StringBuffer->Cat ("<HR /><H5 ALIGN=\"Right\">&copy; ");
      Charset.HtmlCat (copyright, StringBuffer);
      StringBuffer->Cat ("</H5>");
    }
  StringBuffer->Cat ("</BODY></HTML>\n<!-- End of Document -->\n");
}


// Build Head element
void DOCTYPE::DocHead (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, STRING *StringBuffer) const
{
  STRING content;
  if ((ElementSet == SOURCE_MAGIC) || (RecordSyntax == RawRecordSyntax))
    SourceMIMEContent(ResultRecord, &content);
  else if (RecordSyntax.GetLength() == 0 || (RecordSyntax == SutrsRecordSyntax))
    content = "text/plain";
  else if (RecordSyntax == HtmlRecordSyntax)
    content = "text/html";
  else if (RecordSyntax == SgmlRecordSyntax)
    content = "text/sgml";

  *StringBuffer << "Content-type: " << content << "\n\n";


  content = ResultRecord.GetCharsetCode();
  if (content.GetLength() && (content != "us-ascii"))
    *StringBuffer << "; charset=" << content;
#ifdef SHOW_DATE
  SRCH_DATE date (ResultRecord.GetDate());
  if (date.Ok() > 0)
    {
      *StringBuffer << "\nLast-Modified: " << date.RFCdate();
    }
#endif
  *StringBuffer << "\n\n";

  if (ElementSet != SOURCE_MAGIC)
    {
      if (RecordSyntax == HtmlRecordSyntax)
	HtmlHead(ResultRecord, ElementSet, StringBuffer);
      else if (RecordSyntax == SgmlRecordSyntax)
	SgmlHead(ResultRecord, ElementSet, StringBuffer);
    }
}

// Build Tail element
void DOCTYPE::DocTail (const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, STRING *StringBuffer) const
{
  if (ElementSet != SOURCE_MAGIC)
    {
      if (RecordSyntax == HtmlRecordSyntax)
	HtmlTail(ResultRecord, ElementSet, StringBuffer);
      else if (RecordSyntax == SgmlRecordSyntax)
	SgmlTail(ResultRecord, ElementSet, StringBuffer);
   }
}

const STRING& DOCTYPE::URLencode(const STRING& Str, STRING *Code) const
{
  bool quot = false;
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
	    if (isascii(ch) && isalnum(ch))
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
  return (*Code = q);
}

void DOCTYPE::HtmlCat (const RESULT& Result, const CHR ch, STRING *StringBufferPtr) const
{
  Result.GetLocale().Charset().HtmlCat(ch, StringBufferPtr);
}


void DOCTYPE::HtmlCat (const RESULT& Result, const STRING& Input, STRING *StringBufferPtr, bool Anchor) const
{
  Result.GetLocale().Charset().HtmlCat(Input, StringBufferPtr, Anchor);
}


void DOCTYPE::HtmlCat (const RESULT& Result, const STRING& Input, STRING *StringBufferPtr) const
{
  Result.GetLocale().Charset().HtmlCat(Input, StringBufferPtr);
}

void DOCTYPE::HtmlCat (const STRING& Input, STRING *StringBufferPtr, bool Anchor) const
{
  Charset.HtmlCat(Input, StringBufferPtr, Anchor);
}

void DOCTYPE::HtmlCat (const STRING& Input, STRING *StringBufferPtr) const
{
  Charset.HtmlCat(Input, StringBufferPtr);
}

void DOCTYPE::HtmlCat (const CHR Ch, STRING *StringBufferPtr) const
{
  Charset.HtmlCat(Ch, StringBufferPtr);
}


void DOCTYPE::Present (const RESULT& ResultRecord, const STRING& ElementSet,
	 const STRING& RecordSyntax, STRING *StringBufferPtr) const
{
  StringBufferPtr->Clear();

  if (ElementSet.GetLength() == 0) return;

#if 0
  FILETYPE ft = Db->GetFieldType(ElementSet);
  if (ft.IsNumeric() || ft.IsComputed())
    {
      if (RecordSyntax == HtmlRecordSyntax)
	{
	}
    }
#endif

#ifdef WWW_SUPPORT
// ADDED
  // BASE processing
  if (ElementSet == "BASE@HREF")
    {
      if (Db->GetHTTP_server(StringBufferPtr))
        {
          const char *p = getenv("SCRIPT_NAME");
          if (p && *p)
            *StringBufferPtr << p;
          else
            *StringBufferPtr << CGI_DIR << "/" << CGI_FETCH;
	  STRING Key;
	  ResultRecord.GetKey(&Key);
	  STRING DBname;
	  Db->DbName(&DBname);
	  RemovePath(&DBname); // Strip path
	  *StringBufferPtr << "?" << DBname << "+" << Key;
        }
      return;
    }
// END ADDED
#endif

  if (ElementSet == SOURCE_MAGIC)
    {
      DOCTYPE::DocPresent(ResultRecord, ElementSet, RecordSyntax,
	StringBufferPtr);// Get Raw Record Content
      return; // Done
    }
  if (ElementSet == FULLTEXT_MAGIC)
    {
      DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBufferPtr);
      return;
    }

  bool html =  (RecordSyntax == HtmlRecordSyntax);
  if (ElementSet == BRIEF_MAGIC)
    {
      STRING S;
      Present (ResultRecord, ElementSet, &S);
      S.Pack(); // Headlines should be a single "compact" line

      if (S.IsEmpty())
	{ 
	  S = ResultRecord.GetFullFileName();
	  if (Doctype.GetLength())
	    S << " (" << Doctype << ")";
	}
      if (html)
	HtmlCat(ResultRecord, S, StringBufferPtr, false);
      else
	*StringBufferPtr = S;
    }
  else
    {
      STRLIST List; 
      Db->GetFieldData (ResultRecord, ElementSet, &List, this);
      INT Total = List.GetTotalEntries();

      if (Total != 0)
	{
	  STRING S;
	  if (html && Total > 1) StringBufferPtr->Cat("<UL>");
	  for (INT i=1; i<= Total; i++)
	    {
	      List.GetEntry(i, &S);
	      if (html && Total > 1)
		{
		  StringBufferPtr->Cat("<LI>");
		  HtmlCat(ResultRecord, S, StringBufferPtr);
		  StringBufferPtr->Cat("</LI>\n");
		}
	      else
		{
		  if (html) HtmlCat(ResultRecord, S, StringBufferPtr);
		  else StringBufferPtr->Cat(S);
		  if (i < Total) StringBufferPtr->Cat(", ");
		}
	    }
	  if (html && Total > 1) StringBufferPtr->Cat("</UL>");
	}
      else if (html)
	{
	  STRING S;
	  Present (ResultRecord, ElementSet, &S);
	  HtmlCat(ResultRecord, S, StringBufferPtr); // Convert to HTML
	}
      else
	{
	  Present (ResultRecord, ElementSet, StringBufferPtr);
	}
    }

  StringBufferPtr-> Pack( ); // 2022

}


// Highlight the Document
void DOCTYPE::DocHighlight (const RESULT& ResultRecord, const STRING& RecordSyntax,
	STRING *StringBuffer) const
{
  if (RecordSyntax == HtmlRecordSyntax)
    {
      STRING Htmlified;
      STRINGINDEX pos, Start = 1, i = 1;
      STRING mark;

      // WARNING: THis depends on the above code 6m getting translated into <STRONG>
      Db->HighlightedRecord(ResultRecord, "\033[6m", "\033[m", StringBuffer);
      HtmlCat(ResultRecord, *StringBuffer, &Htmlified);

      while ((pos = Htmlified.Search("<STRONG>", Start)) > 0)
	{
	  int j = mark.form("<A HREF=\"#M%d\" onMouseOver=\"self.status='Previous'; return true\">\
<IMAGE SRC=\"/IB/Asset/ll.gif\" BORDER=0 ALT=\"&lt;--\"></A>\
<A NAME=\"M%d\"><!-- Match #%d --></A>\
<font style=\"color:black;background-color:#ffff66\">", i-1, i, i); 
	  Start = pos + 8;
	  Htmlified.Insert(Start, mark);
	  i++;
	  if ((pos = Htmlified.Search("</STRONG>", Start+j)) > 0)
	    {
	      mark.form("</font><A HREF=\"#M%d\" onMouseOver=\"self.status='Next'; return true\">\
<IMAGE SRC=\"/IB/Asset/lr.gif\" BORDER=0 ALT=\"--&gt;\"></A>", i);
	      Htmlified.Insert(pos, mark);
	    }
	}
      StringBuffer->Clear();
      HtmlHead (ResultRecord, FULLTEXT_MAGIC, StringBuffer); // !DOCTYPE and HEAD elements
      if (i > 1)
	{
	  *StringBuffer << "<FONT SIZE=\"-1\">Go to [<A HREF=\"#M1\">the&nbsp;first&nbsp;hit</A>] \
[<A HREF=\"#M" << i << "\">the&nbsp;end&nbsp;of&nbsp;document</A>] </FONT><HR /><P>";
	}
      *StringBuffer << "<PRE><A NAME=\"M0\"></A>" << Htmlified << "<A NAME=\"M" << i << "\"></A></PRE>";
      if (i > 1)
	{
	  *StringBuffer << "</P><P><HR /><FONT SIZE=\"-1\">Go to [<A HREF=\"#M"
		<< (i-1)
		<< "\">the&nbsp;last&nbsp;hit</A>] [<A HREF=\"#M0\">\
the&nbsp;start&nbsp;of&nbsp;document</A>]</FONT></P>";
       }
      HtmlTail (ResultRecord, FULLTEXT_MAGIC, StringBuffer); // Tail bits
    }
  else // Use VT100 style highlighting..
    Db->HighlightedRecord(ResultRecord, "\033[7m", "\033[m", StringBuffer);
}

// Present the specified ElementSet as a document with the
// specified RecordSyntax..
//
// Note the Cgi-bin Content-type header line is produced here!
// --- this convention is to better support HTML/HTTPD conversion.
void DOCTYPE::
DocPresent(const RESULT& ResultRecord, const STRING& ElementSet, 
	const STRING& RecordSyntax, STRING* StringBufferPtr,
	const QUERY&) const
{
  STRING Filter;
  STRING Section;

  Section = "External/";
  Section.Cat (RecordSyntax); 

  // Hook to see if external method is defined..
  if (tagRegistry)
    {
      tagRegistry->ProfileGetString(Section, ElementSet, NulString, &Filter);
      if (Filter.GetLength() == 0)
	{
	  STRING tmp;
	  // ElementSet == F ? *=XXX %s --> XXX F 
	  tagRegistry->ProfileGetString(Section, "*", NulString, &tmp);
	  if (tmp.GetLength())
	    {
	      Filter.form((const char *)tmp, (const char *)ElementSet);
	    }
	}
     }
  // Over-ride in DATABASE.ini for External..
  Section.form("%s External/%s", Doctype.c_str(), RecordSyntax.c_str()); 
  Db->ProfileGetString(Section, ElementSet, Filter, &Filter);

  if (Filter.GetLength() && Filter.Search('*') == 0)
    {
      message_log (LOG_DEBUG, "External presentation filter: '%s'", Filter.c_str());
      // We have a Method
      FILE *fp = _IB_popen(Filter, "w");
      if (fp)
	{
	  STRING Filename;
	  off_t   bytes = 0;
	  if (GetResourcePath(ResultRecord, &Filename))
	    {
	      MMAP MemoryMap(Filename, MapSequential);
	      if (MemoryMap.Ok())
		{
		  const UCHR  *buffer  = (const UCHR *)MemoryMap.Ptr();
		  size_t       len     = MemoryMap.Size();
		  size_t       res;
		  // See fwrite(3s) 
		  do {
		    res = (size_t)fwrite (&buffer[len], sizeof(UCHR), len, fp);
		    len   -= res;
		    bytes += res;
		  } while (res && len > 0);
		  MemoryMap.Unmap();
		}
	    }
	  if (bytes == 0)
	    {
	      STRING Source;
	      ResultRecord.GetRecordData (&Source); // Get RAW source
	      if ((bytes = Source.GetLength()) != 0)
		Source.WriteFile (fp); // Pipe in
	    }
	  fflush(fp);
	  if (bytes)
	    StringBufferPtr->ReadFile (fp); // Read pipe out
	  else
	    message_log (LOG_WARN, "Record was empty!");
	  _IB_pclose(fp);
	  return;
	}
      message_log (LOG_ERROR, "Could not open pipe to %s [%s] presentation filter '%s'!",
		(const char *)Section, (const char *)ElementSet, (const char *)Filter);
    }
  if (ElementSet == METADATA_MAGIC)
    XmlMetaPresent(ResultRecord, RecordSyntax, StringBufferPtr);
  else
    DocPresent (ResultRecord, ElementSet, RecordSyntax, StringBufferPtr);
}



void DOCTYPE::
DocPresent (const RESULT& ResultRecord, const STRING& ElementSet,
	    const STRING& RecordSyntax, STRING *StringBufferPtr) const
{
  StringBufferPtr->Clear();
  if (ElementSet == SOURCE_MAGIC)
    {
      STRING Content;
      // Get Raw Record Content
      ResultRecord.GetRecordData (&Content);
      // HTTP Content Header
      if (RecordSyntax == HtmlRecordSyntax)
	{
	  STRING content;
	  SourceMIMEContent(ResultRecord, &content);
	  *StringBufferPtr << "Content-type: " << content << "\n\n";
	}
      *StringBufferPtr << Content;
      return; // Done
    }

  else if (RecordSyntax == HtmlRecordSyntax)
    {
      STRING content;
      SourceMIMEContent(ResultRecord, &content);

      if (ElementSet == FULLTEXT_MAGIC)
	{
	  // Get Record Content
	  *StringBufferPtr <<"Content-type: " << content << "\n\n";
	  if (content.Compare("text/html", 9) == 0)
	    {
	      ResultRecord.GetRecordData (&content); // Raw
	      *StringBufferPtr << content; // Already HTML
	    }
	  else
	    {
	      // Get Normalized entitities..
	      ResultRecord.GetRecordData (&content, Db->GetDocTypePtr ( ResultRecord.GetDocumentType()));
	      // Convert to HTML
              HtmlHead (ResultRecord, ElementSet, StringBufferPtr); // !DOCTYPE and HEAD elements
	      StringBufferPtr->Cat ("<PRE>");
	      HtmlCat (ResultRecord, content, StringBufferPtr); // Convert to HTML
	      StringBufferPtr->Cat ("</PRE>");
	      HtmlTail (ResultRecord, ElementSet, StringBufferPtr); // Tail bits
	    }
	}
      else // "Real" Element
	{
	  // Convert to HTML
	  HtmlHead (ResultRecord, ElementSet, StringBufferPtr); // !DOCTYPE and HEAD elements
	  STRLIST List;
	  Db->GetFieldData (ResultRecord, ElementSet, &List, this);
	  STRING Value;
	  INT Total = List.GetTotalEntries();
	  // For multiple values for a field we do
	  if (Total > 1)
	    *StringBufferPtr << "<OL TYPE=\"i\">"; // Ordered List
	  else
	    *StringBufferPtr << "<PRE>"; // Singular do Preformated
	  for (INT i=1; i <= Total; i++)
	    {
	      List.GetEntry(i, &Value);
	      if (Total > 1) *StringBufferPtr << "<LI>";
	      HtmlCat (ResultRecord, Value, StringBufferPtr);
	      if (Total > 1) *StringBufferPtr << "</LI>\n";
	    }
	  if (Total > 1)
	    *StringBufferPtr << "</OL>";
	  else
	    *StringBufferPtr << "</PRE>";
	  HtmlTail (ResultRecord, ElementSet, StringBufferPtr); // Tail bits
	}
    }
  else
    Present (ResultRecord, ElementSet, StringBufferPtr);
}

#if 0
STRING DOCTYPE::HtmlGetMetadata(const RESULT& record) const
{
  REGISTRY *meta = GetMetaData (record, Db->GetMetadefaults());
  STRING    data = meta.HtmlMeta();
  delete meta;
  return data;
}

REGISTRY* DOCTYPE::GetMetadata(const RECORD& record, const REGISTRY* defaults)
{
  RESULT result;

  result.SetKey(record.GetKey());
  result.SetPath(record.GetPath());
  result.SetFileName(record.GetFileName());
  result.SetDocumentType(record.GetDocumentType());
  result.SetRecordStart(record.GetRecordStart());
  result.SetRecordEnd(record.GetRecordEnd());

  return GetMetadata(result, defaults);
}

REGISTRY* DOCTYPE::GetMetadata(const RESULT& result, const REGISTRY* defaults)
{
  REGISTRY* meta = defaults->clone();
  STRLIST position, value;

  // add the title
  STRING Title;
  Present(result, BRIEF_MAGIC, SutrsRecordSyntax, &Title);
  if (s.IsEmpty())
    {
      URL(result, &Title, false);
    }
  position.AddEntry("locator");
  position.AddEntry("title");
  value.AddEntry(Title);

// Language-of-Resource
// Charset
// ...
  meta->SetData(position, value);
#if 0
  STRING Rights;
  Db->GetCopyright(&Rights);
  if (!Rights.IsEmpty())
    {
      position.AddEntry("locator");
      position.AddEntry("rights");
      value.AddEntry(Rights);
    }
  // Print the owner of the resource...
  const STRING Path = record.GetFullFileName();
  STRING Owner = RecordOwner(Path);
  if (Owner.GetLength())
    *StringBuffer << "\n<META NAME=\"Record-Owner\" CONTENT=\"" << Owner << "\" />";

  Db->GetDbFileStem(&DBname);
  // Publisher
  Path = DBname;
  Path.Cat (".ini");

  STRING Publisher = RecordPublisher(Path);
  if (Publisher.GetLength())
    *StringBuffer << "\n<META NAME=\"Record-Publisher\" CONTENT=\"" << Publisher << "\" />";

  {
    STRING Name, Address;
    Db->GetMaintainer(&Name, &Address);
    if (Name.GetLength())
      {
	StringBuffer->Cat ("\n<META NAME=\"Maintainer.Name\" CONTENT=\"");
	Charset.HtmlCat(Name, StringBuffer, false);
	StringBuffer->Cat ("\" />");
      }
    if (Address.GetLength())
      {
	StringBuffer->Cat ("\n<META NAME=\"Maintainer.Email\" CONTENT=\"");
	Charset.HtmlCat(Address, StringBuffer, false);
	StringBuffer->Cat ("\" />");
      }
  }

  RemovePath(&DBname); // Strip path

  STRING Value;
  // Show Language
  if (!lang.IsEmpty())
    {
      *StringBuffer << "\n<META NAME=\"Language-of-Record\" CONTENT=\"" << lang << "\" />";
    }
#ifdef SHOW_DATE
  // the date of the conversion
  *StringBuffer << "\n<META NAME=\"Date\" CONTENT=\"(Schema=ISO8601)" << ISOdate(0) << "\" />\n";
  if (date.Ok())
    {
      // The date of the record..
      *StringBuffer << "\n<META NAME=\"Date-of-Record-Publication\" CONTENT=\"(Schema=ISO8601)"
	<< date.ISOdate() << "\" />";
    }
#endif

      *StringBuffer << "<META NAME=\"identifier\" \
CONTENT=\"(Schema=URL)" << base << url << "\" />\n";
      *StringBuffer << "<BASE HREF=\"" << base << "\" />\n";

#endif
  return meta;
}
#endif


DOCTYPE::~DOCTYPE ()
{
  if (tagRegistry) delete tagRegistry;
}

//
// Sample hook code:
// 
//if (IsSpecialField(Field))
//  {
//     char *entry_id = (char *)tmpBuffer2.Want(val_len + 1);
//                  strncpy (entry_id, &RecBuffer[val_start], val_len);
//                  entry_id[val_len] = '\0';
//     HandleSpecialFields(NewRecord, Field, entry_id);
//  }
//


bool DOCTYPE::IsSpecialField(const STRING &FieldName) const
{
  return (FieldName.GetLength() && (
	(FieldName ^= DateField) ||
	(FieldName ^= DateCreatedField) ||
	(FieldName ^= DateModifiedField) ||
	(FieldName ^= KeyField) ||
	(FieldName ^= LanguageField) ||
	(FieldName ^= CategoryField) ||
	(FieldName ^= PriorityField) ||
	(FieldName ^= DateExpiresField) ||
	(FieldName ^= TTLField) ) );
}

void DOCTYPE::HandleSpecialFields(RECORD* NewRecord, const STRING& FieldName, const char *Buffer)
{
  if (Buffer == NULL || *Buffer == '\0')
    return;

  if ((FieldName ^= DateField) || (FieldName ^= DateCreatedField) || (FieldName ^= DateModifiedField) ||
	(FieldName ^= DateExpiresField))
    {
      SRCH_DATE Datum (ParseDate(Buffer));
      if (!Datum.Ok())
	message_log (LOG_WARN, "%s:%s Unsupported/Unrecognized date format: '%s'",
		Doctype.c_str(), FieldName.c_str(), Buffer);
      else if (FieldName ^= DateField)        NewRecord->SetDate( Datum );
      else if (FieldName ^= DateCreatedField) NewRecord->SetDateCreated( Datum );
      else if (FieldName ^= DateModifiedField)NewRecord->SetDateModified( Datum );
      else if (FieldName ^= DateExpiresField) NewRecord->SetDateExpires( Datum );
    }
  else if (FieldName ^= KeyField)
    {
      if (strncasecmp(Buffer, "urn:uuid:", 9) == 0 && strlen(Buffer)>(DocumentKeySize-8))
	Buffer += 10;
      STRING Key (Buffer);
      if (Db->KeyLookup (Key))
        Db->MdtSetUniqueKey(NewRecord, Key);
      else
        NewRecord->SetKey (Key);
    }
  else if (FieldName ^= LanguageField) 
    {
      // Only if valid do we over-ride
      SHORT code = Lang2Id ( Buffer );
      if (code != 0)
	NewRecord->SetLanguage (code);
    }
  else if (FieldName ^= CategoryField)
    {
      NewRecord->SetCategory ( ParseCategory(Buffer) );
    }
  else if (FieldName ^= PriorityField)
    {
      NewRecord->SetPriority( Buffer );
    }
  else if (FieldName ^= TTLField)
    { int ttl = atol( Buffer );
      if (ttl > 0)
      NewRecord->SetDateExpires (SRCH_DATE((const char *)NULL).PlusNminutes(ttl));
    }
  else message_log (LOG_DEBUG, "Unknown handler for special field '%s'", FieldName.c_str());
}


void DOCTYPE::Sort(RSET *Set)
{
#if 0
  const size_t Total = Set->GetTotalEntries();
#endif
}


bool DOCTYPE::_write_resource_path (const STRING& file,
        const RECORD& Filerecord, STRING *PathFilePath) const
{
  if (Db)
    {
      STRING mime;
      SourceMIMEContent(&mime);
      return Db->_write_resource_path (file, Filerecord, mime, PathFilePath);
    }
  return false;
}


bool DOCTYPE::PluginExists(const STRING& doctype)
{
  return Db ? Db->DoctypePluginExists(doctype) : false;
}

