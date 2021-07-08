/*@@@
File:		filter2.cxx
Version:	1.00
Description:	Class FILTER2
Author:		Edward Zimmermann
@@@*/

#include <ctype.h>
#include <sys/stat.h>
//#include <pwd.h>
//#include <grp.h>
#include <errno.h>

#include "doc_conf.hxx"
#include "filter2.hxx"
#include "process.hxx"

#if 0
class pathSTRUCTURE {
public:


private:
  STRING    uriPath;
  STRING    mime_type;
  UINT8     start;
  UINT8     end; 
};

#endif


static const char *mime_application_binary = "application/binary";
static const char *ini_content_type_tag    = "Content-type";

const char *FILTER2HTMLDOC::GetDefaultFilter() const { return ""; }
const char *FILTER2XMLDOC::GetDefaultFilter() const  { return ""; }
const char *FILTER2TEXTDOC::GetDefaultFilter() const { return ""; }
const char *FILTER2MEMODOC::GetDefaultFilter() const { return ""; }

static STRING Help(const STRING& What, const STRING& Doctype)
{
  STRING Help;
  STRING ini (Doctype), section(Doctype);
  ini.ToLower();
  section.ToUpper();

  Help << "Uses an external filter that converts the input file into " << What << " type files.\n\
Options:\n\
   Filter        Specifies the program to use.\n\
   Content-type  Specifies the MIME content-type of the original.\n\
These are defined in the \"" << ini << ".ini\" [General] or <Db>.ini [" << section << "] sections.\n\n\
Filters should take a single argument as the path to the (binary) input file and should write\n\
their output (" << What << ") to standard output (stdout)";
  return Help;
}


const char *FILTER2HTMLDOC::Description(PSTRLIST List) const
{
  static STRING Help;

  const STRING ThisDoctype("FILTER2HTML");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  HTMLHEAD::Description(List);

  if (Help.IsEmpty())
    Help = ::Help("HTML", Doctype);
  return Help;
}
const char *FILTER2XMLDOC::Description(PSTRLIST List) const
{
  static STRING Help;
   const STRING ThisDoctype("FILTER2XML");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  XML::Description(List);

  if (Help.IsEmpty())
    Help = ::Help("XML", Doctype);
  return Help;
}
const char *FILTER2TEXTDOC::Description(PSTRLIST List) const
{
  static STRING Help;

  const STRING ThisDoctype("FILTER2TEXT");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  if (Help.IsEmpty())
    Help = ::Help("TEXT (PTEXT)", Doctype);
  return Help;
}
const char *FILTER2MEMODOC::Description(PSTRLIST List) const
{
  static STRING Help;
  const STRING ThisDoctype("FILTER2MEMO");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  MEMODOC::Description(List);

  if (Help.IsEmpty())
    Help = ::Help("MEMO", Doctype);
  return Help;
}



void FILTER2HTMLDOC::SourceMIMEContent(STRING *stringPtr) const
{
  *stringPtr =  MIME_Type; 
}
void FILTER2XMLDOC::SourceMIMEContent(STRING *stringPtr) const
{
  *stringPtr =  MIME_Type;
}
void FILTER2TEXTDOC::SourceMIMEContent(STRING *stringPtr) const
{
  *stringPtr =  MIME_Type;
}
void FILTER2MEMODOC::SourceMIMEContent(STRING *stringPtr) const
{
  *stringPtr =  MIME_Type;
}


static const char *ini_filter_tag = "Filter";

static STRING SetFilter(const STRING& arg, const STRING& Doctype)
{
  if (arg.GetLength() && (arg != "NULL"))
    {
      STRING Filter (ResolveBinPath(arg));
      if (!IsAbsoluteFilePath(Filter))
        {
	  logf (LOG_WARN, "%s: %s '%s' must be in $PATH.", Doctype.c_str(),
		ini_filter_tag, Filter.c_str());
        }
      else if (!ExeExists(Filter))
        {
          logf (LOG_ERROR, "%s: %s '%s' %s!", Doctype.c_str(), ini_filter_tag, Filter.c_str(),
            Exists(Filter) ?  "is not executable" : "does not exist");
          Filter.Clear();
        }
      else
        logf (LOG_DEBUG, "%s: External filter set to '%s'", Doctype.c_str(), Filter.c_str());
      return Filter;
    }
  return NulString;
}

