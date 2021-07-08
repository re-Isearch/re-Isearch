/*-@@@
File:		roadsdoc.hxx
Version:	1.00
Description:	Class ROADSDOC - Colon Tagged (IAFA-like) Document Type
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
Copyright:	Basis Systeme netzwerk, Munich
@@@-*/

#ifndef ROADSDOC_HXX
#define ROADSDOC_HXX

#ifndef DTREG_HXX
# include "defs.hxx"
# include "doctype.hxx"
#endif
#include "iknowdoc.hxx"

class ROADSDOC :  public IKNOWDOC {
public:
  ROADSDOC(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  virtual void Present(const RESULT& ResultRecord, const STRING& ElementSet,
		const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void SourceMIMEContent(PSTRING StringBufferPtr) const;
  void SourceMIMEContent(const RESULT& ResultRecord, PSTRING StringPtr) const;
~ROADSDOC();
};

typedef ROADSDOC* PROADSDOC;

#endif
