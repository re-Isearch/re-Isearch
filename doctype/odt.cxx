/*@@@
File:		odt.cxx
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

static const char default_filter[] = "odt2txt";
static const char myDescription[] = "\"OASIS Open Document Format Text\" (ODT) Plugin (TEXTDOC)";


class IBDOC_ODT : public FILTER2TEXTDOC {
public:
   IBDOC_ODT(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const {
      List->AddEntry (Doctype);
      FILTER2TEXTDOC::Description(List);
      return desc.c_str();
   }

  void SourceMIMEContent(STRING *stringPtr) const {
    *stringPtr = "application/vnd.oasis.opendocument.text";
  }

   virtual const char *GetDefaultFilter() const { return default_filter;}

   ~IBDOC_ODT() { }
private:
   STRING desc;
};


IBDOC_ODT::IBDOC_ODT(PIDBOBJ DbParent, const STRING& Name) : FILTER2TEXTDOC(DbParent, Name)
{
  desc.form("%s. Uses an external filter to PTEXT.\nOptions:\n\
   FILTER   Specifies the program to use (Default '%s')\n", myDescription, GetDefaultFilter() );
}


// Stubs for dynamic loading
extern "C" {
  IBDOC_ODT *  __plugin_odt_create (IDBOBJ * parent, const STRING& Name)
  {
    return new IBDOC_ODT (parent, Name);
  }
  int          __plugin_odt_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_odt_query (void) { return myDescription; }
}