GDT_BOOLEAN FILTER2HTMLDOC::SetFilter(const STRING& filter)
{
  STRING s (::SetFilter(filter, Doctype) );
  if (s.GetLength()) { Filter = s; return GDT_TRUE; }
  return GDT_FALSE; 
}
GDT_BOOLEAN FILTER2XMLDOC::SetFilter(const STRING& filter)
{
  STRING s (::SetFilter(filter, Doctype) );
  if (s.GetLength()) { Filter = s; return GDT_TRUE; }
  return GDT_FALSE;
}
GDT_BOOLEAN FILTER2TEXTDOC::SetFilter(const STRING& filter)
{
  STRING s (::SetFilter(filter, Doctype) );
  if (s.GetLength()) { Filter = s; return GDT_TRUE; }
  return GDT_FALSE;
}
GDT_BOOLEAN FILTER2MEMODOC::SetFilter(const STRING& filter)
{
  STRING s (::SetFilter(filter, Doctype) );
  if (s.GetLength()) { Filter = s; return GDT_TRUE; }
  return GDT_FALSE;
}

static const STRING NulFilter ("NULL");

void FILTER2HTMLDOC::BeforeIndexing()
{
  DOCTYPE::BeforeIndexing();

  MIME_Type = Getoption(ini_content_type_tag, mime_application_binary);
  Filter = ::SetFilter( Getoption(ini_filter_tag, GetDefaultFilter()) , Doctype);
  if (Filter.IsEmpty()) logf (LOG_ERROR, "%s: No %s set. Nothing to do?", Doctype.c_str(), ini_filter_tag);
  else if (Filter.Equals(  NulFilter ))
    Filter.Clear();
  else
    logf (LOG_INFO, "%s: Using '%s' to filter to HTML.", Doctype.c_str(), Filter.c_str());
}
void FILTER2XMLDOC::BeforeIndexing()
{
  DOCTYPE::BeforeIndexing();
  MIME_Type = Getoption(ini_content_type_tag, mime_application_binary);
  Filter = ::SetFilter( Getoption(ini_filter_tag, GetDefaultFilter()) , Doctype);
  if (Filter.IsEmpty()) logf (LOG_ERROR, "%s: No %s set. Nothing to do?", Doctype.c_str(), ini_filter_tag);
  else if (Filter.Equals (  NulFilter ))
    Filter.Clear();
  else
    logf (LOG_INFO, "%s: Using '%s' to filter to XML.", Doctype.c_str(), Filter.c_str());
}
void FILTER2TEXTDOC::BeforeIndexing()
{
  DOCTYPE::BeforeIndexing();
  MIME_Type = Getoption(ini_content_type_tag, mime_application_binary);
  Filter = ::SetFilter( Getoption(ini_filter_tag, GetDefaultFilter()) , Doctype);
  if (Filter.IsEmpty())
    logf (LOG_ERROR, "%s: No %s set. Nothing to do?", Doctype.c_str(), ini_filter_tag);
  else if (Filter.Equals(  NulFilter ))
    Filter.Clear();
  else
    logf (LOG_INFO, "%s: Using '%s' to filter to Ascii text (PTEXT).", Doctype.c_str(), Filter.c_str());
}
void FILTER2MEMODOC::BeforeIndexing()
{
  DOCTYPE::BeforeIndexing();
  MIME_Type = Getoption(ini_content_type_tag, mime_application_binary);
  Filter = ::SetFilter( Getoption(ini_filter_tag, GetDefaultFilter()) , Doctype);
  if (Filter.IsEmpty())
    logf (LOG_ERROR, "%s: No %s set. Nothing to do?", Doctype.c_str(), ini_filter_tag);
  else if (Filter.Equals(  NulFilter ))
    Filter.Clear();
  else
    logf (LOG_INFO, "%s: Using '%s' to filter to MEMO.", Doctype.c_str(), Filter.c_str());
}


