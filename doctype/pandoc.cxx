/*@@@
File:		pandoc.cxx
Version:	1.00
Description:	Class XPandoc
Author:		Edward Zimmermann
@@@*/

//#include <ctype.h>
//#include <sys/stat.h>
//#include <pwd.h>
//#include <grp.h>
//#include <errno.h>

#if USE_LIBMAGIC
#endif

#include "doc_conf.hxx"
#include "filter2.hxx"
#include "pandoc.hxx"


static char format_ini_tag[] = "format";

const char *PANDOC::Description(PSTRLIST List) const
{
  static STRING Help;
  if (List->IsEmpty() && !format.IsEmpty())
    List->AddEntry(Doctype);
  List->AddEntry ("PANDOC");
  FILTER2HTMLDOC::Description(List);
  if (Help.IsEmpty()) {
    Help << "Uses the program '" << GetDefaultFilter() << " " << GetArgs() << "'  to convert the input file from ";
    if (format.IsEmpty())
      Help << "an autodetected format";
    else
      Help << format;
     Help  << " into HTML.\n\nOption: " << format_ini_tag << "=<format>";
    if (format.GetLength()) Help <<  " (default: " << format << ")";
    Help << "\n";
  }

  return Help;
}


const char *PANDOC::GetDefaultFilter() const
{
  return "pandoc";
}


PANDOC::PANDOC(PIDBOBJ DbParent, const STRING& Name) : FILTER2HTMLDOC(DbParent, Name)
{
#if 1
  format = Getoption(format_ini_tag, (Doctype.CmpNoCase("pandoc") != 0) ? Doctype : NulString); 
  SetFilter ( GetDefaultFilter() );
  STRING args ("-s -t HTML");
  if (format.GetLength()) {
    args << " -f " << format;
  }
  SetArgs(args);
#else
  SetArgs("-s -t HTML");
#endif
}
