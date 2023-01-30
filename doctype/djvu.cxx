/*@@@
File:		djvu.cxx
Version:	1.00
Description:	Class DjVu
Author:		Edward Zimmermann
@@@*/

#include <ctype.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

#include "common.hxx"
#include "filter2.hxx"
#include "doc_conf.hxx"

static const char *djvu2text = "djvutxt";

class IBDOC_DJVU : public FILTER2TEXTDOC {
public:
   IBDOC_DJVU(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const {
      List->AddEntry (Doctype);
      FILTER2TEXTDOC::Description(List);
      return desc.c_str();
   }

  void SourceMIMEContent(STRING *stringPtr) const {
    *stringPtr = "image/vnd.djvu";
  }
   virtual const char *GetDefaultFilter() const { return djvu2text;}

   ~IBDOC_DJVU() { }
private:
   STRING desc;
};

static const char myDescription[] = "\"DjVu eBooks\" (DjVu) Plugin";

IBDOC_DJVU::IBDOC_DJVU(PIDBOBJ DbParent, const STRING& Name) : FILTER2TEXTDOC(DbParent, Name)
{
  const char *def_filter = GetDefaultFilter();
  const char *filter = ResolveBinPath(def_filter);
  desc.form("%s.\nUses an external filter (default: '%s') to PTEXT.\n",
	myDescription, filter && *filter ? filter : def_filter );
}


// Stubs for dynamic loading
extern "C" {
  IBDOC_DJVU *  __plugin_djvu_create (IDBOBJ * parent, const STRING& Name)
  {
    return new IBDOC_DJVU (parent, Name);
  }
  int          __plugin_djvu_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_djvu_query (void) { return myDescription; }
}