//
// Since we pass control over during indexing to the parent document type and it gets
// registered as the true document type we should NEVER end up during a present calling
// these doctypes.
//

//void FILTER2HTMLDOC::BeforeRset(const STRING& RecordSyntax)
//{
//  MIME_Type = Getoption(ini_content_type_tag, mime_application_binary);
//}
//void FILTER2XMLDOC::BeforeRset(const STRING& RecordSyntax)
//{
//  MIME_Type = Getoption(ini_content_type_tag, mime_application_binary);
//}
//void FILTER2TEXTDOC::BeforeRset(const STRING& RecordSyntax)
//{
//  MIME_Type = Getoption(ini_content_type_tag, mime_application_binary);
//}
//void FILTER2MEMODOC::BeforeRset(const STRING& RecordSyntax)
//{
//  MIME_Type = Getoption(ini_content_type_tag, mime_application_binary);
//}


FILTER2HTMLDOC::FILTER2HTMLDOC(PIDBOBJ DbParent, const STRING& Name) : HTMLHEAD(DbParent, Name)
{
  Filter = NulFilter;
}
FILTER2XMLDOC::FILTER2XMLDOC(PIDBOBJ DbParent, const STRING& Name) : XML(DbParent, Name)
{
  Filter = NulFilter;
}
FILTER2TEXTDOC::FILTER2TEXTDOC(PIDBOBJ DbParent, const STRING& Name) : PTEXT(DbParent, Name)
{
  Filter = NulFilter;
}
FILTER2MEMODOC::FILTER2MEMODOC(PIDBOBJ DbParent, const STRING& Name) : MEMODOC(DbParent, Name)
{
  Filter = NulFilter;
}

