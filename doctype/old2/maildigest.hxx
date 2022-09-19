/*-@@@
File:		maildigest.hxx
Version:	1.00
Description:	Class MAILDIGEST - Mail Digest Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef MAILDIGEST_HXX
#define MAILDIGEST_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "mailfolder.hxx"

class MAILDIGEST :  public MAILFOLDER {
public:
  MAILDIGEST(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  virtual void BeforeIndexing() { MAILFOLDER::BeforeIndexing(); };
  virtual void AfterIndexing()  { MAILFOLDER::AfterIndexing();  };

  virtual void ParseRecords(const RECORD& FileRecord);
  virtual void ParseFields(PRECORD NewRecord);

  void SourceMIMEContent(PSTRING StringBuffer) const;
  const CHR *Seperator() const;
  ~MAILDIGEST();

private:
  STRING OldKey;
  long OldInode;
  long OldDevice;
  UINT Count;
};

typedef MAILDIGEST* PMAILDIGEST;

#endif
