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
  // If the document root has not been defined in the INI we
  // define it here...
  if (DocumentRoot.IsEmpty())
    {
      STRING fn = FileRecord.GetPath();
      STRINGINDEX pos = fn.Search("/http/");
      if (pos)
	{
	  fn.EraseAfter(pos); // fn.EraseBefore(pos);

	  Db->ProfileWriteString(Doctype, "BASE", fn);
	  DocumentRoot = fn;
	}
    }
  DOCTYPE::ParseRecords(FileRecord);
}

bool HTMLREMOTE::URL(const RESULT& ResultRecord, PSTRING StringBuffer, bool OnlyRemote) const
{
  STRING Path;
  STRINGINDEX root_len = DocumentRoot.GetLength();

  StringBuffer->Clear(); // Clear the buffer
  // Get the fullpath of the record
  if (root_len || OnlyRemote == false)
    {
      if (GetResourcePath(ResultRecord, &Path) == false)
	{
//	  message_log (LOG_DEBUG, "URL: No Resource Path defined");
	  return false;
	}
    }
  if (root_len)
    {
	  // Is the document in the WWW tree?
	  message_log (LOG_DEBUG, "Looking for %s (ROOT) in %s", DocumentRoot.c_str(), Path.c_str());
	  if (Path.SubString(1, root_len) ==  DocumentRoot)
	    {
	      Path.EraseBefore(root_len + 1);
	      // MIRROR_LAYOUT
#if 0
		  const struct {
		    const char *method;
		    size_t      len;
		    bool dslash;
		  } methods[] = {
			{"ftp",     3, true},
			{"urn",     3, true},
			{"file",    4, true},
			{"http",    4, true},
			{"ipfs",   4, true}, 
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
			{"prospero",8, true},
			{NULL, 0, false}
		  };
#endif

		  //  http/furball.nonmonotonic.net_81/ --> http://furball.nonmonotonic.net:81/
		  STRINGINDEX x = Path.Search("/");
		  if (x > 3) // The shortest method
		    {

			    
		      Path.Insert(x, ":/"); // turn / --> ://
		      if ((x = Path.Search("_", x+1)) != 0 && isdigit (Path.GetChr(x+1)))
			{
			  // we turn _ back into : for port.. We could look to see if 80
			  // and zap it.. but won't bother
			  Path.SetChr(x, ':'); 
			}

		      // default page names
		      size_t trim;
		      if ((Path.Right (trim = 9)) == "/_._.html" ||
			   (Path.Right(trim = 11)) == "/index.html" ) {
		         Path.EraseAfter(Path.GetLength() - trim );
		       }
		      *StringBuffer = Path;
		      return true;
		    }
		  return false; // Bad Mirror
	  }
    }
  *StringBuffer << "file://" << Path;
  return true;
}


HTMLREMOTE::~HTMLREMOTE()
{
}

