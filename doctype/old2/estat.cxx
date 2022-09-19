/*@@@
File:		estat.cxx
Version:	1.00
Description:	Class IBDOC_EUROSTAT
Author:		Edward Zimmermann
@@@*/

#include <ctype.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

#include "common.hxx"
#include "tsldoc.hxx"
#include "doc_conf.hxx"



class IBDOC_EUROSTAT: public TSVDOC {
public:
   IBDOC_EUROSTAT(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const {
      List->AddEntry (Doctype);
      TSVDOC::Description(List);
      return desc.c_str();
   }

   void SourceMIMEContent(STRING *stringPtr) const {
    *stringPtr = "application/x-csl-eurostat";
   }

  void AddFieldDefs() { TSLDOC::AddFieldDefs(); };
  void AfterIndexing() { TSLDOC::AfterIndexing(); };

   ~IBDOC_EUROSTAT() { }

private:
   STRING desc;
};

static const char myDescription[] = "EUROSTAT CSL Plugin";

IBDOC_EUROSTAT::IBDOC_EUROSTAT(PIDBOBJ DbParent, const STRING& Name) : TSVDOC(DbParent, Name)
{
  desc.form("%s. EUROSTAT CSL Dump records (Default: UseFirstRecord=True)\n\n", myDescription);
  TSLDOC::SetIFS(",\t\n\r");
}



// Stubs for dynamic loading
extern "C" {
  IBDOC_EUROSTAT *  __plugin_estat_create (IDBOBJ * parent, const STRING& Name)
  {
    return new IBDOC_EUROSTAT (parent, Name);
  }
  int          __plugin_estat_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_estat_query (void) { return myDescription; }
}

