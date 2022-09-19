/*@@@
File:		null.cxx
Version:	1.00
Description:	Class NULL
Author:		Edward Zimmermann
@@@*/

#include "doctype.hxx"

#ifndef _WIN32
# define __declspec(dllexport)
#endif

class IBDOC_NULL : public DOCTYPE {
public:
   IBDOC_NULL(PIDBOBJ DbParent, const STRING& Name);
   const char *Description(PSTRLIST List) const;

   ~IBDOC_NULL();
};

static const char myDescription[] = "Empty plugin";

// Stubs for dynamic loading
extern "C" __declspec(dllexport) {
  IBDOC_NULL *  __plugin_null_create (IDBOBJ * parent, const STRING& Name)
  {
    return new IBDOC_NULL (parent, Name);
  }
  int          __plugin_null_id  (void) { return DoctypeDefVersion; }
  const char * __plugin_null_query (void) { return myDescription; }
}

IBDOC_NULL::IBDOC_NULL(PIDBOBJ DbParent, const STRING& Name) : DOCTYPE(DbParent, Name) { }

const char *IBDOC_NULL::Description(PSTRLIST List) const
{
  List->AddEntry (Doctype);
  DOCTYPE::Description(List);
  return myDescription;
}


IBDOC_NULL::~IBDOC_NULL() {
}

#ifdef _WIN32
int WINAPI DllMain (HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved) 
{
  return TRUE; 
} 
#endif
