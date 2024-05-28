/************************************************************************
************************************************************************/

/*-@@@
File:		csvdoc.cxx
Version:	$Revision: 2.1 $
Description:	Class CSVDOC - CSV Tab Sep. Values Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@-*/

/*
Based around RFC 4180: Common Format and MIME Type for Comma-Separated Values (CSV) Files (2005)

With the following differences:
- We support either ' or " for quotation (but not mixed both) while the RFC specifies solely
  double quotes as an option.
- We allow double double quotes in fields, even fields that have not been encloded in quotes.
- We support different delimiter characters and not just ',' (comma)
- The delimiter character may be freely defined but when processing the first line in a file
  to derive its fields, it tries to detect the character for among: ',', ';', '\\t' (tab),
  '|' (pipe) or 0x1F (US).
- Column names (fields) are restricted. They may not contain /, *, ?, { or [ as they have
  special functions within the engine.
- Columns whose names contain special restricted characters get mapped to "" (ignored name).
- Each line should (not must) contain the same number of fields. Lines that contain more columns
  that specified by the column names get additional column names named FIELDnn where nn is the
  column number..
- New line characters may be within quoted columns.
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "csvdoc.hxx"
#include "common.hxx"
#include "doc_conf.hxx"

CSVDOC::CSVDOC (PIDBOBJ DbParent, const STRING& Name):
	COLONDOC (DbParent, Name)
{
  STRING Value =  Getoption("UseFirstRecord", "Auto"); 
  if (Value == "Auto")
    UseFirstRecord = -1;
  else
    UseFirstRecord = Value.GetBool() ? 1 : 0;
  // Quote Character can only be " or '
  Value = Getoption ("QuoteChar", "\"");
  if (Value == "'")
    quoteChar = '\'';
  else
    quoteChar = '"'; 
  Lineno = 0;
  Columns = 0;
  quoteChar = '"';
  SetSepChar(',');
}

const char *CSVDOC::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("CSV");
  if (List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);
  COLONDOC::Description(List);
  return "This is a base class for delimeter seperated values files.\n\
  Each line is a record and each column (field) in the record is delimited by a seperator character.\n\
  This is either specified (option SepChar) or when useFirstRecord is true, it tries to guess\n\
  between ',' or ';' or '\\t' (tab) or '|' (pipe) or 0x1F (US). Default is: , (comma).\n\
  Each line should (not must) contain the same number of columns. Lines that contain more columns than\n\
  specified by the defined names get additional column names FIELDnn where nn is the column number.\n\
  Newline characters (CR, LF) may occur within quoted columns.\n\
 [General]\n\
 QuoteChar=<chr> // either ' or \", default is \" or for UseFirstRecord ' if first character  is '.\n\
 UseFirstRecord=<bool> // Use first record for column names (field names)\n\
 TabFields=Comma separated list of field names (e.g. field1,field2,field3) alternatively in the\n\
   doctype.ini to define the \"column\" field names.\n\
   If the option \"UseFirstRecord\" is specified (True) the first record is parsed for the names.\n\
   NOTE: If no TabFields are specified and UseFirstRecord was not specified True of False then\n\
   UseFirstRecord will be assumed as True.\n\
 CategoryField=<Field contains an integer for record category>\n\
 PriorityField=<Field contains an integer to skew scores for record>\n\
   For priority to have an effect one must define in \"Db.ini\"'s [DbInfo]\n\
   PriorityFactor=NN.NN (a floating point number)\n\
   Alternatively in the \"Db.ini\" file [Doctype] section";
}

const char *TSVDOC::Description(PSTRLIST List) const
{ 
  const STRING ThisDoctype("TSV");
  if (List->IsEmpty() && Doctype != ThisDoctype) 
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);
  CSVDOC::Description(List);
  return "This is a base class for tab delimited value files.\n\
 Each line is a record and each column (field) in the record is delimited by a seperator character.\n\
  This is either specified (option SepChar) or when useFirstRecord is true, it tries to guess\n\
  between ',' or ';' or '\\t' (tab) or '|' (pipe) or 0x1F (US). Default is: \\t (tab).\n";
}


void CSVDOC::SourceMIMEContent(const RESULT&, PSTRING StringPtr) const
{
  SourceMIMEContent(StringPtr);
}

void CSVDOC::SourceMIMEContent(PSTRING StringBufferPtr) const
{
  if (UseFirstRecord)
    {
      *StringBufferPtr = "text/csv";
    }
  else
    {
      *StringBufferPtr = "Application/X-CSV-";
      StringBufferPtr->Cat(Doctype);
    }
}

void CSVDOC::AddFieldDefs ()
{
  static const STRING Entry ("TabFields");
  DOCTYPE::AddFieldDefs ();
  STRING string;
  if (Db)
    Db->ProfileGetString(Doctype, Entry, NulString, &string);
  if (string.IsEmpty())
    {
      if (tagRegistry)
	tagRegistry->ProfileGetString("General", Entry, NulString, &string);
    }
  if (!string.IsEmpty())
    {
      STRLIST List;
      List.Split(',', string);
      DFD dfd;
      Columns = 0;
      for (const STRLIST *p = List.Next(); p != &List; p = p->Next())
	{
	  Columns++;
	  message_log (LOG_DEBUG, "%s Column #%d is \"%s\"", Doctype.c_str(), Columns, p->Value().c_str());
	  FieldNames.Add(p->Value());

	  dfd.SetFieldType( Db->GetFieldType(p->Value()) ); // Get the type added 30 Sep 2003
	  dfd.SetFieldName (p->Value());
	  Db->DfdtAddEntry (dfd);
	}
      if (UseFirstRecord == -1)
	UseFirstRecord = 0;
    }
  else if (UseFirstRecord == -1)
    UseFirstRecord = 1;
}

// We accept newlines in fields if within quotes!!!!
void CSVDOC::ParseRecords(const RECORD& FileRecord)
{
  FileName = FileRecord.GetFullFileName();
  FileMap.CreateMap(FileName, MapSequential); // We are sequentially reading
  if (!FileMap.Ok())
    {
      message_log (LOG_ERRNO, "%s could not access '%s'", Doctype.c_str(), FileName.c_str());
      return;			// File not accessed
    }
  off_t RecStart = FileRecord.GetRecordStart();
  off_t RecEnd = FileRecord.GetRecordEnd();
  const off_t FileEnd = FileMap.Size();

  if (RecEnd == 0) RecEnd = FileEnd;

  if (RecEnd - RecStart <= 0)
    {
      message_log (LOG_WARN, "zero-length record '%s'[%ld-%ld] -- skipping",
	FileName.c_str(), (long)RecStart, (long)RecEnd);
      return;
    }
  else if (RecStart > FileEnd)
    {
      message_log (LOG_ERROR, "%s::ParseRecords(): Seek '%s' to %ld failed",
	Doctype.c_str(), FileName.c_str(), RecStart);
      return;
    }
  else if (RecEnd > FileEnd)
    {
      message_log (LOG_WARN, "%s::ParseRecord(): End after EOF (%d>%d) in '%s'?",
	Doctype.c_str(), RecEnd, FileEnd, FileName.c_str());
      RecEnd = FileEnd;
    }
  RECORD Record (FileRecord);
  off_t Position = RecStart;
  
  unsigned char  ci = 0;
  const unsigned char *ptr = FileMap.Ptr(); // NOTE: This is a memory mapped file!

  // Is first character a ' ?
  if (*ptr == '\'') quoteChar = '\''; // if starts with 'xxx then use '

  bool inQuote = false;
  while (Position < RecEnd)
    {
      // Need to handle newlines in quoted columns
      for (; !(!inQuote && (ci == '\n' || ci =='\r')) && Position <= RecEnd; ci=*ptr++, Position++) {
	if (ci == quoteChar) {
	  if (*ptr == quoteChar) ptr++, Position++; // skip
	  else inQuote = !inQuote;
	}
      }
      // I may be in a quote but that's an error so I'm in trouble anyway..
      // as I'll keep going until end of file... ignoring the CR and LF.

      if (*ptr == '\r' || *ptr == '\n') {
        ptr++; Position++; // Skip second newline (such as in MSDOS)
      }

      if (RecStart != Position)
	{
	  Record.SetRecordStart(RecStart);
#if 1
	  Record.SetRecordEnd(Position-1);
#else 
	  off_t Pos;
	  if (Position != FileEnd) 
	    Pos = Position-1; 
	  else
	    Pos = Position-2; 
	  Record.SetRecordEnd(Pos);
#endif
//cerr << "Record is: " << RecStart << "-" << Position-1 << endl;
	  Db->DocTypeAddRecord(Record);
	}
      if (ci=='\n' || ci == '\r')
	ci=0;// save an EOF, but hide a newline so it will loop again
      RecStart = Position;
    }
  if (inQuote) message_log (LOG_ERROR, "Runaway record. Missing end-quotes (%c). Check format.", quoteChar);

}

// We pass a storage for the last value to be re-entrant rather
// than like strtrok which use a static storage
char * CSVDOC::c_tokenize(char *s, char **last)
{
	char *tok;

	if (s == NULL && (last == NULL | (s = *last) == NULL)) return (NULL);
        tok = s;

	bool inQuote = false;
	for (;;) {
		int c = *s++;
		if (c == quoteChar) {
/*
RFC 1480: Common Format and MIME Type for Comma-Separated Values (CSV) Files
5.  Each field may or may not be enclosed in double quotes (however
    some programs, such as Microsoft Excel, do not use double quotes
    at all).  If fields are not enclosed with double quotes, then
    double quotes may not appear inside the fields.  For example:

    "aaa","bbb","ccc" CRLF
    zzz,yyy,xxx

7.  If double-quotes are used to enclose fields, then a double-quote
    appearing inside a field must be escaped by preceding it with
    another double quote.  For example:

    "aaa","b""bb","ccc"
*/
			if (*s == quoteChar) { // double quotes "" 
			   s++;
			   continue; // swallow the double """"
			} else
			   inQuote = !inQuote;
		}
		if (!inQuote && (c == sepChar || c == '\r' || c == '\n')) {
			s[-1] = 0;
			if (*s == '\r' || *s == '\n') *s++ = '\0'; // Zap \r\n
			*last = s;
			return tok;
		} else if (c == 0) {
			if (inQuote) message_log(LOG_ERROR, "Unterminated %c (quote): %s", quoteChar, tok);
			s = NULL;
			*last = s;
			return (tok);
		}
	}
	/* NOTREACHED */
}


