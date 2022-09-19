#pragma ident  "@(#)gils.cxx	1.3 03/28/98 19:32:27 BSN"
#include "gils.hxx"

GILS::GILS (PIDBOBJ DbParent, const STRING& Name):
	SGMLTAG (DbParent, Name)
{
}

const char *GILS::Description(PSTRLIST List) const
{
  List->AddEntry ("GILS");
  SGMLTAG::Description(List);
  return "GILS SGML-like taged records";
}



// Produce MIME content on the basis of the upper level
void GILS::SourceMIMEContent(PSTRING StringPtr) const
{
  *StringPtr = "Application/X-GILS";
}

GILS::~GILS () 
{
}
