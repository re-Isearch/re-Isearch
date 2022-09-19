/*@@@
File:		msoffice.cxx
Version:	1.00
Description:	Class NULL
Author:		Edward Zimmermann
@@@*/


#include <ctype.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

#include <sys/ioctl.h>

#include "common.hxx"
#include "memodoc.hxx"
#include "process.hxx"
#include "doc_conf.hxx"

/// USE Pandoc instead

#define MAX_TIME_BETWEEN_CHARS 2

static const char default_filter[] = "docx2txt";


class IBDOC_MSOFFICE : public _VMEMODOC {
public:
   IBDOC_MSOFFICE(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const {
      List->AddEntry (Doctype);
      _VMEMODOC::Description(List);
      return desc.c_str();
   }

   void SourceMIMEContent(STRING *stringPtr) const {
    *stringPtr = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
   }

  INT UnifiedNames (const STRING& tag, PSTRLIST Value) const
    {
//cerr << "Want Unified Name for " << tag << endl;
    // We don't want these
    if ((tag ^= "Line") || (tag ^= "firstLine"))
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

   const char *GetDefaultFilter() const { return  default_filter;  }

  off_t RunPipe(FILE *fp, const STRING& Fn);

   void ParseFields(RECORD *RecordPtr);

   GDT_BOOLEAN GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const;

   void Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const;

   ~IBDOC_MSOFFICE() { }

private:
   STRING desc;
   GDT_BOOLEAN usingDefaultFilter;
};

static const char myDescription[] = "M$ Office OOXML Plugin";

IBDOC_MSOFFICE::IBDOC_MSOFFICE(PIDBOBJ DbParent, const STRING& Name) : _VMEMODOC(DbParent, Name)
{
  desc.form("%s. Uses an external filter to convert OOXML (DOCX) to MEMODOC.\nOptions:\n\
   FILTER   Specifies the program to use (Default '%s')\n\n\
   OPTIONS  Specifies options for the filter.\n", myDescription, GetDefaultFilter());

  STRING s (ResolveBinPath(Getoption("FILTER", GetDefaultFilter())));

  usingDefaultFilter = (s.Search(GetDefaultFilter()) != 0);

  logf (LOG_DEBUG, "%s: filter '%s' specified%s", Doctype.c_str(), s.c_str(),
	(usingDefaultFilter ? " (default)" : ""));
  if (s.GetLength() && (s != "NULL"))
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
    Filter = s;

  if (DateModifiedField.IsEmpty()) DateModifiedField = "LastModifiedDate";
  if (DateCreatedField.IsEmpty())  DateCreatedField  = "CreationDate";
   if (Db)
    {
      Db->AddFieldType(DateCreatedField, FIELDTYPE::date);
      Db->AddFieldType(DateModifiedField, FIELDTYPE::date);
    }
}


GDT_BOOLEAN IBDOC_MSOFFICE::GetResourcePath(const RESULT& ResultRecord, STRING *StringBuffer) const
{
  STRING fullpath (  Db->_get_resource_path(ResultRecord.GetFullFileName()) );
  if (StringBuffer)
    *StringBuffer = fullpath;
  return Exists(fullpath);
}


off_t IBDOC_MSOFFICE::RunPipe(FILE *fp, const STRING& Fn)
{
  if (Filter.IsEmpty() || Filter == "NULL")
    return -1; // Do nothing

  STRING inputfile;

  inputfile = Fn.Escape(); 

  int argc = 0;
  const char * argv[256];

  if (usingDefaultFilter)
   {
      // This is based on Perl so we want to run Perl
      argv[argc++] = "perl";
   }
  argv[argc++] = (char *)(Filter.c_str()); 

  STRING options (Getoption("OPTIONS"));
  STRLIST list;


  if (!options.IsEmpty())
   {
     // Split the options
     STRLIST *ptr = &list.SplitTerms(options);
     do {
	argv[argc++] = ptr->Value();
    } while ((ptr = ptr->Next()) != NULL && argc < 255);
   }
  argv[argc++] = (char *) Fn.c_str();
  argv[argc] = NULL;

  if (usingDefaultFilter)
    {
      //pipe += " -";
      fprintf(fp, "-------------------------\n\n");
    }

  FILE *pp = _IB_popen(argv, "r");
  if (pp == NULL)
    {
      if (!IsAbsoluteFilePath (Filter))
	{
	  logf (LOG_ERROR, "%s: Check configuration for filter '%s'. Skipping rest.",
		Doctype.c_str(), Filter.c_str());
	  Filter.Clear();
	}
      else
	logf (LOG_ERRNO, "%s: Could not open pipe '%s'", Doctype.c_str(), argv[0]);
      return -1;
    }

  int ch;
  off_t  len = 0;

  fd_set rfds;
  struct timeval tv;

  FD_ZERO(&rfds);
  FD_SET(fileno(pp), &rfds);
  /* Wait up to MAX_TIME_BETWEEN_CHARS seconds. */
  tv.tv_sec =  MAX_TIME_BETWEEN_CHARS;
  tv.tv_usec = 0;

//  GDT_BOOLEAN   nonblocking =  (ioctl(fileno(pp), FIONBIO) == EINVAL) ? GDT_FALSE : GDT_TRUE;
  GDT_BOOLEAN   nonblocking = GDT_FALSE;
  int           retval      = nonblocking ? select(1, &rfds, NULL, NULL, &tv) : 1;

//cerr << "NonBlock = " << (int)nonblocking << " First retval = " << (int)retval << endl;
//retval = 1;

  while (retval && (ch = fgetc(pp)) != EOF)
    {
      fputc(ch, fp);
      len++;
      if (feof(pp))
	break;
      if (nonblocking)
	retval = select(1, &rfds, NULL, NULL, &tv);
    }
  if (retval == 0 && !feof(pp))
    logf(LOG_WARN, "Subprocess '%s' timed out within %d seconds", *argv, MAX_TIME_BETWEEN_CHARS);
  _IB_pclose(pp);
  return len;
}

void IBDOC_MSOFFICE::ParseFields(RECORD *RecordPtr)
{
  _VMEMODOC::ParseFields(RecordPtr);
}


void IBDOC_MSOFFICE::Present (const RESULT& ResultRecord,
         const STRING& ElementSet, const STRING& RecordSyntax,
         PSTRING StringBuffer) const
{
  StringBuffer->Clear();
  if (ElementSet.Equals(BRIEF_MAGIC))
    {
      _VMEMODOC::Present(ResultRecord, "Title", RecordSyntax, StringBuffer);
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
	    _VMEMODOC::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
	}
    }
  else if (ElementSet.Equals(FULLTEXT_MAGIC) || ElementSet.Equals(SOURCE_MAGIC))
    DocPresent(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
  else
   _VMEMODOC::Present(ResultRecord, ElementSet, RecordSyntax, StringBuffer);
}



// Stubs for dynamic loading
extern "C" {
  IBDOC_MSOFFICE *  __plugin_msoffice_create (IDBOBJ * parent, const STRING& Name)
  {
    return new IBDOC_MSOFFICE (parent, Name);
  }
  int          __plugin_msoffice_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_msoffice_query (void) { return myDescription; }
}


