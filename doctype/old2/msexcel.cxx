/*@@@
File:		msexcel.cxx
Version:	1.00
Description:	Class NULL
Author:		Edward Zimmermann
@@@*/

#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>

#include "common.hxx"
#include "tsldoc.hxx"
#include "process.hxx"
#include "doc_conf.hxx"

#define DEBUG 0


static char mime_type[] = "application/vnd.ms-excel";

class IBDOC_MSEXCEL : public TSLDOC {
public:
   IBDOC_MSEXCEL(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const {
      List->AddEntry (Doctype);
      TSLDOC::Description(List);
      return desc.c_str();
   }

   void SourceMIMEContent(STRING *stringPtr) const {
    *stringPtr = mime_type;
   }
   const char *GetDefaultFilter() const { return "xls2tsl";     }

   void ParseRecords(const RECORD& FileRecord);
   void ParseFields(RECORD *RecordPtr);

   GDT_BOOLEAN GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const;

   void Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const;

   void DocPresent (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const;

   ~IBDOC_MSEXCEL() { }
private:
   STRING      Filter;
   STRING      desc;
   off_t        HostID;
   GDT_BOOLEAN oneline; 
};

static const char myDescription[] = "M$ Excel (XLS) Plugin";


IBDOC_MSEXCEL::IBDOC_MSEXCEL(PIDBOBJ DbParent, const STRING& Name) : TSLDOC(DbParent, Name)
{
  oneline = GDT_TRUE;

  const char *oneline_default_value = oneline ? "Yes" : "No";

  desc.form("%s. Uses an external filter to TSLDOC.\nOptions:\n\
   FILTER   Specifies the program to use (Default '%s')\n\
   ONELINE  Yes/No specifies if each line should be a record (Default %s)\n",
	myDescription, GetDefaultFilter(), oneline_default_value);

 oneline =  Getoption("ONELINE", oneline_default_value).GetBool(); 

  STRING s (ResolveBinPath(Getoption("FILTER", GetDefaultFilter())));
  if (s.GetLength() && (s != "NULL"))
    {
      Filter = ResolveBinPath(s);
      if (!IsAbsoluteFilePath(Filter))
	{
	  logf (LOG_WARN, "%s: Specified filter '%s' not found. Check Installation.",
		Doctype.c_str(), Filter.c_str()); 
	  //Filter.Clear();
	}
      else if (!ExeExists(Filter))
	{
	  logf (LOG_ERROR, "%s: Filter '%s' %s!", Doctype.c_str(), Filter.c_str(),
	    Exists(Filter) ?  "is not executable" : "does not exist");
	  Filter.Clear();
	}
      else
	logf (LOG_DEBUG, "%s: External filter set to '%s'", Doctype.c_str(), Filter.c_str());
    }
  else
    Filter = s;
}


void IBDOC_MSEXCEL::ParseRecords(const RECORD& FileRecord)
{
  if (Filter.IsEmpty() || Filter == "NULL")
    return; // Do nothing

  STRING key, s, outfile, docfile, urifile;

  const STRING Fn (FileRecord.GetFullFileName () );

  const INODE Inode(Fn);
  if (!Inode.Ok())
    {
      if (Inode.isDangling())
        logf(LOG_ERROR, "%s: '%s' is a dangling symbollic link", Doctype.c_str(), Fn.c_str());
      else
        logf(LOG_ERRNO, "%s: Can't stat '%s'.", Doctype.c_str(), Fn.c_str());
      return;
    }
  if (Inode.st_size == 0)
    {
      logf(LOG_ERROR, "'%s' has ZERO (0) length? Skipping.", Fn.c_str());
      return;
    }

  logf (LOG_DEBUG, "%s: Input = '%s'", Doctype.c_str(), Fn.c_str());

  off_t start = FileRecord.GetRecordStart();
  off_t end   = FileRecord.GetRecordEnd();
  
  if ((end == 0) || (end > Inode.st_size) ) end = Inode.st_size;
  
  if ((key = FileRecord.GetKey()).IsEmpty())
    key = Inode.Key(start, end); // Get Key
  for (int i=1; Db->KeyLookup (key); i++)
    key.form("%s.%04x", s.c_str(), i);

  // Now we have a good key
  logf (LOG_DEBUG, "Key set to '%s'", key.c_str());


  Db->ComposeDbFn (&s, DbExtCat);
  if (MkDir(s, 0, GDT_TRUE) == -1)
    {
      logf (LOG_ERRNO, "Can't create filter directory '%s'", s.c_str() );
      return;
    }
  // <db_ext>.cat/<Hash>/<Key>.memo
  outfile =  AddTrailingSlash(s);
  outfile.Cat (((long)key.CRC16()) % 1000);
  if (MkDir(outfile, 0, GDT_TRUE) == -1)
    outfile = s; // Can't make it
  AddTrailingSlash(&outfile);
  outfile.Cat (key);

  if (Db->_write_resource_path(outfile, FileRecord, mime_type, &urifile) == GDT_FALSE) 
    {
      logf (LOG_ERRNO, "%s: Could not create '%s'", urifile.c_str());
      return;
    }

  FILE *fp;
  // So we pipe Fn bytes Start to End into outfile
  if ((fp = fopen(outfile, "w")) == NULL)
   {
     unlink(urifile);
     logf (LOG_ERRNO, "%s: Could not create '%s'", outfile.c_str());
     return;
   }

  char *argv[5];
  int   argc = 0;

  argv[argc++] = (char *)Filter.c_str();
  if (Filter.Search(GetDefaultFilter()))
   {
     const char *charset =  Db->GetLocale().GetCharsetName();
     if (strncmp(charset, "iso", 3) == 0)
        charset += 3;
     if (*charset == '-')
        charset++;
     argv[argc++] = (char *)"-d";
     argv[argc++] = (char *)charset;
   }

  argv[argc++] = (char *)Fn.c_str();
  argv[argc] = NULL;

  off_t   len = 0;

  FILE *pp = _IB_popen(argv, "r");
  if (pp == NULL)
    {
      logf (LOG_ERRNO, "%s: Could not open pipe '%s'", Doctype.c_str(), STRING(argv).c_str());
      fclose(fp);
      unlink(urifile);
      unlink(outfile);

      if (!IsAbsoluteFilePath (Filter))
	{
	  logf (LOG_ERROR, "%s: Check configuration for filter '%s'. Skipping rest.",
		Doctype.c_str(), argv[0]);
	  Filter.Clear();
	}
      return;
    }

  int ch;
  while ((ch = fgetc(pp)) != EOF)
    {
      fputc(ch, fp);
      len++;
    }
  _IB_pclose(pp);

  if (len <= 8)
    {
      fclose(fp);
      logf (LOG_ERROR, "%s: Pipe '%s' returned only %u bytes. Skipping.", Doctype.c_str(), argv[0], len);
      return;
    }
  len = ftell(fp);

  fclose(fp);

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

  if (oneline)
    {
      MMAP mapping;
      mapping.CreateMap(outfile, MapSequential);

      if (mapping.Ok())
	{
	  const UCHR   *ptr = (const UCHR *)mapping.Ptr();
	  const size_t  len  = mapping.Size();
	  unsigned long column = 0, lineno = 0;
	  unsigned long column0 = 0;
	  const UCHR   *start_of_line = ptr;
	  for (size_t i = 0; i < len && oneline; i++)
	    {
	      switch (*ptr++)
		{
		  case '\t': column++; break;
		  case '\n':
		  case '\r': lineno++;
		    if (column > 0)
		      {
			if (column0 == 0)
			  column0 = column;
			else if (column != column0)
			  {
			    unsigned long acolumns = column;
			    for (int i=-1; i > -20; i--)
			      {
				if (ptr[i] == '\t')
				  acolumns--;
				else if (isalnum(ptr[i]))
				   break;
			      }
			    if (acolumns == column0)
			      {
				continue;
			      }
#if DEBUG
			    if (ptr - start_of_line < 2058)
			      {
				STRING  line(start_of_line, ptr-start_of_line);
				line.Replace("\t", "|");
				logf (LOG_DEBUG, "LINE= %s", line.c_str());
			      }
#endif
			    
			    logf (LOG_INFO, "\
%s: '%s' has irregular columns (line #%d, %d != %d). ONELINE turned off",
				Doctype.c_str(), outfile.c_str(), lineno+1, column+1, column0+1);
			    oneline = GDT_FALSE;
			  }
		      }
		     column = 0;
		     start_of_line = ptr+1;
		  default:
		    break;
		}
	    }

	  mapping.Unmap();
	}
      if (oneline)
	{
	  logf (LOG_INFO, "Parsing '%s' records", outfile.c_str());
	  TSLDOC::ParseRecords( NewRecord );
	  return;
	}
    }
  Db->DocTypeAddRecord(NewRecord);
}

void IBDOC_MSEXCEL::ParseFields(RECORD *RecordPtr)
{
  if (oneline)
    TSLDOC::ParseFields(RecordPtr);
  else
    DOCTYPE::ParseFields(RecordPtr);
}

void IBDOC_MSEXCEL:: DocPresent (const RESULT& ResultRecord,
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
      StringBuffer->CatFile( Db->_get_resource_path(ResultRecord.GetFullFileName())  );
    }
  else if ( (RecordSyntax == SutrsRecordSyntax) ?
        ElementSet.Equals(FULLTEXT_MAGIC) : ElementSet.Equals(SOURCE_MAGIC) )
    {
      if (RecordSyntax == HtmlRecordSyntax)
         *StringBuffer  = DOCTYPE::Httpd_Content_type (ResultRecord, "text/tab-separated-values");
      StringBuffer->CatFile( ResultRecord.GetFullFileName() );
    }

  else
   TSLDOC::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}

GDT_BOOLEAN IBDOC_MSEXCEL::GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const
{
  STRING fullpath (  Db->_get_resource_path(ResultRecord.GetFullFileName()) );
  if (StringBuffer)
    *StringBuffer = fullpath;
  return Exists(fullpath);
}


void IBDOC_MSEXCEL::Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const
{
  StringBuffer->Clear();
  if (ElementSet.Equals(SOURCE_MAGIC) || ElementSet.Equals(FULLTEXT_MAGIC))
    DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
  else
   TSLDOC::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}



// Stubs for dynamic loading
extern "C" {
  IBDOC_MSEXCEL *  __plugin_msexcel_create (IDBOBJ * parent, const STRING& Name)
  {
    return new IBDOC_MSEXCEL (parent, Name);
  }
  int          __plugin_msexcel_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_msexcel_query (void) { return myDescription; }
}

