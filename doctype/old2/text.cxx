/*@@@
File:		text.cxx
Version:	1.00
Description:	Class NULL
Author:		Edward Zimmermann
@@@*/

#include "doctype.hxx"

static const char *MyDescription = "Plain Text Plugin";

class TEXTDOC : public DOCTYPE {
public:
   TEXTDOC(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const;

   ~TEXTDOC();
};


// Stubs for dynamic loading
extern "C" {
  TEXTDOC *    __plugin_text_create (IDBOBJ * parent, const STRING& Name) { return new TEXTDOC (parent, Name); }
  int          __plugin_text_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_text_query (void) { return MyDescription; }
}

TEXTDOC::TEXTDOC(PIDBOBJ DbParent, const STRING& Name) : DOCTYPE(DbParent, Name) { }

const char *TEXTDOC::Description(PSTRLIST List) const
{
  if (List)
    {
      List->AddEntry ("NULL");
      DOCTYPE::Description(List);
    }
  return MyDescription;
}

TEXTDOC::~TEXTDOC() {
}
