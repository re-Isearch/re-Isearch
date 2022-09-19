/*-@@@
File:		ozsearch.hxx
Version:	1.00
Description:	Class OZSEARCH - TSL Tab Sep. List Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef OZSEARCH_HXX
#define OZSEARCH_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "tsldoc.hxx"
#endif
#include "buffer.hxx"

class OZSEARCH :  public TSLDOC {
public:
  OZSEARCH(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void ParseFields(PRECORD NewRecord);
  void SourceMIMEContent(PSTRING StringBufferPtr) const;
  void SourceMIMEContent(const RESULT& ResultRecord, STRING *StringPtr) const;
  ~OZSEARCH();
};

typedef OZSEARCH* POZSEARCH;

#endif
