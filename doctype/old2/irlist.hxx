/*-@@@
File:		irlist.hxx
Version:	1.00
Description:	Class IRLIST - IRList Mail Digest Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef IRLIST_HXX
#define IRLIST_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "mailfolder.hxx"

class IRLIST :  public MAILFOLDER {
public:
  IRLIST(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;
  const CHR  *Seperator() const;

  void ParseRecords(const RECORD& FileRecord);

  void SourceMIMEContent(PSTRING StringBuffer) const;
  ~IRLIST();
};

typedef IRLIST* PIRLIST;

#endif