void CSVDOC::ParseFields (PRECORD NewRecord)
{
  const STRING fn ( NewRecord->GetFullFileName ());
  if (fn != FileName)
    {
      FileMap.CreateMap(fn, MapSequential);
      if (!FileMap.Ok())
	{
	  message_log (LOG_ERROR, "%s:ParseFields Could not address file '%s'", Doctype.c_str(), fn.c_str());
	  return;
	}
      Lineno = 0;
    }
  off_t RecStart = NewRecord->GetRecordStart ();
  off_t RecEnd = NewRecord->GetRecordEnd ();
  const off_t FileEnd = FileMap.Size();
  if (RecEnd == 0)
    RecEnd = FileEnd; 
  if (RecEnd - RecStart <= 0)
    return; // ERROR
  else if (RecStart > FileEnd)
    return; // ERROR
  else if (RecEnd > FileEnd)
    RecEnd = FileEnd;

  off_t RecLength = RecEnd - RecStart + 1; 
  // We need to make a buffer since we're working destructively.
  // Since these are records its pretty small. Notice we used the
  // cached Buffer and enlarge it as needed.
  PCHR RecBuffer = (PCHR)Buffer.Want(RecLength + 1);
  memcpy(RecBuffer, FileMap.Ptr()+RecStart, RecLength);
  RecBuffer[RecLength] = '\0';

  if (RecStart == 0)
    {
      Lineno = 1;
      if (UseFirstRecord == 1)
	{
	  bool inQuote = false;
	  FieldNames.Empty();
	  DFD dfd;
	  Columns = 0;
	  PCHR tcp = RecBuffer;
	  char *fieldname = RecBuffer;
	  // ; or . or \t ??
	  if (*tcp == '\'') quoteChar = '\''; // if starts with 'xxx then use '
	  while (*tcp && *tcp != '\r' && *tcp != '\n') {
	    if (*tcp == quoteChar) {
	      if (*(tcp + 1) == quoteChar) // "" is to include " inside "
		tcp++; // Need to skip it
	      else 
	        inQuote = !inQuote; // Need to ignore , or ; in quotes
	    } else if (!inQuote &&
		( *tcp == ',' || *tcp == ';' || *tcp == '\t' || *tcp == '|' || *tcp == 0x1F)) {
	      // Seperator Character can be: comman, semicolon, tab,..
	      if (tcp[-1] == quoteChar) tcp[-1] = '\0'; // Zap the quote
	      sepChar = *tcp; 
	      *tcp++ = '\0'; // Next character
	      break; // We got the first ...
	    }
	    tcp++;
           }
	   /* loop */;

	  // We now know the sepChar..
	  if (inQuote) {
	    message_log (LOG_WARN, "%s: Runawy quote in first element of first record of '%s'!",
		Doctype.c_str(), fn.c_str());
	    inQuote = false;
	  }
	  if (*tcp && *tcp != '\n' && *tcp != '\r') for (;;) {
	   if (!inQuote) {
	    Columns++;
	    if (*fieldname == quoteChar) fieldname++; // skip the quote in the name
	    if (*tcp == '\n' || *tcp == '\r') {
	      if (*(tcp-1) == quoteChar) *(tcp-1) = '\0';
	      *tcp = '\0';
	    }
	    // Use fieldname that was specified (mapped)
	    STRING name = UnifiedName(fieldname);
	    if (name.IsEmpty() || dfd.checkFieldName(name) == false) {
	      name = NulString;
	      message_log(LOG_WARN, "%s Column #%d (\"%s\") shall be ignored (but not its content)",
		Doctype.c_str(), Columns, fieldname);
	    } else {
              // NOTE: Since "" is used for " a string may contain ""
	      name.Replace("\"\"", "\"");

	      message_log (LOG_DEBUG, "%s Column #%d is \"%s\" (%s)", Doctype.c_str(),
		Columns, name.c_str(), fieldname);
	      dfd.SetFieldType( Db->GetFieldType(name) ); // Get the type added 30 Sep 2003
	      dfd.SetFieldName (name);
	      Db->DfdtAddEntry (dfd); // Push into DFDT
	    }
	    FieldNames.Add(name);

	    if (*tcp == '\0' || *tcp == '\n' || *tcp == '\r')
	      break;
	    fieldname = tcp; // Looking at the next fieldname
	   }
	   // Scan for next..
	   while (*tcp && *tcp != '\n' && *tcp != '\r') {
	     if (*tcp == quoteChar) {
              if (*(tcp + 1) == quoteChar) // "" is to include " inside "
                tcp++; // Need to skip it
               else 
                inQuote = !inQuote; // Need to ignore , or ; in quotes
	      } else if (!inQuote && *tcp == sepChar) {
		 if (tcp[-1] == quoteChar) tcp[-1] = '\0'; // Zap the quote
		 *tcp++ = '\0';
		 break; // Got it..
	      }
	    tcp++;
	  } // while
	 } // for
	  return; // Done this
	} // end First Record
    }
  else if (Lineno > 0)
    Lineno++;

  char  *last = NULL;
  const char *tcp = c_tokenize(RecBuffer, &last);
  DF df;
  PDFT pdft;
  pdft = new DFT ();
  DFD dfd;
  int i = 0;
  size_t val_start, val_end;
  STRING fieldName;
  if (tcp) do {
    if (*tcp == '\0' || *tcp == sepChar)
      {
	i++;
	continue;
      }
    if (i >= FieldNames.Count())
      {
	if (RecStart != 0)
	  {
	    if (Lineno > 0)
	      message_log (LOG_ERROR, "File '%s', line %ld (%ld-%ld), has irregular number of columns (%d), adjusting.",
		fn.c_str(), Lineno, RecStart, RecEnd, i+1);
	    else
	      message_log (LOG_WARN, "File '%s' (%ld-%ld) has irregular number of columns (%d), adjusting.",
                fn.c_str(), RecStart, RecEnd, i+1);
	  }
	else
	  Columns++;
	fieldName.form("Field%d", ++i);
	FieldNames.Add(fieldName);

	dfd.SetFieldType( Db->GetFieldType(fieldName) ); // Get the type added 30 Sep 2003
	dfd.SetFieldName (fieldName);
	Db->DfdtAddEntry (dfd);
      }
    else
      {
	if (i > Columns && RecStart != 0)
	  {
	    if (Lineno > 0)
	     message_log (LOG_WARN, "File '%s', line %ld (%ld-%ld), has more than %ld columns (%d).",
		fn.c_str(), Lineno, RecStart, RecEnd, Columns, i);
	    else
	      message_log (LOG_WARN, "File '%s' (%ld-%ld) has more than %d columns (%d).",
		fn.c_str(), RecStart, RecEnd, Columns, i);
	  }
        fieldName = FieldNames[i++];
      }
    // Sanity for empty field
    if (*tcp == '\0' || (*tcp == quoteChar && *(tcp+1) == quoteChar) ||
	(*(tcp+1) == '\0' && _ib_iswhite(*tcp)))
      continue; // Keep going

    val_start = tcp - RecBuffer;
    val_end   = val_start + strlen(tcp) -1; // 2024 FIX (added -1 to not include , or \n)
    df.SetFct ( FC(val_start, val_end) );

//cerr << "FIELDNAME " << fieldName << "  add: " << tcp << "  ->" <<  val_start << "-" << val_end << endl;
    df.SetFieldName (fieldName);
    pdft->AddEntry (df);

    // Handle the special fields (Key etc.)
    HandleSpecialFields(NewRecord, fieldName, tcp);
  } while ((tcp = c_tokenize(NULL, &last)) != NULL); 
  NewRecord->SetDft (*pdft);

  delete pdft;
}

void CSVDOC::AfterIndexing()
{
  FileMap.Unmap();
}

CSVDOC::~CSVDOC ()
{
}

