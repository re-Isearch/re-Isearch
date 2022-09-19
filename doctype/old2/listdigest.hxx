/*-@@@
File:		listdigest.hxx
Version:	1.00
Description:	Class LISTDIGEST - Listserver Mail Digest Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef LISTDIGEST_HXX
#define LISTDIGEST_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "maildigest.hxx"

class LISTDIGEST :  public MAILDIGEST {
public:
  LISTDIGEST(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  virtual void BeforeIndexing() { MAILDIGEST::BeforeIndexing(); };
  virtual void AfterIndexing()  { MAILDIGEST::AfterIndexing();  };

  virtual void ParseRecords(const RECORD& FileRecord);

  void SourceMIMEContent(PSTRING StringBuffer) const;
  const CHR *Seperator() const;
  ~LISTDIGEST();
};

typedef LISTDIGEST* PLISTDIGEST;

#endif
