/*-@@@
File:		aollist.hxx
Version:	1.00
Description:	Class AOLLIST - AOL's Digest Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef AOLLIST_HXX
#define AOLLIST_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "listdigest.hxx"

class AOLLIST :  public LISTDIGEST {
public:
  AOLLIST(PIDBOBJ DbParent, const STRING& Name) : LISTDIGEST(DbParent, Name) {;}
  const char *Description(PSTRLIST List) const {
   List->AddEntry ("AOLLIST");
   LISTDIGEST::Description(List);
   return "AOL's RFC-noncompliant Listserver Mail Digests";
  }

  const CHR *Seperator() const { 
   return "\n=========================================================================";
  }
};

#endif
