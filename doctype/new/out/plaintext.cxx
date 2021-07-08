#pragma ident  "@(#)plaintext.cxx	1.4 02/24/01 17:45:32 BSN"
#include "plaintext.hxx"

PLAINTEXT::PLAINTEXT (PIDBOBJ DbParent, const STRING& Name) :
	DOCTYPE (DbParent, Name)
{ }


const char *PLAINTEXT::Description(PSTRLIST List) const
{
  const STRING ThisDoctype("PLAINTEXT");
  if ( List->IsEmpty() && Doctype != ThisDoctype)
    List->AddEntry(Doctype);
  List->AddEntry (ThisDoctype);

  DOCTYPE::Description(List);
  return "Plain unfielded text";
}


PLAINTEXT::~PLAINTEXT () { }
