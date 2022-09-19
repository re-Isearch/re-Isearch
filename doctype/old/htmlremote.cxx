/*-@@@
File:           htmlhead.cxx
Version:        1.0
Description:    Class HTMLREMOTE - HTML documents, <HEAD> only
Author:         Nassib Nassar <nassar@etymon.com>
@@@*/

#pragma ident  "%Z%%Y%%M%  %I% %G% %U% BSN"

#define MAX_TOKEN_LENGTH 65536

//#include <stdio.h>
//#include <ctype.h>
//#include <string.h>
//#include <iostream.h>
#include "isearch.hxx"
#include "htmlremote.hxx"
#include "doc_conf.hxx"

HTMLREMOTE::HTMLREMOTE(PIDBOBJ DbParent, const STRING& Name):HTMLHEAD(DbParent, Name)
{
  DocumentRoot = DOCTYPE::Getoption("BASE", NulString);
}

const char *HTMLREMOTE::Description(PSTRLIST List) const
{
  List->AddEntry (Doctype);
  HTMLHEAD::Description(List);

  return "\
HyperText Markup Language (W3C HTML) Documents\n\
  Search time Options:\n\
    BASE // Specifies the base of the HTML tree (root)\n\
    This can also be specified in the doctype.ini as BASE=dir\n\
    in the [General] section.";
}

void HTMLREMOTE::ParseRecords(const RECORD& FileRecord)
{
  if (DocumentRoot.IsEmpty())
    {
      STRING fn = FileRecord.GetPath();
      STRINGINDEX pos = fn.Search("/http/");
      if (pos)
	{
	  fn.EraseBefore(pos);
	  Db->ProfileWriteString(Doctype, "BASE", fn);
	  DocumentRoot = fn;
	}
    }
  DOCTYPE::ParseRecords(FileRecord);
}

GDT_BOOLEAN HTMLREMOTE::URL(const RESULT& ResultRecord, PSTRING StringBuffer, GDT_BOOLEAN OnlyRemote) const
{
  STRING Path;
  STRINGINDEX root_len = DocumentRoot.GetLength();

  StringBuffer->Clear(); // Clear the buffer
  // Get the fullpath of the record
  if (root_len || OnlyRemote == GDT_FALSE)
    {
      if (GetResourcePath(ResultRecord, &Path) == GDT_FALSE)
	{
//	  logf (LOG_DEBUG, "URL: No Resource Path defined");
	  return GDT_FALSE;
	}
    }
  if (root_len)
    {
	  // Is the document in the WWW tree?
//	  logf (LOG_DEBUG, "Looking for %s in %s", (const char *)DocumentRoot, (const char *)Path);
	  if (Path.SubString(1, root_len) ==  DocumentRoot)
	    {
	      Path.EraseBefore(root_len + 1);
	      // MIRROR_LAYOUT
#if 0
		  const struct {
		    const char *method;
		    size_t      len;
		    GDT_BOOLEAN dslash;
		  } methods[] = {
			{"ftp",     3, GDT_TRUE},
			{"urn",     3, GDT_TRUE},
			{"file",    4, GDT_TRUE},
			{"http",    4, GDT_TRUE},
			{"news",    4, GDT_FALSE},
			{"nntp",    4, GDT_TRUE},
                        {"ldap",    4, GDT_TRUE},
			{"wais",    4, GDT_TRUE},
			{"x500",    4, GDT_TRUE},
			{"https",   5, GDT_TRUE},
			{"shttp",   5, GDT_TRUE},
			{"whois",   5, GDT_TRUE},
			{"gopher",  6, GDT_TRUE},
			{"z39_50s", 7, GDT_TRUE},
			{"z39_50r", 7, GDT_TRUE},
			{"prospero",8, GDT_TRUE},
			{NULL, 0, GDT_FALSE}
		  };
#endif
		  //  http/furball.bsn.com_80/ --> http://furball.bsn.com:80/
		  STRINGINDEX x = Path.Search("/");
		  if (x > 3) // The shortest method
		    {
		      Path.Insert(x, ":/"); // turn / --> ://
		      if ((x = Path.Search("_", x+1)) != 0 && isdigit (Path.GetChr(x+1)))
			{
			  Path.SetChr(x, ':'); 
			  if (Path.Right ((size_t)9) == "/_._.html")
			    Path.EraseAfter(Path.GetLength() - 9);
			  *StringBuffer = Path;
			  return GDT_TRUE;
			}
		    }
		  return GDT_FALSE; // Bad Mirror
	  }
    }
  *StringBuffer << "file://" << Path;
  return GDT_TRUE;
}


HTMLREMOTE::~HTMLREMOTE()
{
}