//
// This is the common routine that does some of the significant work
//
static GDT_BOOLEAN GenRecord(IDBOBJ *Db, const RECORD& FileRecord,
  STRING *Filter, DOCTYPE* Doctype)
{
  const char *doctype = Doctype ? Doctype->c_str() : "Filter2";
  if (Filter == NULL || Filter->IsEmpty())
    {
      logf (LOG_WARN, "%s: Filter is NIL", doctype);
      return GDT_TRUE; // Do nothing
    }

  STRING key, s, Fn, outfile;
  struct stat stbuf;

  Fn = FileRecord.GetFullFileName ();

  logf (LOG_DEBUG, "%s: Input = '%s'", doctype, Fn.c_str());

  INODE Inode (Fn);

  if (!Inode.Ok())
    {
      if (Inode.isDangling())
	logf(LOG_ERROR, "%s: '%s' is a dangling symbollic link", doctype, Fn.c_str());
      else
	logf(LOG_ERRNO, "%s: Can't stat '%s'.", doctype , Fn.c_str());
      return GDT_FALSE;
    }
  if (Inode.st_size == 0)
    {
      logf(LOG_ERROR, "'%s' has ZERO (0) length? Skipping.", Fn.c_str());
      return GDT_FALSE;
    }

  // Need to have not just the inode but also the start and end since
  // the bits to pass to the filter may be subsets of the file.. 
  const long inode = stbuf.st_ino;
  off_t start = FileRecord.GetRecordStart();
  off_t end   = FileRecord.GetRecordEnd();

  if ((end == 0) || (end > stbuf.st_size) ) end = stbuf.st_size;

  key = Inode.Key(start, end);
  for (unsigned long version = 0; Db->KeyLookup (key); version++)
    key.form("%s.%ld", s.c_str(), version); 
  // Now we have a good key

  logf (LOG_DEBUG, "%s: Key set to '%s'", doctype, key.c_str());

  Db->ComposeDbFn (&s, DbExtCat);
  if (MkDir(s, 0, GDT_TRUE) == -1) // Force creation
    {
      logf (LOG_ERRNO, "Can't create filter directory '%s'", s.c_str() );
      return GDT_FALSE;
    }
//  outfile.form ("%s/%s", s.c_str(), key.c_str());

  // We want to not have single directories get too filled with files since
  // this can slow down things under some Unix file systems
  //
  // <db_ext>.cat/<Hash>/<Key>
  outfile =  AddTrailingSlash(s);
  outfile.Cat (((long)key.CRC16()) % 1000);
  if (MkDir(outfile, 0, GDT_TRUE) == -1)
    outfile = s; // Can't make it
  AddTrailingSlash(&outfile);
  outfile.Cat (key);

  // So we pipe Fn bytes Start to End into outfile
  logf (LOG_DEBUG, "Output to '%s'", outfile.c_str());

  FILE *fp;
  if ((fp = fopen(outfile, "w")) == NULL)
   {
     logf (LOG_ERRNO, "%s: Could not create '%s'",  doctype, outfile.c_str());
     return GDT_FALSE;
   }
  off_t len = 0;

  // We write full file path as ordinary string
  start = Fn.WriteFile(fp);
  start += fwrite("\0\0", 1, 2, fp) - 1;

#if 0
  // Here we want to write some metadata into the .cat file
  len += Doctype->CatMetaInfoIntoFile(fp, Fn);
#endif

  STRING pipe = *Filter + " " +  Fn.Escape();

  FILE *pp = _IB_popen(pipe, "r");
  if (pp == NULL)
    {
      logf (LOG_ERRNO, "%s: Could not open pipe '%s'", doctype, pipe.c_str());
      fclose(fp);
      UnlinkFile(outfile);

      if (!IsAbsoluteFilePath (*Filter))
	{
	  logf (LOG_ERROR, "%s: Check configuration for filter '%s'. Skipping rest.", doctype, Filter->c_str());
	  Filter->Clear();
	}
      return GDT_FALSE;
    }

  int ch;
  while ((ch = fgetc(pp)) != EOF)
    {
      fputc(ch, fp);
      len++;
    }
  _IB_pclose(pp);
  fclose(fp);

  if (len > 5)
    {

      RECORD NewRecord(FileRecord);
      // We now have a record in outfile from start (just after the path) to len
      NewRecord.SetRecordStart (start);
      NewRecord.SetRecordEnd ( start + len);
      NewRecord.SetFullFileName ( outfile );
      NewRecord.SetKey( key ); // Set the key since we did the effort

      // Set some default dates
      SRCH_DATE mod_filter, mod_input;
      if (mod_filter.SetTimeOfFile(*Filter) && mod_input.SetTimeOfFile(Fn) && mod_filter > mod_input)
        NewRecord.SetDateModified ( mod_filter );
      NewRecord.SetDate ( mod_input ); 

      STRING urifile;
      if (Db->_write_resource_path(outfile, FileRecord, &urifile) == GDT_FALSE)
	logf (LOG_ERRNO, "%s: Could not create '%s'", doctype, urifile.c_str());

      Db->DocTypeAddRecord(NewRecord);
      return GDT_TRUE;
    }
  logf (LOG_ERROR, "%s: pipe '%s' returned ONLY %d bytes!", doctype, pipe.c_str(), len);
  // Remove junk
  UnlinkFile(outfile);
  return GDT_FALSE;
}


