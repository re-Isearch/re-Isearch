/*-@@@
File:		mailman.hxx
Version:	1.00
Description:	Class MAILMAN - Mailman Digest Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef MAILMAN_HXX
#define MAILMAN_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "maildigest.hxx"

class MAILMAN :  public MAILDIGEST {
public:
  MAILMAN(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;
  const CHR *Seperator() const;

  virtual void BeforeIndexing() { MAILDIGEST::BeforeIndexing(); };
  virtual void AfterIndexing()  { MAILDIGEST::AfterIndexing();  };

  ~MAILMAN();
};

#endif
