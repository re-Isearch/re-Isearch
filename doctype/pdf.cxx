/*@@@
File:		pdf.cxx
Version:	1.00
Description:	Class PDF
Author:		Edward Zimmermann
@@@*/



#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>

#include "common.hxx"
#include "process.hxx"
#include "pdf.hxx"
#include "doc_conf.hxx"

static const STRING TEXT_ELEMENT ("text");

const char *ADOBE_PDFDOC::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("PDF");
  if (Doctype.Search(ThisDoctype) == 0 && List->IsEmpty())
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);
  _VMEMODOC::Description(List);
  return desc.c_str();
}

void ADOBE_PDFDOC::SourceMIMEContent(STRING *stringPtr) const
{
  *stringPtr = "application/pdf";
}

INT ADOBE_PDFDOC::UnifiedNames (const STRING& tag, PSTRLIST Value) const
{
    // We don't want these
    if ((tag ^= "Line") || (tag ^= "firstLine") || (tag ^= "ERROR"))
      {
	Value->Clear();
	return 0;
      }
    if (tag.SearchAny("-BODY"))
      {
	*Value = STRLIST(TEXT_ELEMENT);
	return 1;
      }
    return DOCTYPE::UnifiedNames(tag, Value);
}
const char *ADOBE_PDFDOC::GetDefaultFilter() const { return "pdftomemo";     }


ADOBE_PDFDOC::ADOBE_PDFDOC(PIDBOBJ DbParent, const STRING& Name) : _VMEMODOC(DbParent, Name)
{
  out_ext = ".memo";

  AllowZeroLengthPages(true);
  SetParseMessageStructure(true);
  desc.form("Adobe PDF: Uses an external filter to MEMODOC.\nOptions:\n\
   FILTER   Specifies the program to use (Default '%s')\n\
If a <filename>.meta file is available (colon standard format) its used to\n\
supplement metadata.", GetDefaultFilter());

  STRING ini (Doctype), section(Doctype);
  ini.ToLower();
  section.ToUpper();

  desc << "\nOptions are defined in the \"" << ini << ".ini\" [General] or <Db>.ini [" << section << "] \
sections.\n\nFilters should take a single argument as the path to the (binary) input file and should write\n\
their MEMODOC style output to standard output (stdout)";

  STRING s (Getoption("FILTER", GetDefaultFilter()));
  if (s.GetLength() && (s != "NULL") && s.GetChr(1) != '<')
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
    {
      message_log (LOG_DEBUG, "%s de-activated: External filter was set to '%s'", Doctype.c_str(), s.c_str());
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
      Db->AddFieldType(TEXT_ELEMENT, FIELDTYPE::text);
    }
   DateField = DateCreatedField;
}


off_t ADOBE_PDFDOC::RunPipe(FILE *fp, const STRING& Fn)
{
  const char *argv[7];
  int    argc = 0;

  if (Filter.IsEmpty()) return -1; // ERROR

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
      if (!IsAbsoluteFilePath (Filter))
	{
	  message_log (LOG_ERROR, "%s: Check configuration for filter '%s'. Skipping rest.",
		Doctype.c_str(), Filter.c_str());
	  Filter.Clear();
	}
      else
	message_log (LOG_ERRNO, "%s: Could not open pipe '%s'", Doctype.c_str(), *argv);
      return -1;
    }

  int ch;
  off_t  len = 0;
  while ((ch = fgetc(pp)) != EOF)
    {
      fputc(ch, fp);
      len++;
    }
  _IB_pclose(pp);
  return len;
}

void ADOBE_PDFDOC::ParseFields(RECORD *RecordPtr)
{
  _VMEMODOC::ParseFields(RecordPtr);
}

void ADOBE_PDFDOC::Present (const RESULT& ResultRecord,
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

