/*@@@
File:		msrtf.cxx
Version:	1.00
Description:	Class NULL
Author:		Edward Zimmermann
@@@*/

#include <ctype.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

#include "common.hxx"
#include "xml.hxx"
#include "process.hxx"
#include "doc_conf.hxx"

static const char mime_type[] = "application/rtf";


class IBDOC_MSRTF : public XML {
public:
   IBDOC_MSRTF(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const {
      List->AddEntry (Doctype);
      XML::Description(List);
      return desc.c_str();
   }

   void SourceMIMEContent(STRING *stringPtr) const {
    *stringPtr = mime_type;
   }
   const char *GetDefaultFilter() const { return "unrtf";     }

   void ParseRecords(const RECORD& FileRecord);
   void ParseFields(RECORD *RecordPtr);

   bool GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const;

   void Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const;

   void DocPresent (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const;

   ~IBDOC_MSRTF() { }

private:
   STRING Filter;
   STRING desc;
};

static const char myDescription[] = "M$ RTF (Rich Text Format) Plugin [XML]";

IBDOC_MSRTF::IBDOC_MSRTF(PIDBOBJ DbParent, const STRING& Name) : XML(DbParent, Name)
{
  desc.form("%s. Uses an external filter to XML.\nOptions:\n\
   FILTER   Specifies the program to use (Default '%s')\n", myDescription, GetDefaultFilter());

  STRING s (ResolveBinPath(Getoption("FILTER", GetDefaultFilter())));
  if (s.GetLength() && (s != "NULL"))
    {
      Filter = ResolveBinPath(s);
      if (!IsAbsoluteFilePath(Filter))
	{
	  message_log (LOG_WARN, "%s: Specified filter '%s' not found. Check Installation.",
		Doctype.c_str(), Filter.c_str()); 
	}
      else if (!ExeExists(Filter))
	{
	  message_log (LOG_ERROR, "%s: Filter '%s' %s!", Doctype.c_str(), Filter.c_str(),
	    Exists(Filter) ?  "is not executable" : "does not exist");
	  Filter.Clear();
	}
      else
	message_log (LOG_DEBUG, "%s: External filter set to '%s'", Doctype.c_str(), Filter.c_str());
    }
  else
    Filter = s;
}

void IBDOC_MSRTF::ParseRecords(const RECORD& FileRecord)
{
  if (Filter.IsEmpty() || Filter == "NULL")
    return; // Do nothing

  STRING key, s, Fn, outfile, docfile, urifile;
  struct stat stbuf;

  Fn = FileRecord.GetFullFileName ();

  message_log (LOG_DEBUG, "%s: Input = '%s'", Doctype.c_str(), Fn.c_str());
  if (_IB_lstat(Fn, &stbuf) == -1)
    {
      message_log(LOG_ERRNO, "%s: Can't stat '%s'.", Doctype.c_str(), Fn.c_str());
      return;
    }
#ifndef _WIN32
  if (stbuf.st_mode & S_IFLNK)
    {
      if (stat(Fn, &stbuf) == -1)
        {
          message_log(LOG_ERROR, "%s: '%s' is a dangling symbollic link", Doctype.c_str(), Fn.c_str());
          return;
        }
    }
#endif
  if (stbuf.st_size == 0)
    {
      message_log(LOG_ERROR, "'%s' has ZERO (%ld) length? Skipping.", Fn.c_str(), stbuf.st_size);
      return;
    }

  const long inode = stbuf.st_ino;
  off_t start = FileRecord.GetRecordStart();
  off_t end   = FileRecord.GetRecordEnd();

  if ((end == 0) || (end > stbuf.st_size) )
    end = stbuf.st_size;



  if (start == 0 && end == stbuf.st_size)
    s.form("%lX", (long)inode);
  else if (end == stbuf.st_size)
    s.form("%lx-%llx", inode, (long long)start); 
  else
    s.form("%lX-%lx%llX-%llx", (long)inode, (long long)start, (long long)end);
  if (s.GetLength() > DocumentKeySize-8)
    s.form("%lXlx", (long)inode, (long)(end-start));
  key = s;

  for (int i=1; Db->KeyLookup (key); i++)
    key.form("%s.%04x", s.c_str(), i);
  // Now we have a good key
     
  message_log (LOG_DEBUG, "Key set to '%s'", key.c_str());

  Db->ComposeDbFn (&s, DbExtCat);
  if (MkDir(s, 0, true) == -1 )
    {
      message_log (LOG_ERRNO, "Can't create filter directory '%s'", s.c_str() );
      return;
    }

  // <db_ext>.cat/<Hash>/<Key>
  outfile =  AddTrailingSlash(s);
  outfile.Cat (((long)key.CRC16()) % 1000);
  if (MkDir(outfile, 0, true) == -1)
    outfile = s; // Can't make it
  AddTrailingSlash(&outfile);
  outfile.Cat (key);


  // So we pipe Fn bytes Start to End into outfile

  message_log (LOG_DEBUG, "Output to '%s'", outfile.c_str());

  if (Db->_write_resource_path(outfile, FileRecord, mime_type, &urifile) == false)
    {
      message_log (LOG_ERRNO, "%s: Could not create '%s'", Doctype.c_str(), urifile.c_str());
      return;
    }

  FILE *fp;
  if ((fp = fopen(outfile, "wb")) == NULL)
    {
      unlink(urifile);
      message_log (LOG_ERRNO, "%s: Could not create '%s'", outfile.c_str());
      return;
    }


  docfile = Fn.Escape();  // encode the stuff
      
  STRING options;

  if (Filter.Search(GetDefaultFilter()))
   {
     options = "-t xml";
   }

  STRING pipe (Filter + " " +  options + docfile);

  FILE *pp = _IB_popen(pipe, "r");
  if (pp == NULL)
    {
      message_log (LOG_ERRNO, "%s: Could not open pipe '%s'", Doctype.c_str(), pipe.c_str());
      fclose(fp);
      unlink(urifile);
      unlink(outfile);

      if (!IsAbsoluteFilePath (Filter))
	{
	  message_log (LOG_ERROR, "%s: Check configuration for filter '%s'. Skipping rest.",
		Doctype.c_str(), Filter.c_str());
	  Filter.Clear();
	}
      return;
    }

  int ch;
  off_t  len = 0;
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
      // We now have a record in outfile from 0 to len
      NewRecord.SetRecordStart (0);
      NewRecord.SetRecordEnd ( len - 1 );
      NewRecord.SetFullFileName ( outfile );
      NewRecord.SetKey( key ); // Set the key since we did the effort

      // Set some default dates
      SRCH_DATE mod_filter, mod_input;
      if (mod_filter.SetTimeOfFile(Filter) && mod_input.SetTimeOfFile(Fn) && mod_filter > mod_input)
        NewRecord.SetDateModified ( mod_filter );
      NewRecord.SetDate ( mod_input ); 

      Db->DocTypeAddRecord(NewRecord);
   }
}

void IBDOC_MSRTF::ParseFields(RECORD *RecordPtr)
{
  XML::ParseFields(RecordPtr);
}


bool IBDOC_MSRTF::GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const
{
  if (StringBuffer)
    *StringBuffer = Db->_get_resource_path(ResultRecord.GetFullFileName());
  return true;
}


void IBDOC_MSRTF:: DocPresent (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const
{
  StringBuffer->Clear();

  if ( (RecordSyntax == SutrsRecordSyntax) ?
	ElementSet.Equals(SOURCE_MAGIC) : ElementSet.Equals(FULLTEXT_MAGIC) )
    {
      // Read here the .rtf file
      if (RecordSyntax == HtmlRecordSyntax)
	 *StringBuffer  = DOCTYPE::Httpd_Content_type (ResultRecord);
      StringBuffer->CatFile( Db->_get_resource_path(ResultRecord.GetFullFileName())  );
    }
  else if ( (RecordSyntax == SutrsRecordSyntax) ?
        ElementSet.Equals(FULLTEXT_MAGIC) : ElementSet.Equals(SOURCE_MAGIC) )
    {
      // Read here the converted .rtf file
      if (RecordSyntax == HtmlRecordSyntax)
         *StringBuffer  = DOCTYPE::Httpd_Content_type (ResultRecord, "plain/text");
      StringBuffer->CatFile( ResultRecord.GetFullFileName() );
    }
  else
   XML::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}



void IBDOC_MSRTF::Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const
{
  StringBuffer->Clear();
  if (ElementSet.Equals(FULLTEXT_MAGIC) || ElementSet.Equals(SOURCE_MAGIC))
    DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
  else
   XML::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}



// Stubs for dynamic loading
extern "C" {
  IBDOC_MSRTF *  __plugin_msrtf_create (IDBOBJ * parent, const STRING& Name)
  {
    return new IBDOC_MSRTF (parent, Name);
  }
  int          __plugin_msrtf_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_msrtf_query (void) { return myDescription; }
}

