/*@@@
File:		adobe_pdf.cxx
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
#include "process.hxx"
#include "memodoc.hxx"
#include "doc_conf.hxx"

static const char out_ext[] = ".memo";
static const STRING SOURCE_PATH_ELEMENT ("Path");


class IBDOC_PDF : public MEMODOC {
public:
   IBDOC_PDF(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const {
      List->AddEntry (Doctype);
      MEMODOC::Description(List);
      return desc.c_str();
   }

   void SourceMIMEContent(STRING *stringPtr) const {
    *stringPtr = "application/pdf";
   }

 void BeforeIndexing() {
//cerr << "PDF Before Indexing..." << endl;
    MEMODOC::BeforeIndexing(); }


  INT UnifiedNames (const STRING& tag, PSTRLIST Value) const
    {
    // We don't want these
    if ((tag ^= "Line") || (tag ^= "firstLine") || (tag ^= "ERROR"))
      {
	Value->Clear();
	return 0;
      }
    if (tag.SearchAny("-BODY"))
      {
	*Value = STRLIST("text");
	return 1;
      }
    return DOCTYPE::UnifiedNames(tag, Value);
  }

   const char *GetDefaultFilter() const { return "pdftomemo";     }

   void ParseRecords(const RECORD& FileRecord);
   void ParseFields(RECORD *RecordPtr);

   GDT_BOOLEAN GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const;

   void Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const;

   void DocPresent (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const;

   ~IBDOC_PDF() { }

private:
   STRING Filter;
   STRING desc;
   off_t   HostID;
};

static const char myDescription[] = "Adobe PDF Plugin";

IBDOC_PDF::IBDOC_PDF(PIDBOBJ DbParent, const STRING& Name) : MEMODOC(DbParent, Name)
{
  AllowZeroLengthPages(GDT_TRUE);
  SetParseMessageStructure(GDT_TRUE);
  desc.form("%s. Uses an external filter to MEMODOC.\nOptions:\n\
   FILTER   Specifies the program to use (Default '%s')\n\
If a <filename>.meta file is available (colon standard format) its used to\n\
supplement metadata.", myDescription, GetDefaultFilter());

  STRING s (Getoption("FILTER", GetDefaultFilter()));
  if (s.GetLength() && (s != "NULL") && s.GetChr(1) != '<')
    {
      Filter = ResolveBinPath(s);
      if (!IsAbsoluteFilePath(Filter))
	{
	  logf (LOG_WARN, "%s: Specified filter '%s' not found. Check Installation.",
		Doctype.c_str(), Filter.c_str()); 
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
    {
      logf (LOG_DEBUG, "%s de-activated: External filter was set to '%s'", Doctype.c_str(), s.c_str());
      Filter = NulString;
    }

  if (DateModifiedField.IsEmpty()) DateModifiedField = "ModDate";
  if (DateCreatedField.IsEmpty())  DateCreatedField  = "CreationDate";
  if (DateExpiresField.IsEmpty())  DateExpiresField  = "expirationDate";
   if (Db)
    {
      Db->AddFieldType(DateCreatedField, FIELDTYPE::date);
      Db->AddFieldType(DateModifiedField, FIELDTYPE::date);
      Db->AddFieldType(DateExpiresField, FIELDTYPE::date);
    }
   DateField = DateCreatedField;
}


void IBDOC_PDF::ParseRecords(const RECORD& FileRecord)
{
  if (Filter.IsEmpty())
    return; // Do nothing

  STRING key, s, Fn, outfile, pdffile;
  unsigned long version = 0;
  struct stat stbuf;

  Fn = FileRecord.GetFullFileName ();

  logf (LOG_DEBUG, "%s: Input = '%s'", Doctype.c_str(), Fn.c_str());
  if (_IB_lstat(Fn, &stbuf) == -1)
    {
      logf(LOG_ERRNO, "%s: Can't stat '%s'.", Doctype.c_str(), Fn.c_str());
      return;
    }
#ifndef _WIN32
  if (stbuf.st_mode & S_IFLNK)
    {
      if (stat(Fn, &stbuf) == -1)
        {
          logf(LOG_ERROR, "%s: '%s' is a dangling symbollic link", Doctype.c_str(), Fn.c_str());
          return;
        }
    }
#endif
  if (stbuf.st_size == 0)
    {
      logf(LOG_ERROR, "'%s' has ZERO (%ld) length? Skipping.", Fn.c_str(), stbuf.st_size);
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

  while (Db->KeyLookup (key))
    key.form("%s.%ld", s.c_str(), ++version); 
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

  if (out_ext[0])
    outfile.Cat (out_ext);


  FILE *fp;

  if ((fp = fopen(outfile, "w")) == NULL)
   {
     logf (LOG_ERRNO, "%s: Could not create '%s'", Doctype.c_str(), outfile.c_str());
     return;
   }

  STRING source ( ((Db && Db->getUseRelativePaths()) ? Db->RelativizePathname(Fn) : Fn) );

  fprintf(fp, "%s: %s\n", SOURCE_PATH_ELEMENT.c_str(), source.c_str());
  fprintf(fp, "%s-orig: %s\n", SOURCE_PATH_ELEMENT.c_str(), Fn.c_str());
  CatMetaInfoIntoFile(fp, Fn); // Add extra metainfo

  const char *argv[7];
  int    argc = 0;

  argv[argc++] = Filter.c_str();

  int    default_filter = 0;
  if (Filter.Search(GetDefaultFilter()))
   {
     default_filter = 1;
     argv[argc++] = "-memo";
#if 0
     const char *charset =  Db->GetLocale().GetCharsetName();
     if (strncmp(charset, "iso", 3) == 0)
	charset += 3;
     if (*charset == '-')
	charset++;
     argv[argc++] = "-enc";
     argv[argc++] = charset; 
#endif
   }
  argv[argc++] = Fn.c_str();
  if (default_filter)
    argv[argc++] = "-";

  argv[argc] = NULL;

  FILE *pp = _IB_popen(argv, "r");
  if (pp == NULL)
    {
      logf (LOG_ERRNO, "%s: Could not open pipe '%s'", Doctype.c_str(), *argv);
      fclose(fp);
      UnlinkFile(outfile);

      if (!IsAbsoluteFilePath (Filter))
	{
	  logf (LOG_ERROR, "%s: Check configuration for filter '%s'. Skipping rest.",
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

void IBDOC_PDF::ParseFields(RECORD *RecordPtr)
{
  MEMODOC::ParseFields(RecordPtr);
}

#if 0

static const char in_ext[]  = ".pdf";

static const STRING _pdfname(const STRING& Fullpath)
{
  int x = Fullpath.GetLength() - sizeof(out_ext)+1;
  STRING result = Fullpath;

  if (x > 0)
    {
      const char *ptr = result.c_str() + x;
      if (memcmp(ptr, out_ext, sizeof(out_ext)) == 0)
	result.EraseAfter(x);
    }
  result.Cat (in_ext);
  return result;
}
#endif


GDT_BOOLEAN IBDOC_PDF::GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const
{
  if (StringBuffer)
    {
      StringBuffer->Clear();
      Present(ResultRecord, SOURCE_PATH_ELEMENT, NulString, StringBuffer);
//cerr << "ResourcePath = " << *StringBuffer << endl;
      return StringBuffer->GetLength() != 0;
    }
  return GDT_TRUE;
}


void IBDOC_PDF:: DocPresent (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const
{
  StringBuffer->Clear();

  if ( (RecordSyntax == SutrsRecordSyntax) ?
	ElementSet.Equals(SOURCE_MAGIC) : ElementSet.Equals(FULLTEXT_MAGIC) )
    {
      // Read here the .pdf file
      if (RecordSyntax == HtmlRecordSyntax)
	 *StringBuffer  = DOCTYPE::Httpd_Content_type (ResultRecord);

      STRING Rawfile;
      Present(ResultRecord, SOURCE_PATH_ELEMENT, NulString, &Rawfile);
      if (GetFileSize(Rawfile) > 10)
	{
	  StringBuffer->CatFile( Rawfile );
	}
    }
  else if ( (RecordSyntax == SutrsRecordSyntax) ?
        ElementSet.Equals(FULLTEXT_MAGIC) : ElementSet.Equals(SOURCE_MAGIC) )
    {
      // Read here the .pdf file
      if (RecordSyntax == HtmlRecordSyntax)
         *StringBuffer  = DOCTYPE::Httpd_Content_type (ResultRecord, "plain/text");
      StringBuffer->CatFile( ResultRecord.GetFullFileName() );
    }
  else
   MEMODOC::DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}



void IBDOC_PDF::Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const
{
  StringBuffer->Clear();
  if (ElementSet.Equals(BRIEF_MAGIC))
    {
      MEMODOC::Present(ResultRecord, "Title", RecordSyntax, StringBuffer);
      if (StringBuffer->IsEmpty())
	{
	  STRING Fn (ResultRecord.GetFullFileName ());
	  FILE  *fp = fopen(Fn, "rb");
	  if (fp)
	    {
	      char  buf[BUFSIZ];
	      char  headline[BUFSIZ];
	      int   body = 0;
	      headline[0] = '\0';
	      while (fgets(buf, sizeof(buf)-1, fp))
		{
		  if (body && strlen(buf) > 10)
		    {
		      if (headline[0])
			strcat(headline, " ");
		      strcat(headline, buf);
		      if (strlen(headline) > 256)
			{
			  char *ptr = headline + 128;
			  while (isalnum(*ptr)) ptr++;
			  *ptr = '\0';
			  break;
			}
		    }
		  if (strncmp(buf, "----", 4) == 0)
		    body = 1; 
		}
	      fclose(fp);
	      *StringBuffer = headline;
	    }
	  if (StringBuffer->IsEmpty())
	    MEMODOC::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
	}
    }
  else if (ElementSet.Equals(FULLTEXT_MAGIC) || ElementSet.Equals(SOURCE_MAGIC))
    DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
  else
   MEMODOC::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}



// Stubs for dynamic loading
extern "C" {
  IBDOC_PDF *  __plugin_adobe_pdf_create (IDBOBJ * parent, const STRING& Name)
  {
    return new IBDOC_PDF (parent, Name);
  }
  int          __plugin_adobe_pdf_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_adobe_pdf_query (void) { return myDescription; }
}

