/*@@@
File:		odt2.cxx
Version:	1.01
Description:	Class ODT2 
Author:		Edward Zimmermann
@@@*/


#include <ctype.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

#include "common.hxx"
#include "filter2.hxx"
#include "doc_conf.hxx"

static const char default_filter[] = "odt-search";
static const char myDescription[] = "\"OASIS Open Document Format Text\" (ODT) Plugin (MEMODOC)";


class IBDOC_ODT2 : public FILTER2MEMODOC {
public:
   IBDOC_ODT2(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const {
      List->AddEntry (Doctype);
      FILTER2MEMODOC::Description(List);
      return desc.c_str();
   }

  void SourceMIMEContent(STRING *stringPtr) const {
    *stringPtr = "application/vnd.oasis.opendocument.text";
  }

   virtual const char *GetDefaultFilter() const { return default_filter;}


   // The default headline is from the field HEADLINE
   virtual void Present (const RESULT& ResultRecord, const STRING& ElementSet,
                   const STRING& RecordSyntax, PSTRING StringBuffer) const {
     if (ElementSet.Equals(BRIEF_MAGIC)) {
       Present (ResultRecord, "TITLE", RecordSyntax, StringBuffer);
       if (StringBuffer->IsEmpty())
	 Present (ResultRecord, "SUBJECT", RecordSyntax, StringBuffer);
       if (StringBuffer->IsEmpty())
         Present (ResultRecord, "HEADLINE", RecordSyntax, StringBuffer);
       if (StringBuffer->IsEmpty() && ! GetResourcePath(ResultRecord, StringBuffer))
	 DOCTYPE::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);
     } else DOCTYPE::Present (ResultRecord, ElementSet, RecordSyntax, StringBuffer);

   }


   ~IBDOC_ODT2() { }
private:
   STRING desc;
};


IBDOC_ODT2::IBDOC_ODT2(PIDBOBJ DbParent, const STRING& Name) : FILTER2MEMODOC(DbParent, Name)
{
  desc.form("%s. Uses an external filter to MEMODOC.\nOptions:\n\
   FILTER   Specifies the script to use (Default '%s')\n", myDescription, GetDefaultFilter() );
}


// Stubs for dynamic loading
extern "C" {
  IBDOC_ODT2 *  __plugin_odt2_create (IDBOBJ * parent, const STRING& Name)
  {
    return new IBDOC_ODT2 (parent, Name);
  }
  int          __plugin_odt2_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_odt2_query (void) { return myDescription; }
}

