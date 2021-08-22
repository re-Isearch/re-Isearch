/*@@@
File:		xfilter.cxx
Version:	1.00
Description:	Class XFilter // was XPandoc
Author:		Edward Zimmermann
@@@*/

#include <ctype.h>
#include <sys/stat.h>
//#include <pwd.h>
//#include <grp.h>
#include <errno.h>

#if USE_LIBMAGIC
#endif

#include "doc_conf.hxx"
#include "xfilter.hxx"
#include "process.hxx"

static const char *mime_application_binary = "application/binary";
static const char *ini_content_type_tag    = "Content-type";

static const char *ini_filter_tag = "Filter";
static const char *ini_options_tag = "Options";

const char *XFILTER::GetDefaultFilter() const  { return "pandoc"; }
const char *XFILTER::GetDefaultOptions() const { return "-s -t docbook5";}

static STRING genHelp(const char *What, const char *Filter, const char *Options, const char *Doctype)
{
  STRING help;
  STRING ini (Doctype), section(Doctype);
  ini.ToLower();
  section.ToUpper();

  help << "Uses an external filter that converts input file into " << What << " type files.\n\
Options:\n\
   " << ini_filter_tag << "\tSpecifies the program to use (default \"" << Filter << "\").\n\
   " << ini_options_tag << "\tSpecifies the options for Filter (default \"" << Options << "\").\n\
   " << ini_content_type_tag <<"\tSpecifies the MIME content-type of the original.\n\
These are defined in the \"" << ini << ".ini\" [General] or <Db>.ini [" << section << "] sections.\n\n\
Suitable filters must write their output (" << What << ") to standard output (stdout)";
  return help;
}

const char *XFILTER::Description(PSTRLIST List) const
{
  static STRING help;

  const STRING ThisDoctype("XFILTER");
  if (List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);
  XML::Description(List);

  if (help.IsEmpty()) help = genHelp("XML", GetDefaultFilter(), GetDefaultOptions(), Doctype);
  return help;
}


void XFILTER::SourceMIMEContent(STRING *stringPtr) const
{
  *stringPtr =  MIME_Type;
}

STRING  XFILTER::SetFilter(const STRING& arg)
{
  STRING filter (arg);

  if (arg.GetLength() && (arg != "NULL"))
    {
      filter =  ResolveBinPath(arg);
      if (!IsAbsoluteFilePath(filter))
        {
	  message_log (LOG_WARN, "%s: %s '%s' must be in $PATH.", Doctype.c_str(),
		ini_filter_tag, filter.c_str());
        }
      else if (!ExeExists(filter))
        {
          message_log (LOG_ERROR, "%s: %s '%s' %s!", Doctype.c_str(), ini_filter_tag, filter.c_str(),
            Exists(Filter) ?  "is not executable" : "does not exist");
          filter.Clear();
        }
      else
        message_log (LOG_DEBUG, "%s: External filter set to '%s'", Doctype.c_str(), filter.c_str());
    }
  Filter = filter;
  return filter;
}

static const STRING NulFilter ("NULL");

void XFILTER::BeforeIndexing()
{
  DOCTYPE::BeforeIndexing();

#if USE_LIBMAGIC
  if (magic_cookie == NULL)
    {
      magic_cookie = magic_open( MAGIC_SYMLINK|MAGIC_ERROR|  MAGIC_MIME);
      if (magic_cookie)
	magic_load (magic_cookie, NULL);
    }
#endif

  if (Filter.IsEmpty()) message_log (LOG_ERROR, "%s: No %s set. Nothing to do?", Doctype.c_str(), ini_filter_tag);
  else if (Filter.Equals (  NulFilter ))
    Filter.Clear();
  else
    message_log (LOG_INFO, "%s: Using '%s' to filter to XML.", Doctype.c_str(), Filter.c_str());
}


void XFILTER::AfterIndexing()
{
// XML:AfterIndexing();
#if USE_LIBMAGIC
  if (magic_cookie != NULL)
    {
      magic_close (magic_cookie);
      magic_cookie = NULL;
    }
#endif
}

XFILTER::XFILTER(PIDBOBJ DbParent, const STRING& Name) : XML(DbParent, Name)
{
  Filter = SetFilter( Getoption(ini_filter_tag, GetDefaultFilter()));
  Options = Getoption(ini_options_tag, GetDefaultOptions());
  MIME_Type = Getoption(ini_content_type_tag, mime_application_binary);
#if USE_LIBMAGIC
  magic_cookie = NULL;
#endif
}