void FILTER2XMLDOC::ParseRecords(const RECORD& FileRecord)
{
  if (Filter.Equals( NulFilter ))
    {
      logf (LOG_WARN, "%s:BeforeIndexing() not called", Doctype.c_str());
      BeforeIndexing();
    }
  GenRecord(Db, FileRecord, &Filter, this);
}
void FILTER2TEXTDOC::ParseRecords(const RECORD& FileRecord)
{
  if (Filter.Equals( NulFilter ))
    {
      logf (LOG_WARN, "%s:BeforeIndexing() not called", Doctype.c_str());
      BeforeIndexing();
    }
  GenRecord(Db, FileRecord, &Filter, this);
}
void FILTER2HTMLDOC::ParseRecords(const RECORD& FileRecord)
{
  if (Filter.Equals( NulFilter ))
    {
      logf (LOG_WARN, "%s:BeforeIndexing() not called", Doctype.c_str());
      BeforeIndexing();
    }
  GenRecord(Db, FileRecord, &Filter, this);
}
void FILTER2MEMODOC::ParseRecords(const RECORD& FileRecord)
{
  if (Filter.Equals( NulFilter ))
    {
      logf (LOG_WARN, "%s:BeforeIndexing() not called", Doctype.c_str());
      BeforeIndexing();
    }
  GenRecord(Db, FileRecord, &Filter, this);
}


void FILTER2HTMLDOC::ParseFields(RECORD *RecordPtr)
{
  HTMLHEAD::ParseFields(RecordPtr);
}
void FILTER2XMLDOC::ParseFields(RECORD *RecordPtr)
{
  XML::ParseFields(RecordPtr);
}
void FILTER2TEXTDOC::ParseFields(RECORD *RecordPtr)
{
  PTEXT::ParseFields(RecordPtr);
}
void FILTER2MEMODOC::ParseFields(RECORD *RecordPtr)
{
  MEMODOC::ParseFields(RecordPtr);
}



