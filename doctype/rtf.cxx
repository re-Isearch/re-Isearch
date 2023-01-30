/*@@@
File:		rtf.cxx
Version:	1.00
Description:	Class NULL
Author:		Edward Zimmermann
@@@*/

#include <ctype.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

#include "common.hxx"
#include "filter2.hxx"
#include "doc_conf.hxx"

class IBDOC_RTF : public FILTER2TEXTDOC {
public:
   IBDOC_RTF(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const {
      List->AddEntry (Doctype);
      FILTER2TEXTDOC::Description(List);
      return desc.c_str();
   }

  void SourceMIMEContent(STRING *stringPtr) const {
    *stringPtr = "application/rtf";
  }


   virtual const char *GetDefaultFilter() const { return "rtf2text";}

   ~IBDOC_RTF() { }
private:
   STRING desc;
};

static const char myDescription[] = "\"Rich Text Format\" (RTF) Plugin";

IBDOC_RTF::IBDOC_RTF(PIDBOBJ DbParent, const STRING& Name) : FILTER2TEXTDOC(DbParent, Name)
{
  const char *def_filter = GetDefaultFilter();
  const char *filter = ResolveBinPath(def_filter);
  desc.form("%s.\nUses an external filter (default: '%s') to PTEXT.\n",
 	myDescription, filter && *filter ? filter : def_filter );
}


// Stubs for dynamic loading
extern "C" {
  IBDOC_RTF *  __plugin_rtf_create (IDBOBJ * parent, const STRING& Name)
  {
    return new IBDOC_RTF (parent, Name);
  }
  int          __plugin_rtf_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_rtf_query (void) { return myDescription; }
}