//
GDT_BOOLEAN XFILTER::GenRecord(const RECORD& FileRecord)
{
  const char *doctype = Doctype.c_str();
  if (Filter.IsEmpty())
    {
      message_log (LOG_WARN, "%s: Filter is NIL", doctype);
      return GDT_TRUE; // Do nothing
    }

  STRING key, s, Fn, outfile;
  struct stat stbuf;

  Fn = FileRecord.GetFullFileName ();

#if USE_LIBMAGIC
  STRING mime_typ = magic_file (magic_cookie, Fn.c_str());
#endif


  message_log (LOG_DEBUG, "%s: Input = '%s'", doctype, Fn.c_str());

  INODE Inode (Fn);

  if (!Inode.Ok())
    {
      if (Inode.isDangling())
	message_log(LOG_ERROR, "%s: '%s' is a dangling symbollic link", doctype, Fn.c_str());
      else
	message_log(LOG_ERRNO, "%s: Can't stat '%s'.", doctype , Fn.c_str());
      return GDT_FALSE;
    }
  if (Inode.st_size == 0)
    {
      message_log(LOG_ERROR, "'%s' has ZERO (0) length? Skipping.", Fn.c_str());
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

  message_log (LOG_DEBUG, "%s: Key set to '%s'", doctype, key.c_str());

  Db->ComposeDbFn (&s, DbExtCat);
  if (MkDir(s, 0, GDT_TRUE) == -1) // Force creation
    {
      message_log (LOG_ERRNO, "Can't create filter directory '%s'", s.c_str() );
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
  message_log (LOG_DEBUG, "Output to '%s'", outfile.c_str());

  FILE *fp;
  if ((fp = fopen(outfile, "w")) == NULL)
   {
     message_log (LOG_ERRNO, "%s: Could not create '%s'",  doctype, outfile.c_str());
     return GDT_FALSE;
   }
  off_t len = 0;

  // We write full file path as ordinary string
  start = Fn.WriteFile(fp);
  start += fwrite("\0\0", 1, 2, fp) - 1;

#if 0
  // Here we want to write some metadata into the .cat file
  len += Doctype.CatMetaInfoIntoFile(fp, Fn);
#endif

  STRING pipe = Filter + " " + Options.Escape() + " " +  Fn.Escape();

  FILE *pp = _IB_popen(pipe, "r");
  if (pp == NULL)
    {
      message_log (LOG_ERRNO, "%s: Could not open pipe '%s'", doctype, pipe.c_str());
      fclose(fp);
      UnlinkFile(outfile);

      if (!IsAbsoluteFilePath (Filter))
	{
	  message_log (LOG_ERROR, "%s: Check configuration for filter '%s'. Skipping rest.", doctype, Filter.c_str());
	  Filter.Clear();
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
      if (mod_filter.SetTimeOfFile(Filter) && mod_input.SetTimeOfFile(Fn) && mod_filter > mod_input)
        NewRecord.SetDateModified ( mod_filter );
      NewRecord.SetDate ( mod_input ); 

      STRING urifile;
      if (Db->_write_resource_path(outfile, FileRecord,
#if USE_LIBMAGIC
 		mime_typ, 
#endif
		&urifile) == GDT_FALSE)
        message_log (LOG_ERRNO, "%s: Could not create '%s'", doctype, urifile.c_str());

      Db->DocTypeAddRecord(NewRecord);
      return GDT_TRUE;
    }
  message_log (LOG_ERROR, "%s: pipe '%s' returned ONLY %d bytes!", doctype, pipe.c_str(), len);
  // Remove junk
  UnlinkFile(outfile);
  return GDT_FALSE;
}


void XFILTER::ParseRecords(const RECORD& FileRecord)
{
  if (Filter.Equals( NulFilter ))
    {
      message_log (LOG_WARN, "%s:BeforeIndexing() not called", Doctype.c_str());
      BeforeIndexing();
    }

  GenRecord(FileRecord);
}

void XFILTER::ParseFields(RECORD *RecordPtr)
{
  XML::ParseFields(RecordPtr);
}



void XFILTER:: DocPresent (const RESULT& ResultRecord,
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

void XFILTER::Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const
{
  StringBuffer->Clear();
  if (ElementSet.Equals(BRIEF_MAGIC)) {
    XML::Present(ResultRecord, "TITLE", RecordSyntax, StringBuffer);
    if (StringBuffer->IsEmpty())
	*StringBuffer = Db->_get_resource_path(ResultRecord.GetFullFileName()) + " (Untitled)";
  } else if (ElementSet.Equals(FULLTEXT_MAGIC) || ElementSet.Equals(SOURCE_MAGIC))
    DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
  else
   XML::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}

GDT_BOOLEAN XFILTER::GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const
{
  STRING fullpath (  Db->_get_resource_path(ResultRecord.GetFullFileName()) );
  if (StringBuffer)
    *StringBuffer = fullpath;
  return Exists (fullpath);
}