void FILTER2HTMLDOC:: DocPresent (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const
{
  StringBuffer->Clear();

  if ( (RecordSyntax == SutrsRecordSyntax) ?
	ElementSet.Equals(SOURCE_MAGIC) : ElementSet.Equals(FULLTEXT_MAGIC) )
    {
      // Read here the .doc file
      if (RecordSyntax == HtmlRecordSyntax)
	 *StringBuffer  = DOCTYPE::Httpd_Content_type (ResultRecord);
      StringBuffer->CatFile(  Db->_get_resource_path(ResultRecord.GetFullFileName())  );
    }
  else if ( (RecordSyntax == SutrsRecordSyntax) ?
        ElementSet.Equals(FULLTEXT_MAGIC) : ElementSet.Equals(SOURCE_MAGIC) )
    {
      // Read here the .doc file
      if (RecordSyntax == HtmlRecordSyntax)
         *StringBuffer  = DOCTYPE::Httpd_Content_type (ResultRecord, "plain/text");
      StringBuffer->CatFile( ResultRecord.GetFullFileName() );
    }
  else
   HTMLHEAD::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}
void FILTER2XMLDOC:: DocPresent (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const
{
  StringBuffer->Clear();

  if ( (RecordSyntax == SutrsRecordSyntax) ?
        ElementSet.Equals(SOURCE_MAGIC) : ElementSet.Equals(FULLTEXT_MAGIC) )
    {
      // Read here the .doc file
      if (RecordSyntax == HtmlRecordSyntax)
         *StringBuffer  = DOCTYPE::Httpd_Content_type (ResultRecord);
      StringBuffer->CatFile(  Db->_get_resource_path(ResultRecord.GetFullFileName())  );
    }
  else if ( (RecordSyntax == SutrsRecordSyntax) ?
        ElementSet.Equals(FULLTEXT_MAGIC) : ElementSet.Equals(SOURCE_MAGIC) )
    {
      // Read here the .doc file
      if (RecordSyntax == HtmlRecordSyntax)
         *StringBuffer  = DOCTYPE::Httpd_Content_type (ResultRecord, "plain/text");
      StringBuffer->CatFile( ResultRecord.GetFullFileName() );
    }
  else
   XML::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}
void FILTER2TEXTDOC:: DocPresent (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const
{
  StringBuffer->Clear();

  if ( (RecordSyntax == SutrsRecordSyntax) ?
        ElementSet.Equals(SOURCE_MAGIC) : ElementSet.Equals(FULLTEXT_MAGIC) )
    {
      // Read here the .doc file
      if (RecordSyntax == HtmlRecordSyntax)
         *StringBuffer  = DOCTYPE::Httpd_Content_type (ResultRecord);
      StringBuffer->CatFile(  Db->_get_resource_path(ResultRecord.GetFullFileName())  );
    }
  else if ( (RecordSyntax == SutrsRecordSyntax) ?
        ElementSet.Equals(FULLTEXT_MAGIC) : ElementSet.Equals(SOURCE_MAGIC) )
    {
      // Read here the .doc file
      if (RecordSyntax == HtmlRecordSyntax)
         *StringBuffer  = DOCTYPE::Httpd_Content_type (ResultRecord, "plain/text");
      StringBuffer->CatFile( ResultRecord.GetFullFileName() );
    }
  else
   PTEXT::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}
void FILTER2MEMODOC::DocPresent (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const
{
  StringBuffer->Clear();

  if ( (RecordSyntax == SutrsRecordSyntax) ?
        ElementSet.Equals(SOURCE_MAGIC) : ElementSet.Equals(FULLTEXT_MAGIC) )
    {
      // Read here the .doc file
      if (RecordSyntax == HtmlRecordSyntax)
         *StringBuffer  = DOCTYPE::Httpd_Content_type (ResultRecord);
      StringBuffer->CatFile(  Db->_get_resource_path(ResultRecord.GetFullFileName()) );
    }
  else if ( (RecordSyntax == SutrsRecordSyntax) ?
        ElementSet.Equals(FULLTEXT_MAGIC) : ElementSet.Equals(SOURCE_MAGIC) )
    {
      // Read here the .doc file
      if (RecordSyntax == HtmlRecordSyntax)
         *StringBuffer  = DOCTYPE::Httpd_Content_type (ResultRecord, "plain/text");
      StringBuffer->CatFile( ResultRecord.GetFullFileName() );
    }
  else
   MEMODOC::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}



void FILTER2HTMLDOC::Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const
{
  StringBuffer->Clear();
  if (ElementSet.Equals(FULLTEXT_MAGIC) || ElementSet.Equals(SOURCE_MAGIC))
    DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
  else
   HTMLHEAD::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}
void FILTER2XMLDOC::Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const
{
  StringBuffer->Clear();
  if (ElementSet.Equals(FULLTEXT_MAGIC) || ElementSet.Equals(SOURCE_MAGIC))
    DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
  else
   XML::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}
void FILTER2TEXTDOC::Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const
{
  StringBuffer->Clear();
  if (ElementSet.Equals(FULLTEXT_MAGIC) || ElementSet.Equals(SOURCE_MAGIC))
    DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
  else
   PTEXT::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}
void FILTER2MEMODOC::Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const
{
  StringBuffer->Clear();
  if (ElementSet.Equals(FULLTEXT_MAGIC) || ElementSet.Equals(SOURCE_MAGIC))
    DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
  else
   MEMODOC::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}



GDT_BOOLEAN FILTER2HTMLDOC::GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const
{
  STRING fullpath (  Db->_get_resource_path(ResultRecord.GetFullFileName()) );
  if (StringBuffer)
    *StringBuffer = fullpath;
  return Exists (fullpath);
}
GDT_BOOLEAN FILTER2XMLDOC::GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const
{
  STRING fullpath (  Db->_get_resource_path(ResultRecord.GetFullFileName()) );
  if (StringBuffer)
    *StringBuffer = fullpath;
  return Exists (fullpath);
}
GDT_BOOLEAN FILTER2TEXTDOC::GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const
{
  STRING fullpath (  Db->_get_resource_path(ResultRecord.GetFullFileName()) );
  if (StringBuffer)
    *StringBuffer = fullpath;
  return Exists (fullpath);
}
GDT_BOOLEAN FILTER2MEMODOC::GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const
{
  STRING fullpath (  Db->_get_resource_path(ResultRecord.GetFullFileName()) );
  if (StringBuffer)
    *StringBuffer = fullpath;
  return Exists (fullpath);
}

