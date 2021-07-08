/************************************************************************
************************************************************************/

/*@@@
File:		dif.hxx
Version:	1.00
Description:	Class DIF - DIF data
Author:		Archie Warnock, warnock@clark.net
Author:		Edward C. Zimmermann, edz@nonmonotonic.net
@@@*/

#ifndef DIF_HXX
#define DIF_HXX

#include "defs.hxx"
#include "doctype.hxx"
#include "colongrp.hxx"

class DIF : public COLONGRP {
public:
  DIF(PIDBOBJ DbParent, const STRING& Name);
  const char *Description(PSTRLIST List) const;

  void Present(const RESULT& ResultRecord, const STRING& ElementSet, 
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void DocPresent(const RESULT& ResultRecord, const STRING& ElementSet,
	const STRING& RecordSyntax, PSTRING StringBuffer) const;
  void SourceMIMEContent(PSTRING StringBufferPtr) const;
  ~DIF();
};

typedef DIF* PDIF;

#endif
